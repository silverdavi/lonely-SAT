// SAT encoding for Lonely Runner Conjecture verification
// Translates Rosenfeld's verification into CNF (DIMACS format)
// 
// David H. Silver, 2025
// Based on Matthieu Rosenfeld's verification approach (arXiv:2509.14111)
// GitHub Copilot was used for code formatting and boilerplate

#include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <functional>
#include <map>
#include <unordered_map>
#include <chrono>
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

// CNF builder

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

// Sequential counter with both directions: s[n][R] true iff ≥R of xs true
// Enforces both "at most R" and enables "at least R" via return value
int buildSequentialCounter(CNF& cnf, const vector<int>& xs, int R) {
    int n = (int)xs.size();
    if (R > n || R <= 0 || n == 0) return 0;

    // s[i][j] = true iff at least j of the first i literals are true
    vector<vector<int>> s(n + 1, vector<int>(R + 1, 0));

    // Base: i=1
    s[1][1] = cnf.newVar();
    cnf.addClause({ -xs[0], s[1][1] });      // x1 -> s[1][1]
    cnf.addClause({ -s[1][1], xs[0] });      // s[1][1] -> x1

    // DP
    for (int i = 2; i <= n; ++i) {
        int xi = xs[i - 1];

        for (int j = 1; j <= min(i, R); ++j) {
            s[i][j] = cnf.newVar();

            // Forward: s[i][j] can be made true if...
            if (j <= i - 1 && s[i - 1][j] != 0) {
                cnf.addClause({ -s[i - 1][j], s[i][j] });  // s[i-1][j] -> s[i][j]
            }
            if (j >= 2 && s[i - 1][j - 1] != 0) {
                cnf.addClause({ -xi, -s[i - 1][j - 1], s[i][j] });  // (xi ∧ s[i-1][j-1]) -> s[i][j]
            }
            if (j == 1) {
                cnf.addClause({ -xi, s[i][j] });  // xi -> s[i][1]
            }

            // Backward: if s[i][j] is true, it must be justified
            if (j == 1) {
                // s[i][1] -> (s[i-1][1] ∨ xi)
                if (s[i - 1][1] != 0) {
                    cnf.addClause({ -s[i][j], s[i - 1][1], xi });
                } else {
                    cnf.addClause({ -s[i][j], xi });
                }
            } else if (j <= i - 1) {
                // s[i][j] -> (s[i-1][j] ∨ (xi ∧ s[i-1][j-1]))
                if (s[i - 1][j] != 0) {
                    cnf.addClause({ -s[i][j], s[i - 1][j], xi });
                }
                if (s[i - 1][j - 1] != 0 && s[i - 1][j] != 0) {
                    cnf.addClause({ -s[i][j], s[i - 1][j], s[i - 1][j - 1] });
                }
            }
        }

        // At most R: forbid having ≥R trues in prefix and adding xi
        if (s[i - 1][R] != 0) {
            cnf.addClause({ -xi, -s[i - 1][R] });
        }
    }

    return s[n][R];
}

// At most R of xs are true
void addAtMostK(CNF& cnf, const vector<int>& xs, int R) {
    if ((int)xs.size() == 0 || R >= (int)xs.size()) return;
    buildSequentialCounter(cnf, xs, R);  // Build counter, don't assert s[n][R]
}

// Exactly K of xs must be true
void addExactlyK(CNF& cnf, const vector<int>& xs, int Kexact) {
    int N = (int)xs.size();
    if (Kexact < 0 || Kexact > N || N == 0) return;

    int sAtLeastK = buildSequentialCounter(cnf, xs, Kexact);
    if (sAtLeastK != 0) {
        cnf.addClause({ sAtLeastK });  // Force ≥K
    }
}

// Preprocessing: dominance reduction

vector<int> getPrimeDivisors(int n) {
    vector<int> divisors;
    if (n == 3 || n == 5 || n == 7 || n == 11 || n == 13) {
        divisors.push_back(n);
    } else if (n == 4 || n == 8 || n == 16) {
        divisors.push_back(2);
    } else if (n == 6) {
        divisors.push_back(2);
        divisors.push_back(3);
    } else if (n == 9) {
        divisors.push_back(3);
    } else if (n == 10) {
        divisors.push_back(2);
        divisors.push_back(5);
    } else if (n == 12) {
        divisors.push_back(2);
        divisors.push_back(3);
    } else if (n == 14) {
        divisors.push_back(2);
        divisors.push_back(7);
    } else if (n == 15) {
        divisors.push_back(3);
        divisors.push_back(5);
    }
    return divisors;
}

// Check if velocity a dominates b (covers everything b does, GCD at least as restrictive)
bool dominatesVelocity(int a, int b, const vector<bitset<maxM>>& nearZero, const vector<int>& primeDivisors) {
    if ((nearZero[a] | nearZero[b]) != nearZero[a]) return false;
    
    for (int q : primeDivisors) {
        if (b % q == 0 && a % q != 0) return false;
    }
    
    return true;
}

vector<int> reduceCandidatesByDominance(const vector<int>& candidates, 
                                         const vector<bitset<maxM>>& nearZero,
                                         const vector<int>& primeDivisors) {
    int numCand = candidates.size();
    vector<bool> dominated(numCand, false);
    
    for (int i = 0; i < numCand; ++i) {
        if (dominated[i]) continue;
        for (int j = 0; j < numCand; ++j) {
            if (i == j || dominated[j]) continue;
            if (dominatesVelocity(candidates[i], candidates[j], nearZero, primeDivisors)) {
                dominated[j] = true;
            }
        }
    }
    
    vector<int> reduced;
    for (int i = 0; i < numCand; ++i) {
        if (!dominated[i]) reduced.push_back(candidates[i]);
    }
    return reduced;
}

