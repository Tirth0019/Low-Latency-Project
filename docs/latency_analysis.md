# Latency Analysis

## Day 3 Baseline — Pre-Networking, Synthetic Feed, Single-Threaded
*Recorded on Windows via MSVC (`__rdtsc`), local run.*

### Methodology
- Engine orchestration runs purely single-threaded (no context switches, no locks on the hot path).
- TSC measurement begins **before** the risk check and ends **after** pushing to the outbound ring.
- Calibrated using `MonotonicClock` over a 100ms window at startup.
- Run on 100,000 synthetically generated orders (60% limits, 30% aggressive crosses, 10% cancels).

### Baseline Results
| Metric | Latency (ns) |
|--------|--------------|
| **p50** | ~110.2 ns |
| **p99** | ~650.5 ns |
| **p99.9**| ~1350.2 ns |

### Verification Notes
- **Risk Gate Firing:** 395/100,000 orders were reliably rejected by the branchless risk limit checks (due to net-long/net-short position limits breached during the random order walk). The risk gate is confirmed fully functional.
- **Stability and Jitter Check:** Back-to-back testing proves the numbers are stable. The p99 hovers between `580ns` and `720ns` across multiple runs. Because this variance is within `~20%`, we can confidently trust these measurements. The p99 is not artificially hiding OS scheduler jitter!

> [!NOTE]
> *TSC frequency on speed-step laptops can drift. This acts as a relative baseline for future day optimizations.*

## Day 4 — Networking & I/O Overhead
*Recorded on Loopback (127.0.0.1) using TCP/IP.*

### Loopback RTT Results
| Metric | Latency (Cycles) | Latency (Estimate us) |
|--------|------------------|-----------------------|
| **RTT p50** | ~350,000 | ~115 us |
| **RTT p99** | ~450,000 | ~150 us |

### Networking Overhead Analysis
- **Matching Latency (Day 3):** ~650 ns
- **Total RTT (Day 4):** ~125,000 ns
- **Networking Overhead:** ~124,350 ns (99.5% of total time)

The delta between Day 3 and Day 4 highlights the cost of the OS networking stack, even over loopback. This validates our design choice to move all I/O to dedicated threads, ensuring the matching engine's 650ns core loop is never stalled by the 125us networking delays.
