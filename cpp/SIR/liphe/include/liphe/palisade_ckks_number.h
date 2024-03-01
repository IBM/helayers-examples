#ifndef ___PALISADE_CKKS_NUMBER___
#define ___PALISADE_CKKS_NUMBER___

#include <assert.h>
#include <liphe/palisade_ckks_keys.h>
#include <liphe/zp.h>

//void breakDebugger() { }

class PalisadeCkksNumber {
private:
	static PalisadeCkksKeys *_prev_keys;
	static PalisadeCkksKeys *getPrevKeys() { return _prev_keys; }
	static std::function<PalisadeCkksKeys *(void)> _getKeys;

	PalisadeCkksKeys *_keys;
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> _val;

	int _mul_depth;
	int _add_depth;

//	void print(const char *s) { std::cerr << s << " HELIB " << (&_val) << std::endl; }
	void print(const char *s) {}
public:
	PalisadeCkksNumber() : _keys(_getKeys()), _mul_depth(0), _add_depth(0) { print("allocating"); }
	PalisadeCkksNumber(double v) : _keys(_getKeys()), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, std::vector<double>(simd_factor(), v)); print("allocating"); }
	PalisadeCkksNumber(const std::vector<double> &v) : _keys(_getKeys()), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, v); print("allocating"); }
	PalisadeCkksNumber(const PalisadeCkksNumber &n) : _keys(n._keys), _val(n._val), _mul_depth(n._mul_depth), _add_depth(n._add_depth) {print("allocating"); }
	PalisadeCkksNumber(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &n) : _keys(_getKeys()), _val(n), _mul_depth(0), _add_depth(0) {print("allocating"); }

	~PalisadeCkksNumber() {print("deleting"); }

	static void set_global_keys(PalisadeCkksKeys *k) { _prev_keys = k; }
	float to_float() const { return _keys->decrypt(_val); }
	double to_double() const { return _keys->decrypt(_val); }
	std::vector<double> to_vector() const { std::vector<double> ret; _keys->decrypt(ret, _val); return ret; }
	void from_int(double i) { _keys->encrypt(_val, i); }
	void from_float(float i) { _keys->encrypt(_val, i); }
	void from_double(double i) { _keys->encrypt(_val, i); }
	void from_vector(const std::vector<double> &i) { _keys->encrypt(_val, i); }

	static PalisadeCkksNumber static_from_int(int i) {
			PalisadeCkksNumber ret;
			_getKeys()->encrypt(ret._val, i);
			return ret;
		}

	static unsigned int simd_factor() { return _getKeys()->simd_factor(); }
	void assert_co_prime(int) const {}
//	PalisadeCkksNumber clone() { return PalisadeCkksNumber(*this); }

	PalisadeCkksNumber &operator=(const PalisadeCkksNumber &b) {
		_keys = b._keys;
		_val = b._val;
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		return *this;
	}

	const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &v() const { return _val; }

	int add_depth() const { return _add_depth; }
	int mul_depth() const { return _mul_depth; }

	void add_depth(int d) { _add_depth = d; }
	void mul_depth(int d) { _mul_depth = d; }

//	void shift_right() { _val.divideByP(); }

	void negate() { _val = - _val; }




