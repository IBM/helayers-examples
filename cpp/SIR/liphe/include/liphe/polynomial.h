#ifndef __POLYNOMIAL__
#define __POLYNOMIAL__

#include <assert.h>
#include <string.h>
#include <algorithm>

#include <NTL/ZZ_pX.h>

#include "thread_pool.h"
#include "binomial_tournament.h"


template<class Number>
class Polynomial {
private:
	std::vector<int> _coef;
	int _mod;

	void set(int n, const char *x = NULL) {
		_coef.resize(2);
		_coef[0] = n;

		if (x == NULL) {
			_coef.resize(1);
		} else if (strcmp(x, "-x") == 0) {
			_coef[1] = -1;
		} else if ((strcmp(x, "+x") == 0) || (strcmp(x, "x") == 0)) {
			_coef[1] = 1;
		} else {
			fprintf(stderr, "unknown term %s\n", x);
			exit(1);
		}
	}
public:
	Polynomial(const Polynomial<Number> &p) : _coef(p._coef), _mod(p._mod) {}
	Polynomial(const std::vector<int> &coef, int mod = 0) : _coef(coef), _mod(mod) {}

	Polynomial() : _mod(0) {}

	Polynomial(const char *x, int n) : _mod(0) { set(n, x); }
	Polynomial(int n, const char *x = NULL) : _mod(0) { set(n, x); }

	Polynomial &set_mod(int n) { _mod = n; return *this; }

	Polynomial &operator=(const Polynomial &p) {
		_coef = p._coef;
		_mod = p._mod;
		return *this;
	}

	Polynomial operator*(const Polynomial<Number> &p) const { Polynomial<Number> q(*this); q *= p; return q; }
	void operator*=(const Polynomial<Number> &p) {
		if (_mod == 0)
			_mod = p._mod;
		assert((_mod == p._mod) || (p._mod == 0));

		std::vector<int> c;
		c.resize(_coef.size() + p._coef.size());
		for (int i = 0; i < c.size(); ++i)
			c[i] = 0;

		for (int this_i = 0; this_i < _coef.size(); ++this_i) {
			for (int p_i = 0; p_i < p._coef.size(); ++p_i) {
				c[this_i + p_i] += _coef[this_i] * p._coef[p_i];
			}
		}

		_coef = c;

		apply_mod();
	}


	Polynomial<Number> operator+(const Polynomial<Number> &p) const { Polynomial<Number> q(*this); q += p; return q; }
	void operator+=(const Polynomial<Number> &p) {
		if (_mod == 0)
			_mod = p._mod;
		assert((_mod == p._mod) || (p._mod == 0));

		int prev_size = _coef.size();
		if (p._coef.size() > _coef.size())
			_coef.resize(p._coef.size());
		for (int i = prev_size - 1; i < _coef.size(); ++i)
			_coef[i] = 0;

		for (int i = 0; i < p._coef.size(); ++i) {
			_coef[i] += p._coef[i];
		}

		apply_mod();
	}

	Polynomial<Number> operator-(const Polynomial<Number> &p) const { Polynomial<Number> q(*this); q -= p; return q; }
	void operator-=(const Polynomial<Number> &p) { operator+=(-p); }

	Polynomial<Number> operator+(int p) const { return *this + Polynomial<Number>(p); }
	void operator+=(int p) { *this += Polynomial<Number>(p); }

	Polynomial<Number> operator-(int p) const { return *this - Polynomial<Number>(p); }
	void operator-=(int p) { *this -= Polynomial<Number>(p); }

	Polynomial<Number> operator-() const {
		Polynomial<Number> ret(*this);
		ret.set_mod(_mod);
		for (int i = 0; i < ret._coef.size(); ++i)
			ret._coef[i] = -ret._coef[i];
		ret.apply_mod();
		return ret;
	}

	// exponent of polynomials
	// no need to optimize this because this is happening in plain text anyway
	void operator^=(int p) { *this = (*this) ^ p; }
	Polynomial<Number> operator^(int p) const {
		if (p == 0)
			return Polynomial<Number>(1).set_mod(_mod);

		if (p == 1)
			return *this;

		Polynomial<Number> ret(*this);

		if ((p % 2) == 0) {
			ret *= ret;
			ret ^= p/2; 
			return ret;
//			return (ret*ret)^(p/2);
		}

		ret = ret * ((ret*ret)^(p/2));
		ret.apply_mod();
		return ret;
	}


