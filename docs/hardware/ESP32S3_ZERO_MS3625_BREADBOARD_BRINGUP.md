<!-- SPDX-License-Identifier: CERN-OHL-S-2.0 -->

# ESP32-S3-Zero + MS3625 Breadboard Bring-Up

Guia rapido para o primeiro teste de bancada com:

```text
Waveshare ESP32-S3-Zero
+ MS3625 I2S microphone module
+ breadboard / jumpers curtos
+ firmware ESP-IDF atual do PhonemeFree Unplugged
```

Este setup e para prototipagem inicial. O microfone oficial/preferencial da v0.1 continua sendo o ICS-43434; o MS3625 fica como modulo I2S de validacao/fallback ate termos medidas reais.

## 1. Antes De Ligar

Checklist seco, sem USB conectado:

- [ ] O modulo de microfone tem labels I2S claros: `SCK`/`BCLK`, `WS`/`LRCLK`, `SD`/`DOUT`, `VDD`/`VCC`, `GND` e `L/R` ou `SEL`.
- [ ] O ESP32-S3-Zero tem os pads/pinos `IO4`, `IO5`, `IO6`, `3V3` e `GND` acessiveis.
- [ ] O cabo USB-C e cabo de dados, nao apenas carga.
- [ ] Nenhum fio do microfone vai para `5V`.
- [ ] Nada usa `GPIO19` ou `GPIO20`; eles pertencem ao USB nativo.

Se o seu MS3625 nao tiver os labels acima, pare aqui e compare a serigrafia/foto do modulo antes de alimentar.

## 2. Ligacao Do Microfone

Use o pinout default do firmware atual:

| MS3625 I2S Module | ESP32-S3-Zero | Funcao |
| --- | --- | --- |
| `VDD` / `VCC` / `3V` | `3V3` | Alimentacao do microfone, somente 3.3 V |
| `GND` | `GND` | Terra comum |
| `SCK` / `BCLK` | `IO4` / `GPIO4` | I2S bit clock |
| `WS` / `LRCLK` | `IO5` / `GPIO5` | I2S word select |
| `SD` / `DOUT` / `DATA` | `IO6` / `GPIO6` | Dados do mic para o ESP32-S3 |
| `L/R` / `SEL` | `GND` | Selecao inicial de canal esquerdo |

Notas:

- Use fios curtos, principalmente em `SCK`, `WS` e `SD`.
- Coloque o microfone na borda da breadboard, com a porta acustica livre.
- Comece com `L/R` ou `SEL` em `GND`.
- Se o firmware iniciar mas o audio ficar sempre zerado, desligue tudo e teste `L/R`/`SEL` em `3V3`.
- Se o modulo tiver dois pinos separados chamados `L/R` e `SEL` sem documentacao clara do vendedor, nao chute. Confira o datasheet/listing antes de alimentar.

## 3. O Que Nao Usar

Evite nesta prototipagem:

- `5V` no microfone.
- `GPIO19` / `GPIO20`, porque sao USB D- / D+.
- `GPIO4`, `GPIO5` ou `GPIO6` para botao ou LED, porque ja sao I2S.
- `GPIO0`, `GPIO45` ou `GPIO46` para qualquer coisa de teste rapido, porque podem interferir em boot/strap.
- Fios longos atravessando a breadboard inteira para sinais I2S.

Se quiser deixar o botao fisico de configuracao ja preparado para trabalho futuro, use um GPIO seguro separado, por exemplo `IO7` para `GND`, esperando pull-up interno no firmware. Esse botao ainda nao e necessario para o teste atual.

## 4. Primeiro Deploy Sem Microfone

Antes de ligar o MS3625, valide que a placa grava e boota sozinha.

Use o perfil de bring-up. Ele desliga USB Audio e Wi-Fi para manter a porta serial/JTAG disponivel para logs. Isto evita que uma falha de descriptor USB Audio no Windows bloqueie o teste de hardware.

No PowerShell, a partir da raiz do repo:

```powershell
eim --do-not-track true run "idf.py --version"
eim --do-not-track true run "idf.py -B build-bringup-isolated -D SDKCONFIG=build-bringup-isolated/sdkconfig -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.bringup' build"
```

Do not omit `SDKCONFIG=build-bringup-isolated/sdkconfig`; without it, ESP-IDF writes the bring-up options into the main local `sdkconfig`.

