# PRD: Projeto "PhonemeFree Unplugged" v2.0
### Air-Gapped Voice Obfuscator — Firmware Specification for Claude Code

> **Instruções de Ingestão para o Agente:** Este PRD é a fonte única de verdade. Não assuma padrões, não extrapole arquitetura. Toda decisão ambígua está resolvida abaixo. Implemente na ordem do grafo de dependências (Seção 5). Pergunte antes de desviar.

---

## 0. Nome, Etimologia e Tom do Projeto (Contexto para README)

**Nome:** `PhonemeFree Unplugged`

**Repositório próprio planejado:** `https://github.com/ffreitasb/phonemefree-unplugged.git`

### 0.1 Nota de Nomenclatura — PhonemeFree Unplugged

| Contexto | Formato |
|---|---|
| Título, README, UI | PhonemeFree Unplugged |
| Nome do repo / pasta | phonemefree-unplugged |
| Variáveis C / defines CMake | PHONEMEFREE_UNPLUGGED |
| Package / module Kotlin | phonemefreeUnplugged |

*Regra:* o espaço é para humanos, o hífen é para máquinas, o underscore é para compiladores.

**Etimologia e referências intencionais — usar no README:**

- **Phoneme** — unidade mínima de som distintivo em fonética. É exatamente o que os modelos modernos de síntese de voz (TTS, voice cloning, deepfake de áudio) capturam e replicam para clonar uma identidade vocal. O dispositivo opera precisamente nessa camada: desvincular os fonemas do vínculo biométrico com seu emissor original.
- **Free** — homenagem direta ao **SpeakFreely** (1991–2000), um dos primeiros softwares de VoIP criptografado da internet, símbolo da era cypherpunk e da "idade média da internet" — quando privacidade era um ato de engenharia, não uma feature de produto. O sufixo carrega a mesma energia: fale livremente, mas fale desvinculado.
- **Cruzamento com deepfake:** PhonemeFree é conceitualmente o inverso de um deepfake de voz. Onde o deepfake *cria* uma identidade vocal sintética, o PhonemeFree *destrói* a identidade vocal original antes que possa ser capturada. É um anti-deepfake preventivo — opera no sinal antes que qualquer modelo tenha acesso a ele.

**Tom do README:** técnico, sem marketing, sem disclaimers corporativos. Estilo de um projeto da era PGP: explica o problema, explica a solução, lista as dependências, termina. O leitor que entende a referência ao SpeakFreely já sabe por que o projeto existe.

---

## 1. Objetivo e Escopo

Firmware para um periférico USB de ofuscação de voz em tempo real. O dispositivo:

- Ingere áudio via I2S DMA (microfone INMP441 ou compatível)
- Aplica pipeline DSP configurável (pitch shift + ruído + formant shift opcional)
- Entrega áudio processado como dispositivo USB Audio Class 1.0 (UAC 1.0) class-compliant
- Expõe interface de configuração via Wi-Fi AP + Captive Portal (WebSocket)

**Fora de escopo:** criptografia de rede, autenticação do portal, logging persistente.

---

## 2. Hardware Target

| Componente | Especificação |
|---|---|
| MCU | ESP32-S3 (Dual-core Xtensa LX7 @ 240MHz) |
| Microfone | INMP441 (I2S PDM, 24-bit, mono) |
| USB OTG | Porta nativa ESP32-S3 (pinos GPIO19/20), modo Device |
| Flash | ≥ 4MB (partição LittleFS para assets web) |
| Energia | 5V via USB-C host (sem negociação PD ativa) |

### 2.1 Pinagem (Definida — Não Inventar)

```c
// menuconfig / sdkconfig.defaults
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_BCK_PIN=4
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_WS_PIN=5
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_DATA_PIN=6
CONFIG_PHONEMEFREE_UNPLUGGED_I2S_PORT=I2S_NUM_0
```

> Se o hardware do usuário divergir, estas constantes são o único ponto de mudança.

---

## 3. Parâmetros de Áudio (Fixos)

| Parâmetro | Valor | Justificativa |
|---|---|---|
| Taxa de amostragem | **16.000 Hz** | Banda suficiente para voz; minimiza carga DSP e risco de underrun |
| Profundidade de bits (I2S) | 24-bit (expandido para 32-bit no DMA) | Padrão INMP441 |
| Profundidade de bits (USB/DSP) | **16-bit** | UAC 1.0 padrão; truncar após normalização |
| Canais | **Mono** | Caso de uso de voz; 44.1kHz/stereo é opção de compilação desabilitada |
| Frame USB (UAC) | 1ms @ 16kHz = 16 amostras por frame | Isocrônico FS |
| Tamanho do Ring Buffer | **512 amostras (32ms)** | Headroom suficiente; ajustável via `CONFIG_PHONEMEFREE_UNPLUGGED_RINGBUF_SAMPLES` |

