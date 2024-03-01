#ifndef ___PRIME_LIPHE___
#define ___PRIME_LIPHE___

#include <vector>

struct Primes {
	static unsigned long find_prime_bigger_than(unsigned long p);
	static unsigned long prime(int p);
	static int prime_no();
};

class Factorization {
	std::vector<unsigned long> primes;
	std::vector<int> multiplicity;

	int find_prime(unsigned long p) {
		unsigned long i;
		for (i = 0; i < primes.size(); ++i)
			if (primes[i] == p)
				return i;
		
		multiplicity.push_back(0);
		primes.push_back(p);
		return i;
	}

	void add_factor(int p) { ++multiplicity[find_prime(p)]; }
public:
	Factorization() {}
	Factorization(int p) { factor(p); }

	void factor(unsigned long x) {
		int p_i = 0;
		while ((p_i < Primes::prime_no()) && (x != 1) && (Primes::prime(p_i) <= x)) {
			while (((x % Primes::prime(p_i)) == 0) && (x != 1)) {
				add_factor(Primes::prime(p_i));
				x /= Primes::prime(p_i);
			}
			++p_i;
		}
		assert(x == 1);
	}

	int factors() const { return primes.size(); }
	unsigned long get_prime(int i) const { return primes[i]; }
	int get_multiplicity(int i) const { return multiplicity[i]; }
};

template<class CLASS>
CLASS power(const CLASS &x, long e);

inline int phi(int p) {
	int phi = 1;

	Factorization f(p);
	for (int i = 0; i < f.factors(); ++i) {
		if (f.get_multiplicity(i) == 1) {
			 phi *= f.get_prime(i) - 1;
		} else {
			 phi *= power(f.get_prime(i), f.get_multiplicity(i)) - power(f.get_prime(i), f.get_multiplicity(i) - 1);
		}
	}

	return phi;
}

#include "eq.h"

#endif

