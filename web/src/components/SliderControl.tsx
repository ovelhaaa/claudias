interface Props {
  label: string;
  value: number;
  min?: number;
  max?: number;
  step?: number;
  onChange: (value: number) => void;
}

export function SliderControl({ label, value, min = 0, max = 1, step = 0.001, onChange }: Props) {
  return (
    <label className="control">
      <span>{label}: {value.toFixed(3)}</span>
      <input type="range" min={min} max={max} step={step} value={value} onChange={(e) => onChange(Number(e.target.value))} />
    </label>
  );
}