---

## 4. Arquitetura de Software

### 4.1 Alocação de Cores (FreeRTOS)

```
Core 1 (App CPU) — Crítico de Latência
├── Task: dsp_engine_task        (Priority: 23, Stack: 8KB IRAM)
│   ├── Leitura do Ring Buffer
│   ├── Pipeline DSP (pitch → noise → formant)
│   └── Escrita no TX Buffer USB
└── ISR: i2s_dma_isr             (Coloca amostras no Ring Buffer)

Core 0 (Pro CPU) — Não-Crítico
├── Task: usb_device_task        (Priority: 5, Stack: 4KB)
│   └── Gerencia stack tinyusb + callbacks UAC
├── Task: wifi_ap_task           (Priority: 4, Stack: 4KB)
│   └── AP + DHCP + DNS (Captive Portal redirect)
└── Task: webserver_task         (Priority: 3, Stack: 6KB)
    └── AsyncWebServer + WebSocket handler
```

> **Regra de ouro:** Nenhum código de Core 0 escreve diretamente em variáveis do DSP sem passar pelo mecanismo de atualização atômica (Seção 4.3).

### 4.2 Ring Buffer (Especificação Explícita)

Usar **`esp_ringbuf`** da ESP-IDF com tipo `RINGBUF_TYPE_BYTEBUF`:

```c
#include "freertos/ringbuf.h"

// Criação (em app_main, antes das tasks)
RingbufHandle_t s_audio_ringbuf;
s_audio_ringbuf = xRingbufferCreate(
    CONFIG_PHONEMEFREE_UNPLUGGED_RINGBUF_SAMPLES * sizeof(int16_t),
    RINGBUF_TYPE_BYTEBUF
);

// Escrita (ISR i2s_dma — produtor)
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xRingbufferSendFromISR(s_audio_ringbuf, pcm_frame, frame_bytes, &xHigherPriorityTaskWoken);

// Leitura (dsp_engine_task — consumidor)
size_t received_bytes;
int16_t *pcm = (int16_t *)xRingbufferReceive(s_audio_ringbuf, &received_bytes, pdMS_TO_TICKS(10));
if (pcm) {
    // processar
    vRingbufferReturnItem(s_audio_ringbuf, pcm);
}
```

### 4.3 Atualização Atômica de Parâmetros DSP

Parâmetros do DSP (pitch, noise level, formant) são atualizados pelo WebSocket handler (Core 0) e lidos pelo dsp_engine_task (Core 1). Usar estrutura com flag atômico:

```c
typedef struct {
    _Atomic int   pitch_semitones;    // Range: -12 a +12
    _Atomic int   noise_level_pct;    // Range: 0 a 100
    _Atomic int   formant_shift_pct;  // Range: -50 a +50 (0 = desabilitado)
    _Atomic bool  dsp_enabled;        // Bypass global
} phonemefree_unplugged_dsp_params_t;

extern aegis_dsp_params_t g_dsp_params; // Definido em dsp_engine.c
```

> `_Atomic` garante leitura/escrita atômica sem mutex nos tipos integrais no Xtensa LX7. Não usar `volatile` puro — comportamento indefinido em multi-core.

---

## 5. Grafo de Dependências dos Módulos

```
[hal_i2s] ──────────────────────────────┐
                                         ▼
[hal_ringbuf] ──────────────────► [dsp_engine]
                                         │
[dsp_pitch]  ──────────────────────────►│
[dsp_noise]  ──────────────────────────►│
[dsp_formant (optional)] ──────────────►│
                                         ▼
                                   [usb_audio_uac]
                                         │
[wifi_ap] ──────────────────────────────┤
[webserver_portal] ─────────────────────┤
[ws_handler] ── g_dsp_params ──────────►│
                                         ▼
                                    [app_main]
```

**Implementar nesta ordem:** hal_i2s → hal_ringbuf → dsp_pitch → dsp_noise → dsp_formant → dsp_engine → usb_audio_uac → wifi_ap → webserver_portal → ws_handler → app_main

---

## 6. Especificação de Módulos

