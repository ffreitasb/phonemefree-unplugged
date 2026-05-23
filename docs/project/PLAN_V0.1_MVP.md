# Planejamento v0.1 MVP - PhonemeFree Unplugged

Este documento transforma o PRD v2.0 em um plano executavel para a v0.1 funcional do firmware. O PRD continua sendo a fonte unica de verdade; este plano define o menor conjunto coerente de entregas para chegar a um dispositivo utilizavel em bancada.

## 0. Status atual

Marco atual: planejamento/hardware v0.1 concluidos e firmware ESP-IDF em desenvolvimento ativo.

Concluido nesta primeira etapa:

- README inicial criado com estetica alinhada ao projeto PhonemeFree principal.
- `.gitignore` criado para manter PRD, `ideas/`, BoM interna e notas internas fora do Git.
- Estrutura do repositorio organizada por assunto:
  - `docs/` para documentacao publica;
  - `docs/project/` para planejamento;
  - `docs/hardware/` para BoM, reference build, schematic concept e bring-up;
  - `docs/firmware/` para futuras notas de firmware;
  - `hardware/` para breadboard, perfboard, schematic, PCB, mecanica e manufacturing;
  - `assets/branding/` para assets publicos;
  - `tools/` para scripts futuros.
- BoM publica criada em `docs/hardware/HARDWARE_BOM_PUBLIC.md`.
- Conceito de esquematico criado em `docs/hardware/SCHEMATIC_CONCEPT.md`.
- Diagramas Mermaid versionados em `hardware/schematic/`.
- Reference build v0.1 travado em `docs/hardware/REFERENCE_BUILD_V0.1.md`.
- Checklist de bring-up criado em `docs/hardware/BRINGUP_CHECKLIST.md`.
- KiCad, PCB custom, diagramas completos de producao e manufacturing package foram explicitamente adiados para v0.2.
- Binarios de release e instalador web via browser foram registrados como etapa de empacotamento/lancamento, nao como bloqueio do desenvolvimento inicial.

Concluido nesta etapa de firmware inicial:

- ESP-IDF v6.0.1 instalado localmente via Espressif Installation Manager CLI.
- Scaffold ESP-IDF criado na raiz:
  - `CMakeLists.txt`;
  - `main/`;
  - `components/`;
  - `data/`;
  - `partitions.csv`;
  - `sdkconfig.defaults`.
- Componentes base criados com interfaces do PRD:
  - `hal_i2s`;
  - `hal_ringbuf`;
  - `dsp_noise`;
  - `dsp_pitch`;
  - `dsp_formant`;
  - `dsp_engine`;
  - `usb_audio_uac`;
  - `wifi_ap`;
  - `webserver_portal`.
- `data/index.html` inicial criado para futura particao LittleFS.
- Notas de setup ESP-IDF criadas em `docs/firmware/ESP_IDF_SETUP.md`.
- `idf.py set-target esp32s3` validado via EIM.
- `idf.py build` validado via EIM; binario gerado em `build/phonemefree-unplugged.bin`.
- Dependencia USB alinhada ao fluxo nativo do ESP-IDF v6 com `espressif/esp_tinyusb`, mantendo TinyUSB como base transitiva.

Concluido nesta etapa de firmware core:

- Configuracoes `CONFIG_PHONEMEFREE_UNPLUGGED_*` movidas para componente compartilhado `components/phonemefree_config/`, permitindo reuso pelo app principal e pelo app de testes.
- App de testes Unity criado em `test_apps/firmware_core/`.
- `hal_ringbuf` validado por testes de inicializacao idempotente e fluxo basico de PCM.
- `dsp_noise` validado por testes deterministas, nivel zero, nivel saturado e mix 50%.
- `dsp_engine` ganhou teste de reset dos parametros atomicos e estatisticas.
- `hal_i2s` deixou de ser stub:
  - configura I2S STD RX em 16 kHz;
  - usa frames de 32 bits no slot esquerdo;
  - converte ICS-43434/INMP441-style 24-bit I2S para PCM 16-bit;
  - envia blocos ao ring buffer de entrada sem bloquear;
  - contabiliza capturas, timeouts, erros e drops.
- `dsp_engine` deixou de ser stub:
  - task fixada no Core 1 em builds dual-core;
  - consome `s_audio_ringbuf`;
  - aplica bypass, `dsp_pitch_process` scaffold e `dsp_noise_mix`;
  - escreve em `s_dsp_output_buf` sem bloquear;
  - contabiliza amostras processadas, underflows e drops.
- `app_main` passou a inicializar e iniciar ring buffers, I2S, DSP, USB Audio, Wi-Fi scaffold e webserver scaffold.
- Build do app principal validado via EIM/ESP-IDF v6.0.1.
- Build do app Unity `test_apps/firmware_core` validado via EIM/ESP-IDF v6.0.1.

Concluido nesta etapa de firmware USB inicial:

- `usb_audio_uac` deixou de ser stub:
  - instala TinyUSB via `espressif/esp_tinyusb`;
  - define descriptor de dispositivo com VID `0x303A`, PID `0x4001`, manufacturer e product do projeto;
  - define configuracao UAC2 mono 16 kHz / 16-bit usando endpoint isocrono IN;
  - implementa callbacks de controle UAC para mute, volume, clock source e terminal connector;
  - cria task alimentadora no Core 0;
  - consome pacotes de 16 amostras / 1 ms de `s_dsp_output_buf`;
  - preenche underrun com silencio;
  - contabiliza pacotes escritos, underruns, short writes e estado de montagem.
