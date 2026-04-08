import initCloudsModule from '../wasm/clouds_wasm_engine.js';

const DSP_RATE = 32000;
const BLOCK = 32;
const METER_UPDATE_HZ = 30;
const WORKLET_BASE_URL = import.meta.url.replace(/[^/]*$/, '');

class CloudsProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ready = false;
    this.playing = false;
    this.bypass = false;
    this.wetDry = 1;
    this.browserRate = sampleRate;
    this.playhead = 0;
    this.sourceL = null;
    this.sourceR = null;
    this.pendingTrigger = false;
    this.params = {
      position: 0.5, size: 0.5, pitch: 0.5, density: 0.5, texture: 0.5,
      dry_wet: 1, stereo_spread: 0.5, feedback: 0.2, reverb: 0.2,
      freeze: false, gate: false, trigger: false, playback_mode: 0, quality: 0
    };
    this.inputMeter = 0;
    this.outputMeter = 0;
    this.meterUpdateIntervalSamples = Math.max(1, Math.floor(this.browserRate / METER_UPDATE_HZ));
    this.samplesSinceMeterUpdate = 0;
    this.bufferCapacity = 0;
    this.dryL = new Float32Array(0);
    this.dryR = new Float32Array(0);
    this.wetL = new Float32Array(0);
    this.wetR = new Float32Array(0);
    this.loadingWasm = false;
    this.port.onmessage = (event) => this.onMessage(event.data);
    this.loadWasm();
  }

  ensureBufferCapacity(requiredFrames) {
    if (requiredFrames <= this.bufferCapacity) {
      return;
    }

    const nextCapacity = Math.max(requiredFrames, this.bufferCapacity ? this.bufferCapacity * 2 : BLOCK);
    this.dryL = new Float32Array(nextCapacity);
    this.dryR = new Float32Array(nextCapacity);
    this.wetL = new Float32Array(nextCapacity);
    this.wetR = new Float32Array(nextCapacity);
    this.bufferCapacity = nextCapacity;
  }

  async loadWasm(wasmBinary = null) {
    if (this.ready || this.loadingWasm) {
      return;
    }

    this.loadingWasm = true;

    try {
      let resolvedBinary = wasmBinary;

      if (!resolvedBinary) {
        if (typeof fetch !== 'function') {
          this.port.postMessage({ type: 'request-wasm' });
          return;
        }

        const wasmUrl = `${WORKLET_BASE_URL}../wasm/clouds_wasm_engine.wasm`;
        resolvedBinary = await fetch(wasmUrl).then((response) => {
          if (!response.ok) {
            throw new Error(`Unable to load WASM binary: ${response.status} ${response.statusText}`);
          }
          return response.arrayBuffer();
        });
      }

      this.module = await initCloudsModule({
        wasmBinary: resolvedBinary,
        locateFile: (p) => `${WORKLET_BASE_URL}../wasm/${p}`
      });
      this.engine = this.module._clouds_create_engine();
      this.module._clouds_init_engine(this.engine, DSP_RATE, BLOCK);
      this.inPtr = this.module._malloc(BLOCK * 2 * 4);
      this.outPtr = this.module._malloc(BLOCK * 2 * 4);
      this.ready = true;
      this.port.postMessage({ type: 'ready' });
    } catch (error) {
      this.port.postMessage({
        type: 'error',
        message: error instanceof Error ? error.message : String(error)
      });
    } finally {
      this.loadingWasm = false;
    }
  }

  onMessage(msg) {
    if (msg.type === 'load-buffer') {
      this.sourceL = msg.left;
      this.sourceR = msg.right ?? msg.left;
      this.playhead = 0;
    } else if (msg.type === 'transport') {
      this.playing = msg.playing;
      if (typeof msg.position === 'number') this.playhead = Math.floor(msg.position * (this.sourceL?.length ?? 0));
    } else if (msg.type === 'params') {
      this.params = { ...this.params, ...msg.params };
      this.bypass = !!msg.params.bypass;
      this.wetDry = this.params.dry_wet;
    } else if (msg.type === 'trigger') {
      this.pendingTrigger = true;
    } else if (msg.type === 'reset') {
      if (this.ready) this.module._clouds_reset_state(this.engine);
    } else if (msg.type === 'wasm-binary') {
      this.loadWasm(msg.wasmBinary);
    }
  }

  process(_, outputs) {
    const output = outputs[0];
    const outL = output[0];
    const outR = output[1] ?? output[0];
    outL.fill(0);
    outR.fill(0);

    if (!this.sourceL || !this.playing) return true;

    const ratioIn = this.browserRate / DSP_RATE;
    const ratioOut = DSP_RATE / this.browserRate;
    const dspFrames = Math.max(BLOCK, Math.ceil(outL.length * ratioOut));
    this.ensureBufferCapacity(dspFrames);
    const dryL = this.dryL;
    const dryR = this.dryR;

    for (let i = 0; i < dspFrames; i++) {
      const srcIndex = this.playhead + Math.floor(i * ratioIn);
      const l = srcIndex < this.sourceL.length ? this.sourceL[srcIndex] : 0;
      const r = srcIndex < this.sourceR.length ? this.sourceR[srcIndex] : l;
      dryL[i] = l;
      dryR[i] = r;
      this.inputMeter = Math.max(this.inputMeter * 0.95, Math.abs(l), Math.abs(r));
    }
    this.playhead += Math.floor(outL.length);

    const wetL = this.wetL;
    const wetR = this.wetR;

    if (this.ready) {
      for (const [id, k] of [['position',0],['size',1],['pitch',2],['density',3],['texture',4],['dry_wet',5],['stereo_spread',6],['feedback',7],['reverb',8]]) {
        this.module._clouds_set_parameter(this.engine, k, this.params[id]);
      }
      this.module._clouds_set_playback_mode(this.engine, this.params.playback_mode | 0);
      this.module._clouds_set_quality(this.engine, this.params.quality | 0);
      this.module._clouds_set_freeze(this.engine, !!this.params.freeze);
      this.module._clouds_set_gate(this.engine, !!this.params.gate);
      if (this.pendingTrigger || this.params.trigger) this.module._clouds_trigger_pulse(this.engine);
      this.pendingTrigger = false;

      const heap = this.module.HEAPF32;
      let done = 0;
      while (done < dspFrames) {
        const n = Math.min(BLOCK, dspFrames - done);
        for (let i = 0; i < BLOCK; i++) {
          heap[(this.inPtr >> 2) + i * 2] = i < n ? dryL[done + i] : 0;
          heap[(this.inPtr >> 2) + i * 2 + 1] = i < n ? dryR[done + i] : 0;
        }
        this.module._clouds_process_interleaved(this.engine, this.inPtr, this.outPtr, BLOCK);
        for (let i = 0; i < n; i++) {
          wetL[done + i] = heap[(this.outPtr >> 2) + i * 2];
          wetR[done + i] = heap[(this.outPtr >> 2) + i * 2 + 1];
        }
        done += n;
      }
    }

    for (let i = 0; i < outL.length; i++) {
      const pos = Math.min(dspFrames - 1, Math.floor(i * ratioOut));
      const dryl = dryL[pos];
      const dryr = dryR[pos];
      const wetl = wetL[pos];
      const wetr = wetR[pos];
      const mix = this.bypass ? 0 : this.wetDry;
      outL[i] = dryl * (1 - mix) + wetl * mix;
      outR[i] = dryr * (1 - mix) + wetr * mix;
      this.outputMeter = Math.max(this.outputMeter * 0.95, Math.abs(outL[i]), Math.abs(outR[i]));
    }

    this.samplesSinceMeterUpdate += outL.length;
    if (this.samplesSinceMeterUpdate >= this.meterUpdateIntervalSamples) {
      this.samplesSinceMeterUpdate = 0;
      this.port.postMessage({ type: 'meter', input: this.inputMeter, output: this.outputMeter, playhead: this.playhead });
    }
    return true;
  }
}

registerProcessor('clouds-processor', CloudsProcessor);
