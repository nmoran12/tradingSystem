export const REPLAY_SCHEMA_VERSION = '1.0';

export type PriceLevel = {
  price_ticks: number;
  quantity: number;
};

export type BookView = {
  event_index: number;
  events_remaining: number;
  timestamp_ms: number;
  symbol: string;
  bids: PriceLevel[];
  asks: PriceLevel[];
};

export type OpenOrder = {
  order_id: number;
  type: string;
  price_ticks: number;
  remaining_quantity: number;
};

export type Portfolio = {
  target_quantity: number;
  filled_quantity: number;
  remaining_quantity: number;
  total_cost_tick_units: number;
  average_fill_price_ticks: number | null;
  open_orders: OpenOrder[];
};

export type ReplayRecord = {
  replay_schema_version: string;
  record_index: number;
  episode_sequence: number;
  seed: number;
  event_index: number | null;
  type: string;
  [key: string]: unknown;
};

export type ActionView = {
  actionIndex: number;
  status: 'accepted' | 'rejected';
  action: unknown;
  reason: string | null;
};

export type FillView = {
  orderId: number;
  priceTicks: number;
  quantity: number;
  source: string;
};

export type EventFrame = {
  eventIndex: number;
  book: BookView | null;
  portfolio: Portfolio | null;
  actions: ActionView[];
  fills: FillView[];
  records: ReplayRecord[];
};

export type EpisodeReplay = {
  seed: number;
  strategy: string;
  records: ReplayRecord[];
  frames: EventFrame[];
  result: ReplayRecord;
};

