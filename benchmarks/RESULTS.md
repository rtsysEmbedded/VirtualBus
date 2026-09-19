# VirtualBus Benchmark Results

Recorded runs of `bench_latency` (see `bench_latency.cpp` for what each
scenario measures and why). Latency is sendMessage() -> callback
dispatch, measured with `steady_clock`, in microseconds.

## How to record a new run

After a change to `libs/unicore/` (or anything else that could plausibly
affect message-passing latency), build and run the benchmark, then paste
the raw output as a new dated section below -- oldest at the bottom,
newest at the top, so the most recent baseline is always the first thing
a reader sees:

```bash
cmake -S . -B build
cmake --build build --target bench_latency
./build/benchmarks/bench_latency | tee /tmp/bench_output.txt
```

Paste the full output verbatim (it's already self-describing: commit
hash, timestamp, and `hardware_concurrency()`). Note the CPU/environment
alongside it if it differs from previous entries -- these numbers are
machine-dependent in their absolute values, so a comparison across
different machines is only meaningful for the *relative* shape of the
results (does fan-out still scale roughly linearly with receiver count?
does logging still add a fixed multiplier?), not the raw microsecond
figures.

---

## 2026-09-19 -- targeted delivery + priority added (commit `7bb8bd1`)

Follow-up to the baseline below, after adding targeted delivery
(`sendMessage(..., targetId)`), per-message priority (dequeued/dispatched
highest-first on both the polling and callback paths), and an injectable
clock. New scenario F isolates the one architectural change this
benchmark exists to validate: does routing a message to a specific task
instead of broadcasting to everyone stop that message's latency from
scaling with how many *other* tasks happen to be attached?

**Environment:** same 4-core cloud sandbox VM as the baseline below,
`-O2 -DNDEBUG`. Total run time: ~4.3s.

```
VirtualBus latency/jitter benchmark
commit: 7bb8bd1
run at: 2026-09-19 21:52:08 UTC
hardware_concurrency (ThreadPool size): 4

=== A. Baseline: 1 sender -> 1 receiver, no logger, busy-send ===
1 receiver, 2000 msgs, no logger         n=2000   min=     1.3  p50=     5.3  p90=    17.6  p99=    53.9  max=      95.0  mean=    10.5  stddev=    10.3  (us)

=== B. Fan-out scaling: broadcast-to-all cost, no logger, busy-send ===
1 receiver,  1000 msgs                   n=1000   min=     2.6  p50=     5.1  p90=    15.8  p99=    23.5  max=      67.2  mean=     8.4  stddev=     5.8  (us)
5 receivers, 1000 msgs                   n=5000   min=     2.5  p50=    47.1  p90=    77.1  p99=   105.7  max=     166.8  mean=    43.9  stddev=    26.9  (us)
20 receivers, 1000 msgs                  n=20000  min=     4.2  p50=   182.5  p90=   329.9  p99=   636.6  max=     786.0  mean=   190.9  stddev=   124.9  (us)
50 receivers, 1000 msgs                  n=50000  min=     9.3  p50=   429.4  p90=   775.5  p99=   884.4  max=    2178.5  mean=   432.3  stddev=   252.5  (us)

=== C. Logging on the hot path: 1 receiver, 1000 msgs, busy-send ===
no logger                                n=1000   min=     1.7  p50=     5.0  p90=    15.7  p99=    28.9  max=      63.0  mean=     8.3  stddev=     5.9  (us)
StdCoutLogger enabled                    n=1000   min=     5.3  p50=    30.2  p90=    32.1  p99=    61.1  max=     193.9  mean=    31.5  stddev=     8.3  (us)

=== D. Fan-out + logging combined worst case, busy-send ===
20 receivers, 1000 msgs, no logger       n=20000  min=     4.2  p50=   177.4  p90=   322.5  p99=   477.8  max=    1350.5  mean=   182.3  stddev=   115.9  (us)
20 receivers, 1000 msgs, w/ logger       n=20000  min=    17.2  p50=   182.6  p90=   320.5  p99=   381.5  max=     512.8  mean=   185.1  stddev=   100.2  (us)

=== E. Paced send (~200us between sends, 1 receiver) ===
1 receiver, 1000 msgs, paced             n=1000   min=     4.3  p50=    34.7  p90=    45.7  p99=    73.8  max=     102.0  mean=    37.3  stddev=     8.8  (us)
20 receivers, 1000 msgs, paced           n=20000  min=    10.9  p50=   194.3  p90=   340.6  p99=   403.0  max=     654.0  mean=   195.4  stddev=   109.0  (us)

=== F. Targeted delivery vs. broadcast, no logger, busy-send ===
(targeted: 1000 messages addressed to a single receiver via sendMessage(..., targetId), with N total attached-but-uninvolved receivers)
broadcast,  1 attached, 1000 msgs        n=1000   min=     1.6  p50=     5.2  p90=    15.9  p99=    28.4  max=      70.7  mean=     8.5  stddev=     5.8  (us)
targeted,   1 attached, 1000 msgs        n=1000   min=     1.6  p50=     5.1  p90=    15.8  p99=    20.2  max=      44.4  mean=     8.2  stddev=     5.2  (us)
broadcast, 20 attached, 1000 msgs        n=20000  min=     4.1  p50=   164.0  p90=   307.7  p99=   376.3  max=     726.5  mean=   170.8  stddev=   101.2  (us)
targeted,  20 attached, 1000 msgs        n=1000   min=     2.4  p50=     5.1  p90=    16.5  p99=    22.9  max=      70.7  mean=     8.3  stddev=     6.3  (us)
broadcast, 50 attached, 1000 msgs        n=50000  min=    11.5  p50=   431.1  p90=   780.8  p99=   896.4  max=    1167.6  mean=   434.2  stddev=   251.8  (us)
targeted,  50 attached, 1000 msgs        n=1000   min=     2.2  p50=     5.0  p90=    15.7  p99=    28.6  max=     112.4  mean=     8.5  stddev=     6.8  (us)
```

