import React, { useMemo, useState } from 'react';

import { ReplayPage } from './ReplayPage';
import { parseArenaReplay } from './replay';
import type { ArenaReplay } from './replay';
import {
  meanEpisodeImprovementBps,
  parseArenaResult,
  validateResultReplay,
} from './result';
import type { ArenaResult, EpisodeResult } from './result';


type LoadedArtifact<T> = {
  name: string;
  value: T;
};

function formatNumber(value: number | null | undefined, digits = 3): string {
  if (value === null || value === undefined) return '—';
  return Number.isInteger(value) ? String(value) : value.toFixed(digits);
}

function percent(value: number): string {
  return `${(value * 100).toFixed(1)}%`;
}

function episodeStatusClass(episode: EpisodeResult): string {
  if (!episode.valid) return 'invalid';
  return episode.completed ? 'completed' : 'incomplete';
}

export const ResultPage: React.FC = () => {
  const [resultArtifact, setResultArtifact] =
    useState<LoadedArtifact<ArenaResult> | null>(null);
  const [replayArtifact, setReplayArtifact] =
    useState<LoadedArtifact<ArenaReplay> | null>(null);
  const [resultError, setResultError] = useState<string | null>(null);
  const [replayError, setReplayError] = useState<string | null>(null);
  const [selectedSeed, setSelectedSeed] = useState<number | null>(null);

  const loadResultText = (text: string, name: string) => {
    try {
      const result = parseArenaResult(text);
      setResultArtifact({ name, value: result });
      setReplayArtifact(null);
      setSelectedSeed(result.episodes[0].seed);
      setResultError(null);
      setReplayError(null);
    } catch (error) {
      setResultArtifact(null);
      setReplayArtifact(null);
      setSelectedSeed(null);
      setResultError(error instanceof Error ? error.message : String(error));
      setReplayError(null);
    }
  };

  const loadReplayText = (text: string, name: string) => {
    if (!resultArtifact) {
      setReplayError('Load a result JSON file before its replay.');
      return;
    }
    try {
      const replay = parseArenaReplay(text);
      validateResultReplay(resultArtifact.value, replay);
      setReplayArtifact({ name, value: replay });
      setReplayError(null);
    } catch (error) {
      setReplayArtifact(null);
      setReplayError(error instanceof Error ? error.message : String(error));
    }
  };

  const loadSampleResult = async () => {
    try {
      const response = await fetch('/arena-simple-reference.result.json');
      if (!response.ok) {
        throw new Error(`Could not load sample result: HTTP ${response.status}.`);
      }
      loadResultText(
        await response.text(),
        'arena-simple-reference.result.json',
      );
    } catch (error) {
      setResultError(error instanceof Error ? error.message : String(error));
    }
  };

  const loadSampleReplay = async () => {
    try {
      const response = await fetch('/arena-simple-reference.replay.jsonl');
      if (!response.ok) {
        throw new Error(`Could not load sample replay: HTTP ${response.status}.`);
      }
      loadReplayText(
        await response.text(),
        'arena-simple-reference.replay.jsonl',
      );
    } catch (error) {
      setReplayError(error instanceof Error ? error.message : String(error));
    }
  };

  const onResultFile: React.ChangeEventHandler<HTMLInputElement> = async (
    event,
  ) => {
    const file = event.target.files?.[0];
    if (!file) return;
    loadResultText(await file.text(), file.name);
    event.target.value = '';
  };

  const onReplayFile: React.ChangeEventHandler<HTMLInputElement> = async (
    event,
  ) => {
    const file = event.target.files?.[0];
    if (!file) return;
    loadReplayText(await file.text(), file.name);
    event.target.value = '';
  };

  const selectedEpisode = resultArtifact?.value.episodes.find(
    (episode) => episode.seed === selectedSeed,
  );
  const aggregateImprovementBps = useMemo(
    () =>
      resultArtifact
        ? meanEpisodeImprovementBps(resultArtifact.value)
        : null,
    [resultArtifact],
  );

  return (
    <main className="site-main">
      <section className="result-hero">
        <div>
          <p className="eyebrow">Local artifact inspection</p>
          <h1>Result and replay viewer</h1>
          <p>
            Import result JSON from the local evaluator, inspect aggregate and
            per-episode metrics, then attach the matching replay JSONL.
          </p>
        </div>
        <aside className="status-callout">
          <strong>Local and unverified</strong>
          <p>
            These files are generated on your machine. They are not
            server-verified and are not eligible for a trusted leaderboard.
          </p>
        </aside>
      </section>

      <section className="artifact-import-grid">
        <article className="artifact-import-card">
          <p className="eyebrow">Step 1</p>
          <h2>Import result JSON</h2>
          <p>Expected result schema: <code>1.1</code>.</p>
          <div className="load-actions">
            <button type="button" onClick={loadSampleResult}>
              Load sample result
            </button>
            <label className="file-button">
              Open result JSON
              <input
                type="file"
                accept=".json,application/json"
                onChange={onResultFile}
              />
            </label>
          </div>
          {resultArtifact && (
            <span className="artifact-ok">{resultArtifact.name}</span>
          )}
          {resultError && <span className="artifact-error">{resultError}</span>}
        </article>

        <article className="artifact-import-card">
          <p className="eyebrow">Step 2</p>
          <h2>Attach replay JSONL</h2>
          <p>
            {resultArtifact
              ? `Expected: ${resultArtifact.value.replay.artifact_name}`
              : 'Load a result first to establish compatibility metadata.'}
          </p>
          <div className="load-actions">
            <label
              className={`file-button ${!resultArtifact ? 'disabled-control' : ''}`}
            >
              Open matching replay
              <input
                type="file"
                disabled={!resultArtifact}
                accept=".jsonl,.ndjson,application/x-ndjson,text/plain"
                onChange={onReplayFile}
              />
            </label>
            {resultArtifact?.name === 'arena-simple-reference.result.json' && (
              <button type="button" onClick={loadSampleReplay}>
                Load matching sample
              </button>
            )}
          </div>
          {replayArtifact && (
            <span className="artifact-ok">
              Compatible: {replayArtifact.name}
            </span>
          )}
          {replayError && <span className="artifact-error">{replayError}</span>}
        </article>
      </section>

      {!resultArtifact && (
        <section className="empty-panel result-empty">
          Generate artifacts with the local CLI, then import the result JSON
          here. The browser does not execute strategy code.
        </section>
      )}

      {resultArtifact && (
        <>
          <section className="result-heading">
            <div>
              <div className="tag-row">
                <span className="tag">{resultArtifact.value.challenge.type}</span>
                <span className="tag">
                  result schema {resultArtifact.value.schema_version}
                </span>
                <span className="tag">
                  score {resultArtifact.value.scoring.score_version}
                </span>
              </div>
              <h2>{resultArtifact.value.challenge.title}</h2>
              <p>
                {resultArtifact.value.strategy.identifier} ·{' '}
                {resultArtifact.value.strategy.mode}
              </p>
            </div>
            <span className="unverified-pill">Unverified local result</span>
          </section>

          <section className="result-summary-grid">
            <article><span>Aggregate score</span><strong>{formatNumber(resultArtifact.value.scoring.aggregate_score, 6)}</strong></article>
            <article><span>Mean fill rate</span><strong>{percent(resultArtifact.value.aggregate_metrics.mean_fill_rate)}</strong></article>
            <article><span>Mean improvement</span><strong>{formatNumber(resultArtifact.value.aggregate_metrics.mean_completed_improvement_ticks, 6)} ticks</strong></article>
            <article><span>Mean improvement</span><strong>{formatNumber(aggregateImprovementBps, 6)} bps</strong></article>
            <article><span>Episodes</span><strong>{resultArtifact.value.evaluation.episode_count}</strong></article>
            <article><span>Replay records</span><strong>{resultArtifact.value.replay.record_count}</strong></article>
          </section>

          <section className="result-metadata-grid">
            <article className="content-card">
              <p className="eyebrow">Evaluation identity</p>
              <dl className="detail-list">
                <div><dt>Challenge</dt><dd>{resultArtifact.value.challenge.id} v{resultArtifact.value.challenge.version}</dd></div>
                <div><dt>Strategy mode</dt><dd>{resultArtifact.value.strategy.mode}</dd></div>
                <div><dt>Simulator</dt><dd>{resultArtifact.value.simulation.model}</dd></div>
                <div><dt>Generator</dt><dd>{resultArtifact.value.simulation.scenario_generator.name} v{resultArtifact.value.simulation.scenario_generator.version}</dd></div>
              </dl>
            </article>
            <article className="content-card">
              <p className="eyebrow">Replay reference</p>
              <dl className="detail-list">
                <div><dt>Artifact</dt><dd>{resultArtifact.value.replay.artifact_name}</dd></div>
                <div><dt>Schema</dt><dd>{resultArtifact.value.replay.schema_version}</dd></div>
                <div><dt>Records</dt><dd>{resultArtifact.value.replay.record_count}</dd></div>
                <div><dt>Seeds</dt><dd>{resultArtifact.value.replay.seeds.join(', ')}</dd></div>
              </dl>
            </article>
          </section>

          <section className="content-card episode-section">
            <div className="section-heading compact-heading">
              <div>
                <p className="eyebrow">Per-episode results</p>
                <h2>Select a seed to inspect</h2>
              </div>
            </div>
            <div className="episode-table-wrap">
              <table className="episode-table">
                <thead>
                  <tr>
                    <th>Seed</th>
                    <th>Status</th>
                    <th>Score</th>
                    <th>Filled</th>
                    <th>Fill rate</th>
                    <th>Avg fill</th>
                    <th>Baseline</th>
                    <th>Improvement</th>
                    <th>Events / actions / fills</th>
                  </tr>
                </thead>
                <tbody>
                  {resultArtifact.value.episodes.map((episode) => (
                    <tr
                      className={episode.seed === selectedSeed ? 'selected-row' : ''}
                      key={episode.seed}
                      onClick={() => setSelectedSeed(episode.seed)}
                    >
                      <td>{episode.seed}</td>
                      <td><span className={`episode-status ${episodeStatusClass(episode)}`}>{episode.status}</span></td>
                      <td>{formatNumber(episode.score, 6)}</td>
                      <td>{episode.filled_quantity} / {episode.target_quantity}</td>
                      <td>{percent(episode.fill_rate)}</td>
                      <td>{formatNumber(episode.average_fill_price_ticks, 3)}</td>
                      <td>{formatNumber(episode.baseline_average_fill_price_ticks, 3)}</td>
                      <td>{formatNumber(episode.improvement_ticks, 3)} ticks<br /><small>{formatNumber(episode.improvement_bps, 6)} bps</small></td>
                      <td>{episode.events_processed} / {episode.actions_submitted} / {episode.fill_count}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            {selectedEpisode?.invalid_reason && (
              <div className="episode-error">
                Invalid reason: {selectedEpisode.invalid_reason}
              </div>
            )}
          </section>

          {replayArtifact && selectedSeed !== null && (
            <section className="linked-replay-section">
              <div className="section-heading compact-heading">
                <div>
                  <p className="eyebrow">Matching replay</p>
                  <h2>Inspect seed {selectedSeed}</h2>
                </div>
                <a className="text-link" href="#/replay">
                  Open standalone replay viewer
                </a>
              </div>
              <ReplayPage
                embedded
                initialReplay={replayArtifact.value}
                initialSource={replayArtifact.name}
                initialSeed={selectedSeed}
              />
            </section>
          )}
        </>
      )}
    </main>
  );
};
