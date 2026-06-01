import type { ReplayStep } from './replayTypes';

export type RunMetrics = {
  totalSteps: number;
  totalTrades: number;
  totalTradedQuantity: number;
  stepsWithTrades: number;
  finalRestingOrders: number | null;
  finalRestingQuantity: number | null;
  finalBestBid: number | null;
  finalBestAsk: number | null;
  finalSpread: number | null;
  minSpread: number | null;
  maxSpread: number | null;
  maxRestingQuantity: number | null;
  maxActiveOrders: number | null;
};

function finiteNumber(value: unknown): number | null {
  return typeof value === 'number' && Number.isFinite(value) ? value : null;
}

export function formatMetricValue(value: number | null): string {
  if (value === null) {
    return '—';
  }
  return String(value);
}

export function computeRunMetrics(steps: ReplayStep[]): RunMetrics | null {
  if (steps.length === 0) {
    return null;
  }

  const sorted = [...steps].sort((a, b) => a.index - b.index);
  const last = sorted[sorted.length - 1];

  let totalTrades = 0;
  let totalTradedQuantity = 0;
  let stepsWithTrades = 0;
  let minSpread: number | null = null;
  let maxSpread: number | null = null;
  let maxRestingQuantity: number | null = null;
  let maxActiveOrders: number | null = null;

  for (const step of sorted) {
    const trades = Array.isArray(step.trades) ? step.trades : [];
    if (trades.length > 0) {
      stepsWithTrades += 1;
    }

    for (const trade of trades) {
      totalTrades += 1;
      const quantity = finiteNumber(trade.quantity);
      if (quantity !== null) {
        totalTradedQuantity += quantity;
      }
    }

    const spread = finiteNumber(step.spread);
    if (spread !== null) {
      minSpread = minSpread === null ? spread : Math.min(minSpread, spread);
      maxSpread = maxSpread === null ? spread : Math.max(maxSpread, spread);
    }

    const restingQty = finiteNumber(step.totalRestingQuantity);
    if (restingQty !== null) {
      maxRestingQuantity =
        maxRestingQuantity === null ? restingQty : Math.max(maxRestingQuantity, restingQty);
    }

    const activeOrders = finiteNumber(step.totalRestingOrders);
    if (activeOrders !== null) {
      maxActiveOrders =
        maxActiveOrders === null ? activeOrders : Math.max(maxActiveOrders, activeOrders);
    }
  }

  return {
    totalSteps: sorted.length,
    totalTrades,
    totalTradedQuantity,
    stepsWithTrades,
    finalRestingOrders: finiteNumber(last.totalRestingOrders),
    finalRestingQuantity: finiteNumber(last.totalRestingQuantity),
    finalBestBid: finiteNumber(last.bestBid),
    finalBestAsk: finiteNumber(last.bestAsk),
    finalSpread: finiteNumber(last.spread),
    minSpread,
    maxSpread,
    maxRestingQuantity,
    maxActiveOrders,
  };
}