	// composition
	Polynomial<Number> operator()(const Polynomial<Number> &p) {
		int mod = _mod;
		if (mod == 0)
			mod = p._mod;
		assert((mod == p._mod) || (p._mod == 0));

		Polynomial<Number> ret(0);
		ret.set_mod(mod);
		for (int i = 0; i < _coef.size(); ++i)
			ret += p^i * _coef[i];
		ret.apply_mod();
		return ret;
	}

	// Modulo, i.e. make it a polynomial in Z_p(x)
	// all coefficients are modulo p
	// and x^{p-1} = 1
	void apply_mod() { *this %= _mod; }
	Polynomial<Number> operator%(int p) const { Polynomial<Number> poly(*this);  poly %= p; return poly; }
	void operator%=(int p) {
		if (p == 0)
			return;

		std::vector<int> coef;

		unsigned int phi = ::phi(p);
		if (phi + 1 < _coef.size()) {
			coef.resize(phi + 1);
			for (unsigned int i = 0; i < coef.size(); ++i)
				coef[i] = _coef[i];
		} else {
			coef = _coef;
		}

		for (unsigned int i = coef.size(); i < _coef.size(); ++i) {
			// x^{phi+1} = x^1
			if ((i % phi) == 0)
				coef[phi] = _coef[i];
			else
				coef[i % phi] += _coef[i];
		}

		for (unsigned int i = 0; i < coef.size(); ++i)
			coef[i] %= p;

		_coef = coef;
	}

	int deg() const {
		int d = _coef.size() - 1;
		while ((d > 0) && (_coef[d] == 0))
			--d;
		return d;
	}


	void set_coef(int n, int c) {
		if (n >= _coef.size())
			_coef.resize(n + 1);
		_coef[n] = c;
	}

	static Number batch_coefficient(const std::vector<int> &coef, int start, int end, const Number *powers) {
		AddBinomialTournament<Number> ret;

		ret.add_to_tournament(Number(coef[start]));

		for (int i = start+1; i < end; ++i) {
			ret.add_to_tournament( powers[i-start] * coef[i] );
		}
		return ret.unite_all();
	}

	Number compute(const Number &x, const Number *powers, int batch_size, ThreadPool *threads) const {
		Number ret;
		compute(ret, x, powers, batch_size, threads);
		return ret;
	}

	void compute(Number &ret, const Number &x, const Number *powers, int batch_size, ThreadPool *threads) const {
		Polynomial<Number> p = (*this) % x.p();

//		for (int i = 0; i < coef.size(); ++i)
//			std::cout << "    coef[" << i << "] = " << coef[i];
//		std::cout << std::endl;

		int batch_number = (p.deg() + 1) / batch_size;

		Number *batch_multiplier = new Number[batch_number];
		// batch_multiplier holds the values of x^{sqrt_d}, x^{2sqrt_d}, ..., x^{d-1}
		if (batch_size == 1)
			batch_multiplier[1] = powers[(batch_size+1)/2];
		else
			batch_multiplier[1] = powers[batch_size/2] * powers[(batch_size+1)/2];


		AddBinomialTournament<Number> add_ret;
		std::mutex access_ret;

		std::function<void(Number)> add_to_ret([&add_ret, &access_ret](Number a) {
			access_ret.lock();
			add_ret.add_to_tournament(a);
			access_ret.unlock();
		});

		std::function<void(std::function<void(void)>) > run([threads](const std::function<void(void)> &f) {
			if (threads == NULL) {
				f();
			} else {
				threads->submit_job(f);
			}
		});

		run(std::function<void(void)>([&p, batch_size, powers, &add_to_ret](){
			add_to_ret( batch_coefficient(p._coef, /*start=*/ 0, /*end=*/batch_size, powers) );
		}));

		run(std::function<void(void)>([&p, batch_size, powers, batch_multiplier, &add_to_ret](){
			add_to_ret( batch_coefficient(p._coef, /*start=*/ batch_size, /*end=*/ std::min(2*batch_size, p.deg()+1), powers) * batch_multiplier[1] );
		}));


		for (int i = 2; i < batch_number; ++i) {
			int a = i/2;
			int b = (i+1)/2;
			batch_multiplier[i] = batch_multiplier[a] * batch_multiplier[b];

			run(std::function<void(void)>([&p, i, batch_size, powers, batch_multiplier, &add_to_ret]() {
				add_to_ret( batch_coefficient(p._coef, /*start=*/ i*batch_size, (i+1)*batch_size, powers) * batch_multiplier[i] );
			}));
		}

		if (batch_number * batch_size < p.deg()+1) {

			run(std::function<void(void)>([&p, batch_number, batch_size, powers, batch_multiplier, &add_to_ret]() {
				int a = batch_number/2;
				int b = (batch_number+1)/2;
				add_to_ret( batch_coefficient(p._coef, /*start=*/ batch_number*batch_size, p.deg()+1, powers) * batch_multiplier[a] * batch_multiplier[b] );
			}));
		}

		if (threads != NULL)
			threads->process_jobs();

//		for (int i = 2; i < batch_number; ++i) {
//			int a = i/2;
//			int b = (i+1)/2;
//			batch_multiplier[i] = batch_multiplier[a] * batch_multiplier[b];
//			ret += batch_coefficient(p._coef, /*start=*/ i*batch_size, (i+1)*batch_size, powers) * batch_multiplier[i];
//		}
//
//		if (batch_number * batch_size < p.deg()+1) {
//			int a = batch_number/2;
//			int b = (batch_number+1)/2;
//			Number batch_mult = batch_multiplier[a] * batch_multiplier[b];
//			ret += batch_coefficient(p._coef, /*start=*/ batch_number*batch_size, p.deg()+1, powers) * batch_mult;
//		}

		delete[] batch_multiplier;
		ret = add_ret.unite_all();
	}