### 6.1 `hal_i2s` — Ingestão I2S via DMA

**Arquivo:** `components/hal_i2s/hal_i2s.c`

```c
// Interface pública
esp_err_t hal_i2s_init(RingbufHandle_t ringbuf_handle);
void      hal_i2s_start(void);
void      hal_i2s_stop(void);
```

**Configuração I2S:**
```c
i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX,
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // INMP441 entrega 24bit em frame 32bit
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_IRAM,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
};
```

**Conversão 32→16 bit no ISR (IRAM_ATTR obrigatório):**
```c
IRAM_ATTR static void i2s_dma_callback(void *arg) {
    // Ler buffer DMA, deslocar 8 bits (24bit em 32bit frame), truncar para int16_t
    // Colocar no ringbuf via xRingbufferSendFromISR
}
```

### 6.2 `dsp_pitch` — Pitch Shifter (SOLA-based)

**Arquivo:** `components/dsp_pitch/dsp_pitch.c`

**Algoritmo:** Implementação do **smbPitchShift de Stephan Bernsee** (licença livre para uso não-comercial/FOSS). Portado para C puro, com substituição de `malloc` por buffers estáticos de tamanho fixo alocados em DRAM.

**Interface:**
```c
// IRAM_ATTR: função chamada no hot path do dsp_engine_task
IRAM_ATTR void dsp_pitch_process(
    const int16_t *input,
    int16_t       *output,
    size_t         num_samples,
    float          pitch_factor   // 2^(semitones/12); 1.0 = sem alteração
);
```

**Restrições de implementação:**
- Sem `malloc`/`free` em runtime — buffers estáticos dimensionados para `CONFIG_PHONEMEFREE_UNPLUGGED_RINGBUF_SAMPLES`
- FFT via **`dsps_fft2r_fc32`** da biblioteca `esp-dsp` (FOSS, Apache 2.0)
- Tamanho FFT: 512 pontos (suficiente para 16kHz, latência ~32ms)
- Flag `IRAM_ATTR` em todas as funções do hot path

### 6.3 `dsp_noise` — White Noise Injector

**Arquivo:** `components/dsp_noise/dsp_noise.c`

**Algoritmo:** LFSR (Linear Feedback Shift Register) de 32 bits — determinístico, sem heap, adequado para ISR.

```c
IRAM_ATTR void dsp_noise_mix(
    int16_t *samples,
    size_t   num_samples,
    uint8_t  level_pct    // 0-100; 0 = bypass
);
```

**Implementação LFSR:**
```c
static uint32_t s_lfsr_state = 0xACE1u;

IRAM_ATTR static inline int16_t lfsr_next_sample(void) {
    s_lfsr_state ^= s_lfsr_state << 13;
    s_lfsr_state ^= s_lfsr_state >> 17;
    s_lfsr_state ^= s_lfsr_state << 5;
    return (int16_t)(s_lfsr_state & 0xFFFF);
}
```

Mix linear: `output[i] = (input[i] * (100 - level)) / 100 + (noise * level) / 100`

### 6.4 `dsp_formant` — Formant Shift (Condicional)

**Habilitado por:** `CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE=y` (default: n)

**Algoritmo:** VTLN simplificado — warping do espectro pós-FFT (no mesmo passo do pitch shifter para evitar FFT duplo). Implementado como modificação dos bins de frequência antes da IFFT no `dsp_pitch_process`.

**Interface:**
```c
#if CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE
IRAM_ATTR void dsp_formant_apply_warp(
    float *fft_bins,
    size_t num_bins,
    float  warp_factor   // 0.5-2.0; 1.0 = sem alteração
);
#endif
```

> O formant shift é aplicado **dentro** do `dsp_pitch_process` (no domínio da frequência), não como estágio separado, para economizar o custo de FFT adicional.

### 6.5 `usb_audio_uac` — USB Audio Class 1.0

**Arquivo:** `components/usb_audio_uac/usb_audio_uac.c`

**Driver:** TinyUSB via ESP-IDF (`idf_component.yml` com `espressif/tinyusb`)

**Configuração obrigatória no `menuconfig`:**
```
CONFIG_TINYUSB_ENABLED=y
CONFIG_TINYUSB_AUDIO_ENABLED=y
CONFIG_TINYUSB_AUDIO_FUNC_NUM=1
CONFIG_TINYUSB_AUDIO_IN_ENABLED=y  // Microfone (device → host)
```

