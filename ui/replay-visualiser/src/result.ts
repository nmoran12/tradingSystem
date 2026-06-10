import type { ArenaReplay } from './replay';


export const RESULT_SCHEMA_VERSION = '1.1';

export type EpisodeResult = {
  seed: number;
  status: 'completed' | 'incomplete' | 'invalid';
  valid: boolean;
  completed: boolean;
  score: number;
  target_quantity: number;
  filled_quantity: number;
  remaining_quantity: number;
  fill_rate: number;
  average_fill_price_ticks: number | null;
  baseline_average_fill_price_ticks: number | null;
  improvement_ticks: number | null;
  improvement_bps: number | null;
  events_processed: number;
  actions_submitted: number;
  fill_count: number;
  invalid_reason: string | null;
};

export type ArenaResult = {
  schema_version: string;
  challenge: {
    id: string;
    title: string;
    version: string;
    type: string;
  };
  strategy: {
    mode: string;
    identifier: string;
  };
  simulation: {
    model: string;
    scenario_generator: {
      name: string;
      version: number;
    };
  };
  evaluation: {
    seeds: number[];
    episode_count: number;
  };
  scoring: {
    score_version: string;
    aggregate_score: number;
  };
  aggregate_metrics: {
    episode_count: number;
    mean_fill_rate: number;
    mean_completed_improvement_ticks: number | null;
  };
  episodes: EpisodeResult[];
  replay: {
    format: string;
    schema_version: string;
    artifact_name: string;
    record_count: number;
    seeds: number[];
  };
};

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function requireObject(
  value: unknown,
  field: string,
): Record<string, unknown> {
  if (!isObject(value)) {
    throw new Error(`${field} must be an object.`);
  }
  return value;
}

function requireString(value: unknown, field: string): string {
  if (typeof value !== 'string' || value.length === 0) {
    throw new Error(`${field} must be a non-empty string.`);
  }
  return value;
}

function requireNumber(value: unknown, field: string): number {
  if (typeof value !== 'number' || !Number.isFinite(value)) {
    throw new Error(`${field} must be a finite number.`);
  }
  return value;
}

function requireInteger(value: unknown, field: string): number {
  const number = requireNumber(value, field);
  if (!Number.isInteger(number)) {
    throw new Error(`${field} must be an integer.`);
  }
  return number;
}

function requireBoolean(value: unknown, field: string): boolean {
  if (typeof value !== 'boolean') {
    throw new Error(`${field} must be a boolean.`);
  }
  return value;
}

function requireNullableNumber(value: unknown, field: string): number | null {
  if (value === null) return null;
  return requireNumber(value, field);
}

function requireNullableString(value: unknown, field: string): string | null {
  if (value === null) return null;
  return requireString(value, field);
}

function requireSeeds(value: unknown, field: string): number[] {
  if (!Array.isArray(value) || value.length === 0) {
    throw new Error(`${field} must be a non-empty seed list.`);
  }
  return value.map((seed, index) =>
    requireInteger(seed, `${field}[${index}]`),
  );
}

function parseEpisode(value: unknown, index: number): EpisodeResult {
  const episode = requireObject(value, `episodes[${index}]`);
  const status = requireString(
    episode.status,
    `episodes[${index}].status`,
  );
  if (!['completed', 'incomplete', 'invalid'].includes(status)) {
    throw new Error(`episodes[${index}].status is unsupported.`);
  }
  return {
    seed: requireInteger(episode.seed, `episodes[${index}].seed`),
    status: status as EpisodeResult['status'],
    valid: requireBoolean(episode.valid, `episodes[${index}].valid`),
    completed: requireBoolean(
      episode.completed,
      `episodes[${index}].completed`,
    ),
    score: requireNumber(episode.score, `episodes[${index}].score`),
    target_quantity: requireInteger(
      episode.target_quantity,
      `episodes[${index}].target_quantity`,
    ),
    filled_quantity: requireInteger(
      episode.filled_quantity,
      `episodes[${index}].filled_quantity`,
    ),
    remaining_quantity: requireInteger(
      episode.remaining_quantity,
      `episodes[${index}].remaining_quantity`,
    ),
    fill_rate: requireNumber(
      episode.fill_rate,
      `episodes[${index}].fill_rate`,
    ),
    average_fill_price_ticks: requireNullableNumber(
      episode.average_fill_price_ticks,
      `episodes[${index}].average_fill_price_ticks`,
    ),
    baseline_average_fill_price_ticks: requireNullableNumber(
      episode.baseline_average_fill_price_ticks,
      `episodes[${index}].baseline_average_fill_price_ticks`,
    ),
    improvement_ticks: requireNullableNumber(
      episode.improvement_ticks,
      `episodes[${index}].improvement_ticks`,
    ),
    improvement_bps: requireNullableNumber(
      episode.improvement_bps,
      `episodes[${index}].improvement_bps`,
    ),
    events_processed: requireInteger(
      episode.events_processed,
      `episodes[${index}].events_processed`,
    ),
    actions_submitted: requireInteger(
      episode.actions_submitted,
      `episodes[${index}].actions_submitted`,
    ),
    fill_count: requireInteger(
      episode.fill_count,
      `episodes[${index}].fill_count`,
    ),
    invalid_reason: requireNullableString(
      episode.invalid_reason,
      `episodes[${index}].invalid_reason`,
    ),
  };
}

