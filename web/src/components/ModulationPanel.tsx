import type { ModDestination } from '../types/audio';
import type { ModSource } from '../modulation/modulation';

const destinations: ModDestination[] = [
  'position','size','pitch','density','texture','dry_wet','stereo_spread','feedback','reverb','freeze','gate','trigger'
];

export function ModulationPanel({ sources, onChange }: { sources: ModSource[]; onChange: (src: ModSource[]) => void }) {
  const patch = (idx: number, update: Partial<ModSource>) => {
    const next = sources.slice();
    next[idx] = { ...next[idx], ...update };
    onChange(next);
  };

  return (
    <div>
      <h3>Modulation Matrix</h3>
      {sources.map((s, i) => (
        <div key={s.id} className="mod-row">
          <label><input type="checkbox" checked={s.enabled} onChange={(e) => patch(i, { enabled: e.target.checked })} />{s.label}</label>
          <select value={s.destination} onChange={(e) => patch(i, { destination: e.target.value as ModDestination })}>
            {destinations.map((d) => <option key={d} value={d}>{d}</option>)}
          </select>
          <input type="range" min={0} max={1} step={0.01} value={s.amount} onChange={(e) => patch(i, { amount: Number(e.target.value) })} />
          <input type="range" min={0.01} max={10} step={0.01} value={s.rateHz} onChange={(e) => patch(i, { rateHz: Number(e.target.value) })} />
          <span>{s.value.toFixed(2)}</span>
        </div>
      ))}
    </div>
  );
}
