#ifndef ___HELIB_NUMBER___
#define ___HELIB_NUMBER___

#include <assert.h>
#include <liphe/helib_keys.h>
#include <liphe/zp.h>

class HelibNumber {
private:
	static HelibKeys *_prev_keys;
	static HelibKeys *getPrevKeys() { return _prev_keys; }
	static std::function<HelibKeys *(void)> _getKeys;

	HelibKeys *_keys;
	Ctxt _val;

	int _mul_depth;
	int _add_depth;

	static long long power(long long base, long long exp) {
		long long ret = 1;
		for (int i = 0; i < exp; ++i)
			ret *= base;
		return ret;
	}

	HelibNumber mult_by_recursive_adding(const HelibNumber &x, long e) {
		if (e == 0)
			return HelibNumber(0);

		while (e < 0)
			e += x.get_ring_size();

		if (e == 2) {
			HelibNumber y = x + x;
			return y;
		}
		if (e == 1)
			return x;

		if (e & 1) {
			return x + mult_by_recursive_adding(x, e-1);
		} else {
			return mult_by_recursive_adding(x + x, e/2);
		}
	}

//	void print(const char *s) { std::cerr << s << " HELIB " << (&_val) << std::endl; }
	void print(const char *s) {}
public:
	HelibNumber() : _keys(_getKeys()), _val(_getKeys()->publicKey()), _mul_depth(0), _add_depth(0) { print("allocating"); }
	HelibNumber(long long v) : _keys(_getKeys()), _val(_keys->publicKey()), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, std::vector<long>(simd_factor(), v)); print("allocating"); }
	HelibNumber(const std::vector<long> &v) : _keys(_getKeys()), _val(_keys->publicKey()), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, v); print("allocating"); }
	HelibNumber(const HelibNumber &n) : _keys(n._keys), _val(n._val), _mul_depth(n._mul_depth), _add_depth(n._add_depth) {print("allocating"); }
	HelibNumber(const Ctxt &n) : _keys(_getKeys()), _val(n), _mul_depth(0), _add_depth(0) {print("allocating"); }

	~HelibNumber() {print("deleting"); }

	static int global_p() { return _getKeys()->p(); }
	static void set_global_keys(HelibKeys *k) { _prev_keys = k; }
	int in_range(int a) const { while (a < 0) a += _keys->p(); return a % _keys->p(); }
	static int static_in_range(int a) { while (a < 0) a += _getKeys()->p(); return a % _getKeys()->p(); }
	long to_int() const { return _keys->decrypt(_val); }
	std::vector<long int> to_vector() const { std::vector<long int> ret; _keys->decrypt(ret, _val); return ret; }
	void from_int(long i) { _keys->encrypt(_val, i); }
	void from_vector(const std::vector<long> &i) { _keys->encrypt(_val, i); }

	static HelibNumber static_from_int(int i) {
			HelibNumber ret;
			_getKeys()->encrypt(ret._val, i);
			return ret;
		}

	static unsigned int simd_factor() { return _getKeys()->simd_factor(); }
	int get_ring_size() const { return power(_keys->p(), _keys->r()); }
	static int get_global_ring_size() { return power(_getKeys()->p(), _getKeys()->r()); }
	int p() const { return _keys->p(); }
	int r() const { return _keys->r(); }
	void assert_co_prime(int) const {}
