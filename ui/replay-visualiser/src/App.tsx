import React, { useMemo, useState } from 'react';

type Trade = {
  price: number;
  quantity: number;
  aggressiveOrderId?: number;
  restingOrderId?: number;
};

type Level = {
  price: number;
  quantity: number;
};

type ReplayStep = {
  schemaVersion?: number;
  index: number;
  commandType: string;
  side: string;
  orderType: string;
  orderId: number;
  price: number;
  quantity: number;
  symbol?: string;
  bestBid: number | null;
  bestAsk: number | null;
  spread: number | null;
  restingBidLevels: Level[];
  restingAskLevels: Level[];
  trades: Trade[];
  totalRestingOrders: number;
  totalRestingQuantity: number;
};

type Source =
  | { kind: 'none' }
  | { kind: 'scenario'; label: string; file: string }
  | { kind: 'file'; name: string };

type Scenario = { label: string; file: string };

function parseNdjson(text: string): ReplayStep[] {
  const lines = text.split('\n');
  const steps: ReplayStep[] = [];
  for (let i = 0; i < lines.length; i += 1) {
    const raw = lines[i].trim();
    if (raw.length === 0) continue;
    try {
      steps.push(JSON.parse(raw) as ReplayStep);
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      throw new Error(`Invalid NDJSON at line ${i + 1}: ${message}`);
    }
  }
  return steps.sort((a, b) => a.index - b.index);
}

function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value));
}

function formatNullableNumber(value: number | null | undefined): string {
  if (value === null || value === undefined) return '—';
  return String(value);
}

function buildSeriesPoints(
  steps: ReplayStep[],
  getY: (s: ReplayStep) => number | null,
): Array<{ x: number; y: number }> {
  const points: Array<{ x: number; y: number }> = [];
  for (let i = 0; i < steps.length; i += 1) {
    const y = getY(steps[i]);
    if (y === null) continue;
    points.push({ x: i, y });
  }
  return points;
}

