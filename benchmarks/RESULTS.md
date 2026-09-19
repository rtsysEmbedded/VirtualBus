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