- `CONFIG_FREERTOS_HZ=1000` foi adicionado aos defaults para builds limpos sustentarem tick de 1 ms.
- Build do app principal validado novamente com USB Audio real.
- Build do app Unity `test_apps/firmware_core` validado novamente apos a mudanca.

Concluido nesta etapa de portal inicial:

- `wifi_ap` deixou de ser stub:
  - inicializa NVS, esp_netif, event loop e Wi-Fi;
  - cria SoftAP aberto `PhonemeFree Unplugged` como implementacao provisoria de desenvolvimento;
  - usa canal 6, ate 4 conexoes e largura `WIFI_BW20`;
  - registra eventos de conexao/desconexao.
- `webserver_portal` deixou de ser stub:
  - monta LittleFS em `/littlefs`;
  - inclui `data/index.html` no build via `littlefs_create_partition_image`;
  - serve `/`;
  - expoe `GET /api/status` com contadores de DSP, I2S e USB;
  - registra `/ws` quando `CONFIG_HTTPD_WS_SUPPORT=y`;
  - aceita mensagens JSON pequenas para `enabled`, `pitch`, `noise` e `formant`, sem cJSON.
- `data/index.html` virou GUI local minima com sliders, toggle, WebSocket e fallback de status.
- Build do app principal validado com `littlefs.bin` gerado.
- Build do app Unity `test_apps/firmware_core` validado novamente apos adicionar dependencia LittleFS.

Concluido nesta etapa de licenciamento inicial:

- Modelo dual definido e documentado:
  - hardware e documentacao publica de hardware sob `CERN-OHL-S-2.0`;
  - firmware, UI embarcada, testes, ferramentas e documentacao de firmware/projeto sob `AGPL-3.0-or-later`.
- Textos completos adicionados em `LICENSES/`.
- Politica raiz adicionada em `LICENSE.md`.
- Guias adicionados em `docs/legal/` para:
  - escopo por diretorio;
  - licencas de terceiros;
  - checklist de compliance para releases;
  - politica de marca e forks.
- `AGENTS.md`, `README.md`, `docs/README.md`, `hardware/README.md` e `docs/hardware/README.md` atualizados para refletir o modelo.

Decisoes atualizadas para hardware/rede v0.1:

- ICS-43434 passa a ser o microfone oficial/preferencial da v0.1.
- INMP441 continua suportado como fallback comum, mas nao e mais o alvo preferencial.
- O firmware deve ser radio-silent por padrao: Wi-Fi desligado durante operacao normal como microfone USB.
- AP de configuracao deve ser WPA2, nunca aberto para aceite da v0.1.
- AP e portal devem iniciar somente durante janela fisica de manutencao acionada por botao.
- A ISR do botao deve apenas notificar uma task/fila/timer; `esp_wifi_start()` e `esp_wifi_stop()` nao devem ser chamados dentro da ISR.
- Timeout do AP:
  - 2 minutos sem cliente/heartbeat derruba o AP;
  - 10 minutos de hard cap derrubam o AP mesmo com cliente conectado.
- Documentacao publica deve usar a postura "radio-silent by default" em vez de prometer "air-gapped" permanente.

Ainda nao iniciado ou ainda scaffold:

- Captive DNS redirect real.
- Pitch shifting real.
- Flash e monitor em hardware.
- Refatoracao do AP para on-demand WPA2 com botao fisico, timeout de inatividade e hard cap.
- Configuracao persistente da senha WPA2 em NVS.

Pendente de validacao em bancada:

- Captura I2S real com ICS-43434 fisico.
- Captura I2S comparativa com INMP441 fallback.
- Qualidade da conversao 32-bit I2S -> PCM 16-bit.
- Estabilidade das tasks de audio com Wi-Fi desligado durante operacao normal e com AP ativo apenas na janela de manutencao.

Proximo passo aprovado:

- Fazer bring-up em hardware: flash, logs de boot, enumeracao USB como microfone no host e captura I2S real do ICS-43434, mantendo INMP441 como fallback de comparacao.

## 1. Objetivo da v0.1

Entregar um firmware ESP-IDF para ESP32-S3 que:

- usa como alvo primario o reference build `ESP32-S3-WROOM-1 N8R8/N16R8 + ICS-43434`;
- inicializa em hardware alvo ESP32-S3;
- captura audio mono 16 kHz do microfone I2S ICS-43434, com INMP441 como fallback;
- normaliza o fluxo para PCM `int16_t`;
- aplica pipeline DSP minimo em tempo real;
- enumera como dispositivo USB Audio Class 1.0, microfone mono 16 kHz / 16-bit;
- transmite audio processado para o host via USB;
- permanece com Wi-Fi desligado durante operacao normal;
- sobe AP WPA2 offline com captive portal somente apos acionamento fisico de manutencao;
- derruba AP/portal por inatividade ou hard cap;
- permite alterar parametros DSP por WebSocket;
- expoe uma GUI local simples em `index.html` via LittleFS;
- possui testes unitarios de componentes criticos com Unity quando viavel em host.

## 2. Definicao de MVP funcional

A v0.1 sera considerada funcional quando um usuario conseguir:

1. Compilar o projeto com ESP-IDF.
2. Gravar o firmware em um ESP32-S3 com USB nativo.
3. Conectar um ICS-43434 nos pinos definidos no reference build.
4. Ver o dispositivo enumerar no host como `PhonemeFree Unplugged Mic`.
5. Selecionar esse microfone no sistema operacional.
6. Ouvir/capturar audio vindo do microfone fisico.
7. Acionar o botao fisico de manutencao para abrir o AP WPA2 temporario.
8. Ativar bypass e obfuscacao via portal Wi-Fi.
9. Ajustar ao menos `pitch`, `noise` e `enabled` pelo WebSocket/GUI.
10. Confirmar que o AP cai por inatividade e por hard cap.
11. Rodar por 10 minutos em bancada sem crash, assert ou watchdog.

## 3. Escopo incluido na v0.1

### Hardware reference build

- Reference build publico travado em `docs/hardware/REFERENCE_BUILD_V0.1.md`.
- Checklist de bring-up em `docs/hardware/BRINGUP_CHECKLIST.md`.
- MVP em breadboard primeiro, depois perfboard dupla face 2.54 mm.
- KiCad, PCB custom e diagramas completos ficam para v0.2.

### Firmware base

- Scaffold ESP-IDF completo.
- `CMakeLists.txt` raiz, `main/CMakeLists.txt`, `partitions.csv`, `sdkconfig.defaults`.
- `idf_component.yml` com dependencias externas permitidas pelo PRD.
- Definicoes `CONFIG_PHONEMEFREE_UNPLUGGED_*` para pinos, buffers, tasks e flags.

### Audio input

- Componente `hal_i2s`.
- Configuracao I2S RX conforme PRD:
  - `sample_rate = 16000`;
  - `bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT`;
  - `channel_format = I2S_CHANNEL_FMT_ONLY_LEFT`;
  - `dma_buf_count = 4`;
  - `dma_buf_len = 64`;
  - `intr_alloc_flags = ESP_INTR_FLAG_IRAM`.
- Conversao 32-bit frame / 24-bit mic para PCM `int16_t`.
- Escrita no ring buffer de entrada.

### Buffers

- Componente `hal_ringbuf`.
- Instancias globais:
  - `s_audio_ringbuf`: I2S -> DSP.
  - `s_dsp_output_buf`: DSP -> USB.
- Uso exclusivo de `esp_ringbuf` com `RINGBUF_TYPE_BYTEBUF`.
- Politica documentada para overflow/underrun.

### DSP minimo

- Componente `dsp_noise` completo.
- Componente `dsp_pitch` integrado ao pipeline.
- Componente `dsp_engine` com:
  - task fixada no Core 1;
  - leitura do ring buffer de entrada;
  - aplicacao `pitch -> noise`;
  - bypass global por `enabled`;
  - escrita no ring buffer de saida USB;
  - parametros globais atomicos.
- `dsp_formant` fica fora do caminho obrigatorio da v0.1, mas o scaffold e a flag podem existir.

### USB audio

- Componente `usb_audio_uac`.
- TinyUSB habilitado para Audio IN.
- Descriptor UAC inicial baseado no helper UAC2 do TinyUSB:
  - VID `0x303A`;
  - PID `0x4001`;
  - manufacturer `PhonemeFree Unplugged`;
  - product `PhonemeFree Unplugged Mic`;
  - serial `001`;
  - PCM mono 16 kHz / 16-bit.
- Task de alimentacao USB consumindo 16 amostras por frame USB de 1 ms.
- Fallback seguro para underrun: preencher silencio e contabilizar contador de underrun.

### Wi-Fi AP e portal

- Componente `wifi_ap`.
- SSID `PhonemeFree Unplugged`.
- AP WPA2 com senha default documentada e alteravel pelo usuario.
- Wi-Fi desligado por padrao durante operacao normal como microfone USB.
- AP iniciado apenas por botao fisico de manutencao.
- ISR do botao limitada a notificacao de task/fila/timer.
- `esp_wifi_start()` e `esp_wifi_stop()` chamados apenas fora da ISR.
- Timer de inatividade: 2 minutos sem cliente/heartbeat derrubam AP e portal.
- Hard cap: 10 minutos derrubam AP e portal mesmo com cliente ativo.
- AP isolado, sem roteamento/NAT para internet.
- `esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20)`.
- Captive portal DNS basico respondendo `192.168.4.1`.

### Web server e WebSocket

- Componente `webserver_portal`.
- `esp_http_server`, sem AsyncWebServer.
- LittleFS montado em `/littlefs`.
- Servidor HTTP iniciado e parado junto com a janela de manutencao do AP.
- Endpoints:
  - `GET /`;
  - `GET /ws`;
  - `GET /api/status`.
- Parser JSON leve, sem `cJSON`.
- Validacao de ranges:
  - `pitch`: `-12` a `+12`;
  - `noise`: `0` a `100`;
  - `formant`: `-50` a `+50`, armazenado mas sem efeito se formant estiver desabilitado;
  - `enabled`: boolean.
- Atualizacao via `atomic_store`.

### GUI

- `data/index.html` unico arquivo.
- CSS/JS inline.
- Sem CDN e sem dependencias externas.
- Estetica terminal/stealth conforme PRD.
- Sliders para pitch, noise e formant.
- Toggle bypass/enable.
- Reconexao WebSocket com backoff.
- Fallback de status via `/api/status`.

### Testes e verificacao

- Testes Unity para:
  - `dsp_noise`;
  - validacao de parametros WebSocket;
  - conversao de amostras I2S 32->16 quando isolavel;
  - comportamento basico de ring buffer wrapper;
  - bypass e ordem do pipeline no `dsp_engine` com mocks.