function toPolyline(
  points: Array<{ x: number; y: number }>,
  width: number,
  height: number,
  padding: number,
): string {
  if (points.length === 0) return '';
  const xs = points.map((p) => p.x);
  const ys = points.map((p) => p.y);
  const xMin = Math.min(...xs);
  const xMax = Math.max(...xs);
  const yMin = Math.min(...ys);
  const yMax = Math.max(...ys);

  const innerW = Math.max(1, width - padding * 2);
  const innerH = Math.max(1, height - padding * 2);

  const xDen = Math.max(1, xMax - xMin);
  const yDen = Math.max(1, yMax - yMin);

  return points
    .map((p) => {
      const x = padding + ((p.x - xMin) / xDen) * innerW;
      const y = padding + (1 - (p.y - yMin) / yDen) * innerH;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');
}

export const App: React.FC = () => {
  const [steps, setSteps] = useState<ReplayStep[]>([]);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [source, setSource] = useState<Source>({ kind: 'none' });
  const scenarios: Scenario[] = useMemo(
    () => [
      { label: 'Basic replay', file: '/sample-replay.ndjson' },
      { label: 'Deep book', file: '/sample-deep-book.ndjson' },
      { label: 'Crossing trades', file: '/sample-crossing-trades.ndjson' },
      { label: 'Spread movement', file: '/sample-spread-movement.ndjson' },
    ],
    [],
  );
  const [selectedScenarioFile, setSelectedScenarioFile] = useState<string>(
    scenarios[0].file,
  );

  const safeIndex = steps.length > 0 ? clamp(currentIndex, 0, steps.length - 1) : 0;
  const current = steps.length > 0 ? steps[safeIndex] : null;

  React.useEffect(() => {
    if (steps.length === 0) {
      if (currentIndex !== 0) setCurrentIndex(0);
      if (isPlaying) setIsPlaying(false);
      return;
    }
    if (currentIndex >= steps.length) {
      setCurrentIndex(steps.length - 1);
    }
  }, [steps.length, currentIndex, isPlaying]);

  const tradesSoFar = useMemo(() => {
    const out: Array<{ stepIndex: number; trade: Trade }> = [];
    for (const step of steps.slice(0, currentIndex + 1)) {
      for (const trade of step.trades) {
        out.push({ stepIndex: step.index, trade });
      }
    }
    return out;
  }, [steps, currentIndex]);

  React.useEffect(() => {
    if (!isPlaying || steps.length === 0) {
      return;
    }
    const handle = window.setInterval(() => {
      setCurrentIndex((idx) => {
        if (idx + 1 >= steps.length) {
          setIsPlaying(false);
          return idx;
        }
        return idx + 1;
      });
    }, 200);
    return () => window.clearInterval(handle);
  }, [isPlaying, steps.length]);

  const onFileChange: React.ChangeEventHandler<HTMLInputElement> = async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    try {
      const text = await file.text();
      const parsed = parseNdjson(text);
      setSteps(parsed);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(parsed.length === 0 ? 'File contains no replay steps.' : null);
      setSource({ kind: 'file', name: file.name });
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setSteps([]);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(message);
      setSource({ kind: 'file', name: file.name });
    }
  };

  const loadScenario = async (file: string) => {
    try {
      const res = await fetch(file);
      if (!res.ok) {
        throw new Error(`Failed to fetch replay (HTTP ${res.status})`);
      }
      const text = await res.text();
      const parsed = parseNdjson(text);
      setSteps(parsed);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(parsed.length === 0 ? 'Replay contains no steps.' : null);
      const label = scenarios.find((s) => s.file === file)?.label ?? 'Scenario';
      setSource({ kind: 'scenario', label, file });
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setSteps([]);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(message);
      const label = scenarios.find((s) => s.file === file)?.label ?? 'Scenario';
      setSource({ kind: 'scenario', label, file });
    }
  };

  const bestBid = current?.bestBid ?? null;
  const bestAsk = current?.bestAsk ?? null;
  const spread = current?.spread ?? null;
  const symbol = current?.symbol ?? steps.find((s) => s.symbol)?.symbol ?? null;

  const bidLevels = current?.restingBidLevels ?? [];
  const askLevels = current?.restingAskLevels ?? [];
  const maxBidQty = Math.max(1, ...bidLevels.map((l) => l.quantity));
  const maxAskQty = Math.max(1, ...askLevels.map((l) => l.quantity));

  const chartWidth = 640;
  const chartHeight = 140;
  const chartPadding = 10;
  const bidSeries = useMemo(
    () => buildSeriesPoints(steps, (s) => s.bestBid),
    [steps],
  );
  const askSeries = useMemo(
    () => buildSeriesPoints(steps, (s) => s.bestAsk),
    [steps],
  );
  const bidPolyline = useMemo(
    () => toPolyline(bidSeries, chartWidth, chartHeight, chartPadding),
    [bidSeries],
  );
  const askPolyline = useMemo(
    () => toPolyline(askSeries, chartWidth, chartHeight, chartPadding),
    [askSeries],
  );

  const loadedLabel =
    source.kind === 'none'
      ? 'No replay loaded'
      : source.kind === 'scenario'
        ? source.label
        : `File: ${source.name}`;

  return (
    <div className="app">
      <header className="topbar">
        <div className="topbar-left">
          <div className="title">
            <div className="title-main">Order Book Replay Visualiser</div>
            <div className="title-sub">C++ Matching Engine Replay</div>
          </div>
          <div className="pills">
            <span className="pill">{loadedLabel}</span>
            <span className="pill">
              Step {steps.length === 0 ? '—' : safeIndex + 1} / {steps.length === 0 ? '—' : steps.length}
            </span>
            <span className="pill">
              Symbol {symbol ?? '—'}
            </span>
          </div>
        </div>

        <div className="topbar-right">
          <div className="load-controls">
            <label className="select-wrap">
              <span className="select-label">Scenario</span>
              <select
                className="select"
                value={selectedScenarioFile}
                onChange={(e) => setSelectedScenarioFile(e.target.value)}
              >
                {scenarios.map((s) => (
                  <option key={s.file} value={s.file}>
                    {s.label}
                  </option>
                ))}
              </select>
            </label>
            <button
              type="button"
              className="btn btn-primary"
              onClick={() => loadScenario(selectedScenarioFile)}
            >
              Load
            </button>
            <label className="file-input btn">
              Load NDJSON
              <input
                type="file"
                accept=".ndjson,application/x-ndjson,application/jsonl,text/plain"
                onChange={onFileChange}
              />
            </label>
          </div>
        </div>
      </header>

      <section className="controls-bar">
        <div className="controls">
          <button
          type="button"
          className="btn"
          onClick={() => {
            setIsPlaying(false);
            setCurrentIndex(0);
          }}
          disabled={steps.length === 0}
        >
          Reset
          </button>
          <button
          type="button"
          className="btn"
          onClick={() => {
            setIsPlaying(false);
            setCurrentIndex((idx) => Math.max(0, idx - 1));
          }}
          disabled={steps.length === 0 || currentIndex === 0}
        >
          Step back
          </button>
          <button
          type="button"
          className="btn"
          onClick={() => {
            setIsPlaying(false);
            setCurrentIndex((idx) => Math.min(steps.length - 1, idx + 1));
          }}
          disabled={steps.length === 0 || currentIndex >= steps.length - 1}
        >
          Step forward
          </button>
          <button
          type="button"
          className="btn btn-primary"
          onClick={() => setIsPlaying((v) => !v)}
          disabled={steps.length === 0}
        >
          {isPlaying ? 'Pause' : 'Play'}
          </button>
        </div>
        <div className="now">
          <div className="now-label">Current command</div>
          <div className="now-value">
            {current
              ? `${current.commandType.toUpperCase()} ${current.side.toUpperCase()} ${current.orderType.toUpperCase()} @ ${current.price} × ${current.quantity}`
              : '—'}
          </div>
        </div>
      </section>

      <main className="dashboard">
        {loadError && (
          <section className="panel error">
            <div className="panel-title">Load error</div>
            <div className="error-text">{loadError}</div>
          </section>
        )}
        <section className="cards">
          <div className="card">
            <div className="card-label">Best bid</div>
            <div className="card-value">{formatNullableNumber(bestBid)}</div>
          </div>
          <div className="card">
            <div className="card-label">Best ask</div>
            <div className="card-value">{formatNullableNumber(bestAsk)}</div>
          </div>
          <div className="card">
            <div className="card-label">Spread</div>
            <div className="card-value">{formatNullableNumber(spread)}</div>
          </div>
          <div className="card">
            <div className="card-label">Resting orders</div>
            <div className="card-value">{current ? String(current.totalRestingOrders) : '—'}</div>
          </div>
          <div className="card">
            <div className="card-label">Resting quantity</div>
            <div className="card-value">{current ? String(current.totalRestingQuantity) : '—'}</div>
          </div>
          <div className="card">
            <div className="card-label">Trades so far</div>
            <div className="card-value">{String(tradesSoFar.length)}</div>
          </div>
        </section>

        <section className="panel chart">
          <div className="panel-head">
            <div className="panel-title">Top of book</div>
            <div className="panel-meta">Bid/ask over time (per replay step)</div>
          </div>
          <div className="chart-wrap">
            {steps.length === 0 ? (
              <div className="empty-state">Load a replay to view the timeline.</div>
            ) : (
              <svg
                width="100%"
                viewBox={`0 0 ${chartWidth} ${chartHeight}`}
                preserveAspectRatio="none"
                className="chart-svg"
                role="img"
                aria-label="Top of book timeline"
              >
                <defs>
                  <linearGradient id="gBid" x1="0" x2="0" y1="0" y2="1">
                    <stop offset="0%" stopColor="rgba(34,197,94,0.35)" />
                    <stop offset="100%" stopColor="rgba(34,197,94,0.05)" />
                  </linearGradient>
                  <linearGradient id="gAsk" x1="0" x2="0" y1="0" y2="1">
                    <stop offset="0%" stopColor="rgba(56,189,248,0.30)" />
                    <stop offset="100%" stopColor="rgba(56,189,248,0.05)" />
                  </linearGradient>
                </defs>

                <rect x="0" y="0" width={chartWidth} height={chartHeight} fill="rgba(2,6,23,1)" />
                <g opacity="0.5">
                  {Array.from({ length: 8 }).map((_, i) => (
                    <line
                      key={i}
                      x1={(chartWidth / 8) * i}
                      y1={0}
                      x2={(chartWidth / 8) * i}
                      y2={chartHeight}
                      stroke="rgba(17,24,39,1)"
                      strokeWidth="1"
                    />
                  ))}
                </g>

                {bidPolyline && (
                  <polyline points={bidPolyline} fill="none" stroke="rgba(34,197,94,0.95)" strokeWidth="2" />
                )}
                {askPolyline && (
                  <polyline points={askPolyline} fill="none" stroke="rgba(56,189,248,0.95)" strokeWidth="2" />
                )}

                <line
                  x1={chartPadding + (safeIndex / Math.max(1, steps.length - 1)) * (chartWidth - chartPadding * 2)}
                  y1={0}
                  x2={chartPadding + (safeIndex / Math.max(1, steps.length - 1)) * (chartWidth - chartPadding * 2)}
                  y2={chartHeight}
                  stroke="rgba(148,163,184,0.55)"
                  strokeWidth="1.5"
                />
              </svg>
            )}
          </div>
        </section>

        <section className="panel ladder">
          <div className="panel-head">
            <div className="panel-title">Order book ladder</div>
            <div className="panel-meta">Top levels (current step)</div>
          </div>
          <div className="ladder-grid">
            <div>
              <div className="subhead">Bids</div>
              <table className="book-table book-bids">
                <thead>
                  <tr>
                    <th>Price</th>
                    <th>Qty</th>
                  </tr>
                </thead>
                <tbody>
                  {bidLevels.map((lvl) => {
                    const pct = Math.round((lvl.quantity / maxBidQty) * 100);
                    return (
                    <tr
                      key={lvl.price}
                      className={
                        bestBid !== null && lvl.price === bestBid
                          ? 'best'
                          : undefined
                      }
                    >
                      <td>{lvl.price}</td>
                      <td className="qty-cell">
                        <div className="qty-bar bid" style={{ width: `${pct}%` }} />
                        <span className="qty-text">{lvl.quantity}</span>
                      </td>
                    </tr>
                    );
                  })}
                  {bidLevels.length === 0 && (
                    <tr>
                      <td colSpan={2} className="empty">
                        No bids
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
            <div>
              <div className="subhead">Asks</div>
              <table className="book-table book-asks">
                <thead>
                  <tr>
                    <th>Price</th>
                    <th>Qty</th>
                  </tr>
                </thead>
                <tbody>
                  {askLevels.map((lvl) => {
                    const pct = Math.round((lvl.quantity / maxAskQty) * 100);
                    return (
                    <tr
                      key={lvl.price}
                      className={
                        bestAsk !== null && lvl.price === bestAsk
                          ? 'best'
                          : undefined
                      }
                    >
                      <td>{lvl.price}</td>
                      <td className="qty-cell">
                        <div className="qty-bar ask" style={{ width: `${pct}%` }} />
                        <span className="qty-text">{lvl.quantity}</span>
                      </td>
                    </tr>
                    );
                  })}
                  {askLevels.length === 0 && (
                    <tr>
                      <td colSpan={2} className="empty">
                        No asks
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        </section>

        <section className="panel trades">
          <div className="panel-head">
            <div className="panel-title">Trade tape</div>
            <div className="panel-meta">Recent executed trades</div>
          </div>
          <table className="trade-table">
            <thead>
              <tr>
                <th>Index</th>
                <th>Price</th>
                <th>Qty</th>
              </tr>
            </thead>
            <tbody>
              {tradesSoFar.slice(-50).map(({ stepIndex, trade }, idx) => (
                <tr key={`${stepIndex}-${idx}`}>
                  <td>{stepIndex}</td>
                  <td>{trade.price}</td>
                  <td>{trade.quantity}</td>
                </tr>
              ))}
              {tradesSoFar.length === 0 && (
                <tr>
                  <td colSpan={3} className="empty">
                    No trades yet
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </section>
      </main>
    </div>
  );
};

