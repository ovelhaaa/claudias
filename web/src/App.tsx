import { useEffect, useMemo, useRef, useState } from 'react';
import { CloudsAudioEngine } from './audio/CloudsAudioEngine';
import { SliderControl } from './components/SliderControl';
import { ModulationPanel } from './components/ModulationPanel';
import { ModulationMatrix, defaultSources } from './modulation/modulation';
import type { EngineParams, PlaybackMode, QualityMode } from './types/audio';

const defaultParams: EngineParams = {
  position: 0.5, size: 0.5, pitch: 0.5, density: 0.5, texture: 0.5,
  dry_wet: 1, stereo_spread: 0.5, feedback: 0.2, reverb: 0.2
};

export function App() {
  const engine = useMemo(() => new CloudsAudioEngine(), []);
  const matrix = useMemo(() => new ModulationMatrix(), []);
  const [ready, setReady] = useState(false);
  const [playing, setPlaying] = useState(false);
  const [params, setParams] = useState<EngineParams>(defaultParams);
  const [sources, setSources] = useState(defaultSources);
  const [playbackMode, setPlaybackMode] = useState<PlaybackMode>('GRANULAR');
  const [quality, setQuality] = useState<QualityMode>('STEREO_16');
  const [freeze, setFreeze] = useState(false);
  const [gate, setGate] = useState(false);
  const [bypass, setBypass] = useState(false);
  const [meter, setMeter] = useState({ input: 0, output: 0, playhead: 0 });
  const lastTick = useRef(performance.now());
  const stateRef = useRef({ params, sources, meter, freeze, gate, playbackMode, quality, bypass });

  stateRef.current = { params, sources, meter, freeze, gate, playbackMode, quality, bypass };

  useEffect(() => {
    engine.init((m) => setMeter(m)).then(() => setReady(true));
  }, [engine]);

  useEffect(() => {
    const id = setInterval(() => {
      const {
        params: paramsRef,
        sources: sourcesRef,
        meter: meterRef,
        freeze: freezeRef,
        gate: gateRef,
        playbackMode: playbackModeRef,
        quality: qualityRef,
        bypass: bypassRef
      } = stateRef.current;
      const now = performance.now();
      const dt = (now - lastTick.current) / 1000;
      lastTick.current = now;
      const frame = matrix.process(
        paramsRef,
        sourcesRef,
        dt,
        meterRef.input,
        meterRef.output,
        { freeze: freezeRef, gate: gateRef, trigger: false }
      );
      engine.setParams({
        ...frame.params,
        freeze: frame.freeze,
        gate: frame.gate,
        trigger: frame.trigger,
        playback_mode: playbackModeRef,
        quality: qualityRef,
        bypass: bypassRef
      });
    }, 20);
    return () => clearInterval(id);
  }, [engine, matrix]);

  return (
    <div className="app">
      <h1>Clouds Web Preview (WASM)</h1>
      <div className="toolbar">
        <button disabled={!ready} onClick={() => engine.resume()}>Start Audio</button>
        <input type="file" accept="audio/*,.wav,.aif,.aiff" onChange={(e) => e.target.files?.[0] && engine.loadFile(e.target.files[0])} />
        <button onClick={() => { const next = !playing; setPlaying(next); engine.transport(next); }}>{playing ? 'Pause' : 'Play'}</button>
        <button onClick={() => { engine.transport(true, 0); }}>Scrub Start</button>
        <button onClick={() => { setParams(defaultParams); setFreeze(false); setGate(false); engine.reset(); }}>Reset</button>
      </div>

      <div className="grid">
        {(Object.keys(defaultParams) as (keyof EngineParams)[]).map((key) => (
          <SliderControl key={key} label={key} value={params[key]} onChange={(v) => setParams((p) => ({ ...p, [key]: v }))} />
        ))}
      </div>

      <div className="toolbar">
        <label>Playback Mode
          <select value={playbackMode} onChange={(e) => setPlaybackMode(e.target.value as PlaybackMode)}>
            <option>GRANULAR</option><option>STRETCH</option><option>LOOPING_DELAY</option><option>SPECTRAL</option>
          </select>
        </label>
        <label>Quality
          <select value={quality} onChange={(e) => setQuality(e.target.value as QualityMode)}>
            <option>STEREO_16</option><option>MONO_16</option><option>STEREO_8</option><option>MONO_8</option>
          </select>
        </label>
        <label><input type="checkbox" checked={bypass} onChange={(e) => setBypass(e.target.checked)} />Bypass</label>
        <label><input type="checkbox" checked={freeze} onChange={(e) => setFreeze(e.target.checked)} />Freeze</label>
        <label><input type="checkbox" checked={gate} onChange={(e) => setGate(e.target.checked)} />Gate</label>
        <button onClick={() => engine.trigger()}>Trigger Pulse</button>
      </div>

      <ModulationPanel sources={sources} onChange={setSources} />

      <div className="meters">
        <div>Input meter: {meter.input.toFixed(3)}</div>
        <div>Output meter: {meter.output.toFixed(3)}</div>
      </div>
    </div>
  );
}