- Smoke test em hardware:
  - boot;
  - USB enumeration;
  - Wi-Fi AP invisivel no boot normal;
  - Wi-Fi AP visivel somente apos botao fisico;
  - cliente conecta via WPA2;
  - AP cai por inatividade;
  - AP cai por hard cap;
  - portal responde;
  - WebSocket altera parametros;
  - audio chega no host.

## 4. Fora de escopo da v0.1

- `dsp_formant` funcional completo.
- Autenticacao web-app do portal alem da protecao WPA2.
- Criptografia de rede alem de WPA2 no AP de configuracao.
- Logging persistente.
- OTA update.
- UI responsiva sofisticada alem do necessario.
- Medicao formal de 60 minutos sem underrun.
- Otimizacoes finas de CPU para meta final de `< 80%` em carga maxima.
- Compatibilidade com 44.1 kHz, stereo ou outros perfis UAC.
- Suporte dinamico a pinagem diferente via UI.
- Persistencia dos parametros DSP em NVS.
- KiCad schematic.
- PCB custom.
- Diagramas completos de producao.
- Gerbers, BOM de fabricacao, pick-and-place ou pacote fab.
- Instalador web via browser Chromium.
- Binarios finais de release para usuario final.

## 5. Decisoes tecnicas assumidas para v0.1

1. O PRD manda usar `esp_http_server`; a mencao posterior a AsyncWebServer sera tratada como inconsistencia documental.
2. O tipo global correto sera `phonemefree_unplugged_dsp_params_t`; a mencao a `aegis_dsp_params_t` sera tratada como typo do PRD.
3. `formant` sera aceito pelo protocolo e armazenado, mas so tera efeito quando `CONFIG_PHONEMEFREE_UNPLUGGED_FORMANT_ENABLE=y` e o componente existir.
4. Em underrun USB, a v0.1 deve enviar silencio em vez de bloquear.
5. Em overflow I2S/DSP, a v0.1 deve descartar o frame mais recente ou falhar a escrita sem bloquear o hot path, contabilizando contador de diagnostico.
6. A primeira versao do captive portal pode ser DNS UDP manual se `esp_dns_lite` gerar atrito de dependencia.
7. Todo codigo chamado no hot path deve evitar heap, locks e logs verbosos.
8. O firmware v0.1 deve mirar primeiro o reference build documentado; XIAO, Waveshare e SuperMini sao compatibilidade posterior/best-effort.
9. O fluxo de desenvolvimento e ESP-IDF primeiro; binarios e instalador web ficam para empacotamento de release.
10. A implementacao USB inicial usa UAC2 porque o helper de descriptor disponivel no TinyUSB/ESP-IDF v6.0.1 e UAC2. A meta de compatibilidade do PRD continua registrada; a decisao final entre manter UAC2 ou implementar descriptor UAC1 manual depende da validacao em host real.
11. O microfone oficial/preferencial da v0.1 e ICS-43434; INMP441 permanece fallback comum.
12. O AP aberto atual e apenas scaffold de desenvolvimento. Para aceite v0.1, AP deve ser WPA2 e on-demand.
13. O dispositivo deve ser descrito publicamente como radio-silent by default; "air-gapped" so vale para operacao normal com radio desligado, nao para a janela temporaria de manutencao.

## 6. Arquitetura alvo da v0.1

Fluxo de audio:

```text
ICS-43434
  -> hal_i2s ISR/callback
  -> s_audio_ringbuf
  -> dsp_engine_task Core 1
  -> dsp_pitch_process
  -> dsp_noise_mix
  -> s_dsp_output_buf
  -> TinyUSB UAC feeder task
  -> USB host
```

Fluxo de controle:

```text
Physical maintenance button
  -> GPIO ISR
  -> config task/event queue
  -> WPA2 Wi-Fi AP + portal window
  -> Browser
  -> esp_http_server
  -> /ws
  -> JSON validation
  -> atomic_store(g_dsp_params)
  -> dsp_engine_task atomic_load
```

## 7. Estrutura de arquivos esperada ao final da v0.1

```text
phonemefree-unplugged/
├── CMakeLists.txt
├── README.md
├── idf_component.yml
├── partitions.csv
├── sdkconfig.defaults
├── assets/
│   └── branding/
├── docs/
│   ├── firmware/
│   ├── hardware/
│   │   └── HARDWARE_BOM_PUBLIC.md
│   └── project/
│       └── PLAN_V0.1_MVP.md
├── hardware/
│   ├── breadboard/
│   ├── perfboard/
│   ├── pcb/
│   ├── mechanical/
│   └── manufacturing/
├── data/
│   └── index.html
├── main/
│   ├── CMakeLists.txt
│   └── app_main.c
├── components/
    ├── hal_i2s/
    ├── hal_ringbuf/
    ├── dsp_noise/
    ├── dsp_pitch/
    ├── dsp_formant/
    ├── dsp_engine/
    ├── usb_audio_uac/
    ├── config_button/
    ├── wifi_ap/
    └── webserver_portal/
└── tools/
```

Cada componente deve ter, quando aplicavel:

- `CMakeLists.txt`;
- header publico `.h`;
- implementacao `.c`;
- `test/` com teste Unity quando o componente for testavel sem hardware.

## 8. Plano de execucao granular

### Fase 0 - Preparacao do repositorio

Entregaveis:

- Validacao do estado inicial do repo.
- Definicao do target ESP-IDF `esp32s3`.
- Documento `docs/project/PLAN_V0.1_MVP.md`.
- Reference build `docs/hardware/REFERENCE_BUILD_V0.1.md`.
- Checklist `docs/hardware/BRINGUP_CHECKLIST.md`.

Tarefas:

1. Confirmar que o PRD esta na raiz.
2. Criar plano v0.1.
3. Registrar decisoes tecnicas e inconsistencias conhecidas.
4. Travar o hardware reference build v0.1.
5. Criar checklist de bring-up antes do firmware.

Criterio de pronto:

- Plano, reference build e checklist versionados em `docs/`.

### Fase 1 - Scaffold ESP-IDF

Entregaveis:

- `CMakeLists.txt` raiz.
- `main/CMakeLists.txt`.
- `main/app_main.c` minimo.
- `partitions.csv`.
- `sdkconfig.defaults`.
- `idf_component.yml`.
- Pastas vazias/placeholder dos componentes.

Tarefas:

1. Criar estrutura ESP-IDF minima.
2. Configurar projeto como `phonemefree-unplugged`.
3. Adicionar particao LittleFS conforme PRD.
4. Definir configs:
   - pinos I2S;
   - ring buffer samples;
   - stack task DSP;
   - flag formant default `n`;
   - TinyUSB audio via `espressif/esp_tinyusb`;
   - LittleFS.
5. Criar `app_main` inicial com logs de boot e versao.
6. Rodar build inicial.

Criterio de pronto:

- `idf.py build` conclui com app minimo. Status: concluido em ESP-IDF v6.0.1 via EIM.

### Fase 2 - `hal_ringbuf`

Entregaveis:

- `components/hal_ringbuf/hal_ringbuf.h`.
- `components/hal_ringbuf/hal_ringbuf.c`.
- Wrappers de init/get para buffers globais.
- Teste Unity basico.

Tarefas:

1. Criar `s_audio_ringbuf`.
2. Criar `s_dsp_output_buf`.
3. Implementar init idempotente.
4. Expor getters.
5. Definir tamanho do output buffer como 64 amostras.
6. Garantir uso de `RINGBUF_TYPE_BYTEBUF`.
7. Testar criacao e envio/recebimento basico quando viavel.

Criterio de pronto:

- Buffers inicializam sem heap no hot path.
- Componentes consumidores nao criam ring buffers por conta propria.

### Fase 3 - `hal_i2s`

Entregaveis:

- `components/hal_i2s/hal_i2s.h`.
- `components/hal_i2s/hal_i2s.c`.
- Conversor 32->16 isolado para teste.
- Teste Unity do conversor.

Tarefas:

1. Implementar `hal_i2s_init(RingbufHandle_t ringbuf_handle)`.
2. Configurar driver I2S nos pinos do PRD.
3. Implementar `hal_i2s_start`.
4. Implementar `hal_i2s_stop`.
5. Implementar conversao de amostra:
   - receber frame 32-bit;
   - considerar 24-bit significativo;
   - deslocar/truncar para `int16_t`.
6. Enviar blocos PCM para `s_audio_ringbuf`.
7. Evitar logs dentro de ISR/callback.
8. Tratar erro de ring buffer cheio sem bloquear.

Criterio de pronto:

- Captura I2S inicializa no hardware.
- Amostras chegam ao ring buffer de entrada.

### Fase 4 - `dsp_noise`

Entregaveis:

- `components/dsp_noise/dsp_noise.h`.
- `components/dsp_noise/dsp_noise.c`.
- Testes Unity.

Tarefas:

1. Implementar LFSR 32-bit do PRD.
2. Implementar `IRAM_ATTR void dsp_noise_mix(...)`.
3. Garantir bypass quando `level_pct == 0`.
4. Aplicar clamp interno para `level_pct > 100`.
5. Implementar mix linear com saturacao segura para `int16_t`.
6. Testar:
   - bypass preserva buffer;
   - nivel 100 altera amostras;
   - nivel intermediario fica dentro de range;
   - chamadas sucessivas sao deterministicas a partir do estado inicial quando houver reset de teste.

Criterio de pronto:

- `dsp_noise` passa nos testes e nao usa heap.

### Fase 5 - `dsp_pitch`

Entregaveis:

- `components/dsp_pitch/dsp_pitch.h`.
- `components/dsp_pitch/dsp_pitch.c`.
- Buffers estaticos dimensionados por config.
- Teste com sinal sintetico.

Tarefas:

1. Avaliar licenca do algoritmo antes de portar.
2. Integrar `esp-dsp` para FFT.
3. Implementar `dsp_pitch_process`.
4. Usar buffers estaticos em DRAM.
5. Evitar `malloc/free` em runtime.
6. Garantir caminho rapido para `pitch_factor == 1.0`.
7. Implementar clamp defensivo de `pitch_factor`.
8. Testar:
   - fator `1.0` preserva sinal dentro de tolerancia;
   - nao escreve fora do buffer;
   - aceita blocos ate `CONFIG_PHONEMEFREE_UNPLUGGED_RINGBUF_SAMPLES`.

Criterio de pronto:

- Pitch compila e integra ao build.
- Bypass/fator `1.0` e estavel.
- Pitch diferente de `1.0` gera saida audivelmente modificada, mesmo que ainda demande refinamento.

### Fase 6 - `dsp_engine`

Entregaveis:

