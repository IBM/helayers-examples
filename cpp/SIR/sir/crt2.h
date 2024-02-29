#ifndef ___CRT___
#define ___CRT___

#include <assert.h>
#include <vector>
#include "times.h"

#include <boost/multiprecision/cpp_int.hpp>
using boost::multiprecision::cpp_int;
using namespace std;

clock_t decodeTime = 0;

// CrtContainer - keeps the keys for multiple CRT digits
// CrtDigit - keeps: (1) the value of one CRT digit; (2) a pointer to the CrtContainer it is a part of; (3) a pointer the keys
// CrtBundle - keeps multiple CrtDigits

// Usage 1:
//		foreach (crtDigit) {
//			crtBundleOut += Alg(crtDigit)
//		}
//		out = crtBundleOut.to_int()

// Usage 2:
//		crtBundleOut = Alg(crtBundleIn)
//		out = crtBundleOut.to_int()

cpp_int powerMod(const cpp_int &x, const cpp_int &e, const cpp_int &mod) {
	assert(e > 0);
	if (e == 2) {
		cpp_int y = (x * x) % mod;
		return y;
	}
	if (e == 1) {
		return x;
	}

	if (e & 1) {
		return (x*powerMod(x, e-1, mod)) % mod;
	} else {
		return powerMod((x*x) % mod, e/2, mod);
	}
}



template<class Keys, class X>
class CrtContainer {
public:
	CrtContainer() {}
	CrtContainer(const CrtContainer<Keys, X> &c) : m_keys(c.m_keys) {}

	void add_key(const Keys *k) { m_keys.push_back(k); }

	const Keys *key(int i) const { return m_keys[i]; }

	size_t key_number() const { return m_keys.size(); }

	size_t simd_factor() const {
		assert(key_number() > 0);
		unsigned long s = key(0)->simd_factor();
		for (size_t i = 1; i < key_number(); ++i)
			s = std::min(s, key(i)->simd_factor());
		return s;
	}

	cpp_int get_ring_size() const {
		cpp_int r = 1;
		for (auto k : m_keys)
			r *= k->get_ring_size();
		return r;
	}

	cpp_int get_phi() const {
		cpp_int r = 1;
		for (auto k : m_keys)
			r *= k->get_ring_size() - 1;
		return r;
	}

	void init_inversion_keys() {
		product.resize(m_keys.size());
		phiProduct.resize(m_keys.size());
		product[0] = m_keys[0]->get_ring_size();
		phiProduct[0] = m_keys[0]->get_ring_size() - 1;
		for (size_t i = 1; i < m_keys.size(); ++i) {
			product[i] = product[i-1] * m_keys[i]->get_ring_size();
			phiProduct[i] = phiProduct[i-1] * (m_keys[i]->get_ring_size() - 1);
		}

		productInv.resize(m_keys.size());
#		pragma omp parallel for
		for (size_t i = 0; i < m_keys.size() - 1; ++i) {
			productInv[i] = powerMod(product[i], phiProduct[i+1]-1, product[i+1]);
		}
	}

	const std::vector<cpp_int> &getProductInv() const { return productInv; }
	const std::vector<cpp_int> &getProduct() const { return product; }

private:
	std::vector<const Keys *> m_keys;
	// inversion keys
	std::vector<cpp_int> product;
	std::vector<cpp_int> phiProduct;
	std::vector<cpp_int> productInv;
};

template<class Keys, class X>
class CrtDigit {
public:
	CrtDigit() : m_keys(NULL), m_container(NULL) {} // To allow std::vector
	CrtDigit(const X &v, const Keys *k, const CrtContainer<Keys, X> *c) : m_val(v), m_keys(k), m_container(c) {}
	CrtDigit(const X &v, CrtDigit<Keys, X> &c) : m_val(v), m_keys(c.m_keys), m_container(c.m_container) {}
	CrtDigit(const CrtDigit<Keys, X> &c) : m_val(c.m_val), m_keys(c.m_keys), m_container(c.m_container) {}
	CrtDigit(CrtDigit<Keys, X> &&c) : m_val(std::move(c.m_val)), m_keys(c.m_keys), m_container(c.m_container) {}