export type ArenaReplay = {
  schemaVersion: string;
  recordCount: number;
  episodes: EpisodeReplay[];
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

function requireInteger(value: unknown, field: string): number {
  if (!Number.isInteger(value)) {
    throw new Error(`${field} must be an integer.`);
  }
  return value as number;
}

function parseBook(value: unknown, line: number): BookView {
  const book = requireObject(value, `Line ${line} book`);
  if (!Array.isArray(book.bids) || !Array.isArray(book.asks)) {
    throw new Error(`Line ${line} book requires bids and asks arrays.`);
  }
  return book as BookView;
}

function parsePortfolio(value: unknown, line: number): Portfolio {
  const portfolio = requireObject(value, `Line ${line} portfolio`);
  if (!Array.isArray(portfolio.open_orders)) {
    throw new Error(`Line ${line} portfolio requires open_orders.`);
  }
  return portfolio as Portfolio;
}

export function parseArenaReplay(text: string): ArenaReplay {
  const records: ReplayRecord[] = [];
  const lines = text.split(/\r?\n/);

  for (let lineIndex = 0; lineIndex < lines.length; lineIndex += 1) {
    const raw = lines[lineIndex].trim();
    if (!raw) continue;

    let parsed: unknown;
    try {
      parsed = JSON.parse(raw);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      throw new Error(`Invalid JSONL at line ${lineIndex + 1}: ${message}`);
    }

    const record = requireObject(parsed, `Line ${lineIndex + 1}`);
    if (record.replay_schema_version !== REPLAY_SCHEMA_VERSION) {
      const found = String(record.replay_schema_version ?? 'missing');
      throw new Error(
        `Unsupported replay schema at line ${lineIndex + 1}: ` +
          `expected ${REPLAY_SCHEMA_VERSION}, found ${found}.`,
      );
    }

    const recordIndex = requireInteger(
      record.record_index,
      `Line ${lineIndex + 1} record_index`,
    );
    if (recordIndex !== records.length) {
      throw new Error(
        `Line ${lineIndex + 1} has record_index ${recordIndex}; ` +
          `expected ${records.length}.`,
      );
    }
    requireInteger(record.seed, `Line ${lineIndex + 1} seed`);
    requireInteger(
      record.episode_sequence,
      `Line ${lineIndex + 1} episode_sequence`,
    );
    if (typeof record.type !== 'string' || record.type.length === 0) {
      throw new Error(`Line ${lineIndex + 1} requires a record type.`);
    }
    if (
      record.event_index !== null &&
      record.event_index !== undefined &&
      !Number.isInteger(record.event_index)
    ) {
      throw new Error(`Line ${lineIndex + 1} event_index must be an integer.`);
    }
    if (record.type === 'book_update') {
      parseBook(record.book, lineIndex + 1);
      parsePortfolio(record.portfolio, lineIndex + 1);
    }
    if (record.type === 'episode_result') {
      parseBook(record.book, lineIndex + 1);
      parsePortfolio(record.portfolio, lineIndex + 1);
    }

    records.push(record as ReplayRecord);
  }

  if (records.length === 0) {
    throw new Error('Replay file is empty.');
  }

  const episodeOrder: number[] = [];
  const grouped = new Map<number, ReplayRecord[]>();
  for (const record of records) {
    if (!grouped.has(record.seed)) {
      grouped.set(record.seed, []);
      episodeOrder.push(record.seed);
    }
    grouped.get(record.seed)?.push(record);
  }

  const episodes = episodeOrder.map((seed) => {
    const episodeRecords = grouped.get(seed) ?? [];
    if (episodeRecords[0]?.type !== 'episode_start') {
      throw new Error(`Episode ${seed} does not start with episode_start.`);
    }
    if (episodeRecords.at(-1)?.type !== 'episode_result') {
      throw new Error(`Episode ${seed} does not end with episode_result.`);
    }
    episodeRecords.forEach((record, index) => {
      if (record.episode_sequence !== index) {
        throw new Error(
          `Episode ${seed} has non-contiguous episode_sequence at record ` +
            `${record.record_index}.`,
        );
      }
    });

    let currentBook: BookView | null = null;
    let currentPortfolio: Portfolio | null = null;
    const frames = new Map<number, EventFrame>();
    const frameOrder: number[] = [];

    const getFrame = (eventIndex: number): EventFrame => {
      const existing = frames.get(eventIndex);
      if (existing) return existing;
      const frame: EventFrame = {
        eventIndex,
        book: currentBook,
        portfolio: currentPortfolio,
        actions: [],
        fills: [],
        records: [],
      };
      frames.set(eventIndex, frame);
      frameOrder.push(eventIndex);
      return frame;
    };

    for (const record of episodeRecords) {
      const eventIndex = record.event_index ?? 0;
      const frame = getFrame(eventIndex);
      frame.records.push(record);

      if (record.type === 'book_update' || record.type === 'episode_result') {
        currentBook = parseBook(record.book, record.record_index + 1);
        currentPortfolio = parsePortfolio(
          record.portfolio,
          record.record_index + 1,
        );
        frame.book = currentBook;
        frame.portfolio = currentPortfolio;
      } else if (record.type === 'portfolio_update') {
        currentPortfolio = parsePortfolio(
          record.portfolio,
          record.record_index + 1,
        );
        frame.portfolio = currentPortfolio;
      } else if (record.type === 'action_result') {
        const status = record.status;
        if (status !== 'accepted' && status !== 'rejected') {
          throw new Error(
            `Record ${record.record_index} has invalid action status.`,
          );
        }
        frame.actions.push({
          actionIndex: requireInteger(
            record.action_index,
            `Record ${record.record_index} action_index`,
          ),
          status,
          action: record.action,
          reason: typeof record.reason === 'string' ? record.reason : null,
        });
      } else if (record.type === 'fill') {
        frame.fills.push({
          orderId: requireInteger(
            record.order_id,
            `Record ${record.record_index} order_id`,
          ),
          priceTicks: requireInteger(
            record.price_ticks,
            `Record ${record.record_index} price_ticks`,
          ),
          quantity: requireInteger(
            record.quantity,
            `Record ${record.record_index} quantity`,
          ),
          source: String(record.source ?? 'unknown'),
        });
      }
    }

    const result = episodeRecords.at(-1) as ReplayRecord;
    const start = episodeRecords[0];
    return {
      seed,
      strategy: String(start.strategy ?? 'unknown'),
      records: episodeRecords,
      frames: frameOrder.map((eventIndex) => frames.get(eventIndex) as EventFrame),
      result,
    };
  });

  return {
    schemaVersion: REPLAY_SCHEMA_VERSION,
    recordCount: records.length,
    episodes,
  };
}
