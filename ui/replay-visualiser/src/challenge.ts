import challengeDefinition from '../../../arena/challenges/beat_market_order.v1.json';


type ScenarioGenerator = {
  event_count: number;
  event_interval_ms: number;
  initial_mid_price_ticks: number;
  initial_spread_ticks: number;
  initial_depth_levels: number;
  initial_quantity_per_level: number;
};

export type ArenaChallenge = {
  schema_version: string;
  challenge_version: string;
  challenge_id: string;
  title: string;
  difficulty: string;
  tags: string[];
  prompt: string;
  challenge_type: string;
  strategy: {
    supported_languages: string[];
    callbacks: {
      python: string;
      cpp: string;
    };
    book_view: {
      visible_depth_levels: number;
      price_unit: string;
      quantity_unit: string;
    };
  };
  allowed_actions: string[];
  market: {
    symbol: string;
    price_tick_size: number;
    scenario_generator: ScenarioGenerator;
  };
  task: {
    side: string;
    target_quantity: number;
    start_event_index: number;
    end_event_index: number;
    require_full_completion: boolean;
  };
  episodes: {
    local_evaluation: number[];
    local_evaluation_note: string;
  };
  scoring: {
    version: string;
    baseline_strategy: string;
    completed_episode_score_metric: string;
    incomplete_or_invalid_episode_score: number;
    aggregate: string;
    reported_metrics: string[];
  };
  outputs: {
    result: {
      schema_version: string;
    };
    replay: {
      schema_version: string;
    };
  };
};

export const challenges: ArenaChallenge[] = [
  challengeDefinition as ArenaChallenge,
];

export function challengeDescription(challenge: ArenaChallenge): string {
  return (
    `Buy ${challenge.task.target_quantity} units during a deterministic ` +
    `${challenge.market.scenario_generator.event_count}-event episode and ` +
    'improve on the immediate-market baseline.'
  );
}

export function formatMetric(metric: string): string {
  return metric
    .split('_')
    .map((word) => word.charAt(0).toUpperCase() + word.slice(1))
    .join(' ');
}
