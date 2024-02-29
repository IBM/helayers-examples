#ifndef ___SEAL_BFV_NUMBER___
#define ___SEAL_BFV_NUMBER___

#include <assert.h>
#include <seal/seal.h>
#include <liphe/seal_bfv_keys.h>
#include <liphe/zp.h>

class SealBfvNumber {
private:
//	static SealBfvKeys *_prev_keys;
//	static SealBfvKeys *getPrevKeys() { return _prev_keys; }
//	static std::function<SealBfvKeys *(void)> _getKeys;

	const SealBfvKeys *_keys = NULL;
	seal::Ciphertext *_val;

	int _mul_depth = 0;
	int _add_depth = 0;

	static long long power(long long base, long long exp) {
		long long ret = 1;
		for (int i = 0; i < exp; ++i)
			ret *= base;
		return ret;
	}

	SealBfvNumber mult_by_recursive_adding(const SealBfvNumber &x, long e) {
		if (e == 0)
			return SealBfvNumber(_keys, 0);

		while (e < 0)
			e += x.get_ring_size();

		if (e == 2) {
			SealBfvNumber y = x + x;
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
	SealBfvNumber() : _keys(NULL), _val(NULL) { }
	SealBfvNumber(const SealBfvKeys *k) : _keys(k), _val(NULL) { }
	SealBfvNumber(const SealBfvKeys *k, long v) : _keys(k), _val(NULL), _mul_depth(0), _add_depth(0) { from_int(v); }
	SealBfvNumber(const SealBfvNumber &n) : _keys(n._keys), _val(NULL), _mul_depth(n._mul_depth), _add_depth(n._add_depth) { _val = new seal::Ciphertext(*n._val); }
	SealBfvNumber(SealBfvNumber &&n) : _keys(n._keys), _val(NULL), _mul_depth(n._mul_depth), _add_depth(n._add_depth) { _val = n._val; n._val = NULL; }
	SealBfvNumber(const seal::Ciphertext &n, const SealBfvKeys *k) : _keys(k), _val(NULL) { _val = new seal::Ciphertext(); *_val = n; }

	~SealBfvNumber() { null_val(); }

	void null_val() { if (_val != NULL) delete _val; _val = NULL; }

	void set_keys(const SealBfvKeys *k) { _keys = k; }
	const SealBfvKeys* keys() const { return _keys; }

	int in_range(int a) const { while (a < 0) a += _keys->p(); return a % _keys->p(); }
	long to_int() const { return _keys->decrypt(*_val); }
	std::vector<long int> to_vector() const { std::vector<long int> ret; _keys->decrypt(ret, *_val); return ret; }
	void from_int(long i) { _keys->encrypt(*_val, i); }
	void from_vector(const std::vector<long> &i) { _keys->encrypt(*_val, i); }

	void recrypt() { throw std::runtime_error("recrypt is not implemented for BFV in SEAL"); }

	const seal::Ciphertext &val() const { return *_val; }

	unsigned long simd_factor() const { return _keys->simd_factor(); }
	int get_ring_size() const { return _keys->p(); }
	int p() const { return _keys->p(); }
	SealBfvNumber clone() { return SealBfvNumber(*this); }

	SealBfvNumber &operator=(const SealBfvNumber &b) {
		_keys = b._keys;
		null_val();
		_val = new seal::Ciphertext(b.val());
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		return *this;
	}
	SealBfvNumber &operator=(SealBfvNumber &&b) {
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

	void negate() { _keys->eval().negate_inplace(*_val); }



	//SealBfvNumber operator!() const { SealBfvNumber zp(_keys,1); zp -= *this; return zp; }
	SealBfvNumber operator-() const { SealBfvNumber zp(*this); zp.negate(); return zp; }
	SealBfvNumber operator-(const SealBfvNumber &z) const { SealBfvNumber zp(*this); zp -= z; return zp; }
	SealBfvNumber operator+(const SealBfvNumber &z) const { SealBfvNumber zp(*this); zp += z; return zp; }
	SealBfvNumber operator*(const SealBfvNumber &z) const { SealBfvNumber zp(*this); zp *= z; return zp; }

	void operator-=(const SealBfvNumber &z) {
		assert(_keys == z._keys);
		_keys->eval().sub_inplace(*_val, *z._val);
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator+=(const SealBfvNumber &z) {
		assert(_keys == z._keys);
		_keys->eval().add_inplace(*_val, *z._val);
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator*=(const SealBfvNumber &z) {
		assert(_keys == z._keys);
		_keys->eval().multiply_inplace(*_val, *z._val);
		_keys->relinearize_inplace(*_val);
		_mul_depth = std::max(_mul_depth, z._mul_depth) + 1;
		_add_depth = std::max(_add_depth, z._add_depth);
	}


	SealBfvNumber operator-(long z) const { SealBfvNumber zp(*this); zp -= z; return zp; }
	SealBfvNumber operator+(long z) const { SealBfvNumber zp(*this); zp += z; return zp; }
	SealBfvNumber operator*(long z) const { SealBfvNumber zp(*this); zp *= z; return zp; }

	SealBfvNumber operator-(const std::vector<long> &z) const { SealBfvNumber zp(*this); zp -= z; return zp; }
	SealBfvNumber operator+(const std::vector<long> &z) const { SealBfvNumber zp(*this); zp += z; return zp; }
	SealBfvNumber operator*(const std::vector<long> &z) const { SealBfvNumber zp(*this); zp *= z; return zp; }

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
		seal::Plaintext zPtxt;
		_keys->encode(zPtxt, z);
		*this -= zPtxt;
	}
	void operator+=(const std::vector<long> &z) {
		seal::Plaintext zPtxt;
		_keys->encode(zPtxt, z);
		*this += zPtxt;
	}
	void operator*=(const std::vector<long> &z) {
		seal::Plaintext zPtxt;
		_keys->encode(zPtxt, z);
		*this *= zPtxt;
	}

	SealBfvNumber operator-(const seal::Plaintext &z) const { SealBfvNumber zp(*this); zp -= z; return zp; }
	SealBfvNumber operator+(const seal::Plaintext &z) const { SealBfvNumber zp(*this); zp += z; return zp; }
	SealBfvNumber operator*(const seal::Plaintext &z) const { SealBfvNumber zp(*this); zp *= z; return zp; }

	void operator-=(const seal::Plaintext &z) {
		_keys->eval().sub_plain_inplace(*_val, z);
	}
	void operator+=(const seal::Plaintext &z) {
		_keys->eval().add_plain_inplace(*_val, z);
	}
	void operator*=(const seal::Plaintext &z) {
		_keys->eval().multiply_plain_inplace(*_val, z);
		_keys->relinearize_inplace(*_val);
	}

	SealBfvNumber operator-(const ZP &z) const { SealBfvNumber zp(*this); zp -= z; return zp; }
	SealBfvNumber operator+(const ZP &z) const { SealBfvNumber zp(*this); zp += z; return zp; }
	SealBfvNumber operator*(const ZP &z) const { SealBfvNumber zp(*this); zp *= z; return zp; }

	void operator-=(const ZP &z) { operator-=(z.to_vector()); }
	void operator+=(const ZP &z) { operator+=(z.to_vector()); }
	void operator*=(const ZP &z) { operator*=(z.to_vector()); }


//	template<class BITS>
//	BITS to_digits() const {
//		assert(p() == 2);
//		assert(r() > 1);
//
//		SealBfvNumber n = *this;
//		BITS ret;
//		ret.set_bit_length(r());
//
//		for (int i = 0; i < r(); ++i) {
//			std::vector<helib::Ctxt> bits;
//			extractDigits(bits, n._val, 0, false);
//
//			SealBfvNumber bit(bits[0]);
//			bit.mul_depth(mul_depth());
//			bit.add_depth(add_depth());
//			ret.set_bit(i, bit);
//
//			if (i != r() - 1) {
//				std::cout << "before dividing " << n.to_int() << std::endl;
//				n -= bit;
//				SealBfvNumber half_n = n;
//				half_n._val.divideByP();
//				n -= half_n;
//				std::cout << "after dividing " << n.to_int() << std::endl;
//			}
//		}
//
//		return ret;
//	}

	void self_rotate_left(int step) {
		_keys->rotate(*_val, step);
	}

	void self_rotate_right(int step) {
		_keys->rotate(*_val, -step);
	}

	SealBfvNumber rotate_left(int step) {
		SealBfvNumber ret(*this);
		_keys->rotate(*ret._val, step);
		return ret;
	}

//	SealBfvNumber shift_left(int step) {
//		SealBfvNumber ret(*this);
//		_keys->shift(*ret._val, step);
//		return ret;
//	}

	// SealBfvNumber shift_right(int step) {
	// 	SealBfvNumber ret(*this);
	// 	_keys->shift(*ret._val, step);
	// 	return ret;
	// }

//	void reduceNoiseLevel() {
//		_val.modDownToLevel(_val.findBaseLevel());
//	}

	void save(std::ostream &out) const {
		_val->save(out, seal::compr_mode_type::none);
	}

	friend std::ostream &operator<<(std::ostream &out, const SealBfvNumber &z);
	friend std::istream &operator>>(std::istream &in, SealBfvNumber &z);
};

inline SealBfvNumber operator-(const ZP &z, const SealBfvNumber &x) { return (-x)+z; }
inline SealBfvNumber operator+(const ZP &z, const SealBfvNumber &x) { return x+z; }
inline SealBfvNumber operator*(const ZP &z, const SealBfvNumber &x) { return x*z; }


//inline std::ostream &operator<<(std::ostream &out, const SealBfvNumber &z) {
//	out << *z._val << " ";
//
//	out
//		<< z._mul_depth << " "
//		<< z._add_depth << " ";
//
//	return out;
//}

//inline std::istream &operator>>(std::istream &in, SealBfvNumber &z) {
//	z.null_val();
//	z._val = std::make_shared<seal::Ciphertext>(new seal::Ciphertext());
//	in >> *z._val;
//
//	in
//		>> z._mul_depth
//		>> z._add_depth;
//
//	return in;
//}

#endif
