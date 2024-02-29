#ifndef ___UNSIGNED_WORD_PLAINTEXT___
#define ___UNSIGNED_WORD_PLAINTEXT___

// Author: Hayim Shaul 2020
//
// Emulates a number held by indefinite number of bits.
// There are interfaces for its bits similarly to UnsignedWordHelib2Binary
//
// Note: UnsignedWordPlaintext can be only be positive, negative numbers can be implemented with 2-complement but require
// a fixed number of bit representation, or some other TIHKUMATION like keeping the sign in a separate bit
// and for every + operation compute both +/- and choose by multiplying with sign bit
//

#include <boost/multiprecision/cpp_int.hpp>

// This is a container for the keys.
// It includes the public key, possibly the private key, but also the (maximal) number of bits in the word
class UnsignedWordPlaintextKeys {
private:
	const ZPKeys *_priv_key;
	const ZPKeys *_pub_key;
	size_t _bits;
public:
	UnsignedWordPlaintextKeys(const ZPKeys *privKey, const ZPKeys *pubKey, size_t bits) : _priv_key(privKey), _pub_key(pubKey), _bits(bits) {}
	size_t simd_factor() const { return _pub_key->simd_factor(); }
	size_t bits() const { return _bits; }
};

class UnsignedWordPlaintext {
private:
	std::vector<boost::multiprecision::cpp_int> _val;
	const UnsignedWordPlaintextKeys *_keys;

//	bool isZeros(const std::vector<long int> &n) {
//		for (auto v: n)
//			if (v != 0)
//				return false;
//		return true;
//	}

	size_t bits() const { return _keys->bits(); }

	boost::multiprecision::cpp_int mod() const { return boost::multiprecision::cpp_int(1) << bits(); }
	boost::multiprecision::cpp_int sign_bit() const { return boost::multiprecision::cpp_int(1) << (bits() - 1); }

	void init(const std::vector<long int> &n) {
		assert(n.size() <= simd_factor());

		_val.resize(simd_factor());

		for (size_t i = 0; i < n.size(); ++i)
			_val[i] = n[i] % mod();
		for (size_t i = n.size(); i < _val.size(); ++i)
			_val[i] = 0;
	}

	void init(const std::vector<boost::multiprecision::cpp_int> &n) {
		assert(n.size() <= simd_factor());

		_val.resize(simd_factor());

		for (size_t i = 0; i < n.size(); ++i)
			_val[i] = n[i] % mod();
		for (size_t i = n.size(); i < _val.size(); ++i)
			_val[i] = 0;
	}

public:
	UnsignedWordPlaintext(const UnsignedWordPlaintextKeys *keys = NULL) : _keys(keys) { init( std::vector<long int>(1, 0)); }
	UnsignedWordPlaintext(long int n, const UnsignedWordPlaintextKeys *keys) : _keys(keys) { init( std::vector<long int>(1, n)); }
	UnsignedWordPlaintext(const std::vector<long int> &n, const UnsignedWordPlaintextKeys *keys) : _keys(keys) { init(n); }
	UnsignedWordPlaintext(const std::vector<boost::multiprecision::cpp_int> &n, const UnsignedWordPlaintextKeys *keys) : _keys(keys) { init(n); }
	UnsignedWordPlaintext(const UnsignedWordPlaintext &u) : _val(u._val), _keys(u._keys) {}

	~UnsignedWordPlaintext() {}

	void keys(const UnsignedWordPlaintextKeys *k) { _keys = k; init(_val); }
	const UnsignedWordPlaintextKeys *keys() const { return _keys; }

	size_t simd_factor() const { return _keys->simd_factor(); }

	void from_int(long n, const UnsignedWordPlaintextKeys *keys) { _keys = keys; init(std::vector<long>(1, n)); }
	void from_int(const std::vector<long int> &v, const UnsignedWordPlaintextKeys *keys) { _keys = keys; init(v); }
	void from_vector(const std::vector<long int> &v, const UnsignedWordPlaintextKeys *keys) { _keys = keys; init(v); }
	void from_vector(const std::vector<boost::multiprecision::cpp_int> &v, const UnsignedWordPlaintextKeys *keys) { _keys = keys; init(v); }

	static UnsignedWordPlaintext static_from_int(int c, UnsignedWordPlaintextKeys *keys) {
		UnsignedWordPlaintext ret(c, keys);
		return ret;
	}

	static UnsignedWordPlaintext static_from_vector(const std::vector<long int> &c, UnsignedWordPlaintextKeys *keys) {
		UnsignedWordPlaintext ret(c, keys);
		return ret;
	}

