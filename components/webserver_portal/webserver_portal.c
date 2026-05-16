#include "webserver_portal.h"

#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsp_engine.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "hal_i2s.h"
#include "sdkconfig.h"
#include "usb_audio_uac.h"
#include "wifi_ap.h"

static const char *TAG = "webserver_portal";

#define WEB_ROOT_PATH "/littlefs"
#define WEB_INDEX_PATH WEB_ROOT_PATH "/index.html"
#define WS_RX_MAX_BYTES 256

static httpd_handle_t s_server;
static bool s_littlefs_mounted;

static const char s_fallback_index[] =
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>PhonemeFree Unplugged</title>"
    "<style>body{margin:0;min-height:100vh;display:grid;place-items:center;"
    "background:#0a0a0a;color:#00ff88;font-family:ui-monospace,Consolas,monospace}"
    "main{width:min(680px,calc(100vw - 32px));border:1px solid #00ff88;padding:24px}"
    "h1{font-size:20px;margin:0 0 16px;letter-spacing:0}</style></head>"
    "<body><main><h1>PHONEMEFREE UNPLUGGED</h1>"
    "<p>STATUS: FALLBACK UI</p><p>Open <code>/api/status</code> for diagnostics.</p>"
    "</main></body></html>";

static esp_err_t mount_littlefs(void)
{
    if (s_littlefs_mounted) {
        return ESP_OK;
    }

    const esp_vfs_littlefs_conf_t conf = {
        .base_path = WEB_ROOT_PATH,
        .partition_label = "littlefs",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err == ESP_ERR_INVALID_STATE) {
        s_littlefs_mounted = true;
        return ESP_OK;
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "LittleFS mount failed, using fallback UI: %s", esp_err_to_name(err));
        return err;
    }

    s_littlefs_mounted = true;
    ESP_LOGI(TAG, "LittleFS mounted at %s", WEB_ROOT_PATH);
    return ESP_OK;
}

static esp_err_t send_status_json(httpd_req_t *req)
{
    dsp_engine_stats_t dsp_stats = {0};
    hal_i2s_stats_t i2s_stats = {0};
    usb_audio_uac_stats_t usb_stats = {0};

    dsp_engine_get_stats(&dsp_stats);
    hal_i2s_get_stats(&i2s_stats);
    usb_audio_uac_get_stats(&usb_stats);

    const int pitch = atomic_load(&g_dsp_params.pitch_semitones);
    const int noise = atomic_load(&g_dsp_params.noise_level_pct);
    const int formant = atomic_load(&g_dsp_params.formant_shift_pct);
    const bool enabled = atomic_load(&g_dsp_params.dsp_enabled);

    char json[768];
    const int written = snprintf(
        json,
        sizeof(json),
        "{"
        "\"wifi\":{\"ssid\":\"%s\"},"
        "\"dsp\":{\"enabled\":%s,\"pitch\":%d,\"noise\":%d,\"formant\":%d,"
        "\"samples_processed\":%" PRIu32 ",\"input_underflows\":%" PRIu32 ",\"output_drops\":%" PRIu32 "},"
        "\"i2s\":{\"samples_captured\":%" PRIu32 ",\"ringbuf_drops\":%" PRIu32 ",\"read_timeouts\":%" PRIu32 ",\"read_errors\":%" PRIu32 "},"
        "\"usb\":{\"mounted\":%s,\"packets_written\":%" PRIu32 ",\"underruns\":%" PRIu32 ",\"short_writes\":%" PRIu32 "}"
        "}",
        PHONEMEFREE_UNPLUGGED_WIFI_SSID,
        enabled ? "true" : "false",
        pitch,
        noise,
        formant,
        dsp_stats.samples_processed,
        dsp_stats.input_underflows,
        dsp_stats.output_drops,
        i2s_stats.samples_captured,
        i2s_stats.ringbuf_drops,
        i2s_stats.read_timeouts,
        i2s_stats.read_errors,
        usb_stats.mounted ? "true" : "false",
        usb_stats.packets_written,
        usb_stats.underruns,
        usb_stats.short_writes);

    if (written < 0 || written >= (int)sizeof(json)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status too large");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    FILE *index = fopen(WEB_INDEX_PATH, "r");
    if (!index) {
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, s_fallback_index, HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_set_type(req, "text/html");
    char buffer[512];
    while (!feof(index)) {
        const size_t bytes_read = fread(buffer, 1, sizeof(buffer), index);
        if (bytes_read > 0) {
            esp_err_t ret = httpd_resp_send_chunk(req, buffer, bytes_read);
            if (ret != ESP_OK) {
                fclose(index);
                return ret;
            }
        }
    }

    fclose(index);
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    return send_status_json(req);
}

#if CONFIG_HTTPD_WS_SUPPORT
static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}
static bool json_find_value(const char *json, const char *key, const char **value_start)
{
    char pattern[32];
    const int written = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (written < 0 || written >= (int)sizeof(pattern)) {
        return false;
    }

    const char *key_pos = strstr(json, pattern);
    if (!key_pos) {
        return false;
    }

    const char *colon = strchr(key_pos + written, ':');
    if (!colon) {
        return false;
    }

    ++colon;
    while (*colon == ' ' || *colon == '\t' || *colon == '\r' || *colon == '\n') {
        ++colon;
    }

    *value_start = colon;
    return true;
}

static bool json_get_int(const char *json, const char *key, int *out)
{
    const char *value = NULL;
    if (!json_find_value(json, key, &value)) {
        return false;
    }

    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value) {
        return false;
    }

    *out = (int)parsed;
    return true;
}