Descubra a porta serial:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description
```

Procure algo como `USB JTAG/serial debug unit`, `USB Serial Device` ou outro dispositivo Espressif. Anote o `COMx`.

Grave e abra o monitor:

```powershell
eim --do-not-track true run "idf.py -B build-bringup-isolated -D SDKCONFIG=build-bringup-isolated/sdkconfig -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.bringup' -p COMx flash monitor"
```

Troque `COMx` pela porta real, por exemplo `COM7`.

Para sair do monitor:

```text
Ctrl + ]
```

## 5. Se A Gravacao Falhar

O ESP32-S3-Zero usa USB nativo e nao um conversor USB-UART dedicado. Se `idf.py flash` nao conseguir entrar em modo de download:

1. Desconecte o USB-C.
2. Segure `BOOT`.
3. Conecte o USB-C mantendo `BOOT` pressionado.
4. Solte `BOOT` depois que o Windows detectar a porta.
5. Rode novamente:

```powershell
eim --do-not-track true run "idf.py -B build-bringup-isolated -D SDKCONFIG=build-bringup-isolated/sdkconfig -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.bringup' -p COMx flash monitor"
```

Se a placa ja estiver conectada, a alternativa comum e:

1. Segurar `BOOT`.
2. Apertar e soltar `RESET`.
3. Soltar `BOOT`.
4. Rodar o flash de novo.

Se continuar falhando, tente trocar cabo USB-C, porta USB e fechar qualquer monitor serial aberto.

## 6. Depois Do Flash, Ligue O MS3625

1. Saia do monitor.
2. Desconecte o USB-C.
3. Faca a ligacao da tabela da secao 2.
4. Revise `VDD -> 3V3` e `GND -> GND`.
5. Conecte USB-C novamente.
6. Rode:

```powershell
eim --do-not-track true run "idf.py -B build-bringup-isolated -D SDKCONFIG=build-bringup-isolated/sdkconfig -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.bringup' -p COMx monitor"
```

Logs esperados:

```text
Hardware bring-up mode enabled: USB Audio and Wi-Fi portal disabled
I2S RX ready: port=0 bck=4 ws=5 din=6 sample_rate=16000
I2S capture started
bringup stats: i2s_samples=... i2s_window_samples=... i2s_min=... i2s_max=... i2s_peak=... i2s_mean=... i2s_rms=... i2s_dbfs=...dBFS ...
```

No modo bring-up, nao devem aparecer logs de USB Audio nem de portal Wi-Fi. Isso e intencional.

## 7. Teste De Captura I2S

Depois do boot:

1. Deixe o monitor aberto.
2. Observe a linha `bringup stats`.
3. Confirme se `i2s_samples` aumenta a cada segundo.
4. Confirme se `i2s_errors` permanece em zero.
5. Fale perto do MS3625 ou de pequenos taps na mesa perto dele.
6. Confirme se `i2s_peak`, `i2s_min` e `i2s_max` mudam quando ha som perto do modulo.

Este modo ainda nao prova audio no host. Ele prova que o ESP32-S3 esta bootando, que o driver I2S iniciou, que a task de captura esta lendo frames e que existe variacao de amplitude no sinal I2S.

Interpretacao rapida:

- `i2s_samples` subindo a aproximadamente 16000 por segundo: I2S esta rodando.
- `i2s_errors=0`: driver esta estavel.
- `i2s_window_samples` perto de 16000 com log a cada 1 segundo: a janela de diagnostico esta completa.
- `i2s_min` e `i2s_max`: extremos da janela desde o ultimo log, nao apenas do ultimo bloco I2S.
- `i2s_peak` mudando com fala/tap: microfone esta entregando sinal.
- `i2s_mean`: media da janela; use como estimativa de DC offset. Muito longe de zero em silencio pede investigacao.
- `i2s_rms`: energia media da janela. E melhor que `peak` para comparar silencio, ventilador e voz.
- `i2s_dbfs`: RMS em dBFS aproximado. Valores mais perto de 0 dBFS indicam sinal mais forte; valores muito negativos indicam sinal fraco/silencio.
- `i2s_peak` sempre perto de zero: canal errado, `SD` errado, `L/R`/`SEL` errado ou mic sem alimentacao.
- `i2s_peak` sempre muito alto, mesmo em silencio real: ruido, fio longo, GND ruim ou formato/alinhamento ainda errado.

Leitura pratica:

- Silencio real deve ter `i2s_rms` baixo e `i2s_dbfs` bem negativo.
- Ventilador direto no microfone deve aumentar `i2s_rms` e aproximar `i2s_dbfs` de 0.
- Voz perto do microfone deve elevar `i2s_rms` e `i2s_peak` claramente acima do silencio.
- `i2s_peak` perto de 32767 indica clipping/saturacao no PCM de 16 bits.

Se `i2s_samples` nao aumenta, ou se `i2s_peak` nao reage a som:

- desligue tudo e teste `L/R`/`SEL` em `3V3`;
- confira se `SD` esta no `IO6`;
- confira se `SCK` esta no `IO4`;
- confira se `WS` esta no `IO5`;
- encurte os fios I2S;
- confirme que o microfone esta em `3V3`, nao `5V`.

## 8. Estado Atual Do USB Audio No Windows

O firmware normal agora usa um caminho UAC1 mono 16 kHz / 16-bit para compatibilidade com Windows. O dispositivo deve aparecer como `UAC1 Microphone` ou `PhonemeFree Unplugged Mic`.

Para gravar o firmware normal:

```powershell
eim --do-not-track true run "idf.py build"
eim --do-not-track true run "idf.py -p COMx flash monitor"
```

Troque `COMx` pela porta real.

Historico: a tentativa UAC2 anterior no ESP32-S3-Zero em Windows apareceu como `UAC2 Microphone`, mas falhou com:

```text
Este dispositivo nao pode ser iniciado. (Codigo 10)
O intervalo especificado nao pode ser localizado na lista de intervalos.
```

Esse erro indicava problema de compatibilidade/descriptor USB Audio no firmware, nao falta do MS3625. Se a versao UAC1 ainda falhar, remova/desinstale a instancia antiga no Gerenciador de Dispositivos e reconecte o ESP32-S3-Zero, porque o Windows pode manter cache de descritores por VID/PID/serial.

## 9. Troubleshooting Rapido

| Sintoma | Primeiro teste |
| --- | --- |
| `idf.py` nao acha porta | Trocar cabo USB-C, apertar RESET, conferir Gerenciador de Dispositivos. |
| Flash nao entra | Usar BOOT durante conexao USB-C ou BOOT+RESET. |
| Boot loop | Remover o microfone e testar placa sozinha. |
| Placa esquenta | Desligar imediatamente e revisar 3V3/GND/5V. |
| Mic sempre zero | Trocar `L/R`/`SEL` de `GND` para `3V3`, revisar `SD`. |
| Mic so ruido | Encurtar fios, revisar GND, tirar sinais I2S de trilhas longas da breadboard. |
| `UAC1 Microphone` aparece com Codigo 10 | Desinstalar a instancia antiga no Gerenciador de Dispositivos, reconectar e registrar o texto exato do evento. |
| `UAC2 Microphone` aparece com Codigo 10 | Firmware antigo ainda esta gravado ou o Windows esta mostrando instancia cacheada; grave o firmware UAC1 e remova a instancia antiga. |
| Audio com `tac`, picotes ou cara de telefone antigo | Verificar `usb_underruns`/`short_writes`; firmware atual usa buffer DSP maior e rampa em underrun para reduzir bordas duras. |
| Audio some quando Wi-Fi liga | Registrar como risco conhecido; v0.1 ainda precisa refatorar AP para on-demand WPA2. |

## 10. Criterio De Sucesso Inicial

Este bring-up passa quando:

- [ ] O build `build-bringup-isolated` conclui.
- [ ] `idf.py -B build-bringup-isolated ... flash monitor` grava o ESP32-S3-Zero.
- [ ] O log mostra `Hardware bring-up mode enabled`.
- [ ] O log mostra I2S em `bck=4 ws=5 din=6`.
- [ ] `i2s_samples` aumenta continuamente.
- [ ] `i2s_errors` fica em zero.
- [ ] `i2s_window_samples` fica perto de 16000 por segundo.
- [ ] `i2s_peak`, `i2s_rms` e `i2s_dbfs` reagem a fala/tap perto do MS3625.
- [ ] Em silencio real, `i2s_mean` fica relativamente perto de zero e `i2s_rms` cai de forma clara.

Depois disso, o proximo passo e validar a enumeracao UAC1 no Windows e confirmar entrada de audio no painel de som/aplicativo de gravacao. Quando o ICS-43434 estiver disponivel, repita o mesmo bring-up e compare estabilidade/ruido.

## 11. Referencias

- Waveshare ESP32-S3-Zero wiki: https://www.waveshare.com/wiki/ESP32-S3-Zero
- ESP-IDF `idf.py` guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/tools/idf-py.html
- ESP-IDF flashing guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/flashing-troubleshooting.html
- TDK INMP441 datasheet, useful for INMP441-style I2S pin naming: https://product.tdk.com/system/files/dam/doc/product/sw_piezo/mic/mems-mic/data_sheet/inmp441.pdf