//	HelibNumber clone() { return HelibNumber(*this); }

	HelibNumber &operator=(const HelibNumber &b) {
		_keys = b._keys;
		_val = b._val;
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		return *this;
	}

	const Ctxt &v() const { return _val; }

	int add_depth() const { return _add_depth; }
	int mul_depth() const { return _mul_depth; }

	void add_depth(int d) { _add_depth = d; }
	void mul_depth(int d) { _mul_depth = d; }

	void shift_right() { _val.divideByP(); }

	void negate() { _val.negate(); }




	HelibNumber operator!() const { HelibNumber zp(1); zp -= *this; return zp; }
	HelibNumber operator-() const { HelibNumber zp(*this); zp.negate(); return zp; }
	HelibNumber operator-(const HelibNumber &z) const { HelibNumber zp(*this); zp -= z; return zp; }
	HelibNumber operator+(const HelibNumber &z) const { HelibNumber zp(*this); zp += z; return zp; }
	HelibNumber operator*(const HelibNumber &z) const { HelibNumber zp(*this); zp *= z; return zp; }

	void operator-=(const HelibNumber &z) {
		assert(_keys == z._keys);
		_val -= z._val;
		_mul_depth = max(_mul_depth, z._mul_depth);
		_add_depth = max(_add_depth, z._add_depth) + 1;
	}

	void operator+=(const HelibNumber &z) {
		assert(_keys == z._keys);
		_val += z._val;
		_mul_depth = max(_mul_depth, z._mul_depth);
		_add_depth = max(_add_depth, z._add_depth) + 1;
	}

	void operator*=(const HelibNumber &z) {
		assert(_keys == z._keys);
		_val.multiplyBy(z._val);
		_mul_depth = max(_mul_depth, z._mul_depth) + 1;
		_add_depth = max(_add_depth, z._add_depth);
	}


	HelibNumber operator-(long z) const { HelibNumber zp(*this); zp -= z; return zp; }
	HelibNumber operator+(long z) const { HelibNumber zp(*this); zp += z; return zp; }
	HelibNumber operator*(long z) const { HelibNumber zp(*this); zp *= z; return zp; }

	HelibNumber operator-(const std::vector<long> &z) const { HelibNumber zp(*this); zp -= z; return zp; }
	HelibNumber operator+(const std::vector<long> &z) const { HelibNumber zp(*this); zp += z; return zp; }
	HelibNumber operator*(const std::vector<long> &z) const { HelibNumber zp(*this); zp *= z; return zp; }

	void operator-=(long _z) {
		std::vector<long> z((unsigned long)_keys->simd_factor(), _z);
		operator-=(z);
	}
	void operator+=(long _z) {
		std::vector<long> z((unsigned long)_keys->simd_factor(), _z);
		operator+=(z);
	}
	void operator*=(long _z) {
		std::vector<long> z((unsigned long)_keys->simd_factor(), _z);
		operator*=(z);
	}

	void operator-=(const std::vector<long> &_z) {
		std::vector<long> z(std::max(_z.size(), (unsigned long)_keys->simd_factor()));
		for (unsigned int i = 0; i < _z.size(); ++i)
			z[i] = -_z[i];
		for (unsigned int i = _z.size(); i < z.size(); ++i)
			z[i] = 0;
		ZZX z_poly;
		_keys->encode(z_poly, z);
		_val.addConstant(z_poly);
	}
	void operator+=(const std::vector<long> &_z) {
		std::vector<long> z(std::max(_z.size(), (unsigned long)_keys->simd_factor()));
		for (unsigned int i = 0; i < _z.size(); ++i)
			z[i] = _z[i];
		for (unsigned int i = _z.size(); i < z.size(); ++i)
			z[i] = 0;

		ZZX z_poly;
		_keys->encode(z_poly, z);
		_val.addConstant(z_poly);
	}
	void operator*=(const std::vector<long> &_z) {
		std::vector<long> z(std::max(_z.size(), (unsigned long)_keys->simd_factor()));
		for (unsigned int i = 0; i < _z.size(); ++i)
			z[i] = _z[i];
		for (unsigned int i = _z.size(); i < z.size(); ++i)
			z[i] = 0;

		ZZX z_poly;
		_keys->encode(z_poly, z);
		_val.multByConstant(z_poly);
	}

	HelibNumber operator-(const ZP &z) const { HelibNumber zp(*this); zp -= z; return zp; }
	HelibNumber operator+(const ZP &z) const { HelibNumber zp(*this); zp += z; return zp; }
	HelibNumber operator*(const ZP &z) const { HelibNumber zp(*this); zp *= z; return zp; }

	void operator-=(const ZP &z) { operator-=(z.to_vector()); }
	void operator+=(const ZP &z) { operator+=(z.to_vector()); }
	void operator*=(const ZP &z) { operator*=(z.to_vector()); }


//	template<class BITS>
//	BITS to_digits() const {
//		assert(p() == 2);
//		assert(r() > 1);
//
//		vector<Ctxt> bits;
//		extractDigits(bits, _val, 0, false);
//
//		BITS ret;
//		ret.set_bit_length(bits.size());
//		for (int i = 0; i < bits.size(); ++i) {
//			HelibNumber bit(bits[i]);
//			bit.mul_depth(mul_depth());
//			bit.add_depth(add_depth());
//			ret.set_bit(i, bit);
//		}
//		return ret;
//	}

	template<class BITS>
	BITS to_digits() const {
		assert(p() == 2);
		assert(r() > 1);

		HelibNumber n = *this;
		BITS ret;
		ret.set_bit_length(r());

		for (int i = 0; i < r(); ++i) {
			std::vector<Ctxt> bits;
			extractDigits(bits, n._val, 0, false);

			HelibNumber bit(bits[0]);
			bit.mul_depth(mul_depth());
			bit.add_depth(add_depth());
			ret.set_bit(i, bit);

			if (i != r() - 1) {
				std::cout << "before dividing " << n.to_int() << std::endl;
				n -= bit;
				HelibNumber half_n = n;
				half_n._val.divideByP();
				n -= half_n;
				std::cout << "after dividing " << n.to_int() << std::endl;
			}
		}

		return ret;
	}

	HelibNumber rotate_left(int step) {
		HelibNumber ret(*this);
		_keys->rotate(ret._val, step);
		return ret;
	}

//	HelibNumber shift_left(int step) {
//		HelibNumber ret(*this);
//		_keys->shift(ret._val, -step);
//		return ret;
//	}
//
//	HelibNumber shift_right(int step) {
//		HelibNumber ret(*this);
//		_keys->shift(ret._val, step);
//		return ret;
//	}

	void reduceNoiseLevel() {
		_val.modDownToLevel(_val.findBaseLevel());
	}

	friend std::ostream &operator<<(std::ostream &out, const HelibNumber &z);
	friend std::istream &operator>>(std::istream &in, HelibNumber &z);
};

inline HelibNumber operator-(const ZP &z, const HelibNumber &x) { return (-x)+z; }
inline HelibNumber operator+(const ZP &z, const HelibNumber &x) { return x+z; }
inline HelibNumber operator*(const ZP &z, const HelibNumber &x) { return x*z; }


inline std::ostream &operator<<(std::ostream &out, const HelibNumber &z) {
	out << z._val << " ";

	out
		<< z._mul_depth << " "
		<< z._add_depth << " ";

	return out;
}

inline std::istream &operator>>(std::istream &in, HelibNumber &z) {
	in >> z._val;

	in
		>> z._mul_depth
		>> z._add_depth;

	return in;
}

#endif