export function parseArenaResult(text: string): ArenaResult {
  if (text.trim().length === 0) {
    throw new Error('Result file is empty.');
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(text);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    throw new Error(`Invalid result JSON: ${message}`);
  }

  const result = requireObject(parsed, 'result');
  if (result.schema_version !== RESULT_SCHEMA_VERSION) {
    const found = String(result.schema_version ?? 'missing');
    throw new Error(
      `Unsupported result schema: expected ${RESULT_SCHEMA_VERSION}, ` +
        `found ${found}.`,
    );
  }

  const challenge = requireObject(result.challenge, 'challenge');
  const strategy = requireObject(result.strategy, 'strategy');
  const simulation = requireObject(result.simulation, 'simulation');
  const generator = requireObject(
    simulation.scenario_generator,
    'simulation.scenario_generator',
  );
  const evaluation = requireObject(result.evaluation, 'evaluation');
  const scoring = requireObject(result.scoring, 'scoring');
  const aggregate = requireObject(
    result.aggregate_metrics,
    'aggregate_metrics',
  );
  const replay = requireObject(result.replay, 'replay');
  if (!Array.isArray(result.episodes) || result.episodes.length === 0) {
    throw new Error('episodes must be a non-empty list.');
  }

  const episodes = result.episodes.map(parseEpisode);
  const evaluationSeeds = requireSeeds(evaluation.seeds, 'evaluation.seeds');
  const replaySeeds = requireSeeds(replay.seeds, 'replay.seeds');
  const episodeSeeds = episodes.map((episode) => episode.seed);
  if (JSON.stringify(evaluationSeeds) !== JSON.stringify(episodeSeeds)) {
    throw new Error('Episode seeds do not match evaluation.seeds.');
  }
  if (
    requireInteger(evaluation.episode_count, 'evaluation.episode_count') !==
    episodes.length
  ) {
    throw new Error('evaluation.episode_count does not match episodes.');
  }

  return {
    schema_version: RESULT_SCHEMA_VERSION,
    challenge: {
      id: requireString(challenge.id, 'challenge.id'),
      title: requireString(challenge.title, 'challenge.title'),
      version: requireString(challenge.version, 'challenge.version'),
      type: requireString(challenge.type, 'challenge.type'),
    },
    strategy: {
      mode: requireString(strategy.mode, 'strategy.mode'),
      identifier: requireString(strategy.identifier, 'strategy.identifier'),
    },
    simulation: {
      model: requireString(simulation.model, 'simulation.model'),
      scenario_generator: {
        name: requireString(
          generator.name,
          'simulation.scenario_generator.name',
        ),
        version: requireInteger(
          generator.version,
          'simulation.scenario_generator.version',
        ),
      },
    },
    evaluation: {
      seeds: evaluationSeeds,
      episode_count: episodes.length,
    },
    scoring: {
      score_version: requireString(
        scoring.score_version,
        'scoring.score_version',
      ),
      aggregate_score: requireNumber(
        scoring.aggregate_score,
        'scoring.aggregate_score',
      ),
    },
    aggregate_metrics: {
      episode_count: requireInteger(
        aggregate.episode_count,
        'aggregate_metrics.episode_count',
      ),
      mean_fill_rate: requireNumber(
        aggregate.mean_fill_rate,
        'aggregate_metrics.mean_fill_rate',
      ),
      mean_completed_improvement_ticks: requireNullableNumber(
        aggregate.mean_completed_improvement_ticks,
        'aggregate_metrics.mean_completed_improvement_ticks',
      ),
    },
    episodes,
    replay: {
      format: requireString(replay.format, 'replay.format'),
      schema_version: requireString(
        replay.schema_version,
        'replay.schema_version',
      ),
      artifact_name: requireString(
        replay.artifact_name,
        'replay.artifact_name',
      ),
      record_count: requireInteger(
        replay.record_count,
        'replay.record_count',
      ),
      seeds: replaySeeds,
    },
  };
}

export function meanEpisodeImprovementBps(result: ArenaResult): number | null {
  const values = result.episodes
    .map((episode) =>
      episode.valid && episode.completed ? episode.improvement_bps : null,
    )
    .filter((value): value is number => value !== null);
  if (values.length === 0) return null;
  return values.reduce((sum, value) => sum + value, 0) / values.length;
}

export function validateResultReplay(
  result: ArenaResult,
  replay: ArenaReplay,
): void {
  if (result.replay.schema_version !== replay.schemaVersion) {
    throw new Error(
      `Replay schema mismatch: result expects ${result.replay.schema_version}, ` +
        `file uses ${replay.schemaVersion}.`,
    );
  }
  if (result.replay.record_count !== replay.recordCount) {
    throw new Error(
      `Replay record count mismatch: result expects ` +
        `${result.replay.record_count}, file contains ${replay.recordCount}.`,
    );
  }
  const replaySeeds = replay.episodes.map((episode) => episode.seed);
  if (JSON.stringify(result.replay.seeds) !== JSON.stringify(replaySeeds)) {
    throw new Error(
      `Replay seed mismatch: result expects ` +
        `${result.replay.seeds.join(', ')}, file contains ` +
        `${replaySeeds.join(', ')}.`,
    );
  }
}