	void add(const UnsignedWordPlaintext &w) {
		assert(keys() == w.keys());

		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] += w._val[i];
			_val[i] %= mod();
		}
	}

	void sub(const UnsignedWordPlaintext &w) {
		assert(keys() == w.keys());

		for (size_t i = 0; i < _val.size(); ++i) {
			boost::multiprecision::cpp_int negW(w._val[i]);
			if (negW > sign_bit())
				negW = mod() - negW;
			_val[i] += negW;
			_val[i] %= mod();
		}
	}

	void mult(const UnsignedWordPlaintext &w) {
		assert(keys() == w.keys());

		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] *= w._val[i];
			_val[i] %= mod();
		}
	}

	void add(const boost::multiprecision::cpp_int &w) {
		assert(w < mod());
		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] += w;
			_val[i] %= mod();
		}
	}

	void sub(const boost::multiprecision::cpp_int &w) {
		assert(w < mod());
		for (size_t i = 0; i < _val.size(); ++i) {
			boost::multiprecision::cpp_int negW(w);
			if (negW > sign_bit())
				negW = mod() - negW;
			_val[i] += negW;
			_val[i] %= mod();
		}
	}

	void mult(const boost::multiprecision::cpp_int &w) {
		assert(w < mod());
		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] *= w;
			_val[i] %= mod();
		}
	}


	void shiftBitsLeft(int n) {
		assert(n > 0);

		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] <<= n;
			_val[i] %= mod();
		}
	}

	void shiftBitsRight(int n) {
		assert(n > 0);

		for (size_t i = 0; i < _val.size(); ++i) {
			_val[i] >>= n;
		}
	}

	void rotateSimd(int n) {
		std::vector<boost::multiprecision::cpp_int> copy(_val);

		for (size_t i = 0; i < _val.size(); ++i)
			_val[i] = copy[(i + n) % _val.size()];
	}

	void shiftSimd(int n) {
		std::vector<boost::multiprecision::cpp_int> copy(_val);

		for (size_t i = 0; i < _val.size(); ++i)
			if ((i + n >= 0) && (i + n < _val.size()))
				_val[i] = copy[i + n];
			else
				_val[i] = 0;
	}

	void operator+=(const UnsignedWordPlaintext &w) { add(w); }
	void operator-=(const UnsignedWordPlaintext &w) { sub(w); }
	void operator*=(const UnsignedWordPlaintext &w) { mult(w); }

	void operator<<=(int i) { shiftBitsLeft(i); }
	void operator>>=(int i) { shiftBitsRight(i); }

	std::vector<long> operator[](size_t bit) const {
		std::vector<long> ret(simd_factor());
		for (size_t i = 0; i < simd_factor(); ++i)
			ret[i] = ((_val[i] >> bit) % 2).convert_to<int>();
		return ret;
	}

	//neg should expand the bit representation to MAX_BIT_NUM and use 2-complement
	//void neg();

	UnsignedWordPlaintext operator+(const UnsignedWordPlaintext &b) const { UnsignedWordPlaintext c(*this); c+=b; return c; }
	UnsignedWordPlaintext operator-(const UnsignedWordPlaintext &b) const { UnsignedWordPlaintext c(*this); c-=b; return c; }
	UnsignedWordPlaintext operator*(const UnsignedWordPlaintext &b) const { UnsignedWordPlaintext c(*this); c*=b; return c; }
	UnsignedWordPlaintext operator<<(int i) const { UnsignedWordPlaintext c(*this); c<<=i; return c; }
	UnsignedWordPlaintext operator>>(int i) const { UnsignedWordPlaintext c(*this); c>>=i; return c; }

	UnsignedWordPlaintext &operator=(const UnsignedWordPlaintext &u) {
		if (this == &u)
			return *this;
		_val = u._val;

		return *this;
	}

	std::vector<long> operator==(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] == b._val[i]) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator!=(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] != b._val[i]) ? 1 : 0;
		return ret;
	}
		

	std::vector<long> operator>(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] > b._val[i]) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator<(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] < b._val[i]) ? 1 : 0;
		return ret;
	}

	std::vector<long> operator>=(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] >= b._val[i]) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator<=(const UnsignedWordPlaintext &b) const {
		assert(_val.size() == b._val.size());
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] <= b._val[i]) ? 1 : 0;
		return ret;
	}

	std::vector<long> operator==(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] == b) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator!=(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] != b) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator<(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] < b) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator>(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] > b) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator<=(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] <= b) ? 1 : 0;
		return ret;
	}
	std::vector<long> operator>=(long b) const {
		std::vector<long> ret(_val.size());

		for (size_t i = 0; i < ret.size(); ++i)
			ret[i] = (_val[i] >= b) ? 1 : 0;
		return ret;
	}

	std::vector<long> to_vector() const {
		std::vector<long> ret(simd_factor(), 0);

		for (size_t i_simd = 0; i_simd < simd_factor(); ++i_simd) {
			ret[i_simd] = _val[i_simd].convert_to<long>();
		}
		return ret;
	}

	unsigned long to_int() const {
		return _val[0].convert_to<long>();
	}

//  This function is possible only when the underlying HelibNumber is of ring size bigger than 2
//	Bit bits_to_number() const {
//		if (bitLength() == 0) {
//			Bit ret(0);
//			return ret;
//		}
//		Bit ret = (*this)[bitLength() - 1];
//
//		for (int i = bitLength() - 2; i >= 0; --i) {
//			ret += ret;
//			ret += (*this)[i];
//		}
//
//		return ret;
//	}

//	std::string to_bit_stream() const {
//		char buf[100];
//		std::string ret = "";
//		for (int i = bits() - 1; i >= 0; --i) {
//			sprintf(buf, " %d", (*this)[i].to_int());
//			ret += buf;
//		}
//		return ret;
//	}
//
//	template<int S, class B>
//	friend std::ostream &operator<<(std::ostream &out, const UnsignedWordPlaintext<S,B> &z);
//	template<int S, class B>
//	friend std::istream &operator>>(std::istream &in, UnsignedWordPlaintext<S,B> &z);
//
//	static int static_in_range(int a) { return a & (((unsigned int)-1) >> (32 - MAX_BIT_NUM)); }
//	static int max_bit_num() { return MAX_BIT_NUM; }

};

void sub(UnsignedWordPlaintext &out, const boost::multiprecision::cpp_int &in1, const UnsignedWordPlaintext &in2, const UnsignedWordPlaintextKeys *k) {
	out = UnsignedWordPlaintext(std::vector<boost::multiprecision::cpp_int>(in2.simd_factor(), in1), k);
	out -= in2;
}

#endif