**Descriptor USB (não deixar o agente improvisar):**
```c
// VID/PID: usar valores de desenvolvimento abertos (não registrados)
#define PHONEMEFREE_UNPLUGGED_USB_VID           0x303A  // Espressif VID (desenvolvimento)
#define PHONEMEFREE_UNPLUGGED_USB_PID           0x4001
#define PHONEMEFREE_UNPLUGGED_USB_MANUFACTURER  "PhonemeFree Unplugged"
#define PHONEMEFREE_UNPLUGGED_USB_PRODUCT       "PhonemeFree Unplugged Mic"
#define PHONEMEFREE_UNPLUGGED_USB_SERIAL        "001"

// Formato de áudio: 16kHz / 16-bit / Mono / PCM
// Terminal de entrada: IT_USB_STREAMING (não existe — é microfone: IT_MICROPHONE = 0x0201)
```

**Callback de preenchimento do buffer isocrônico:**
```c
// Chamado a cada 1ms pelo stack USB (Core 0, task usb_device_task)
// Deve consumir amostras de um segundo ring buffer (dsp_output_buf) preenchido pelo dsp_engine_task
bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting) {
    // Ler 16 amostras (16kHz × 1ms) do dsp_output_buf
    // Escrever via tud_audio_write()
    // Retornar true
}
```

**Buffer de saída DSP→USB:**
```c
// Ring buffer secundário entre dsp_engine_task (produtor) e usb callback (consumidor)
// Mesmo mecanismo da Seção 4.2, tamanho: 64 amostras (headroom de 4 frames USB)
RingbufHandle_t s_dsp_output_buf;
```

### 6.6 `wifi_ap` — Access Point + Captive Portal

**SSID:** `PhonemeFree Unplugged` (sem senha — rede isolada, sem roteamento para internet)

**IP do AP:** `192.168.4.1` (padrão ESP-IDF)

**Captive Portal:** Redirecionar todo DNS via `lwip` para `192.168.4.1`. Implementar com `esp_dns_lite` ou handler DNS UDP manual na porta 53 que responde com o IP do AP para qualquer query.

**Restrição crítica:** `esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20)` — não usar HT40 para não interferir com a latência do Core 1.

### 6.7 `webserver_portal` — GUI + WebSocket API

**Servidor:** `esp_http_server` (nativo ESP-IDF, sem dependências externas)

**Assets:** `index.html` armazenado em LittleFS (não SPIFFS):
```
# partitions.csv
nvs,      data, nvs,   0x9000,  0x6000,
phy_init, data, phy,   0xF000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,
littlefs, data, littlefs, 0x310000, 0xF0000,
```

**Endpoints:**
```
GET  /            → Serve index.html do LittleFS
GET  /ws          → Upgrade para WebSocket (parâmetros DSP)
GET  /api/status  → JSON com estado atual dos parâmetros (polling fallback)
```

**Protocolo WebSocket (JSON bidirecional):**
```json
// Cliente → Dispositivo (set params)
{ "pitch": 3, "noise": 20, "formant": 0, "enabled": true }

// Dispositivo → Cliente (status push, a cada 2s)
{ "pitch": 3, "noise": 20, "formant": 0, "enabled": true, "cpu_core1_pct": 45 }
```

**Handler WebSocket:**
```c
// Recebe JSON, valida ranges, atualiza g_dsp_params via atomic store
// Não usar mutex — _Atomic garante consistência suficiente para este caso
esp_err_t ws_handler(httpd_req_t *req) {
    // Parse JSON mínimo (sem cJSON alocante — usar parser simples inline ou jsmn)
    // atomic_store(&g_dsp_params.pitch_semitones, validated_value);
}
```

---

## 7. GUI — `index.html` (Arquivo Único)

**Estética:** Terminal/Stealth — fundo `#0a0a0a`, texto `#00ff88` (verde terminal), fonte monospace.

**Conteúdo:**
```
┌─ PHONEMEFREE ────────────────────────────────┐
│  STATUS: ● ACTIVE                            │
│                                              │
│  PITCH SHIFT    [-12 ════════●═══ +12]  +3st │
│  NOISE INJECT   [  0 ═════●═══════ 100] 20%  │
│  FORMANT SHIFT  [  0 ════════●════  50]  0%  │
│                                              │
│  [BYPASS ○]  [ENABLE ●]                      │
│                                              │
│  CPU CORE1: ████████░░ 45%                   │
└──────────────────────────────────────────────┘
```