	CrtDigit<Keys, X> operator=(const CrtDigit<Keys, X> &c) { m_val = c.m_val; m_keys = c.m_keys; m_container = c.m_container; return *this; }
	CrtDigit<Keys, X> operator=(CrtDigit<Keys, X> &&c) { m_val = std::move(c.m_val); m_keys = c.m_keys; m_container = c.m_container; return *this; }

	unsigned long get_ring_size() const { return m_val.get_ring_size(); }
	unsigned int simd_factor() const { return m_val.simd_factor(); }

	const CrtContainer<Keys, X> *container() const { return m_container; }

	X &val() { return m_val; }
	const X &val() const { return m_val; }

	void from_int(unsigned long n) {
		m_keys->from_int(m_val, n);
	}

	void from_bigint(const cpp_int &n) {
		m_keys->from_bigint(m_val, n);
	}

	void from_vector(const std::vector<long> &n) {
		m_keys->from_vector(m_val, n);
	}

	long to_int() const {
		long ret = m_val.to_int();
		return ret;
	}

	std::vector<long> to_vector() const {
		std::vector<long> ret = m_val.to_vector();
		return ret;
	}

	CrtDigit<Keys, X> operator-() const {
		CrtDigit<Keys, X> ret(*this);
		ret.additive_inverse();
		return ret;
	}

	CrtDigit<Keys, X> operator-(const CrtDigit<Keys, X> &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret -= c;
		return ret;
	}

	CrtDigit<Keys, X> operator+(const CrtDigit<Keys, X> &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret += c;
		return ret;
	}

	CrtDigit<Keys, X> operator*(const CrtDigit<Keys, X> &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret *= c;
		return ret;
	}

	CrtDigit<Keys, X> operator*(const int &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret *= c;
		return ret;
	}

	void additive_inverse() {
		m_val = - m_val;
	}

	void operator-=(const CrtDigit<Keys, X> &c) {
		assert((m_keys == c.m_keys) && (m_container == c.m_container));
		m_val -= c.m_val;
	}

	void operator+=(const CrtDigit<Keys, X> &c) {
		assert((m_keys == c.m_keys) && (m_container == c.m_container));
		m_val += c.m_val;
	}

	void operator+=(unsigned long c) {
		m_val += c;
	}


//	void operator+=(long c) {
//		for (size_t i = 0; i < m_digits.size(); ++i)
//			m_digits[i] += c;
//	}

//	void operator*=(const Crt<Keys, X> &c) {
//		assert((m_keys == c.m_keys) && (m_container == c.m_container));
//		assert(m_digits.size() == c.m_digits.size());
//		for (size_t i = 0; i < m_digits.size(); ++i)
//			m_digits[i] *= c.m_digits[i];
//	}

	void operator*=(const CrtDigit<Keys, X> &c) {
		assert((m_keys == c.m_keys) && (m_container == c.m_container));
		m_val *= c.m_val;
	}

	void operator*=(const int &c) {
		m_val *= c;
	}

	template<class Keys2, class X2>
	CrtDigit<Keys, X> operator+(const CrtDigit<Keys2, X2> &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret += c;
		return ret;
	}

	template<class Keys2, class X2>
	void operator+=(const CrtDigit<Keys2, X2> &c) {
		m_val += c.val();
	}

	template<class Keys2, class X2>
	CrtDigit<Keys, X> operator*(const CrtDigit<Keys2, X2> &c) const {
		CrtDigit<Keys, X> ret(*this);
		ret *= c;
		return ret;
	}

	template<class Keys2, class X2>
	void operator*=(const CrtDigit<Keys2, X2> &c) {
		m_val *= c.val();
	}

	void shiftRight(int k) {
		m_val.self_rotate_right(k);
	}

	void shiftLeft(int k) {
		m_val.self_rotate_left(k);
	}

	void save(ostream &s) const {
		m_val.save(s);
	}

private:
	X m_val;
	const Keys *m_keys;
	const CrtContainer<Keys, X> *m_container;
	
};