### Reading this run

- **Scenario F confirms the fix**: broadcast latency still scales with
  attached-receiver count exactly as in the baseline (p50 ~5us at 1
  receiver -> ~164us at 20 -> ~431us at 50). Targeted-delivery p50 stays
  flat at ~5us regardless of whether 1, 20, or 50 *other* tasks happen to
  be attached -- because sendMessage(..., targetId) now pushes into
  exactly one task's queue instead of iterating every attached task
  under the shared busMutex_. This was the single largest jitter source
  identified in the original baseline; it's now opt-in per message
  rather than mandatory for every send.
- **Scenarios A-E are within normal run-to-run variance of the original
  baseline** (compare e.g. fan-out B's 20-receiver p50: 182.5us here vs.
  185.6us in the baseline) -- expected, since broadcast's own code path
  didn't fundamentally change, it just gained a second, faster path
  alongside it. Priority-queue overhead on the still-exercised broadcast
  path (an array-index instead of a single queue push) doesn't show up
  as a measurable regression here.
- Not yet measured here: latency *by priority level* under contention
  (e.g. does a Critical message still get low latency when the queue is
  backed up with Low-priority traffic?). That would need a scenario that
  mixes priorities under load, which scenario F doesn't yet do -- a
  reasonable next addition to this benchmark if priority's effect under
  contention specifically needs validating.

---

## 2026-09-19 -- baseline (commit `e39c7b2`)

First recorded run, establishing the baseline this bug-fix PR's
architecture leaves behind. No architectural jitter fixes have been made
yet at this point -- `VirtualBus::sendMessage()` still broadcasts to every
attached task under one global mutex, and `logger_->info(...)` calls are
still synchronous and on the hot path (see the file-level comment in
`bench_latency.cpp` and the architecture discussion in the project's PR
history for the reasoning behind each scenario).

**Environment:** 4-core cloud sandbox VM, Intel(R) Xeon(R) Processor @
2.80GHz, gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), `-O2 -DNDEBUG`,
Linux. Total run time: ~3.1s.