- `components/dsp_engine/dsp_engine.h`.
- `components/dsp_engine/dsp_engine.c`.
- Tipo `phonemefree_unplugged_dsp_params_t`.
- Global `g_dsp_params`.
- Task Core 1.
- Testes com mocks quando viavel.

Tarefas:

1. Definir struct atomica do PRD.
2. Inicializar defaults:
   - `pitch = 0`;
   - `noise = 0`;
   - `formant = 0`;
   - `enabled = true`.
3. Implementar `dsp_engine_init`.
4. Implementar `dsp_engine_start`.
5. Fixar task no Core 1.
6. Definir prioridade 23.
7. Ler blocos do `s_audio_ringbuf`.
8. Aplicar bypass se `enabled == false`.
9. Calcular `pitch_factor = powf(2.0f, semitones / 12.0f)`.
10. Aplicar `dsp_pitch_process`.
11. Aplicar `dsp_noise_mix`.
12. Escrever no `s_dsp_output_buf`.
13. Atualizar contadores:
    - frames processados;
    - underrun input;
    - overflow output;
    - ultimo tempo de processamento.

Criterio de pronto:

- Pipeline roda sem bloquear.
- Parametros sao lidos via `atomic_load`.
- Audio processado chega ao buffer de saida.

### Fase 7 - `usb_audio_uac`

Entregaveis:

- `components/usb_audio_uac/usb_audio_uac.h`.
- `components/usb_audio_uac/usb_audio_uac.c`.
- Descriptors TinyUSB.
- Task de alimentacao do endpoint IN.
- Teste de enumeracao em host.

Tarefas:

1. Configurar TinyUSB no build. Status: concluido.
2. Definir descriptor UAC inicial. Status: concluido com UAC2 helper do TinyUSB; UAC1 manual depende de validacao de compatibilidade.
3. Implementar init USB. Status: concluido.
4. Implementar task USB Core 0. Status: concluido.
5. Consumir exatamente 16 amostras por frame de 1 ms. Status: concluido.
6. Em underrun, enviar silencio. Status: concluido.
7. Expor contadores de diagnostico. Status: concluido.
8. Testar enumeracao:
   - Windows 11;
   - Linux quando disponivel;
   - Android quando disponivel.

Criterio de pronto:

- Host reconhece o dispositivo como microfone USB.
- Stream de silencio/teste chega ao host antes da integracao com I2S, se necessario.
- Stream real chega ao host apos integracao.

### Fase 8 - Integracao audio fim-a-fim

Entregaveis:

- `app_main` inicializando ringbuf, I2S, DSP e USB.
- Audio I2S -> DSP -> USB funcionando.
- Checklist de bancada.

Tarefas:

1. Inicializar `hal_ringbuf`.
2. Inicializar `hal_i2s`.
3. Inicializar `dsp_engine`.
4. Inicializar `usb_audio_uac`.
5. Iniciar I2S.
6. Iniciar DSP task.
7. Validar stream no host.
8. Testar bypass.
9. Testar noise.
10. Testar pitch basico.
11. Registrar latencia estimada e underruns.

Criterio de pronto:

- Audio capturado pelo ICS-43434 chega ao host via USB.
- INMP441 fallback tambem consegue capturar audio para comparacao, quando disponivel.
- Bypass e obfuscacao basica sao audiveis.

### Fase 9 - `wifi_ap`

Entregaveis:

- `components/wifi_ap/wifi_ap.h`.
- `components/wifi_ap/wifi_ap.c`.
- AP WPA2 on-demand.
- Wi-Fi desligado por padrao.
- Botao fisico de manutencao aciona a abertura temporaria do AP.
- Timer de inatividade de 2 minutos sem cliente/heartbeat.
- Hard cap de 10 minutos.
- DNS captive portal basico.

Tarefas:

1. Inicializar NVS se necessario para Wi-Fi.
2. Criar netif AP apenas.
3. Configurar SSID `PhonemeFree Unplugged`.
4. Configurar IP padrao `192.168.4.1`.
5. Configurar WPA2 com senha default.
6. Persistir senha alteravel pelo usuario em NVS.
7. Garantir AP sem NAT.
8. Aplicar `WIFI_BW20` no ESP-IDF v6.
9. Criar GPIO/button path: ISR minima -> task/event queue -> start/stop Wi-Fi.
10. Garantir que `app_main` nao inicia Wi-Fi nem webserver no boot normal.
11. Implementar inactivity timer de 2 minutos baseado em cliente/heartbeat.
12. Implementar hard cap de 10 minutos.
13. Implementar DNS redirect para qualquer query.
14. Testar conexao a partir de celular/notebook.

Criterio de pronto:

- Rede nao aparece apos boot normal.
- Rede aparece somente apos botao fisico.
- Cliente conecta via WPA2.
- Qualquer dominio resolve para `192.168.4.1`.
- Rede cai apos 2 minutos sem cliente/heartbeat.
- Rede cai apos 10 minutos mesmo com cliente ativo.

### Fase 10 - LittleFS e assets

Entregaveis:

- `data/index.html`.
- Integracao LittleFS no build.
- Montagem de LittleFS no boot.

Tarefas:

1. Adicionar componente LittleFS permitido.
2. Configurar particao `littlefs`.
3. Criar `data/index.html` inicial.
4. Garantir embed/flash dos assets via fluxo ESP-IDF.
5. Testar leitura do arquivo no firmware.

Criterio de pronto:

- Firmware monta LittleFS.
- `index.html` e servido pelo webserver.