template<class Keys, class X>
class CrtBundle {
private:
public:
	CrtBundle() : m_container(NULL) {}
	CrtBundle(const CrtBundle<Keys,X> &b) : m_digits(b.m_digits), m_container(b.m_container) {}
	CrtBundle(CrtBundle<Keys,X> &&b) : m_digits(std::move(b.m_digits)), m_container(b.m_container) {}

	CrtBundle(const CrtContainer<Keys, X> &pubKey, unsigned int v) : m_container(NULL) {
		setPubKey(&pubKey);
		from_int(v, &pubKey);
	}

	template<class PlainKeys, class PlainText>
	CrtBundle(const CrtContainer<Keys, X> &pubKey, const CrtBundle<PlainKeys, PlainText> &plainText) {
		setPubKey(&pubKey);

		m_digits.resize(m_container->key_number());

		for (size_t i = 0; i < m_digits.size(); ++i) {
			convert(m_digits[i], plainText.digit(i), i, m_container);
		}
	}

	CrtBundle<Keys, X> &operator=(const CrtBundle<Keys, X> &b) { m_container = b.m_container; m_digits = b.m_digits; return *this; }
	CrtBundle<Keys, X> &operator=(CrtBundle<Keys, X> &&b) { m_container = b.m_container; m_digits = std::move(b.m_digits); return *this; }

	void setPubKey(const CrtContainer<Keys, X> *p) { m_container = p; }
	const CrtContainer<Keys, X> *pubKey() const { return m_container; }

	CrtBundle<Keys, X> clone_value(unsigned int v) const {
		return CrtBundle<Keys, X>(*m_container, v);
	}

	size_t digit_number() const { return m_digits.size(); }
	void digit_number(size_t i) { m_digits.resize(i); }

	CrtDigit<Keys,X> &digit(int i) { return m_digits[i]; }
	const CrtDigit<Keys,X> &digit(int i) const { return m_digits[i]; }

	unsigned int simd_factor() const {
		assert(m_digits.size() > 0);
		unsigned int simd = m_container->simd_factor();
		for (auto const &digit : m_digits) {
			simd = std::min(digit.simd_factor(), simd);
		}
		return simd;
	}

	void from_int(unsigned long v, const CrtContainer<Keys, X> *pubKey) {
		if (pubKey != NULL)
			setPubKey(pubKey);

		m_digits.resize(m_container->key_number());

#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			m_digits[i] = CrtDigit<Keys,X>(m_container->key(i)->from_int(v), m_container->key(i), m_container);
		}
	}

	void from_bigint(const cpp_int &v, const CrtContainer<Keys, X> *pubKey) {
		if (pubKey != NULL)
			setPubKey(pubKey);

		m_digits.resize(m_container->key_number());

#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			m_digits[i] = CrtDigit<Keys,X>(m_container->key(i)->from_bigint(v), m_container->key(i), m_container);
		}
	}

	void from_vector(const std::vector<long> &n, const CrtContainer<Keys, X> *pubKey) {
		if (pubKey != NULL)
			setPubKey(pubKey);

		m_digits.resize(m_container->key_number());

#		pragma omp parallel for
		for (size_t i = 0; i < digit_number(); ++i) {
			m_digits[i] = CrtDigit<Keys,X>(m_container->key(i)->from_vector(n), m_container->key(i), m_container);
		}
	}

	void from_vector(const std::vector<cpp_int> &n, const CrtContainer<Keys, X> *pubKey) {
		if (pubKey != NULL)
			setPubKey(pubKey);

		m_digits.resize(m_container->key_number());

#		pragma omp parallel for
		for (size_t i_digit = 0; i_digit < digit_number(); ++i_digit) {
			std::vector<long> modN(n.size());
			long mod = m_container->key(i_digit)->get_ring_size();
			for (size_t i_n = 0; i_n < modN.size(); ++i_n)
				modN[i_n] = static_cast<long>(n[i_n] % mod);
			m_digits[i_digit] = CrtDigit<Keys,X>(m_container->key(i_digit)->from_vector(modN), m_container->key(i_digit), m_container);
		}
	}

// 	long to_int() const {
// 		if (m_digits.size() == 0)
// 			return 0;
	
