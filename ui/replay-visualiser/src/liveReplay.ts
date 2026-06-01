import type { ReplayStep } from './replayTypes';

export const DEFAULT_STREAM_URL = 'http://127.0.0.1:9000/stream';

export type LiveConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

export function parseLiveReplayStep(raw: string): ReplayStep | null {
  try {
    const parsed = JSON.parse(raw) as ReplayStep;
    if (parsed.schemaVersion !== 1) {
      return null;
    }
    if (typeof parsed.index !== 'number' || !Number.isFinite(parsed.index)) {
      return null;
    }
    return parsed;
  } catch {
    return null;
  }
}

export function appendReplayStep(steps: ReplayStep[], step: ReplayStep): ReplayStep[] {
  if (steps.some((existing) => existing.index === step.index)) {
    return steps;
  }
  return [...steps, step].sort((a, b) => a.index - b.index);
}