**Implementação:**
- HTML + CSS + JS inline em arquivo único (`<2KB` gzip)
- WebSocket nativo (`new WebSocket('ws://192.168.4.1/ws')`)
- Sliders: `<input type="range">` estilizados via CSS (sem frameworks)
- Reconexão automática com backoff exponencial (2s, 4s, 8s, máx 30s)
- Sem dependências externas — zero CDN — funciona 100% offline

---

## 8. Estrutura de Arquivos do Projeto

```
phonemefree-unplugged/
├── CMakeLists.txt
├── partitions.csv
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   └── app_main.c
├── components/
│   ├── hal_i2s/
│   │   ├── CMakeLists.txt
│   │   ├── hal_i2s.h
│   │   └── hal_i2s.c
│   ├── hal_ringbuf/           (wrapper sobre esp_ringbuf + instâncias globais)
│   ├── dsp_engine/
│   │   ├── CMakeLists.txt
│   │   ├── dsp_engine.h       (inclui phonemefree_unplugged_dsp_params_t)
│   │   └── dsp_engine.c
│   ├── dsp_pitch/
│   ├── dsp_noise/
│   ├── dsp_formant/           (compilado apenas se CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE)
│   ├── usb_audio_uac/
│   ├── wifi_ap/
│   └── webserver_portal/
├── data/                      (conteúdo do LittleFS)
│   └── index.html
└── idf_component.yml          (espressif/tinyusb >= 0.15.0)
```

---

## 9. Restrições e Guardrails (Inegociáveis)

| Restrição | Especificação |
|---|---|
| Licença | Todo código DSP deve ser Apache 2.0, MIT, BSD ou domínio público. Verificar antes de incluir. |
| Alocação dinâmica | **Proibida** em qualquer função com `IRAM_ATTR`. Buffers DSP são estáticos. |
| `IRAM_ATTR` | Obrigatório em: `i2s_dma_callback`, `dsp_pitch_process`, `dsp_noise_mix`, `dsp_formant_apply_warp` |
| Stack de tasks | Definida em `menuconfig` via `CONFIG_PHONEMEFREE_UNPLUGGED_DSP_TASK_STACK_SIZE` (default: 8192) |
| Wi-Fi isolamento | `esp_wifi_set_country()` + sem `esp_netif_create_default_wifi_router_netif()` — AP sem NAT |
| Filesystem | **LittleFS** (`esp_littlefs`, Apache 2.0). SPIFFS não usar. |
| JSON | `jsmn` (domínio público) ou parser inline. Não usar `cJSON` (LGPL, link estático problemático). |
| Testes | Cada componente deve ter um `test/` com `unity` (nativo ESP-IDF) para teste em host via `idf.py host-test` |

---

## 10. Sequência de Implementação para Claude Code

```
Passo 1: Scaffold do projeto (CMakeLists, partitions, sdkconfig.defaults)
Passo 2: hal_i2s — I2S DMA + conversão 32→16bit + ringbuf write
Passo 3: dsp_noise — LFSR, testar com unity
Passo 4: dsp_pitch — smbPitchShift portado, buffers estáticos, testar com sinal sintético
Passo 5: dsp_engine — orquestrar pitch→noise, task Core 1, ler ringbuf, parâmetros atômicos
Passo 6: usb_audio_uac — tinyusb, descriptor, callback isocrônico, dsp_output_buf
Passo 7: wifi_ap — AP + DNS Captive Portal
Passo 8: webserver_portal + ws_handler — AsyncWebServer, JSON, atualização atômica
Passo 9: GUI index.html — compilar no LittleFS, testar em browser
Passo 10: dsp_formant (opcional) — integrar no dsp_pitch, flag de compilação
Passo 11: app_main — orquestrar init, tasks, pinagem
Passo 12: Testes de integração + medição de latência real
```

---

## 11. Métricas de Aceitação

| Métrica | Critério |
|---|---|
| Latência ponta-a-ponta | < 50ms (I2S input → USB output) |
| CPU Core 1 em carga máxima | < 80% (pitch + noise ativos) |
| Heap livre (Core 0) | > 20KB após inicialização completa |
| Enumeração USB | < 3s após powerup em Win 11, Linux, Android |
| GUI WebSocket | Sliders responsivos < 100ms de round-trip |
| Underrun de áudio | Zero em 60 minutos de operação contínua |

---

*PhonemeFree Unplugged v2.0 — FOSS Hardware OPSEC — Especificação para geração determinística via Claude Code*