// 		long ret = m_digits[0].to_int();
// 		long product = m_digits[0].get_ring_size();
// 		for (unsigned int i = 1; i < m_digits.size(); ++i) {
// 			ret += mod(ret - m_digits[i].to_int(), m_digits[i].get_ring_size()) * product;
// 			product *= m_digits[i].get_ring_size();
// 		}
	
// //		std::cout << "CRT decoding: " << std::endl;
// //		for (unsigned int i = 0; i < d.size(); ++i) {
// //			std::cout << d[i].to_int() << " mod " << d[i].base() << std::endl;
// //		}
// //		std::cout << "turned out to be: x = " << ret << std::endl << std::endl;
	
// 		return ret;
// 	}

	cpp_int to_bigint(const CrtContainer<Keys, X> *privKey) const {
		return to_bigint_vector(privKey, 1)[0];
	}

	void save(ostream &s) const {
		for (auto &d : m_digits)
			d.save(s);
	}

// 	std::vector<long> to_vector() const {
// 		if (m_digits.size() == 0)
// 			return std::vector<long>();
	
// 		size_t simd = digit(0).simd_factor();
// 		for (size_t i_digit = 1; i_digit < m_digits.size(); ++i_digit) {
// 			if (simd > digit(i_digit).simd_factor())
// 				simd = digit(i_digit).simd_factor();
// 		}

// 		std::vector<long> ret = m_digits[0].to_vector();
// 		ret.resize(simd);

// 		long product = m_digits[0].get_ring_size();
// 		for (unsigned int i_digit = 1; i_digit < m_digits.size(); ++i_digit) {
// 			std::vector<long> m = m_digits[i_digit].to_vector();
// 			for (size_t i_simd = 0; i_simd < simd; ++i_simd) {
// 				ret[i_simd] += mod(ret[i_simd] - m[i_simd], m_digits[i_digit].get_ring_size()) * product;
// 			}
// 			product *= m_digits[i_digit].get_ring_size();
// 		}

// //		std::cout << "CRT decoding: " << std::endl;
// //		for (unsigned int i = 0; i < d.size(); ++i) {
// //			std::cout << d[i].to_int() << " mod " << d[i].base() << std::endl;
// //		}
// //		std::cout << "turned out to be: x = " << ret << std::endl << std::endl;
	
