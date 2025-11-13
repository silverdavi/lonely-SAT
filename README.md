# Lonely Runner Conjecture: SAT-Based Verification

A SAT-based encoding for verifying the Lonely Runner Conjecture using Rosenfeld's computational approach.

## Overview

The Lonely Runner Conjecture concerns k+1 runners with distinct constant speeds on a unit circular track. We reformulate Rosenfeld's verification procedure (2025) as Boolean satisfiability and analyze the performance.

## Key Results

### Correctness: ✅ Validated

Our SAT encoding is logically equivalent to Rosenfeld's verification:

| k   | Runners | Primes Tested | Agreement        |
|-----|---------|---------------|------------------|
| 4   | 5       | 6             | 6/6 (100%)       |
| 5   | 6       | 12            | 12/12 (100%)     |
| 6   | 7       | 19            | 19/19 (100%)     |
| **Total** | | **37** | **37/37 (100%)** |

### Performance: SAT is Slower

Comparison with Rosenfeld's optimized backtracking:

| k   | p   | Result | Rosenfeld | SAT   | Ratio            |
|-----|-----|--------|-----------|-------|------------------|
| 4   | 17  | UNSAT  | 0.119s    | 0.195s| 1.6× slower      |
| 4   | 31  | UNSAT  | 0.089s    | 0.182s| 2.0× slower      |
| 5   | 23  | UNSAT  | 0.098s    | 1.45s | 14.8× slower     |
| 5   | 31  | UNSAT  | 0.150s    | 14.7s | 98× slower       |
| 6   | 31  | UNSAT  | 0.339s    | 189s  | 558× slower      |
| 8   | 31  | SAT    | 0.124s    | 84.5s | 681× slower      |
| 8   | 37  | SAT    | 0.120s    | 6.5s  | 54× slower       |

**Hardware:** Apple M4 Max (16 performance cores, 4 efficiency cores)

## Why is SAT Slower?

1. **Problem structure:** Rosenfeld's backtracking exploits domain-specific pruning (least-covered positions) that CDCL discovers inefficiently
2. **Minimal reduction:** Preprocessing eliminates few candidates/times (e.g., 0 velocities, 3 of 139 times for k=8, p=31)
3. **Encoding overhead:** Cardinality constraints add ~1000 auxiliary variables and ~5000 clauses
4. **Symmetry:** Substantial symmetry that SAT solvers handle inefficiently

## Repository Structure

```
lonely-runner-sat/
├── README.md                      # This file
├── paper/
│   ├── paper.tex                  # LaTeX source
│   └── paper.pdf                  # Paper (6 pages)
├── src/
│   └── lonely_cnf_generator.cpp   # CNF generator with preprocessing
├── solver/
│   └── kissat                     # Kissat SAT solver binary
└── verify.sh                      # Verification script
```

## Quick Start

### Verify a Single Case

```bash
# Verify k=4, p=17 (UNSAT case)
./verify.sh 4 17

# Verify k=8, p=31 (SAT case - has a covering)
./verify.sh 8 31
```

### Manual Usage

```bash
# Compile CNF generator
g++ -O3 -DK=8 -DPRIME=31 -o gen src/lonely_cnf_generator.cpp

# Generate CNF and solve
./gen | ./solver/kissat --quiet

# For k=8, p=31: expects "s SATISFIABLE" (covering exists)
# For k=4, p=17: expects "s UNSATISFIABLE" (no covering, lemma applies)
```

## Example: k=8, p=37

Both methods correctly find that a covering exists, but different tuples:

**Rosenfeld's covering:** {9, 2, 7, 27, 45, 36, 63, 117} (found in 0.120s)  
**SAT solver's covering:** {9, 18, 41, 45, 54, 63, 101, 108} (found in 6.5s)

Both satisfy all constraints:
- Exactly 8 velocities
- All in {1,...,166}\37ℕ
- Cover all 166 positions
- At most 6 multiples of 3 (GCD constraint)

Multiple valid solutions exist; backtracking is 54× faster.

## Conclusion

This is a **negative result**: the SAT encoding is correct but substantially slower (1.6-681×) than specialized backtracking. It demonstrates that black-box SAT application doesn't universally outperform problem-specific algorithms.

The negative result has value: it shows boundaries of SAT solver effectiveness and highlights the importance of domain-specific optimization.

## Paper

**Title:** "A SAT-Based Encoding for the Lonely Runner Conjecture: Performance Analysis"

**Length:** 6 pages

**Content:** Correct SAT encoding, validation results, honest performance comparison, analysis of why SAT is slower

**Status:** Negative result with scientific value

## Citation

```bibtex
@misc{Silver2025LonelyRunnerSAT,
  title={A SAT-Based Encoding for the Lonely Runner Conjecture: Performance Analysis},
  author={Silver, David H.},
  year={2025},
  howpublished={\url{https://github.com/silverdavi/lonely-SAT}},
  note={Paper: \url{https://github.com/silverdavi/lonely-SAT/blob/main/paper/paper.pdf}}
}
```

## Related Work

- **Rosenfeld (2025):** Proof for 8 runners with optimized backtracking  
  [arXiv:2509.14111](https://arxiv.org/abs/2509.14111) | [Code](https://gite.lirmm.fr/mrosenfeld/the-lonely-runner-conjecture)

- **Malikiosis, Santos, Schymura (2024):** Upper bounds  
  [arXiv:2406.19389](https://arxiv.org/abs/2406.19389)

## Acknowledgments

This work builds on Matthieu Rosenfeld's foundational approach. We thank him for identifying a critical bug in our initial cardinality encoding. We thank the developers of the Kissat SAT solver.

---

**Status:** Correct encoding, validated equivalence, honest performance assessment  
**Result:** SAT is slower (negative result with scientific value)  
**Last updated:** November 2025