```
VirtualBus latency/jitter benchmark
commit: e39c7b2
run at: 2026-09-19 21:21:27 UTC
hardware_concurrency (ThreadPool size): 4

=== A. Baseline: 1 sender -> 1 receiver, no logger, busy-send ===
1 receiver, 2000 msgs, no logger         n=2000   min=     1.7  p50=     5.3  p90=    17.3  p99=    51.7  max=      57.4  mean=    10.1  stddev=     9.4  (us)

=== B. Fan-out scaling: broadcast-to-all cost, no logger, busy-send ===
1 receiver,  1000 msgs                   n=1000   min=     1.3  p50=     5.6  p90=    17.2  p99=    34.1  max=     283.5  mean=     9.5  stddev=    11.1  (us)
5 receivers, 1000 msgs                   n=5000   min=     2.7  p50=    51.2  p90=    91.7  p99=   133.8  max=     218.9  mean=    51.7  stddev=    33.0  (us)
20 receivers, 1000 msgs                  n=20000  min=     4.1  p50=   185.6  p90=   344.0  p99=   700.5  max=     861.7  mean=   199.4  stddev=   140.1  (us)
50 receivers, 1000 msgs                  n=50000  min=     9.1  p50=   432.7  p90=   783.0  p99=   897.0  max=    2255.7  mean=   436.7  stddev=   255.6  (us)

=== C. Logging on the hot path: 1 receiver, 1000 msgs, busy-send ===
no logger                                n=1000   min=     1.5  p50=     5.2  p90=    16.0  p99=    21.9  max=     205.0  mean=     8.6  stddev=     8.2  (us)
StdCoutLogger enabled                    n=1000   min=     4.1  p50=    30.5  p90=    34.8  p99=    60.6  max=      88.5  mean=    31.8  stddev=     6.9  (us)

=== D. Fan-out + logging combined worst case, busy-send ===
20 receivers, 1000 msgs, no logger       n=20000  min=     4.0  p50=   168.5  p90=   311.0  p99=   365.4  max=     548.7  mean=   172.7  stddev=   101.0  (us)
20 receivers, 1000 msgs, w/ logger       n=20000  min=    10.3  p50=   180.6  p90=   321.7  p99=   379.3  max=     603.8  mean=   184.7  stddev=   100.9  (us)

=== E. Paced send (~200us between sends, 1 receiver) ===
1 receiver, 1000 msgs, paced             n=1000   min=     5.6  p50=    35.6  p90=    48.2  p99=    74.8  max=     139.3  mean=    38.7  stddev=    10.0  (us)
20 receivers, 1000 msgs, paced           n=20000  min=    20.3  p50=   200.3  p90=   346.5  p99=   413.4  max=    2134.6  mean=   200.2  stddev=   114.3  (us)
```

### Reading this baseline

- **Fan-out scaling (B) is the dominant jitter source**: p50 latency
  grows roughly linearly with receiver count -- 5.6us at 1 receiver,
  51.2us at 5, 185.6us at 20, 432.7us at 50 (~77x for 50x the receivers).
  p99 at 50 receivers is already ~900us, with a max spike over 2.2ms.
  `sendMessage()` pushes to *every* attached task's queue regardless of
  whether that task actually cares about the message
  (`libs/unicore/src/VirtualBus.cpp`, the `for (auto& [taskId, taskInfo]
  : tasks_)` loop), so this cost scales with total attached task count,
  not with actual subscribers.
- **Synchronous logging (C) adds a fixed ~5x multiplier**: p50 goes from
  5.2us to 30.5us with `StdCoutLogger` enabled, independent of fan-out.
  This is a separate, additive cost from the point above (see D).
- **Pacing the sender (E) makes things *worse*, not better**: a
  `sleep_for(200us)`-paced sender has *higher* p50 latency (35.6us) than
  a tight busy loop with the same receiver count (5.6us). The extra cost
  comes from the OS scheduler's own wakeup jitter after a sleep, which is
  a cost independent of anything VirtualBus does -- but it's the pattern
  `SendTask`/`ReceiveTask` in `src/` actually use, so it applies to this
  project's own example tasks as-is.
- Even the simplest case (A) has a p99/p50 ratio of ~10x and a max more
  than 10x the p50 -- there is real, measurable jitter in the
  thread-pool handoff itself, before any of the scaling effects above
  are even a factor.

None of this baseline has been fixed yet; it's a reference point for
before/after comparisons once (if) the architecture changes discussed
alongside it (per-task queues/condition-variables instead of one shared
one, topic-based routing instead of broadcast-to-all, async logging, a
real-time scheduling policy) are made.