//	PalisadeCkksNumber operator!() const { PalisadeCkksNumber zp(1); zp -= *this; return zp; }
	PalisadeCkksNumber operator-() const { PalisadeCkksNumber zp(*this); zp.negate(); return zp; }
	PalisadeCkksNumber operator-(const PalisadeCkksNumber &z) const { PalisadeCkksNumber zp(*this); zp -= z; return zp; }
	PalisadeCkksNumber operator+(const PalisadeCkksNumber &z) const { PalisadeCkksNumber zp(*this); zp += z; return zp; }
	PalisadeCkksNumber operator*(const PalisadeCkksNumber &z) const { PalisadeCkksNumber zp(*this); zp *= z; return zp; }

	void operator-=(const PalisadeCkksNumber &z) {
		assert(_keys == z._keys);

//		const PalisadeCkksNumber &copyz = z;
//		PalisadeCkksNumber copyz(z);
//		while (_mul_depth < copyz._mul_depth) {
//			breakDebugger();
//			*this *= 1.0;
//		}
//		while (_mul_depth > copyz._mul_depth) {
//			breakDebugger();
//			copyz *= 1.0;
//		}
//
//		assert(_mul_depth == copyz._mul_depth);
//		_val -= copyz._val;
//		_mul_depth = std::max(_mul_depth, copyz._mul_depth);
//		_add_depth = std::max(_add_depth, copyz._add_depth) + 1;
		_val -= z._val;
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator+=(const PalisadeCkksNumber &z) {
		assert(_keys == z._keys);

//		const PalisadeCkksNumber &copyz = z;
//		PalisadeCkksNumber copyz(z);
//		while (_mul_depth < copyz._mul_depth) {
//			breakDebugger();
//			*this *= 1.0;
//		}
//		while (_mul_depth > copyz._mul_depth) {
//			breakDebugger();
//			copyz *= 1.0;
//		}
//
//		assert(_mul_depth == copyz._mul_depth);
//		_val += copyz._val;
//		_mul_depth = std::max(_mul_depth, copyz._mul_depth);
//		_add_depth = std::max(_add_depth, copyz._add_depth) + 1;
		_val += z._val;
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator*=(const PalisadeCkksNumber &z) {
		assert(_keys == z._keys);

//		PalisadeCkksNumber copyz(z);
//		while (_mul_depth < copyz._mul_depth) {
//			breakDebugger();
//			*this *= 1.0;
//		}
//		while (_mul_depth > copyz._mul_depth) {
//			breakDebugger();
//			copyz *= 1.0;
//		}
//
//		assert(_mul_depth == copyz._mul_depth);
//		_val *= copyz._val;
//		_val = _keys->context()->Rescale(_val);
//		_mul_depth = std::max(_mul_depth, copyz._mul_depth) + 1;
//		_add_depth = std::max(_add_depth, copyz._add_depth);

		_val *= z._val;
		_val = _keys->context()->Rescale(_val);
		_mul_depth = std::max(_mul_depth, z._mul_depth) + 1;
		_add_depth = std::max(_add_depth, z._add_depth);
	}


	PalisadeCkksNumber operator-(double z) const { PalisadeCkksNumber zp(*this); zp -= z; return zp; }
	PalisadeCkksNumber operator+(double z) const { PalisadeCkksNumber zp(*this); zp += z; return zp; }
	PalisadeCkksNumber operator*(double z) const { PalisadeCkksNumber zp(*this); zp *= z; return zp; }

	PalisadeCkksNumber operator-(const std::vector<double> &z) const { PalisadeCkksNumber zp(*this); zp -= z; return zp; }
	PalisadeCkksNumber operator+(const std::vector<double> &z) const { PalisadeCkksNumber zp(*this); zp += z; return zp; }
	PalisadeCkksNumber operator*(const std::vector<double> &z) const { PalisadeCkksNumber zp(*this); zp *= z; return zp; }

	void operator-=(double _z) {
		std::vector<double> z((double)_keys->simd_factor(), _z);
		operator-=(z);
	}
	void operator+=(double _z) {
		std::vector<double> z((double)_keys->simd_factor(), _z);
		operator+=(z);
	}
	void operator*=(double _z) {
		std::vector<double> z((double)_keys->simd_factor(), _z);
		operator*=(z);
	}

	void operator-=(const std::vector<double> &_z) {
		lbcrypto::Plaintext z = _keys->encode(_z);
		_val = _keys->context()->EvalAdd(_val, z);
	}
	void operator+=(const std::vector<double> &_z) {
		lbcrypto::Plaintext z = _keys->encode(_z);
		_val = _keys->context()->EvalSub(_val, z);
	}
	void operator*=(const std::vector<double> &_z) {
		lbcrypto::Plaintext z = _keys->encode(_z);
		_val = _keys->context()->EvalMult(_val, z);
		++_mul_depth;
	}

	PalisadeCkksNumber operator-(const ZP &z) const { PalisadeCkksNumber zp(*this); zp -= z; return zp; }
	PalisadeCkksNumber operator+(const ZP &z) const { PalisadeCkksNumber zp(*this); zp += z; return zp; }
	PalisadeCkksNumber operator*(const ZP &z) const { PalisadeCkksNumber zp(*this); zp *= z; return zp; }

	void operator-=(const ZP &z) { operator-=(z.to_vector()); }
	void operator+=(const ZP &z) { operator+=(z.to_vector()); }
	void operator*=(const ZP &z) { operator*=(z.to_vector()); }


//	template<class BITS>
//	BITS to_digits() const {
//		assert(p() == 2);
//		assert(r() > 1);
//
//		vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly> > bits;
//		extractDigits(bits, _val, 0, false);
//
//		BITS ret;
//		ret.set_bit_length(bits.size());
//		for (int i = 0; i < bits.size(); ++i) {
//			PalisadeCkksNumber bit(bits[i]);
//			bit.mul_depth(mul_depth());
//			bit.add_depth(add_depth());
//			ret.set_bit(i, bit);
//		}
//		return ret;
//	}

//	template<class BITS>
//	BITS to_digits() const {
//		assert(p() == 2);
//		assert(r() > 1);
//
//		PalisadeCkksNumber n = *this;
//		BITS ret;
//		ret.set_bit_length(r());
//
//		for (int i = 0; i < r(); ++i) {
//			std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly> > bits;
//			extractDigits(bits, n._val, 0, false);
//
//			PalisadeCkksNumber bit(bits[0]);
//			bit.mul_depth(mul_depth());
//			bit.add_depth(add_depth());
//			ret.set_bit(i, bit);
//
//			if (i != r() - 1) {
//				std::cout << "before dividing " << n.to_int() << std::endl;
//				n -= bit;
//				PalisadeCkksNumber half_n = n;
//				half_n._val.divideByP();
//				n -= half_n;
//				std::cout << "after dividing " << n.to_int() << std::endl;
//			}
//		}
//
//		return ret;
//	}

	PalisadeCkksNumber rotate_left(int step) {
		PalisadeCkksNumber ret(*this);
		ret._val = _keys->context()->EvalAtIndex(_val, step);
		return ret;
	}

	PalisadeCkksNumber shift_left(int step) {
		PalisadeCkksNumber ret(*this);
		ret._val = _keys->context()->EvalAtIndex(_val, -step);
		return ret;
	}

//	PalisadeCkksNumber shift_right(int step) {
//		PalisadeCkksNumber ret(*this);
//		_keys->shift(ret._val, -step);
//		return ret;
//	}

//	void reduceNoiseLevel() {
//		_val.modDownToLevel(_val.findBaseLevel());
//	}

	friend std::ostream &operator<<(std::ostream &out, const PalisadeCkksNumber &z);
	friend std::istream &operator>>(std::istream &in, PalisadeCkksNumber &z);
};

inline PalisadeCkksNumber operator-(const ZP &z, const PalisadeCkksNumber &x) { return (-x)+z; }
inline PalisadeCkksNumber operator+(const ZP &z, const PalisadeCkksNumber &x) { return x+z; }
inline PalisadeCkksNumber operator*(const ZP &z, const PalisadeCkksNumber &x) { return x*z; }


//inline std::ostream &operator<<(std::ostream &out, const PalisadeCkksNumber &z) {
//	out << z._val << " ";
//
//	out
//		<< z._mul_depth << " "
//		<< z._add_depth << " ";
//
//	return out;
//}

//inline std::istream &operator>>(std::istream &in, PalisadeCkksNumber &z) {
//	in >> z._val;
//
//	in
//		>> z._mul_depth
//		>> z._add_depth;
//
//	return in;
//}

#endif