### Fase 11 - `webserver_portal` e WebSocket

Entregaveis:

- `components/webserver_portal/webserver_portal.h`.
- `components/webserver_portal/webserver_portal.c`.
- `GET /`.
- `GET /api/status`.
- `GET /ws`.
- Parser JSON leve.

Tarefas:

1. Inicializar `esp_http_server`.
2. Servir `/` a partir de LittleFS.
3. Implementar `/api/status`.
4. Implementar WebSocket.
5. Validar JSON recebido.
6. Aplicar ranges do PRD.
7. Atualizar `g_dsp_params` com `atomic_store`.
8. Enviar status push a cada 2 s ou sob demanda.
9. Expor contadores basicos:
   - cpu Core 1 estimada ou placeholder inicial;
   - underrun USB;
   - overflow ringbuf.
10. Testar round-trip local.

Criterio de pronto:

- Alterar sliders no browser muda parametros usados pelo DSP.
- Requisicoes invalidas sao rejeitadas sem crash.

### Fase 12 - GUI v0.1

Entregaveis:

- `data/index.html` final para v0.1.

Tarefas:

1. Criar layout terminal/stealth.
2. Adicionar controles:
   - pitch;
   - noise;
   - formant;
   - bypass/enable.
3. Implementar WebSocket nativo.
4. Implementar reconexao 2s, 4s, 8s, max 30s.
5. Implementar fallback `/api/status`.
6. Atualizar status visual.
7. Manter arquivo pequeno e offline.
8. Testar em desktop e celular.

Criterio de pronto:

- GUI permite controlar o DSP sem dependencias externas.

### Fase 13 - Integracao completa v0.1

Entregaveis:

- Firmware integrado.
- README inicial.
- Checklist de testes manuais.
- Lista de riscos pendentes.

Tarefas:

1. Orquestrar inicializacao completa em `app_main`.
2. Garantir ordem:
   - NVS;
   - LittleFS;
   - ring buffers;
   - I2S;
   - DSP;
   - USB;
   - Wi-Fi AP;
   - webserver.
3. Revisar prioridades e core affinity.
4. Remover logs excessivos do hot path.
5. Rodar build limpo.
6. Rodar testes Unity disponiveis.
7. Fazer smoke test em hardware.
8. Atualizar README com:
   - objetivo;
   - hardware;
   - pinagem;
   - build;
   - flash;
   - uso;
   - limitacoes v0.1.

Criterio de pronto:

- v0.1 esta pronta para tag interna quando todos os criterios da secao 12 forem satisfeitos.

## 9. Ordem recomendada de implementacao

Sequencia primaria:

1. Scaffold ESP-IDF.
2. `hal_ringbuf`.
3. `dsp_noise`.
4. `dsp_engine` com entrada/saida mockada.
5. `usb_audio_uac` com silencio/test tone.
6. `hal_i2s`.
7. Integracao I2S -> DSP -> USB.
8. `wifi_ap` on-demand WPA2.
9. LittleFS.
10. `webserver_portal`.
11. GUI.
12. Integracao final.
13. README e checklist.

Observacao: esta ordem antecipa `dsp_noise` antes de `dsp_pitch` para viabilizar obfuscacao audivel mais cedo. O `dsp_pitch` entra antes da tag v0.1, mas nao precisa bloquear os primeiros testes de USB e I2S.

## 10. Entregaveis finais da v0.1

- Firmware ESP-IDF compilavel.
- Estrutura completa de componentes.
- USB microphone UAC funcional, com perfil final UAC1/UAC2 decidido apos validacao em host.
- Captura I2S funcional.
- Pipeline DSP com bypass, pitch e noise.
- Portal Wi-Fi offline, WPA2 e on-demand.
- GUI `index.html` em LittleFS.
- WebSocket de parametros.
- `/api/status`.
- Testes unitarios basicos.
- README inicial.
- Checklist de validacao manual.

## 11. Criterios de aceite da v0.1

### Build

- `idf.py set-target esp32s3` funciona.
- `idf.py build` conclui sem erros.
- Build nao depende de CDN, servicos externos ou assets remotos.

### Flash e boot

- `idf.py flash monitor` grava e inicia.
- Boot nao gera panic/assert.
- Logs mostram init de ringbuf, I2S, DSP e USB.
- Logs mostram Wi-Fi e webserver parados apos boot normal.

### USB

- Host enumera dispositivo em menos de 5 s na v0.1.
- Dispositivo aparece como microfone.
- Host recebe stream mono 16 kHz / 16-bit.
- Em underrun, host recebe silencio em vez de travar.

### Audio

- Bypass transmite voz reconhecivel.
- `noise > 0` altera o sinal audivelmente.
- `pitch != 0` altera o sinal audivelmente.
- Latencia percebida fica aceitavel para uso em bancada, mesmo antes da meta final formal de `< 50 ms`.

### Portal

- AP `PhonemeFree Unplugged` nao aparece no boot normal.
- AP aparece somente apos acionamento fisico de manutencao.
- Cliente conecta via WPA2.
- `http://192.168.4.1/` abre a GUI.
- WebSocket conecta.
- Sliders atualizam parametros.
- `/api/status` retorna JSON valido.
- AP derruba apos 2 minutos sem cliente/heartbeat.
- AP derruba apos 10 minutos mesmo com cliente ativo.

### Estabilidade

- 10 minutos de operacao em bancada sem crash.
- Sem watchdog no Core 1.
- Sem alocacao dinamica no hot path.
- Contadores de underrun/overflow acessiveis por status ou log.

