export type PlaybackMode = 'GRANULAR' | 'STRETCH' | 'LOOPING_DELAY' | 'SPECTRAL';
export type QualityMode = 'STEREO_16' | 'MONO_16' | 'STEREO_8' | 'MONO_8';

export interface EngineParams {
  position: number;
  size: number;
  pitch: number;
  density: number;
  texture: number;
  dry_wet: number;
  stereo_spread: number;
  feedback: number;
  reverb: number;
}

export type ModDestination = keyof EngineParams | 'freeze' | 'gate' | 'trigger';
