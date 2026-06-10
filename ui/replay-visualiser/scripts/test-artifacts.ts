import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { parseArenaReplay } from '../src/replay';
import {
  parseArenaResult,
  validateResultReplay,
} from '../src/result';


const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const resultText = fs.readFileSync(
  path.join(root, 'public/arena-simple-reference.result.json'),
  'utf8',
);
const replayText = fs.readFileSync(
  path.join(root, 'public/arena-simple-reference.replay.jsonl'),
  'utf8',
);

const result = parseArenaResult(resultText);
const replay = parseArenaReplay(replayText);
validateResultReplay(result, replay);

assert.equal(result.challenge.id, 'beat_market_order');
assert.equal(result.episodes.length, 1);
assert.equal(result.episodes[0].seed, 7);
assert.equal(result.episodes[0].filled_quantity, 500);
assert.equal(result.episodes[0].target_quantity, 500);
assert.equal(result.replay.record_count, replay.recordCount);

assert.throws(
  () => parseArenaResult('{broken'),
  /Invalid result JSON/,
);

const unsupported = JSON.parse(resultText);
unsupported.schema_version = '99.0';
assert.throws(
  () => parseArenaResult(JSON.stringify(unsupported)),
  /Unsupported result schema/,
);

assert.throws(
  () =>
    validateResultReplay(
      {
        ...result,
        replay: {
          ...result.replay,
          seeds: [999],
        },
      },
      replay,
    ),
  /Replay seed mismatch/,
);

assert.throws(
  () =>
    validateResultReplay(
      {
        ...result,
        replay: {
          ...result.replay,
          record_count: result.replay.record_count + 1,
        },
      },
      replay,
    ),
  /Replay record count mismatch/,
);

console.log('Arena result and replay artifact checks passed.');
