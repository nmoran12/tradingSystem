import React, { useEffect, useState } from 'react';

import {
  ArenaChallenge,
  challengeDescription,
  challenges,
  formatMetric,
} from './challenge';
import { ReplayPage } from './ReplayPage';
import { ResultPage } from './ResultPage';


const CPP_TEMPLATE = `#include "execution_v1_strategy.hpp"

#include <vector>

namespace arena = orderbook_arena::execution_v1;

std::vector<arena::Action> onBookUpdate(
    const arena::BookView& book,
    const arena::Portfolio& portfolio) {
    if (portfolio.remaining_quantity == 0) {
        return {};
    }

    // Add your execution decisions here.
    return {};
}

int main() {
    return arena::run_strategy_loop(onBookUpdate);
}`;

const PYTHON_TEMPLATE = `def on_book_update(book, portfolio):
    if portfolio["remaining_quantity"] == 0:
        return []

    # Planned Python callback contract.
    # External Python process execution is not implemented yet.
    return []`;

const BUILTIN_COMMAND = `python3 arena/tools/evaluate_execution_v1.py \\
  --challenge arena/challenges/beat_market_order.v1.json \\
  --strategy simple_reference \\
  --results-out arena/results/builtin.result.json \\
  --replay-out arena/replays/builtin.replay.jsonl`;

const CPP_BUILD_COMMAND = `cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp`;

const CPP_EVALUATE_COMMAND = `python3 arena/tools/evaluate_execution_v1.py \\
  --challenge arena/challenges/beat_market_order.v1.json \\
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \\
  --results-out arena/results/cpp.result.json \\
  --replay-out arena/replays/cpp.replay.jsonl`;

const VIEWER_COMMAND = `cd ui/replay-visualiser
npm ci
npm run dev`;

type Route =
  | { page: 'challenges' }
  | { page: 'challenge'; challengeId: string }
  | { page: 'results' }
  | { page: 'replay' };

