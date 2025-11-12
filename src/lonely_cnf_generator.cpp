// Lonely Runner Cover Search via SAT
// Encodes the search as CNF in DIMACS format.
// Author: adapted from Matthieu Rosenfeld's search code.

#include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <map>
using namespace std;

#ifndef PRIME
#define PRIME 43
#endif

#ifndef K
#define K 7
#endif

constexpr int k = K;
constexpr int prime = PRIME;

constexpr int n = k + 1;
constexpr int Q = n * prime;
constexpr int maxM = Q / 2;

// ------------ Simple CNF builder ------------

struct CNF {
    int numVars = 0;
    vector<vector<int>> clauses;

    int newVar() { return ++numVars; }

    void addClause(const vector<int>& lits) {
        if (lits.empty()) return; // avoid empty clause accidentally
        clauses.push_back(lits);
    }

    void printDIMACS(ostream& out) const {
        out << "p cnf " << numVars << " " << clauses.size() << "\n";
        for (const auto& c : clauses) {
            for (int lit : c) out << lit << " ";
            out << "0\n";
        }
    }
};

// Sequential counter encoding: at most R literals in 'xs' are true.
// xs are literals (signed ints), not necessarily positive vars.
// Implementation of Sinz-style encoding.
void addAtMostK(CNF& cnf, const vector<int>& xs, int R) {
    int n = (int)xs.size();
    if (R >= n || n == 0) return; // no constraint needed

    // s[i][j] is auxiliary var for "among first i literals, at least j are true"
    // 1 <= i <= n, 1 <= j <= R
    vector<vector<int>> s(n + 1, vector<int>(R + 1, 0));

    // i = 1, j = 1: s_1,1 <-> x1
    s[1][1] = cnf.newVar();
    // x1 -> s11  (¬x1 ∨ s11)
    cnf.addClause({ -xs[0], s[1][1] });
    // s11 -> x1  (¬s11 ∨ x1)
    cnf.addClause({ -s[1][1], xs[0] });

    // i > 1
    for (int i = 2; i <= n; ++i) {
        int xi = xs[i - 1];

        // j = 1
        s[i][1] = cnf.newVar();
        // xi -> s_i1  (¬xi ∨ s_i1)
        cnf.addClause({ -xi, s[i][1] });
        // s_(i-1)1 -> s_i1  (¬s_(i-1)1 ∨ s_i1)
        cnf.addClause({ -s[i - 1][1], s[i][1] });

        // For 2 <= j <= min(i, R)
        int maxJ = min(i, R);
        for (int j = 2; j <= maxJ; ++j) {
            s[i][j] = cnf.newVar();
            // s_(i-1)j -> s_ij      (¬s_(i-1)j ∨ s_ij)
            cnf.addClause({ -s[i - 1][j], s[i][j] });
            // (xi ∧ s_(i-1)(j-1)) -> s_ij
            // is encoded as (¬xi ∨ ¬s_(i-1)(j-1) ∨ s_ij)
            cnf.addClause({ -xi, -s[i - 1][j - 1], s[i][j] });
        }

        // Exceeding R: if prefix already has ≥R and xi is true, forbidden.
        // That is (¬xi ∨ ¬s_(i-1)R) for i >= R+1.
        if (i - 1 >= R) {
            cnf.addClause({ -xi, -s[i - 1][R] });
        }
    }
}

// Exactly-K on literals xs: at most K and at least K.
// "At least K" is "at most (N-K)" on their negations.
void addExactlyK(CNF& cnf, const vector<int>& xs, int Kexact) {
    int N = (int)xs.size();
    if (Kexact < 0 || Kexact > N) return;
    if (N == 0) return;

    // at most Kexact
    addAtMostK(cnf, xs, Kexact);

    // at least Kexact: at most (N-Kexact) of the negations are true
    vector<int> negs;
    negs.reserve(N);
    for (int lit : xs) negs.push_back(-lit);
    addAtMostK(cnf, negs, N - Kexact);
}

