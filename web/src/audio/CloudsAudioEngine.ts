import type { EngineParams, PlaybackMode, QualityMode } from '../types/audio';

const playbackModeToInt: Record<PlaybackMode, number> = {
  GRANULAR: 0,
  STRETCH: 1,
  LOOPING_DELAY: 2,
  SPECTRAL: 3
};

const qualityToInt: Record<QualityMode, number> = {
  STEREO_16: 0,
  MONO_16: 1,
  STEREO_8: 2,
  MONO_8: 3
};

export class CloudsAudioEngine {
  private context?: AudioContext;
  private node?: AudioWorkletNode;

  async init(onMeter: (m: { input: number; output: number; playhead: number }) => void) {
    if (this.context) return;
    this.context = new AudioContext();
    const workletUrl = `${import.meta.env.BASE_URL}audio/clouds-processor.worklet.js`;
    await this.context.audioWorklet.addModule(workletUrl);
    this.node = new AudioWorkletNode(this.context, 'clouds-processor', { outputChannelCount: [2] });
    this.node.connect(this.context.destination);
    this.node.port.onmessage = (evt) => {
      if (evt.data.type === 'meter') onMeter(evt.data);
    };
  }

  async resume() {
    await this.context?.resume();
  }

  async loadFile(file: File) {
    if (!this.context || !this.node) return;
    const data = await file.arrayBuffer();
    const decoded = await this.context.decodeAudioData(data);
    const left = decoded.getChannelData(0).slice();
    const right = decoded.numberOfChannels > 1 ? decoded.getChannelData(1).slice() : left;
    this.node.port.postMessage({ type: 'load-buffer', left, right }, [left.buffer, right.buffer]);
  }

  transport(playing: boolean, position?: number) {
    this.node?.port.postMessage({ type: 'transport', playing, position });
  }

  setParams(params: Partial<EngineParams> & {
    bypass?: boolean;
    freeze?: boolean;
    gate?: boolean;
    trigger?: boolean;
    playback_mode?: PlaybackMode;
    quality?: QualityMode;
  }) {
    const payload: Record<string, number | boolean> = { ...params };
    if (params.playback_mode) payload.playback_mode = playbackModeToInt[params.playback_mode];
    if (params.quality) payload.quality = qualityToInt[params.quality];
    this.node?.port.postMessage({ type: 'params', params: payload });
  }

  trigger() {
    this.node?.port.postMessage({ type: 'trigger' });
  }

  reset() {
    this.node?.port.postMessage({ type: 'reset' });
  }
}
