#ifndef ___HELIB_NUMBER___
#define ___HELIB_NUMBER___

#include <assert.h>
#include <helib/helib.h>
#include <liphe/helib2_keys.h>
#include <liphe/zp.h>

class Helib2Number {
private:
//	static Helib2Keys *_prev_keys;
//	static Helib2Keys *getPrevKeys() { return _prev_keys; }
//	static std::function<Helib2Keys *(void)> _getKeys;

	const Helib2Keys *_keys = NULL;
	helib::Ctxt *_val;

	int _mul_depth = 0;
	int _add_depth = 0;

	static long long power(long long base, long long exp) {
		long long ret = 1;
		for (int i = 0; i < exp; ++i)
			ret *= base;
		return ret;
	}

	Helib2Number mult_by_recursive_adding(const Helib2Number &x, long e) {
		if (e == 0)
			return Helib2Number(_keys, 0);

		while (e < 0)
			e += x.get_ring_size();

		if (e == 2) {
			Helib2Number y = x + x;
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

public:
	Helib2Number() : _keys(NULL), _val(NULL) { }
	Helib2Number(const Helib2Keys *k) : _keys(k), _val(NULL) { }
	Helib2Number(const Helib2Keys *k, long v) : _keys(k), _val(NULL), _mul_depth(0), _add_depth(0) { _val = new helib::Ctxt(k->publicKey()); from_int(v); }
	Helib2Number(const Helib2Number &n) : _keys(n._keys), _val(NULL), _mul_depth(n._mul_depth), _add_depth(n._add_depth) { _val = new helib::Ctxt(*n._val); }
	Helib2Number(Helib2Number &&n) : _keys(n._keys), _val(NULL), _mul_depth(n._mul_depth), _add_depth(n._add_depth) { _val = n._val; n._val = NULL; }
	Helib2Number(const helib::Ctxt &n, const Helib2Keys *k) : _keys(k), _val(NULL) { _val = new helib::Ctxt(n); }

	~Helib2Number() { null_val(); }

	void null_val() { if (_val != NULL) delete _val; _val = NULL; }

	void set_keys(const Helib2Keys *k) { _keys = k; }
	const Helib2Keys* keys() const { return _keys; }

	int in_range(int a) const { while (a < 0) a += _keys->p(); return a % _keys->p(); }
	long to_int() const { return _keys->decrypt(*_val); }
	std::vector<long int> to_vector() const { std::vector<long int> ret; _keys->decrypt(ret, *_val); return ret; }
	void from_int(long i) { _keys->encrypt(*_val, i); }
	void from_vector(const std::vector<long> &i) { _keys->encrypt(*_val, i); }
	void recrypt() { _keys->recrypt(*_val); }

	const helib::Ctxt &val() const { return *_val; }

	unsigned long simd_factor() const { return _keys->simd_factor(); }
	int get_ring_size() const { return power(_keys->p(), _keys->r()); }
	int p() const { return _keys->p(); }
	int r() const { return _keys->r(); }
	Helib2Number clone() { return Helib2Number(*this); }

	Helib2Number &operator=(const Helib2Number &b) {
		_keys = b._keys;
		null_val();
		_val = new helib::Ctxt(b.val());
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		return *this;
	}
	Helib2Number &operator=(Helib2Number &&b) {
		_keys = b._keys;
		null_val();
		_val = b._val;
		b._val = NULL;
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		return *this;
	}

	int add_depth() const { return _add_depth; }
	int mul_depth() const { return _mul_depth; }

	void add_depth(int d) { _add_depth = d; }
	void mul_depth(int d) { _mul_depth = d; }

//	void shift_right() { _val.divideByP(); }

	void negate() { _val->negate(); }



	Helib2Number operator!() const { Helib2Number zp(_keys,1); zp -= *this; return zp; }
	Helib2Number operator-() const { Helib2Number zp(*this); zp.negate(); return zp; }
	Helib2Number operator-(const Helib2Number &z) const { Helib2Number zp(*this); zp -= z; return zp; }
	Helib2Number operator+(const Helib2Number &z) const { Helib2Number zp(*this); zp += z; return zp; }
	Helib2Number operator*(const Helib2Number &z) const { Helib2Number zp(*this); zp *= z; return zp; }

	void operator-=(const Helib2Number &z) {
		assert(_keys == z._keys);
		*_val -= *z._val;
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator+=(const Helib2Number &z) {
		assert(_keys == z._keys);
		*_val += *z._val;
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator*=(const Helib2Number &z) {
		assert(_keys == z._keys);
		_val->multiplyBy(*z._val);
		_mul_depth = std::max(_mul_depth, z._mul_depth) + 1;
		_add_depth = std::max(_add_depth, z._add_depth);
	}


	Helib2Number operator-(long z) const { Helib2Number zp(*this); zp -= z; return zp; }
	Helib2Number operator+(long z) const { Helib2Number zp(*this); zp += z; return zp; }
	Helib2Number operator*(long z) const { Helib2Number zp(*this); zp *= z; return zp; }

	Helib2Number operator-(const std::vector<long> &z) const { Helib2Number zp(*this); zp -= z; return zp; }
	Helib2Number operator+(const std::vector<long> &z) const { Helib2Number zp(*this); zp += z; return zp; }
	Helib2Number operator*(const std::vector<long> &z) const { Helib2Number zp(*this); zp *= z; return zp; }

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

	void operator-=(const std::vector<long> &z) {
		helib::Ptxt<helib::BGV> zPtxt;
		_keys->encode(zPtxt, z);
		*_val -= zPtxt;
	}
	void operator+=(const std::vector<long> &z) {
		helib::Ptxt<helib::BGV> zPtxt;
		_keys->encode(zPtxt, z);
		*_val += zPtxt;
	}
	void operator*=(const std::vector<long> &z) {
		helib::Ptxt<helib::BGV> zPtxt;
		_keys->encode(zPtxt, z);
		*_val *= zPtxt;
	}

	Helib2Number operator-(const ZP &z) const { Helib2Number zp(*this); zp -= z; return zp; }
	Helib2Number operator+(const ZP &z) const { Helib2Number zp(*this); zp += z; return zp; }
	Helib2Number operator*(const ZP &z) const { Helib2Number zp(*this); zp *= z; return zp; }

	void operator-=(const ZP &z) { operator-=(z.to_vector()); }
	void operator+=(const ZP &z) { operator+=(z.to_vector()); }
	void operator*=(const ZP &z) { operator*=(z.to_vector()); }


//	template<class BITS>
//	BITS to_digits() const {
//		assert(p() == 2);
//		assert(r() > 1);
//
//		Helib2Number n = *this;
//		BITS ret;
//		ret.set_bit_length(r());
//
//		for (int i = 0; i < r(); ++i) {
//			std::vector<helib::Ctxt> bits;
//			extractDigits(bits, n._val, 0, false);
//
//			Helib2Number bit(bits[0]);
//			bit.mul_depth(mul_depth());
//			bit.add_depth(add_depth());
//			ret.set_bit(i, bit);
//
//			if (i != r() - 1) {
//				std::cout << "before dividing " << n.to_int() << std::endl;
//				n -= bit;
//				Helib2Number half_n = n;
//				half_n._val.divideByP();
//				n -= half_n;
//				std::cout << "after dividing " << n.to_int() << std::endl;
//			}
//		}
//
//		return ret;
//	}

	void self_rotate_left(int step) {
		_keys->rotate(*_val, -step);
	}

	void self_rotate_right(int step) {
		_keys->rotate(*_val, step);
	}

	Helib2Number rotate_left(int step) {
		Helib2Number ret(*this);
		_keys->rotate(*ret._val, -step);
		return ret;
	}

//	Helib2Number shift_left(int step) {
//		Helib2Number ret(*this);
//		_keys->shift(*ret._val, step);
//		return ret;
//	}

//	Helib2Number shift_right(int step) {
//		Helib2Number ret(*this);
//		_keys->shift(*ret._val, step);
//		return ret;
//	}

//	void reduceNoiseLevel() {
//		_val.modDownToLevel(_val.findBaseLevel());
//	}

	void save(std::ostream &out) const {
		_val->writeTo(out);
	}

	friend std::ostream &operator<<(std::ostream &out, const Helib2Number &z);
	friend std::istream &operator>>(std::istream &in, Helib2Number &z);
};

inline Helib2Number operator-(const ZP &z, const Helib2Number &x) { return (-x)+z; }
inline Helib2Number operator+(const ZP &z, const Helib2Number &x) { return x+z; }
inline Helib2Number operator*(const ZP &z, const Helib2Number &x) { return x*z; }


inline std::ostream &operator<<(std::ostream &out, const Helib2Number &z) {
	out << *z._val << " ";

	out
		<< z._mul_depth << " "
		<< z._add_depth << " ";

	return out;
}

inline std::istream &operator>>(std::istream &in, Helib2Number &z) {
	z.null_val();
	z._val = new helib::Ctxt(z._keys->publicKey());
	in >> *z._val;

	in
		>> z._mul_depth
		>> z._add_depth;

	return in;
}

#endif
