import React from 'react';
import { computeRunMetrics, formatMetricValue } from './runMetrics';

type RunSummaryPanelProps = {
  stepsLength: number;
  metrics: ReturnType<typeof computeRunMetrics>;
};

type MetricRow = {
  label: string;
  value: string;
};

function buildRows(metrics: NonNullable<ReturnType<typeof computeRunMetrics>>): MetricRow[] {
  return [
    { label: 'Total steps', value: formatMetricValue(metrics.totalSteps) },
    { label: 'Total trades', value: formatMetricValue(metrics.totalTrades) },
    { label: 'Total traded quantity', value: formatMetricValue(metrics.totalTradedQuantity) },
    { label: 'Steps with trades', value: formatMetricValue(metrics.stepsWithTrades) },
    { label: 'Final resting orders', value: formatMetricValue(metrics.finalRestingOrders) },
    { label: 'Final resting quantity', value: formatMetricValue(metrics.finalRestingQuantity) },
    { label: 'Final best bid', value: formatMetricValue(metrics.finalBestBid) },
    { label: 'Final best ask', value: formatMetricValue(metrics.finalBestAsk) },
    { label: 'Final spread', value: formatMetricValue(metrics.finalSpread) },
    { label: 'Min spread seen', value: formatMetricValue(metrics.minSpread) },
    { label: 'Max spread seen', value: formatMetricValue(metrics.maxSpread) },
    { label: 'Max resting quantity seen', value: formatMetricValue(metrics.maxRestingQuantity) },
    { label: 'Max active orders seen', value: formatMetricValue(metrics.maxActiveOrders) },
  ];
}

export const RunSummaryPanel: React.FC<RunSummaryPanelProps> = ({ stepsLength, metrics }) => {
  const rows = metrics ? buildRows(metrics) : [];

  return (
    <section className="panel run-summary" aria-label="Run summary">
      <div className="panel-head">
        <div className="panel-title">Run summary</div>
        <div className="panel-meta">Informational — derived from loaded replay steps, not a benchmark</div>
      </div>
      <p className="run-summary-disclaimer">
        These figures are computed in the browser from <code>schemaVersion: 1</code> visualisation
        records only. They are for education and debugging, not Release throughput or latency.
        See <code>docs/BENCHMARKING.md</code> in the repo for real benchmark methodology.
      </p>
      {stepsLength === 0 || !metrics ? (
        <div className="empty-state">
          Load a scenario, NDJSON file, or live stream to see run summary metrics.
        </div>
      ) : (
        <dl className="run-summary-grid">
          {rows.map((row) => (
            <div key={row.label} className="run-summary-item">
              <dt>{row.label}</dt>
              <dd>{row.value}</dd>
            </div>
          ))}
        </dl>
      )}
    </section>
  );
};