## 12. Checklist de validacao manual

Antes de declarar v0.1:

- [ ] Build limpo concluido.
- [ ] Flash em ESP32-S3 concluido.
- [ ] Boot sem panic.
- [ ] LittleFS montado.
- [ ] USB enumerado como microfone.
- [ ] Host recebe audio.
- [ ] I2S captura audio real do ICS-43434.
- [ ] I2S captura audio real do INMP441 fallback, se disponivel.
- [ ] Bypass funciona.
- [ ] Noise funciona.
- [ ] Pitch funciona.
- [ ] AP Wi-Fi nao aparece no boot normal.
- [ ] Botao fisico aciona janela de manutencao.
- [ ] AP Wi-Fi aparece somente durante janela de manutencao.
- [ ] Cliente conecta no AP via WPA2.
- [ ] AP cai apos 2 minutos sem cliente/heartbeat.
- [ ] AP cai apos hard cap de 10 minutos.
- [ ] Captive portal redireciona.
- [ ] `/` serve `index.html`.
- [ ] `/api/status` retorna JSON.
- [ ] `/ws` conecta.
- [ ] Sliders alteram `g_dsp_params`.
- [ ] 10 minutos sem crash.
- [ ] README atualizado.

## 13. Riscos principais

### TinyUSB UAC no ESP-IDF

Risco: descriptors UAC podem demandar ajustes finos para Windows/Linux/Android.

Mitigacao:

- Primeiro fazer enumeracao com stream de silencio.
- Depois integrar DSP.
- Testar em mais de um host.

### Pitch shifter

Risco: `smbPitchShift` com `esp-dsp` pode consumir mais tempo que o esperado ou exigir adaptacao maior.

Mitigacao:

- Implementar caminho bypass/fator 1.0 primeiro.
- Manter noise como obfuscacao minima funcional.
- Medir tempo de bloco no `dsp_engine`.
- Otimizar somente apos audio fim-a-fim funcionar.

### I2S callback/ISR

Risco: API exata de I2S pode variar conforme versao ESP-IDF.

Mitigacao:

- Isolar conversao 32->16.
- Manter `hal_i2s` pequeno.
- Evitar acoplar DSP ao driver I2S.

### Wi-Fi vs latencia

Risco: Wi-Fi e portal podem interferir em tempo real se ficarem ativos durante operacao normal ou se logs, callbacks ou prioridade forem mal configurados.

Mitigacao:

- Wi-Fi desligado por padrao.
- AP e portal ativos apenas durante janela fisica de manutencao.
- Timeout de 2 minutos sem cliente/heartbeat.
- Hard cap de 10 minutos.
- Core 1 dedicado ao DSP.
- Wi-Fi/web no Core 0.
- HT20.
- Sem escrita direta em parametros DSP fora de atomic store.

### Heap e fragmentacao

Risco: servidor HTTP, Wi-Fi e TinyUSB usam heap; hot path nao pode depender disso.

Mitigacao:

- Nenhum `malloc/free` em `IRAM_ATTR`.
- Buffers DSP estaticos.
- Monitorar heap livre apos boot.

## 14. Instrumentacao minima

A v0.1 deve manter contadores simples:

- frames I2S recebidos;
- bytes/amostras descartadas por ring buffer cheio;
- frames DSP processados;
- overflows do buffer DSP -> USB;
- underruns USB;
- ultimo tempo de processamento DSP por bloco;
- maior tempo de processamento DSP observado;
- heap livre apos boot completo.

Esses dados podem aparecer inicialmente em log e depois em `/api/status`.

## 15. Plano de versionamento sugerido

- `v0.1.0-dev.1`: scaffold compila. Status: concluido localmente.
- `v0.1.0-dev.2`: ringbuf + noise + testes. Status: concluido.
- `v0.1.0-dev.3`: I2S + DSP task + USB Audio feeder compilando. Status: concluido em build; pendente enumeracao em hardware.
- `v0.1.0-dev.4`: I2S captura audio em bancada com ICS-43434.
- `v0.1.0-dev.5`: I2S -> DSP -> USB validado em host.
- `v0.1.0-dev.6`: Wi-Fi AP + portal. Status: scaffold aberto concluido em build; precisa refatorar para WPA2 on-demand antes de aceite.
- `v0.1.0-dev.7`: WebSocket controla DSP. Status: implementado em build; pendente validacao no browser.
- `v0.1.0-dev.8`: botao fisico + radio-silent default + timeouts do AP validados.
- `v0.1.0`: MVP funcional validado em bancada.

## 16. Primeiro proximo passo recomendado

Fases 1, 2, 3, 4, 6, 7, 9, 10, 11 e 12 estao parcialmente ou totalmente compilando. Seguir pelo bring-up em hardware:

1. Flashar o firmware no ESP32-S3 reference build.
2. Confirmar logs de boot, init I2S, DSP e TinyUSB.
3. Validar enumeracao como microfone no host.
4. Validar captura real do ICS-43434 e contadores de underrun/drop.
5. Ajustar descriptor UAC, pinagem ou timing conforme evidencia de bancada.
6. Refatorar AP aberto de desenvolvimento para WPA2 on-demand com botao fisico, inactivity timeout e hard cap.

Isso transforma a base compilavel em evidencia fisica antes de aprofundar captive DNS, pitch real e refinos de UX, sem manter o radio ativo como estado normal do produto.
