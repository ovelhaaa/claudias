import type { EngineParams, ModDestination } from '../types/audio';

export type SourceType = 'constant' | 'lfo_sine' | 'lfo_triangle' | 'lfo_square' | 'envelope' | 'gate' | 'trigger';

export interface ModSource {
  id: string;
  label: string;
  type: SourceType;
  enabled: boolean;
  amount: number;
  offset: number;
  rateHz: number;
  phase: number;
  bipolar: boolean;
  destination: ModDestination;
  value: number;
}

export interface ModFrame {
  params: EngineParams;
  freeze: boolean;
  gate: boolean;
  trigger: boolean;
  meters: { input: number; output: number };
}

const clamp01 = (v: number) => Math.max(0, Math.min(1, v));

export class ModulationMatrix {
  private readonly envAttack = 0.05;
  private readonly envRelease = 0.005;
  private env = 0;
  private gateState = false;
  private triggerTimer = 0;

  process(
    base: EngineParams,
    sources: ModSource[],
    dt: number,
    inputLevel: number,
    outputLevel: number,
    toggles: { freeze: boolean; gate: boolean; trigger: boolean }
  ): ModFrame {
    this.env += (inputLevel > this.env ? this.envAttack : this.envRelease) * (inputLevel - this.env);
    const params = { ...base };
    let freeze = toggles.freeze;
    let gate = toggles.gate;
    let trigger = toggles.trigger;

    for (const s of sources) {
      if (!s.enabled) continue;
      s.value = this.computeSourceValue(s, dt);
      const mod = s.offset + s.amount * (s.bipolar ? s.value : (s.value + 1) * 0.5);

      if (s.destination in params) {
        const key = s.destination as keyof EngineParams;
        params[key] = clamp01(params[key] + mod);
      } else if (s.destination === 'freeze') {
        freeze = mod > 0.5;
      } else if (s.destination === 'gate') {
        gate = mod > 0.5;
      } else if (s.destination === 'trigger' && mod > 0.8) {
        trigger = true;
      }
    }

    return { params, freeze, gate, trigger, meters: { input: inputLevel, output: outputLevel } };
  }

  private computeSourceValue(source: ModSource, dt: number): number {
    source.phase = (source.phase + source.rateHz * dt) % 1;
    switch (source.type) {
      case 'constant':
        return source.offset;
      case 'lfo_sine':
        return Math.sin(source.phase * Math.PI * 2);
      case 'lfo_triangle':
        return 1 - 4 * Math.abs(source.phase - 0.5);
      case 'lfo_square':
        return source.phase < 0.5 ? 1 : -1;
      case 'envelope':
        return this.env * 2 - 1;
      case 'gate':
        this.gateState = source.phase < 0.5;
        return this.gateState ? 1 : -1;
      case 'trigger':
        if (source.phase < dt * source.rateHz) this.triggerTimer = 0.03;
        this.triggerTimer = Math.max(0, this.triggerTimer - dt);
        return this.triggerTimer > 0 ? 1 : -1;
      default:
        return 0;
    }
  }
}

export const defaultSources: ModSource[] = [
  { id: 'const', label: 'Constant', type: 'constant', enabled: false, amount: 0.1, offset: 0, rateHz: 0, phase: 0, bipolar: true, destination: 'position', value: 0 },
  { id: 'lfo1', label: 'LFO Sine', type: 'lfo_sine', enabled: false, amount: 0.2, offset: 0, rateHz: 0.2, phase: 0, bipolar: true, destination: 'texture', value: 0 },
  { id: 'lfo2', label: 'LFO Tri', type: 'lfo_triangle', enabled: false, amount: 0.2, offset: 0, rateHz: 0.15, phase: 0, bipolar: true, destination: 'size', value: 0 },
  { id: 'lfo3', label: 'LFO Square', type: 'lfo_square', enabled: false, amount: 1, offset: 0, rateHz: 2, phase: 0, bipolar: false, destination: 'freeze', value: 0 },
  { id: 'env', label: 'Envelope', type: 'envelope', enabled: false, amount: 0.4, offset: 0, rateHz: 0, phase: 0, bipolar: false, destination: 'density', value: 0 },
  { id: 'gate', label: 'Gate Gen', type: 'gate', enabled: false, amount: 1, offset: 0, rateHz: 1, phase: 0, bipolar: false, destination: 'gate', value: 0 },
  { id: 'trig', label: 'Trigger Gen', type: 'trigger', enabled: false, amount: 1, offset: 0, rateHz: 1, phase: 0, bipolar: false, destination: 'trigger', value: 0 }
];
