# Clouds Web Preview (WASM)

## Plano curto de refatoração
1. Manter `clouds.cc` e o firmware STM32 intactos e criar um target separado em `platform/web/wasm`.
2. Reusar `dsp/granular_processor.*`, `dsp/parameters.h`, `dsp/fx/*`, `dsp/pvoc/*`, `resources.*` e `control/parameter_mapper.*` no wrapper WASM.
3. Expor API C fina (`create/init/set/process/reset`) para o browser.
4. Construir app React + TypeScript e pipeline de áudio via `AudioWorklet`.
5. Implementar modulação virtual roteável no frontend, enviando estados para o core WASM.

## Build

### Firmware STM32
```bash
make -j
```

### Build WASM (Emscripten)
```bash
cd platform/web/wasm
make clean && make
```

### App web
```bash
cd web
npm install
npm run copy-wasm
npm run dev
```

## Como usar
1. Inicie o áudio em **Start Audio**.
2. Carregue `.wav`, `.aif` ou outro formato suportado pelo navegador.
3. Use **Play/Pause**, **Scrub Start** e compare **Bypass** vs processado.
4. Ajuste parâmetros em tempo real: `position`, `size`, `pitch`, `density`, `texture`, `dry_wet`, `stereo_spread`, `feedback`, `reverb`.
5. Controle `freeze`, `gate`, `trigger`, `playback_mode` e `quality`.

## Quality mode exposto na web
A UI mapeia 1:1 para `GranularProcessor::set_quality`:
- `STEREO_16` → `0` (estéreo, 16-bit)
- `MONO_16` → `1` (mono, 16-bit)
- `STEREO_8` → `2` (estéreo, 8-bit)
- `MONO_8` → `3` (mono, 8-bit)

## Roteamento de modulação
A matriz de modulação inclui fontes:
- Constant/DC
- LFO seno
- LFO triângulo
- LFO quadrada
- Envelope follower simples
- Gate generator
- Trigger pulse generator

Cada fonte possui: enable, amount, offset, rate, phase, bipolar/unipolar, destino e visualização de valor atual.

Destinos:
- parâmetros contínuos (`position`, `size`, `pitch`, `density`, `texture`, `dry_wet`, `stereo_spread`, `feedback`, `reverb`)
- `freeze`, `gate`, `trigger`.

## Resampling / comportamento do firmware
- O núcleo DSP permanece em 32 kHz e blocos internos de 32 amostras.
- O `AudioWorklet` faz adaptação simples browser-rate ↔ 32 kHz para entrada e saída.
- O wrapper WASM converte float interleaved para `ShortFrame` internamente.
- Hot path sem alocação no lado C++ por bloco.

## Limitações conhecidas
- A simulação de CV/V-OCT no browser é aproximação do comportamento analógico.
- A precisão do resampling depende do sample rate do navegador e da implementação linear atual.
- A fonte envelope follower usa detector simples no frontend.

## Diferenças inevitáveis entre hardware e browser
- Clock e jitter do browser não replicam o STM32 em tempo real estrito.
- I/O de áudio depende da pilha WebAudio e driver do SO.
- A UI web substitui LEDs/switches físicos.

## Compatibilidade STM32 preservada
- `clouds.cc` continua como entrypoint do firmware.
- `drivers/*`, `bootloader/*`, `hardware_design/*`, `ui.*`, `settings.*` não foram portados para web.
- O DSP principal usado na web é o mesmo (`dsp/granular_processor.*` + dependências), compilado para WASM.
- O target web é adicional e isolado em `platform/web/wasm` e `web/`.