// 		return ret;
// 	}

	cpp_int inv(const cpp_int &x, const cpp_int &p) const {
		return powerMod(x, p-2, p);
	}

	std::vector<cpp_int> to_bigint_vector(const CrtContainer<Keys, X> *privKey, int slots = -1) const {
		if (m_digits.size() == 0)
			return std::vector<cpp_int>();

		decodeTime -= clock();
		size_t simd = digit(0).simd_factor();
		for (size_t i_digit = 1; i_digit < m_digits.size(); ++i_digit) {
			if (simd > digit(i_digit).simd_factor())
				simd = digit(i_digit).simd_factor();
		}

		std::vector<long> temp = m_digits[0].to_vector();
		std::vector<cpp_int> ret;
		ret.resize(simd);
		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = temp[i];

		// decrypt
		vector<vector<long>> messages(m_digits.size());
#		pragma omp parallel for
		for (unsigned int i_digit = 0; i_digit < m_digits.size(); ++i_digit) {
			messages[i_digit] = m_digits[i_digit].to_vector();
		}

		// product[i] = digit[0].ringSize * ... digit[i].ringSize
		// phiProduct[i] = phi(product[i])
		// std::vector<cpp_int> product(m_digits.size());
		// std::vector<cpp_int> phiProduct(m_digits.size());
		// product[0] = m_digits[0].get_ring_size();
		// phiProduct[0] = m_digits[0].get_ring_size() - 1;
		// for (size_t i = 1; i < m_digits.size(); ++i) {
		// 	product[i] = product[i-1] * m_digits[i].get_ring_size();
		// 	phiProduct[i] = phiProduct[i-1] * (m_digits[i].get_ring_size() - 1);
		// }

		// productInv[i] * product[i] = 1   mod product[i+1]
// 		std::vector<cpp_int> productInv(m_digits.size());
// #		pragma omp parallel for
// 		for (size_t i = 0; i < m_digits.size() - 1; ++i) {
// 			productInv[i] = powerMod(product[i], phiProduct[i+1]-1, product[i+1]);
// 		}

		const std::vector<cpp_int> &productInv = m_container->getProductInv();
		const std::vector<cpp_int> &product = m_container->getProduct();
		if (slots == -1)
			slots = simd;
		else
			slots = min(slots, (int)simd);

		for (int i_simd = 0; i_simd < slots; ++i_simd) {
			// ret already contains messages[0]

			for (unsigned int i_digit = 1; i_digit < m_digits.size(); ++i_digit) {
				vector<long> &m = messages[i_digit];
				long digit = m_digits[i_digit].get_ring_size();
				cpp_int retDigit = mod(ret[i_simd], digit);
				if (m[i_simd] != retDigit) {
					cpp_int adjustment = mod(m[i_simd] - retDigit + digit, digit);
					ret[i_simd] += mod(adjustment * product[i_digit - 1] * productInv[i_digit - 1], product[i_digit]);
				}
			}
		}

		decodeTime += clock();
		return ret;
	}

	CrtBundle<Keys, X> operator+(const CrtBundle<Keys, X> &b) const {
		CrtBundle<Keys, X> ret(*this);
		ret += b;
		return ret;
	}

	CrtBundle<Keys, X> operator-() const {
		CrtBundle<Keys, X> ret(*this);
		ret.additive_inverse();
		return ret;
	}

	CrtBundle<Keys, X> operator-(const CrtBundle<Keys, X> &b) const {
		CrtBundle<Keys, X> ret(*this);
		ret -= b;
		return ret;
	}

	CrtBundle<Keys, X> operator*(const CrtBundle<Keys, X> &b) const {
		CrtBundle<Keys, X> ret(*this);
		ret *= b;
		return ret;
	}

	template<class Keys2, class X2>
	CrtBundle<Keys, X> operator*(const CrtBundle<Keys2, X2> &b) const {
		CrtBundle<Keys, X> ret(*this);
		ret *= b;
		return ret;
	}

	CrtBundle<Keys, X> operator*(int b) const {
		CrtBundle<Keys, X> ret(*this);
		ret *= b;
		return ret;
	}

	CrtBundle<Keys, X> operator*(const cpp_int &b) const {
		CrtBundle<Keys, X> ret(*this);
		ret *= b;
		return ret;
	}

	void additive_inverse() {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i)
			m_digits[i].additive_inverse();
	}

	void operator-=(const CrtBundle<Keys, X> &c) {
		assert(m_digits.size() == c.m_digits.size());
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i)
			m_digits[i] -= c.m_digits[i];
	}

	void operator+=(const CrtBundle<Keys, X> &c) {
		assert(m_digits.size() == c.m_digits.size());
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i)
			m_digits[i] += c.m_digits[i];
	}

	template<class Keys2, class X2>
	void operator+=(const CrtBundle<Keys2, X2> &c) {
		assert(digit_number() == c.digit_number());
#		pragma omp parallel for
		for (size_t i = 0; i < digit_number(); ++i) {
			assert(digit(i).simd_factor() == c.digit(i).simd_factor());
			digit(i) += c.digit(i);
		}
	}

	void operator+=(long c) {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i)
			m_digits[i] += c;
	}

//	void operator*=(const Crt<Keys, X> &c) {
//		assert(m_digits.size() == c.m_digits.size());
//		for (size_t i = 0; i < m_digits.size(); ++i)
//			m_digits[i] *= c.m_digits[i];
//	}

	template<class Keys2, class X2>
	void operator*=(const CrtBundle<Keys2, X2> &c) {
		assert(digit_number() == c.digit_number());
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			assert(digit(i).simd_factor() == c.digit(i).simd_factor());
			digit(i) *= c.digit(i);
		}
	}

	void operator*=(int c) {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			digit(i) *= c;
		}
	}

	void operator*=(cpp_int &c) {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			digit(i) *= c;
		}
	}

	void shiftRight(int c) {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			digit(i).shiftRight(c);
		}
	}

	void shiftLeft(int c) {
#		pragma omp parallel for
		for (size_t i = 0; i < m_digits.size(); ++i) {
			digit(i).shiftLeft(c);
		}
	}
