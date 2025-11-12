# Lonely Runner Conjecture: SAT-Based Verification

This repository contains the code and paper for verifying the Lonely Runner Conjecture using Boolean satisfiability (SAT) solving.

## Overview

The Lonely Runner Conjecture, introduced by Wills (1967), concerns k+1 runners with distinct constant speeds on a unit circular track. We extend the computational verification from 8 runners (Rosenfeld, 2025) to **14 runners** using a SAT-based approach that achieves speedups of up to **6 orders of magnitude**.

## Key Results

We provide the **first proofs** for 9, 10, 11, 12, 13, and 14 runners.

| k   | Runners | Primes | Time     | Status                  |
|-----|---------|--------|----------|-------------------------|
| 4   | 5       | 6      | 0.90s    | Previously known        |
| 5   | 6       | 12     | 1.43s    | Previously known        |
| 6   | 7       | 19     | 2.14s    | Previously known        |
| 7   | 8       | 27     | 3.00s    | Rosenfeld (2025)        |
| **8**   | **9**       | **44**     | **5.02s**    | **New (this work)**         |
| **9**   | **10**      | **86**     | **12.62s**   | **New (this work)**         |
| **10**  | **11**      | **105**    | **21.68s**   | **New (this work)**         |
| **11**  | **12**      | **158**    | **75.32s**   | **New (this work)**         |
| **12**  | **13**      | **190**    | **155.36s**  | **New (this work)**         |
| **13**  | **14**      | **241**    | **394.20s**  | **New (this work)**         |

**Hardware:** Apple M4 Max (16 performance cores, 4 efficiency cores)  
**Execution:** Parallel across all available cores

## Performance

Comparison with backtracking (Rosenfeld's approach):

| Case             | Backtracking    | SAT Solver | Speedup          |
|------------------|-----------------|------------|------------------|
| k=7, p=163       | 32 hours        | 0.02s      | **5,760,000×**   |
| k=7 (all 27)     | >32 hours       | 3.00s      | **>38,400×**     |

Our total time for all 27 primes (k=7) is faster than Rosenfeld's single hardest instance.

## Repository Structure

```
lonely-runner-sat/
├── README.md                      # This file
├── paper/
│   ├── paper.tex                  # LaTeX source
│   └── paper.pdf                  # Compiled paper
├── src/
│   └── lonely_cnf_generator.cpp   # CNF generator (core implementation)
├── solver/
│   └── kissat                     # Kissat SAT solver binary
└── verify.sh                      # Simple verification script
```

## Quick Start

### Prerequisites

- C++ compiler (g++ or clang++)
- Make (optional)

### Verify a Single Case

```bash
# Verify k=7, p=163 (the hardest case from Rosenfeld's work)
./verify.sh 7 163
```

### Run Full Verification

```bash
# Verify all cases for k=7 (8 runners)
./verify.sh 7
```

### Manual Usage

```bash
# Compile the CNF generator
g++ -O3 -DK=7 -DPRIME=163 -o gen src/lonely_cnf_generator.cpp

# Generate CNF and solve
./gen | ./solver/kissat --quiet

# Expected output: "s UNSATISFIABLE" (proof successful)
```

## How It Works

Our approach reformulates Rosenfeld's verification problem as Boolean satisfiability:

1. **Variables:** Boolean variables for each candidate velocity
2. **Cardinality constraint:** Exactly k elements selected (Sinz encoding)
3. **Coverage constraint:** All positions must be covered by selected velocities
4. **GCD constraint:** At most k-2 elements divisible by each prime factor of (k+1)

The CNF is UNSATISFIABLE ⟺ No counterexample exists ⟺ Conjecture holds for that (k,p).

## Technical Details

### Equivalence to Rosenfeld's Approach

Our SAT encoding is logically equivalent to the verification condition from Rosenfeld's Lemma 6. We verified identical results (UNSAT) on all 88 instances from k ∈ {3,...,7}, confirming correctness.

**Note:** We use the coverage condition ||·|| < 1/(k+1) from Rosenfeld's code and original Lemma 6. The descriptive text in Rosenfeld's paper contains a typo stating 1/(k-1); the code correctly uses 1/(k+1).

### Prime Lists

Prime lists are determined using Rosenfeld's method: select primes such that their product exceeds the upper bound from Malikiosis, Santos, and Schymura (2024).

- k=8: 44 primes (31 to 251)
- k=9: 86 primes (31 to 503)
- k=10: 105 primes (31 to 631)
- k=11: 158 primes (31 to 997)
- k=12: 190 primes (31 to 1193)
- k=13: 241 primes (31 to 1597)

## Citation

If you use this work, please cite:

```bibtex
@article{Silver2025LonelyRunner,
  title={Extending the Lonely Runner Conjecture to 14 Runners via SAT Solving},
  author={Silver, David H.},
  journal={arXiv preprint},
  year={2025}
}
```

This work builds on:

- **Rosenfeld (2025):** Proof for 8 runners  
  [arXiv:2509.14111](https://arxiv.org/abs/2509.14111) | [Code](https://gite.lirmm.fr/mrosenfeld/the-lonely-runner-conjecture)

- **Malikiosis, Santos, Schymura (2024):** Upper bounds  
  [arXiv:2406.19389](https://arxiv.org/abs/2406.19389)

## License

This project is released for academic and research purposes. The Kissat SAT solver is distributed under the MIT License.

## Acknowledgments

This work builds directly on the foundational approach by Matthieu Rosenfeld and the theoretical results by Malikiosis, Santos, and Schymura. We thank the developers of the Kissat SAT solver.

## Contact

David H. Silver

---

**Status:** Verified through k=13 (14 runners) ✓  
**Last updated:** November 2025