// ------------ Problem-specific encoding ------------

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cerr << "k = " << k << ", n = " << n
         << ", prime = " << prime
         << ", Q = " << Q
         << ", maxM = " << maxM << "\n";

    // Precompute nearZero as in the original program
    vector<bitset<maxM>> nearZero;
    nearZero.reserve(maxM + 1);

    for (int i = 0; i <= maxM; ++i) {
        bitset<maxM> currLine;
        for (int t = 1; t <= maxM; ++t) {
            // || t*i / Q || < 1/n  (via the same arithmetic as original code)
            int ti = (t * i) % Q;
            bool close = (ti * n < Q) || ((Q - ti) * n < Q);
            currLine[maxM - t] = close;
        }
        nearZero.push_back(currLine);
    }

    // Build candidate velocity list: i in [1..maxM], i % prime != 0
    vector<int> candidates;           // candidate index -> velocity value i
    candidates.reserve(maxM);
    for (int i = 1; i <= maxM; ++i) {
        if (i % prime == 0) continue;
        candidates.push_back(i);
    }

    int numCandidates = (int)candidates.size();
    cerr << "Number of candidate velocities (not divisible by prime): "
         << numCandidates << "\n";

    // CNF builder
    CNF cnf;

    // SAT vars for "candidate j is chosen"
    // xVars[j] is variable index (>0) in DIMACS; literal is +xVars[j] for "chosen".
    vector<int> xVars(numCandidates);
    for (int j = 0; j < numCandidates; ++j) {
        xVars[j] = cnf.newVar();
    }

    // ---- Coverage constraints ----
    // For each time t (0..maxM-1), at least one chosen velocity covers t.
    int uncoverable_count = 0;
    for (int t = 0; t < maxM; ++t) {
        vector<int> clause;
        for (int j = 0; j < numCandidates; ++j) {
            int v = candidates[j];
            if (nearZero[v][t]) {
                clause.push_back(+xVars[j]);
            }
        }
        // If clause empty: this time t cannot be covered by any allowed velocity.
        if (clause.empty()) {
            uncoverable_count++;
            // Add trivially UNSAT formula
            if (uncoverable_count == 1) {
                // First uncoverable position - create contradiction
                int dummy = cnf.newVar();
                cnf.addClause({dummy});
                cnf.addClause({-dummy});
                cerr << "Position t = " << t << " is uncoverable (first of possibly more)\n";
            }
        } else {
            cnf.addClause(clause);
        }
    }

    if (uncoverable_count > 0) {
        cerr << "Total uncoverable positions: " << uncoverable_count << "\n";
        cerr << "Instance is trivially UNSAT\n";
    }

    // ---- Exactly k chosen ----
    addExactlyK(cnf, xVars, k);

    // ---- GCD constraints from extraVerif ----
    // For n=(k+1), we need: for each PRIME divisor q of n=(k+1), 
    // at most k-2 chosen velocities are divisible by q.
    // This encodes the condition gcd(S ∪ {Q}) = 1 for all (k-1)-subsets S.
    
    // Get prime divisors of n=(k+1)
    vector<int> divisors;  // Will contain PRIME divisors of n=(k+1)
    
    // For n ≤ 12, we hardcode prime factorizations
    if (n == 3 || n == 5 || n == 7 || n == 11) {
        divisors.push_back(n);  // n is prime
    } else if (n == 4) {
        divisors.push_back(2);  // 4 = 2^2
    } else if (n == 6) {
        divisors.push_back(2);  // 6 = 2×3
        divisors.push_back(3);
    } else if (n == 8) {
        divisors.push_back(2);  // 8 = 2^3
    } else if (n == 9) {
        divisors.push_back(3);  // 9 = 3^2
    } else if (n == 10) {
        divisors.push_back(2);  // 10 = 2×5
        divisors.push_back(5);
    } else if (n == 12) {
        divisors.push_back(2);  // 12 = 2^2×3
        divisors.push_back(3);
    }

    for (int d : divisors) {
        vector<int> lits;
        for (int j = 0; j < numCandidates; ++j) {
            int v = candidates[j];
            if (v % d == 0) {
                lits.push_back(+xVars[j]);
            }
        }
        if (!lits.empty()) {
            int limit = max(0, k - 2);
            addAtMostK(cnf, lits, limit);
            cerr << "GCD constraint: at most " << limit << " of " << lits.size() 
                 << " velocities divisible by " << d << "\n";
        }
    }

    // ---- Output CNF in DIMACS format ----
    cnf.printDIMACS(cout);

    // Mapping for interpretation (to stderr, not stdout)
    cerr << "\nc Variable-to-velocity mapping:\n";
    for (int j = 0; j < numCandidates; ++j) {
        cerr << "c var " << xVars[j] << " <-> v = " << candidates[j] << "\n";
    }
    
    cerr << "\nCNF Statistics:\n";
    cerr << "  Variables: " << cnf.numVars << "\n";
    cerr << "  Clauses: " << cnf.clauses.size() << "\n";

    return 0;
}