	Number *compute_powers(const Number &x, int &batch_size) const {
		if (batch_size <= 0) {
			int degree = deg() % x.p();
			batch_size = sqrt(degree + 1);
			if (batch_size * batch_size < degree + 1)
				++batch_size;
		}

		// TODO: we are allocating and constructing powers[0] which we don't use.
		// would be nice to avoid alloc+construct
		Number *powers = new Number[batch_size];
		// powers holds the values of x^1, x^2, ..., x^{sqrt_d - 1}
		// powers[0] = 1  is not initialized to save encryption. handled explicitly below
		powers[1] = x;
		for (int i = 2; i < batch_size; ++i) {
			int a = i/2;
			int b = (i+1)/2;
			powers[i] = powers[a] * powers[b];
		}

		return powers;
	}

	Number compute(const Number &x, ThreadPool *threads = NULL) const {
		Number ret;
		compute(ret, x, threads);
		return ret;
	}

	void compute(Number &ret, const Number &x, ThreadPool *threads = NULL) const {
		Polynomial<Number> p = (*this) % x.p();

//		for (int i = 0; i < coef.size(); ++i)
//			std::cout << "    coef[" << i << "] = " << coef[i];
//		std::cout << std::endl;

//		int batch_size = sqrt(p.deg() + 1);
//		if (batch_size * batch_size < p.deg() + 1)
//			++batch_size;
		int batch_size = 0;
		
		Number *powers = compute_powers(x, batch_size);

		if (batch_size == 0) {
			std::cerr << "Error: batch size is 0\n";
			powers = compute_powers(x, batch_size);
		}

		compute(ret, x, powers, batch_size, threads);
		delete[] powers;


//		int batch_number = (p.deg() + 1) / batch_size;
//
//		Number *batch_multiplier = new Number[batch_number];
//		// batch_multiplier holds the values of x^{sqrt_d}, x^{2sqrt_d}, ..., x^{d-1}
//		if (batch_size == 1)
//			batch_multiplier[1] = powers[(batch_size+1)/2];
//		else
//			batch_multiplier[1] = powers[batch_size/2] * powers[(batch_size+1)/2];
//
//		Number ret = batch_coefficient(p._coef, /*start=*/ 0, /*end=*/batch_size, powers);
//		ret += batch_coefficient(p._coef, /*start=*/ batch_size, /*end=*/ std::min(2*batch_size, p.deg()+1), powers) * batch_multiplier[1];
//
//		for (int i = 2; i < batch_number; ++i) {
//			int a = i/2;
//			int b = (i+1)/2;
//			batch_multiplier[i] = batch_multiplier[a] * batch_multiplier[b];
//			ret += batch_coefficient(p._coef, /*start=*/ i*batch_size, (i+1)*batch_size, powers) * batch_multiplier[i];
//		}
//
//		if (batch_number * batch_size < p.deg()+1) {
//			int a = batch_number/2;
//			int b = (batch_number+1)/2;
//			Number batch_mult = batch_multiplier[a] * batch_multiplier[b];
//			ret += batch_coefficient(p._coef, /*start=*/ batch_number*batch_size, p.deg()+1, powers) * batch_mult;
//		}
//
//		delete[] powers;
//		delete[] batch_multiplier;
//
//		return ret;
	}

