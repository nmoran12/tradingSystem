import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';


const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const repositoryRoot = path.resolve(root, '../..');
const challenge = JSON.parse(
  fs.readFileSync(
    path.join(
      repositoryRoot,
      'arena/challenges/beat_market_order.v1.json',
    ),
    'utf8',
  ),
);
const appSource = fs.readFileSync(path.join(root, 'src/App.tsx'), 'utf8');
const replaySource = fs.readFileSync(path.join(root, 'src/ReplayPage.tsx'), 'utf8');
const resultPageSource = fs.readFileSync(
  path.join(root, 'src/ResultPage.tsx'),
  'utf8',
);
const resultParserSource = fs.readFileSync(
  path.join(root, 'src/result.ts'),
  'utf8',
);
const bundle = fs
  .readdirSync(path.join(root, 'dist/assets'))
  .filter((file) => file.endsWith('.js'))
  .map((file) => fs.readFileSync(path.join(root, 'dist/assets', file), 'utf8'))
  .join('\n');

function requireCondition(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

for (const expected of [
  challenge.title,
  challenge.prompt,
  challenge.challenge_type,
  'Available challenges',
  'Local execution only',
  'External Python execution and hosted judging are not implemented',
  'Replay viewer',
  'Result and replay viewer',
  'Unverified local result',
  'Per-episode results',
  'Replay reference',
]) {
  requireCondition(
    bundle.includes(expected),
    `Built frontend is missing required content: ${expected}`,
  );
}

for (const route of [
  '/challenges',
  `/challenges/${challenge.challenge_id}`,
  '/results',
  '/replay',
]) {
  requireCondition(
    appSource.includes(route),
    `Frontend source is missing route: ${route}`,
  );
}

requireCondition(
  appSource.includes("from './challenge'"),
  'Challenge pages must use the versioned challenge adapter.',
);
requireCondition(
  replaySource.includes('parseArenaReplay'),
  'Replay visualiser parser is no longer connected.',
);
requireCondition(
  resultPageSource.includes('parseArenaResult'),
  'Result parser is no longer connected.',
);
requireCondition(
  resultPageSource.includes('validateResultReplay'),
  'Result/replay compatibility validation is no longer connected.',
);
requireCondition(
  resultPageSource.includes('<ReplayPage'),
  'Result page no longer reuses the replay visualiser.',
);
requireCondition(
  resultParserSource.includes("RESULT_SCHEMA_VERSION = '1.1'"),
  'Result parser must explicitly support result schema 1.1.',
);
requireCondition(
  !/<button[^>]*>\s*(Run|Submit)\s*<\/button>/i.test(
    `${appSource}\n${resultPageSource}`,
  ),
  'Hosted Run or Submit controls must not be present.',
);

console.log('Arena website route and content checks passed.');