vector<bitset<10000>> buildCoverageSets(const vector<int>& candidates,
                                         const vector<bitset<maxM>>& nearZero) {
    vector<bitset<10000>> cover;
    cover.reserve(maxM);
    
    for (int t = 0; t < maxM; ++t) {
        bitset<10000> coverSet;
        for (int j = 0; j < (int)candidates.size(); ++j) {
            if (nearZero[candidates[j]][t]) coverSet[j] = true;
        }
        cover.push_back(coverSet);
    }
    return cover;
}

vector<int> reduceTimesByDominance(const vector<bitset<10000>>& cover) {
    int numTimes = cover.size();
    vector<bool> redundant(numTimes, false);
    
    for (int t1 = 0; t1 < numTimes; ++t1) {
        if (redundant[t1]) continue;
        for (int t2 = 0; t2 < numTimes; ++t2) {
            if (t1 == t2 || redundant[t2]) continue;
            // If cover[t1] ⊆ cover[t2], then t2 redundant (clause implied)
            if ((cover[t1] | cover[t2]) == cover[t2]) redundant[t2] = true;
        }
    }
    
    vector<int> essential;
    for (int t = 0; t < numTimes; ++t) {
        if (!redundant[t]) essential.push_back(t);
    }
    return essential;
}

// Main encoding

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cerr << "k = " << k << ", n = " << n
         << ", prime = " << prime
         << ", Q = " << Q
         << ", maxM = " << maxM << "\n";

    // Precompute coverage: nearZero[v][t] = (||tv/Q|| < 1/n)
    vector<bitset<maxM>> nearZero;
    nearZero.reserve(maxM + 1);

    for (int i = 0; i <= maxM; ++i) {
        bitset<maxM> currLine;
        for (int t = 1; t <= maxM; ++t) {
            int ti = (t * i) % Q;
            bool close = (ti * n < Q) || ((Q - ti) * n < Q);
            currLine[maxM - t] = close;
        }
        nearZero.push_back(currLine);
    }

    auto t_start = chrono::high_resolution_clock::now();

    // Initial candidates: [1..maxM] \ pZ
    vector<int> candidates;
    candidates.reserve(maxM);
    for (int i = 1; i <= maxM; ++i) {
        if (i % prime == 0) continue;
        candidates.push_back(i);
    }

    int numCandidatesInitial = (int)candidates.size();
    cerr << "Initial candidates: " << numCandidatesInitial << "\n";

    vector<int> primeDivisors = getPrimeDivisors(n);
    
    // Preprocessing: velocity dominance
    auto t_dom_start = chrono::high_resolution_clock::now();
    candidates = reduceCandidatesByDominance(candidates, nearZero, primeDivisors);
    auto t_dom_end = chrono::high_resolution_clock::now();
    
    cerr << "After velocity dominance: " << candidates.size()
         << " (eliminated " << (numCandidatesInitial - candidates.size()) << ")\n";
    cerr << "  Time: " << chrono::duration<double>(t_dom_end - t_dom_start).count() << "s\n";

    auto coverSets = buildCoverageSets(candidates, nearZero);
    
    // Preprocessing: time dominance
    auto t_time_start = chrono::high_resolution_clock::now();
    vector<int> essentialTimes = reduceTimesByDominance(coverSets);
    auto t_time_end = chrono::high_resolution_clock::now();
    
    cerr << "After time dominance: " << essentialTimes.size()
         << " (eliminated " << (maxM - essentialTimes.size()) << ")\n";
    cerr << "  Time: " << chrono::duration<double>(t_time_end - t_time_start).count() << "s\n";

    auto t_preprocess_end = chrono::high_resolution_clock::now();
    cerr << "Total preprocessing: " 
         << chrono::duration<double>(t_preprocess_end - t_start).count() << "s\n\n";

    int numCandidates = (int)candidates.size();

    // CNF builder
    CNF cnf;

    // Variables: one per candidate
    vector<int> xVars(numCandidates);
    for (int j = 0; j < numCandidates; ++j) {
        xVars[j] = cnf.newVar();
    }

    // Coverage clauses
    int uncoverable_count = 0;
    for (int t : essentialTimes) {
        vector<int> clause;
        for (int j = 0; j < numCandidates; ++j) {
            if (nearZero[candidates[j]][t]) {
                clause.push_back(xVars[j]);
            }
        }
        if (clause.empty()) {
            uncoverable_count++;
            if (uncoverable_count == 1) {
                int dummy = cnf.newVar();
                cnf.addClause({dummy});
                cnf.addClause({-dummy});
                cerr << "Uncoverable position found\n";
            }
        } else {
            cnf.addClause(clause);
        }
    }

    if (uncoverable_count > 0) {
        cerr << "Trivially UNSAT (" << uncoverable_count << " uncoverable)\n";
    }

    // Exactly k chosen
    addExactlyK(cnf, xVars, k);

    // GCD constraints: at most k-2 multiples of each prime dividing (k+1)

    for (int d : primeDivisors) {
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

    // Output CNF
    cnf.printDIMACS(cout);

    // Variable mapping (to stderr)
    cerr << "\nc Variable mapping:\n";
    for (int j = 0; j < numCandidates; ++j) {
        cerr << "c var " << xVars[j] << " <-> v = " << candidates[j] << "\n";
    }
    
    cerr << "\nCNF: " << cnf.numVars << " vars, " << cnf.clauses.size() << " clauses\n";

    return 0;
}