private:
	std::vector<CrtDigit<Keys, X> > m_digits;
	const CrtContainer<Keys, X> *m_container;

	inline unsigned long mod(int a, int b) const {
		a = a % b;
		if (a < 0)
			a += b;

		return a;
	}
	inline cpp_int mod(cpp_int a, cpp_int b) const {
		a = a % b;
		if (a < 0)
			a += b;

		return a;
	}
};







template<class OutKeys, class OutX, class InKeys1, class InX1, class InKeys2, class InX2>
CrtBundle<OutKeys, OutX> &mul(CrtBundle<OutKeys, OutX> &out, const CrtBundle<InKeys1, InX1> &in1, const CrtBundle<InKeys2, InX2> &in2) {
	assert(in1.digit_number() == in2.digit_number());
	out.digit_number(in1.digit_number());
	out.setPubKey( pubKeyOfOperation(in1.pubKey(), in2.pubKey()) );
#	pragma omp parallel for
	for (size_t i = 0; i < in1.digit_number(); ++i) {
		out.digit(i) = in1.digit(i) * in2.digit(i);
	}
	return out;
}

template<class OutKeys, class OutX, class InKeys1, class InX1, class InKeys2, class InX2>
CrtBundle<OutKeys, OutX> &add_mul(CrtBundle<OutKeys, OutX> &out, const CrtBundle<InKeys1, InX1> &in1, const CrtBundle<InKeys2, InX2> &in2) {
	assert(in1.digit_number() == in2.digit_number());
	out.digit_number(in1.digit_number());
#	pragma omp parallel for
	for (size_t i = 0; i < in1.digit_number(); ++i) {
		out.digit(i) += in1.digit(i) * in2.digit(i);
	}
	return out;
}

template<class Keys, class Number>
inline CrtDigit<Keys, Number> inverse(const CrtDigit<Keys, Number> &a) {
	CrtDigit<Keys, Number> ret(a);

	// By Euler, X^{\Phi(F)} = 1 mod F
	// So the inverse of X is:   X^{\Phi(F)-1} mod F
	// Since we have F = \Prod p_i, where each p_i is prime, we get  X^{-1} = X^{\Prod (p_i - 1) - 1}  mod \prod p_i
	// When we consider the CRT digit p_i we copute  X^{\Prod (p_i - 1) - 1}  mod p_i 
	// but since X^p_i = X mod p_i    we can compute   X^{exp} mod p_i,  where exp=\prod(p_i-1)-1  mod  p_i-1
	//  with the one exception that if exp=0 we set exp:=p_i-1  to avoid having 0^0

	// unsigned long expMod = a.get_ring_size() - 1;
	// cpp_int exp = 1;
	// // compute phi(ringSize) mod expMod
	// // for (size_t i_digit = 0; i_digit < a.container()->key_number(); ++i_digit) {
	// 	// we want to take it to the power of Pow = \Phi(\prod ring_size) = \prod (ring_size - 1)
	// 	// In each digit with ring_size=m, where m is prime    x^{m-1}=1   so we take x^(Pow % (m-1))
	// 	for (size_t i_prod = 0; i_prod < a.container()->key_number(); ++i_prod) {
	// 		exp = (exp * (a.container()->key(i_prod)->get_ring_size() - 1)) % expMod;
	// 		cout << "after digit " << i_prod << " exp = " << exp << " phi=" << (a.container()->key(i_prod)->get_ring_size() - 1) << endl;
	// 	}
	// // }
	// exp = (exp - 1) % expMod;
	// if (exp == 0)
	// 	exp = expMod;
	// ret.val() = power(a.val(), (unsigned long)exp);
	ret.val() = power(a.val(), a.get_ring_size() - 2);
	return ret;
}


template<class Keys, class Number>
inline CrtBundle<Keys, Number> inverse(const CrtBundle<Keys, Number> &a) {
	CrtBundle<Keys, Number> ret(a);
#	pragma omp parallel for
	for (size_t i_digit = 0; i_digit < a.digit_number(); ++i_digit) {
		ret.digit(i_digit) = inverse(ret.digit(i_digit));
	}
	return ret;
}


#endif
