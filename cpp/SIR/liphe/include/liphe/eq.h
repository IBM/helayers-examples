#ifndef __EQ__
#define __EQ__

#include <iostream>

#include "primes.h"


template<class CLASS>
CLASS power(const CLASS &x, long e) {
	assert(e > 0);
	if (e == 2) {
		CLASS y = x * x;
		return y;
	}
	if (e == 1)
		return x;

	if (e & 1) {
		return x*power(x, e-1);
	} else {
		return power(x*x, e/2);
	}
}

template<class CLASS>
CLASS power_mod(const CLASS &x, long e, long mod) {
	assert(e > 0);
	if (e == 2) {
		CLASS y = (x * x) % mod;
		return y;
	}
	if (e == 1) {
		return x;
	}

	if (e & 1) {
		return (x*power_mod(x, e-1, mod)) % mod;
	} else {
		return power_mod((x*x) % mod, e/2, mod);
	}
}

template<class CLASS>
CLASS isNonZero_Euler(const CLASS &r) {
	int p = r.get_ring_size();
	r.assert_co_prime(p);
	return power(r, phi(p));
}

template<class CLASS>
class IsNonZeroEulerStrategy {
public:
	static CLASS is(const CLASS &r) { return isNonZero_Euler(r); }
	static CLASS is_not(const CLASS &r) { return isZero_Euler(r); }
};


template<class CLASS>
CLASS isZero_Euler(const CLASS &r) { return CLASS::static_from_int(1) - isNonZero_Euler(r); }

template<class CLASS>
class IsZeroEulerStrategy {
public:
	static CLASS is(const CLASS &r) { return isZero_Euler(r); }
	static CLASS is_not(const CLASS &r) { return isNonZero_Euler(r); }
};






template<class CLASS>
CLASS neq_euler(const CLASS &a, const CLASS &b) {
	assert(a.get_ring_size() == b.get_ring_size());
	CLASS r = a - b;

	return isNonZero_Euler(r);
}

template<class CLASS>
CLASS eq_euler(const CLASS &a, const CLASS &b) { return CLASS::static_from_int(1) - neq_euler(a, b); }

#endif

