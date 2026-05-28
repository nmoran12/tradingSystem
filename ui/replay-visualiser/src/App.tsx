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
  bestBid: number | null;
  bestAsk: number | null;
  spread: number | null;
  restingBidLevels: Level[];
  restingAskLevels: Level[];
  trades: Trade[];
  totalRestingOrders: number;
  totalRestingQuantity: number;
};

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

export const App: React.FC = () => {
  const [steps, setSteps] = useState<ReplayStep[]>([]);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [loadError, setLoadError] = useState<string | null>(null);

  const current = steps[currentIndex] ?? null;

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
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setSteps([]);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(message);
    }
  };

  const loadSample = async () => {
    try {
      const res = await fetch('/sample-replay.ndjson');
      const text = await res.text();
      const parsed = parseNdjson(text);
      setSteps(parsed);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(parsed.length === 0 ? 'Sample replay contains no steps.' : null);
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setSteps([]);
      setCurrentIndex(0);
      setIsPlaying(false);
      setLoadError(message);
    }
  };

  const bestBid = current?.bestBid ?? null;
  const bestAsk = current?.bestAsk ?? null;

  return (
    <div className="app">
      <header className="app-header">
        <h1>Order Book Replay Visualiser</h1>
        <div className="load-controls">
          <button type="button" onClick={loadSample}>
            Load sample replay
          </button>
          <label className="file-input">
            Load NDJSON file
            <input type="file" accept=".ndjson,application/x-ndjson,application/jsonl,text/plain" onChange={onFileChange} />
          </label>
        </div>
      </header>

      <section className="controls">
        <button
          type="button"
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
          onClick={() => setIsPlaying((v) => !v)}
          disabled={steps.length === 0}
        >
          {isPlaying ? 'Pause' : 'Play'}
        </button>
        <span className="index-indicator">
          Step {steps.length === 0 ? '-' : currentIndex + 1} /{' '}
          {steps.length === 0 ? '-' : steps.length}
        </span>
      </section>

      <main className="layout">
        {loadError && (
          <section className="panel error">
            <h2>Load error</h2>
            <div className="error-text">{loadError}</div>
          </section>
        )}
        <section className="panel summary">
          <h2>Market summary</h2>
          <div className="summary-grid">
            <div>
              <span className="label">Best bid</span>
              <span className="value">
                {bestBid !== null ? bestBid : '—'}
              </span>
            </div>
            <div>
              <span className="label">Best ask</span>
              <span className="value">
                {bestAsk !== null ? bestAsk : '—'}
              </span>
            </div>
            <div>
              <span className="label">Spread</span>
              <span className="value">
                {current?.spread !== null ? current.spread : '—'}
              </span>
            </div>
            <div>
              <span className="label">Resting orders</span>
              <span className="value">
                {current?.totalRestingOrders ?? '—'}
              </span>
            </div>
            <div>
              <span className="label">Resting quantity</span>
              <span className="value">
                {current?.totalRestingQuantity ?? '—'}
              </span>
            </div>
            <div>
              <span className="label">Trades so far</span>
              <span className="value">{tradesSoFar.length}</span>
            </div>
          </div>
        </section>

        <section className="panel ladder">
          <h2>Order book ladder</h2>
          <div className="ladder-grid">
            <div>
              <h3>Bids</h3>
              <table>
                <thead>
                  <tr>
                    <th>Price</th>
                    <th>Qty</th>
                  </tr>
                </thead>
                <tbody>
                  {current?.restingBidLevels.map((lvl) => (
                    <tr
                      key={lvl.price}
                      className={
                        bestBid !== null && lvl.price === bestBid
                          ? 'best'
                          : undefined
                      }
                    >
                      <td>{lvl.price}</td>
                      <td>{lvl.quantity}</td>
                    </tr>
                  ))}
                  {(!current || current.restingBidLevels.length === 0) && (
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
              <h3>Asks</h3>
              <table>
                <thead>
                  <tr>
                    <th>Price</th>
                    <th>Qty</th>
                  </tr>
                </thead>
                <tbody>
                  {current?.restingAskLevels.map((lvl) => (
                    <tr
                      key={lvl.price}
                      className={
                        bestAsk !== null && lvl.price === bestAsk
                          ? 'best'
                          : undefined
                      }
                    >
                      <td>{lvl.price}</td>
                      <td>{lvl.quantity}</td>
                    </tr>
                  ))}
                  {(!current || current.restingAskLevels.length === 0) && (
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
          <h2>Trade tape</h2>
          <table>
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