static bool json_get_bool(const char *json, const char *key, bool *out)
{
    const char *value = NULL;
    if (!json_find_value(json, key, &value)) {
        return false;
    }

    if (strncmp(value, "true", 4) == 0) {
        *out = true;
        return true;
    }

    if (strncmp(value, "false", 5) == 0) {
        *out = false;
        return true;
    }

    return false;
}

static void apply_control_message(const char *json)
{
    int int_value = 0;
    bool bool_value = false;

    if (json_get_int(json, "pitch", &int_value)) {
        atomic_store(&g_dsp_params.pitch_semitones, clamp_int(int_value, -12, 12));
    }
    if (json_get_int(json, "noise", &int_value)) {
        atomic_store(&g_dsp_params.noise_level_pct, clamp_int(int_value, 0, 100));
    }
    if (json_get_int(json, "formant", &int_value)) {
        atomic_store(&g_dsp_params.formant_shift_pct, clamp_int(int_value, -50, 50));
    }
    if (json_get_bool(json, "enabled", &bool_value)) {
        atomic_store(&g_dsp_params.dsp_enabled, bool_value);
    }
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
    };
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        return ret;
    }

    if (frame.len >= WS_RX_MAX_BYTES) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "websocket frame too large");
    }

    uint8_t payload[WS_RX_MAX_BYTES] = {0};
    frame.payload = payload;
    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret != ESP_OK) {
        return ret;
    }

    payload[frame.len] = '\0';
    if (frame.type == HTTPD_WS_TYPE_TEXT) {
        apply_control_message((const char *)payload);
    }

    char response[768];
    dsp_engine_stats_t dsp_stats = {0};
    hal_i2s_stats_t i2s_stats = {0};
    usb_audio_uac_stats_t usb_stats = {0};
    dsp_engine_get_stats(&dsp_stats);
    hal_i2s_get_stats(&i2s_stats);
    usb_audio_uac_get_stats(&usb_stats);

    const int written = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"enabled\":%s,\"pitch\":%d,\"noise\":%d,\"formant\":%d,"
        "\"usb_underruns\":%" PRIu32 ",\"i2s_samples\":%" PRIu32 ",\"dsp_samples\":%" PRIu32 "}",
        atomic_load(&g_dsp_params.dsp_enabled) ? "true" : "false",
        atomic_load(&g_dsp_params.pitch_semitones),
        atomic_load(&g_dsp_params.noise_level_pct),
        atomic_load(&g_dsp_params.formant_shift_pct),
        usb_stats.underruns,
        i2s_stats.samples_captured,
        dsp_stats.samples_processed);

    if (written < 0 || written >= (int)sizeof(response)) {
        return ESP_FAIL;
    }

    httpd_ws_frame_t response_frame = {
        .payload = (uint8_t *)response,
        .len = strlen(response),
        .type = HTTPD_WS_TYPE_TEXT,
    };
    return httpd_ws_send_frame(req, &response_frame);
}
#endif

esp_err_t webserver_portal_init(void)
{
    (void)mount_littlefs();
    ESP_LOGI(TAG, "Webserver portal initialized");
    return ESP_OK;
}

esp_err_t webserver_portal_start(void)
{
    if (s_server) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = CONFIG_PHONEMEFREE_UNPLUGGED_WEBSERVER_TASK_STACK_SIZE;
    config.lru_purge_enable = true;

    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "Failed to start HTTP server");

    const httpd_uri_t root_handler = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &root_handler), TAG, "Failed to register /");

    const httpd_uri_t status_handler = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &status_handler), TAG, "Failed to register /api/status");

#if CONFIG_HTTPD_WS_SUPPORT
    const httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &ws_uri), TAG, "Failed to register /ws");
#else
    ESP_LOGW(TAG, "HTTPD WebSocket support is disabled; /ws will not be available");
#endif

    ESP_LOGI(TAG, "Webserver portal started");
    return ESP_OK;
}

void webserver_portal_stop(void)
{
    if (!s_server) {
        return;
    }

    esp_err_t err = httpd_stop(s_server);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to stop webserver cleanly: %s", esp_err_to_name(err));
        return;
    }

    s_server = NULL;
}