function parseRoute(): Route {
  const route = window.location.hash.replace(/^#/, '') || '/challenges';
  if (route === '/replay') return { page: 'replay' };
  if (route === '/results') return { page: 'results' };
  if (route.startsWith('/challenges/')) {
    return {
      page: 'challenge',
      challengeId: route.slice('/challenges/'.length),
    };
  }
  return { page: 'challenges' };
}

function useRoute(): Route {
  const [route, setRoute] = useState<Route>(parseRoute);
  useEffect(() => {
    const update = () => setRoute(parseRoute());
    window.addEventListener('hashchange', update);
    return () => window.removeEventListener('hashchange', update);
  }, []);
  return route;
}

function Tag({ children }: { children: React.ReactNode }) {
  return <span className="tag">{children}</span>;
}

function CodeBlock({
  code,
  label,
}: {
  code: string;
  label: string;
}) {
  const [copied, setCopied] = useState(false);
  const copy = async () => {
    await navigator.clipboard.writeText(code);
    setCopied(true);
    window.setTimeout(() => setCopied(false), 1200);
  };
  return (
    <div className="code-card">
      <div className="code-card-head">
        <span>{label}</span>
        <button type="button" className="copy-button" onClick={copy}>
          {copied ? 'Copied' : 'Copy'}
        </button>
      </div>
      <pre><code>{code}</code></pre>
    </div>
  );
}

function SiteHeader() {
  return (
    <header className="site-header">
      <a className="brand" href="#/challenges">
        <span className="brand-mark">OA</span>
        <span>
          <strong>OrderBook Arena</strong>
          <small>Local challenge preview</small>
        </span>
      </a>
      <nav aria-label="Primary navigation">
        <a href="#/challenges">Challenges</a>
        <a href="#/results">Results</a>
        <a href="#/replay">Replay viewer</a>
      </nav>
      <span className="local-badge">Local execution only</span>
    </header>
  );
}

function ChallengeBrowser() {
  return (
    <main className="site-main">
      <section className="product-hero">
        <div>
          <p className="eyebrow">Deterministic market challenges</p>
          <h1>Learn execution by writing strategy decisions</h1>
          <p>
            Browse versioned challenges, understand the rules, then evaluate
            built-in or compiled C++ strategies on your own machine.
          </p>
        </div>
        <aside className="status-callout">
          <strong>Current product boundary</strong>
          <p>
            The website explains challenges and reads local result/replay
            artifacts. It does not execute, upload, or submit strategy code.
          </p>
        </aside>
      </section>

      <section className="section-heading">
        <div>
          <p className="eyebrow">Challenge library</p>
          <h2>Available challenges</h2>
        </div>
        <span>{challenges.length} challenge</span>
      </section>

      <section className="challenge-list" aria-label="Available challenges">
        {challenges.map((challenge) => (
          <article className="challenge-card" key={challenge.challenge_id}>
            <div className="challenge-card-main">
              <div className="tag-row">
                <Tag>{challenge.difficulty}</Tag>
                <Tag>{challenge.challenge_type}</Tag>
                {challenge.tags.map((tag) => <Tag key={tag}>{tag}</Tag>)}
              </div>
              <h3>{challenge.title}</h3>
              <p>{challengeDescription(challenge)}</p>
              <div className="language-row">
                {challenge.strategy.supported_languages.map((language) => (
                  <span key={language}>{language.toUpperCase()}</span>
                ))}
              </div>
            </div>
            <a
              className="text-link"
              href={`#/challenges/${challenge.challenge_id}`}
            >
              View challenge
            </a>
          </article>
        ))}
      </section>
    </main>
  );
}

function ChallengeSummary({ challenge }: { challenge: ArenaChallenge }) {
  const generator = challenge.market.scenario_generator;
  const items = [
    ['Target', `${challenge.task.target_quantity} units`],
    ['Episode', `${generator.event_count} events`],
    ['Interval', `${generator.event_interval_ms} ms simulated`],
    ['Visible depth', `${challenge.strategy.book_view.visible_depth_levels} levels`],
    ['Initial midpoint', `${generator.initial_mid_price_ticks} ticks`],
    ['Initial spread', `${generator.initial_spread_ticks} ticks`],
  ];
  return (
    <div className="fact-grid">
      {items.map(([label, value]) => (
        <div key={label}>
          <span>{label}</span>
          <strong>{value}</strong>
        </div>
      ))}
    </div>
  );
}

function ChallengeDetail({ challenge }: { challenge: ArenaChallenge }) {
  return (
    <main className="site-main">
      <a className="back-link" href="#/challenges">← All challenges</a>
      <section className="challenge-hero">
        <div>
          <div className="tag-row">
            <Tag>{challenge.difficulty}</Tag>
            <Tag>{challenge.challenge_type}</Tag>
            {challenge.tags.map((tag) => <Tag key={tag}>{tag}</Tag>)}
          </div>
          <h1>{challenge.title}</h1>
          <p>{challenge.prompt}</p>
        </div>
        <aside className="status-callout">
          <strong>Execution status</strong>
          <p>
            Built-in and trusted local C++ strategies work today. External
            Python execution and hosted judging are not implemented.
          </p>
        </aside>
      </section>

      <section className="content-layout">
        <div className="content-column">
          <article className="content-card">
            <p className="eyebrow">Objective</p>
            <h2>Complete the order without paying the baseline price</h2>
            <p>
              Buy the full target before the deterministic episode ends.
              Partial or invalid episodes score zero. Completed episodes score
              the immediate-market baseline VWAP minus your strategy VWAP.
            </p>
            <div className="formula">
              score = baseline average fill price − strategy average fill price
            </div>
            <p className="muted">
              Positive is better. A completed strategy can receive a negative
              score when it pays more than the baseline.
            </p>
          </article>

          <article className="content-card">
            <p className="eyebrow">Market settings</p>
            <h2>Deterministic synthetic level book</h2>
            <ChallengeSummary challenge={challenge} />
            <p className="muted">
              Prices use integer ticks and quantities use integer units. The
              current simulator is <code>python_level_book_skeleton_v1</code>,
              not the repository's C++ matching engine.
            </p>
          </article>

          <article className="content-card">
            <p className="eyebrow">Rules</p>
            <h2>Allowed strategy actions</h2>
            <div className="rule-grid">
              {challenge.allowed_actions.map((action) => (
                <div key={action}>
                  <strong>{action}</strong>
                  <span>
                    {action === 'market_order' && 'Buy immediately from visible asks.'}
                    {action === 'limit_order' && 'Buy up to a chosen price and rest any remainder.'}
                    {action === 'cancel_order' && 'Cancel one strategy-owned open order.'}
                  </span>
                </div>
              ))}
            </div>
          </article>

          <article className="content-card">
            <p className="eyebrow">Result contract</p>
            <h2>Metrics reported after evaluation</h2>
            <div className="metric-list">
              {challenge.scoring.reported_metrics.map((metric) => (
                <span key={metric}>{formatMetric(metric)}</span>
              ))}
            </div>
            <p className="muted">
              Aggregate score is the arithmetic mean across all selected seeds,
              including zero scores from incomplete or invalid episodes.
            </p>
          </article>

          <article className="content-card">
            <p className="eyebrow">Starter templates</p>
            <h2>Write only the decision callback</h2>
            <div className="language-status">
              <div>
                <strong>C++</strong>
                <span className="support-ready">Available locally</span>
                <p>
                  The existing header owns the JSONL process loop. Implement
                  <code>{challenge.strategy.callbacks.cpp}</code> and compile it.
                </p>
              </div>
              <div>
                <strong>Python</strong>
                <span className="support-planned">Contract only</span>
                <p>
                  The function shape is documented, but external Python
                  strategy execution is a later milestone.
                </p>
              </div>
            </div>
            <CodeBlock label="C++ starter" code={CPP_TEMPLATE} />
            <CodeBlock label="Python callback contract (not executable yet)" code={PYTHON_TEMPLATE} />
          </article>

          <article className="content-card">
            <p className="eyebrow">Local workflow</p>
            <h2>Evaluate, export, inspect</h2>
            <ol className="workflow-list">
              <li><span>1</span><div><strong>Build the C++ example strategy</strong><CodeBlock label="Build" code={CPP_BUILD_COMMAND} /></div></li>
              <li><span>2</span><div><strong>Evaluate a built-in or C++ strategy</strong><CodeBlock label="Built-in mode" code={BUILTIN_COMMAND} /><CodeBlock label="C++ process mode" code={CPP_EVALUATE_COMMAND} /></div></li>
              <li><span>3</span><div><strong>Generate result JSON and replay JSONL</strong><p>The evaluator writes both artifacts to the paths above.</p></div></li>
              <li><span>4</span><div><strong>Open the local website</strong><CodeBlock label="Website" code={VIEWER_COMMAND} /></div></li>
              <li><span>5</span><div><strong>Import the result and replay</strong><p>Open the result JSON first, then attach <code>builtin.replay.jsonl</code> or <code>cpp.replay.jsonl</code>.</p><a className="text-link" href="#/results">Open result viewer</a></div></li>
            </ol>
          </article>

          <article className="content-card limitation-card">
            <p className="eyebrow">Current limitations</p>
            <h2>What this website does not do</h2>
            <ul>
              <li>No online code execution or submission.</li>
              <li>No sandboxing, accounts, leaderboard, or hidden hosted seeds.</li>
              <li>No external Python strategy runner yet.</li>
              <li>No C++ matching-engine integration yet.</li>
              <li>Local results are inspectable and unverified.</li>
            </ul>
          </article>
        </div>

        <aside className="detail-sidebar">
          <div className="content-card sticky-card">
            <p className="eyebrow">Challenge metadata</p>
            <dl className="detail-list">
              <div><dt>Version</dt><dd>{challenge.challenge_version}</dd></div>
              <div><dt>Score version</dt><dd>{challenge.scoring.version}</dd></div>
              <div><dt>Result schema</dt><dd>{challenge.outputs.result.schema_version}</dd></div>
              <div><dt>Replay schema</dt><dd>{challenge.outputs.replay.schema_version}</dd></div>
              <div><dt>Local seeds</dt><dd>{challenge.episodes.local_evaluation.join(', ')}</dd></div>
            </dl>
            <p className="muted">{challenge.episodes.local_evaluation_note}</p>
          </div>
        </aside>
      </section>
    </main>
  );
}

function NotFound() {
  return (
    <main className="site-main">
      <section className="empty-panel">
        Challenge not found. <a href="#/challenges">Return to challenges.</a>
      </section>
    </main>
  );
}

export const App: React.FC = () => {
  const route = useRoute();
  let page: React.ReactNode;
  if (route.page === 'replay') {
    page = <ReplayPage />;
  } else if (route.page === 'results') {
    page = <ResultPage />;
  } else if (route.page === 'challenge') {
    const challenge = challenges.find(
      (item) => item.challenge_id === route.challengeId,
    );
    page = challenge ? <ChallengeDetail challenge={challenge} /> : <NotFound />;
  } else {
    page = <ChallengeBrowser />;
  }

  return (
    <>
      <SiteHeader />
      {page}
    </>
  );
};