	// generate a polynomial such that y(x0) = y0  and y(x) = 0
	void interpolate(int x0, int y0, int mod) {
		*this = Polynomial(0);
 
		int c = 1;
		for (int x = 0; x < mod; ++x) {
			if (x != x0) {
				*this *= Polynomial("x",-x);
				c *= (x0 - x);
			}
		}
		c = power_mod(c, phi(mod) - 1, mod);
		*this *= c;
		*this *= y0;
		*this %= mod;
	}






	static inline const NTL::ZZ_pX poly_mul(const NTL::ZZ_pX &p1, const NTL::ZZ_pX &p2, int mod) {
		NTL::ZZ_pX ret = p1 * p2;

		int phi_mod = phi(mod);
		for (int i = mod; i <= NTL::deg(ret); ++i) {
			int c = i % phi_mod;
			if (c == 0)
				c = phi_mod;
			ret[c] = ret[c] + ret[i];
			ret[i] = 0;
		}
		ret.normalize();
		return ret;
	}

	static inline const NTL::ZZ_pX poly_power(const NTL::ZZ_pX &poly, int e, int mod) {
		if (e == 1)
			return poly;

		if ((e % 2) == 0)
			return poly_power(poly_mul(poly, poly, mod), e/2, mod);

		return poly_mul(poly, poly_power(poly, e-1, mod), mod);
	}


	// return the polynomial (x-y)
	static NTL::ZZ_pX x_(int y) { return NTL::ZZ_pX(NTL::INIT_MONO, 0, -y) + NTL::ZZ_pX(NTL::INIT_MONO, 1, 1); }


	static Polynomial<Number> build_polynomial(int p, int range_end, const std::function<int(int)> &func) { return build_polynomial(p, 0, range_end, func); }

	static Polynomial<Number> build_polynomial(int p, int range_start, int range_end, const std::function<int(int)> &func) {
		NTL::ZZ_p::init(NTL::ZZ(p));

		NTL::ZZ_pX poly(NTL::INIT_MONO, 0, 0);

		int phi_p = phi(p);

		int x = range_start;

		while (x < range_end) {
			while ((x < range_end) && (func(x) == 0))
				++x;

			int y = func(x);

			NTL::ZZ_pX ind(NTL::INIT_MONO, 0, 1);
//std::cout << ind << std::endl;

			while ((x < range_end) && ((func(x) == y) || (func(x) == 0))) {
				if (func(x) == y) {
//std::cout << "adding x = " << x << std::endl;
					ind = poly_mul(ind, x_(x), p);
				}
				++x;
			}
			// now ind is an inverse indicator getting 0 for x iff func(x)=y

			ind = poly_power(ind, phi_p, p);
			// now ind is an inverse indicator getting 0 for x iff func(x)=y, 1 otherwise

			ind = NTL::ZZ_pX(NTL::INIT_MONO, 0, 1) - ind;
			// now ind is an inverse indicator getting 1 for x iff func(x)=y, 0 otherwise

			poly += poly_mul(ind, NTL::ZZ_pX(NTL::INIT_MONO, 0, y), p);
//std::cout << "poly = " << poly << std::endl;

		}
//std::cout << "poly = " << poly << std::endl;
	
		std::vector<int> coef(NTL::deg(poly) + 1);
		for (int i = 0; i < NTL::deg(poly)+1; ++i)
			conv(coef[i], poly[i]);

		Polynomial<Number> ret = Polynomial(coef, p);
//for (int xx = 0; xx < range; ++xx) {
//	std::cout << (ret.compute(Number(xx))).to_int() << "  ";
//}
//std::cout << std::endl;
//exit(0);
		return ret;
	}


};



#endif
