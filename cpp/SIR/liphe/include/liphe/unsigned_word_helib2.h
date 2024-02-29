#ifndef ___UNSIGNED_WORD_HELIB_BINARY___
#define ___UNSIGNED_WORD_HELIB_BINARY___

// Author: Hayim Shaul 2020
//
// implements a positive number by its bit representation. Bit must be HELibNumber2 (with p = 2, and r = 1)
// UnsignedWords are implemented to have a variable number of bits, with a maximum defined to be MAX_BIT_NUM.
// The bit operations are implemented by HElib.
//
// Note: UnsignedWordHelib2Binary can be only be positive, negative numbers can be implemented with 2-complement but require
// a fixed number of bit representation, or some other TIHKUMATION like keeping the sign in a separate bit
// and for every + operation compute both +/- and choose by multiplying with sign bit
//

class UnsignedWordHelib2Binary {
private:
	enum {
		ALLOW_CARRY = 1,
		ALLOW_BORROW = 2
	};
	static unsigned int defaultFlags = 0;
	NTL::Vec<Ctxt> _val;

//	bool isZeros(const std::vector<long int> &n) {
//		for (auto v: n)
//			if (v != 0)
//				return false;
//		return true;
//	}

	void init(const std::vector<long int> &n, int minBits = 0) {
		assert(n.size() <= simd_factor());

		size_t bitNum = 0;
		for (auto v : n)
			while ((1 << bitNum) < v)
				++bitNum;
		if (bitNum < minBits)
			bitNum = minBits;

		resize(_val, bitNum, Ctxt(secKey));

		size_t i_bit = 0;
		while (!isZeros(n)) {
			std::vector<long> bits(n.size());
			for (size_t i_val = 0; i_val < n.size(); ++i_val) {
				if (i_bit < 63)
					bits[i_val] = (n[i_val] >> i_bit) & 1;
				else
					bits[i_val] = (n[i_val] < 0) ? 1 : 0;
			}

			keys()->encrypt(_val[i_bit], bits);
			++i_bit;
		}
	}

public:

	UnsignedWordHelib2Binary() {}
	UnsignedWordHelib2Binary(long int n, Helib2Keys *keys) { init( std::vector<long int>(1, n), keys); }
	UnsignedWordHelib2Binary(const std::vector<long int> &n, Helib2Keys ) { init(n); }
	UnsignedWordHelib2Binary(const Helib2Number &b, Helib2Number *keys) : _val(1) {
		_val[0] = b._val;
	}
	UnsignedWordHelib2Binary(const UnsignedWordHelib2Binary &u) : _val(u._val.size()) {
		for (int i = 0; i < bitLength(); ++i) {
			_val[i] = u._val[i];
		}
	}

	~UnsignedWordHelib2Binary() {
	}

	void from_int(int n, Helib2Keys *keys) { init(n, keys); }
	void from_int(const std::vector<long int> &v, Helib2Keys *keys) { init(v, keys); }
	void from_vector(const std::vector<long int> &v, Helib2Keys *keys) { init(v, keys); }

	static UnsignedWordHelib2Binary<MAX_BIT_NUM, Bit> static_from_int(int c, Helib2Keys *keys) {
		UnsignedWordHelib2Binary ret(c, keys);
		return ret;
	}

	static UnsignedWordHelib2Binary<MAX_BIT_NUM, Bit> static_from_int(const std::vector<long int> &c) {
		UnsignedWordHelib2Binary ret(c);
		return ret;
	}

	void add(const UnsignedWordHelib2Binary &w, bool allowCarry) {
		int sizeLimit = std::max(bits(), w.bits());
		if (allowCarry)
			++sizeLimit;
		addTwoNumbers(CtPtrs_VecCt(_val), CtPtrs_VecCt(_val), CtPtrs_VecCt(w._val), sizeLimit);
	}

	void sub(const UnsignedWordHelib2Binary &w, bool allowBorrow) {
		int sizeLimit = std::max(bits(), w.bits());
		if (allowBorrow)
			sizeLimit += 2;

		UnsignedWordHelib2Binary negW(w);
		if (negW.bits() < sizeLimit) {
			negW.bits(sizeLimit);
			// 1-complement (negate all bits)
			for (size_t i_bit = 0; i_bit < negW.bits(); ++i_bit)
				negW.bit(i_bit) += 1;
			// 2-complement
			negW.add(1, false);
		}
		add(negW, false);
	}

	void mult(const UnsignedWordHelib2Binary &w, bool allowCarry) {
		int sizeLimit;

		if (bits() == 0)
			return;
		if (w.bits() == 0)
			init(0);

		if (allowCarry)
			sizeLimit = bits() + w.bits();
		else
			sizeLimit = std::max(bits(), w.bits());
		multTwoNumbers(CtPtrs_VecCt(_val), CtPtrs_VecCt(_val), CtPtrs_VecCt(w._val), sizeLimit);
	}

	void shiftBitsLeft(int n, bool allowGrowth) {
		assert(n > 0);

		if (allowGrowth) {
			_val.resize(bits() + n);
		} else if (n >= bits()) {
			_val.resize(0);
			return;
		}

		for (size_t i = 0; i < bits() - n; ++i) {
			_val[bits() - i - 1] = _val[bits() - n - i - 1];
		}
		for (size_t i = 0; i < n; ++i) {
			set _val[i] to zeroes
		}
	}

	void shiftBitsRight(int n, int flags) {
		assert(n > 0);

		if (flags & PAD_WORD) {
			_val.resize(bits() - n);
		} else if (n >= bits()) {
			_val.resize(0);
			return;
		}

		for (size_t i = 0; i < bits() - n; ++i) {
			_val[i] = _val[i + n];
		}

		if ((flags & PAD_BITS) == 0) {
			_val.resize(bits() - n);
		} else {
			if (flags & ALLOW_NEG) {
				for (size_t i = bits() - n; i < bits(); ++i)
					_val[i] = _val[bits() - n];
			} else {
				for (size_t i = bits() - n; i < bits(); ++i) {
					std::vector<long> bits(n.size(), 0);
					keys()->encrypt(_val[i], bits);
				}
			}
		}
	}

	void rotateSimd(int n) {
		for (size_t i = 0; i < bits(); ++i)
			keys()->rotate(_val[i], n);
	}

	void shiftSimd(int n) {
		for (size_t i = 0; i < bits(); ++i)
			keys()->shift(_val[i], n);
	}

	void operator+=(const UnsignedWordHelib2Binary &w) { add(w, defaultFlags & ALLOW_CARRY); }
	void operator-=(const UnsignedWordHelib2Binary &w) { sub(w, defaultFlags & ALLOW_BORROW); }
	void operator*=(const UnsignedWordHelib2Binary &w) { mult(w, defaultFlags & ALLOW_CARRY); }
	void operator*=(const Helib2Number &b) {
		for (size_t i = 0; i < bits(); ++i)
			_val[i] *= b._val;
	}

	void operator<<=(int i) { shiftBitsLeft(i, false); }
	void operator>>=(int i) { shiftBitsRight(i, PAD_WORD | ALLOW_NEG); }

	const Helib2Number operator[](int i) {
		assert(i < bitLength());
		return Helib2Number(_val[i]);
	}

	//neg should expand the bit representation to MAX_BIT_NUM and use 2-complement
	//void neg();

	UnsignedWordHelib2Binary operator+(const UnsignedWordHelib2Binary &b) const { UnsignedWordHelib2Binary<MAX_BIT_NUM, Bit> c(*this); c+=b; return c; }
	UnsignedWordHelib2Binary operator-(const UnsignedWordHelib2Binary &b) const { UnsignedWordHelib2Binary<MAX_BIT_NUM, Bit> c(*this); c-=b; return c; }
	UnsignedWordHelib2Binary operator*(const UnsignedWordHelib2Binary &b) const;
	UnsignedWordHelib2Binary operator*(const Bit &b) const;
	UnsignedWordHelib2Binary operator<<(int i) const { UnsignedWordHelib2Binary c(*this); c<<=i; return c; }
	UnsignedWordHelib2Binary operator>>(int i) const { UnsignedWordHelib2Binary c(*this); c>>=i; return c; }

	UnsignedWordHelib2Binary &operator=(const UnsignedWordHelib2Binary &u) {
		if (this == &u)
			return *this;

		_val.resize(u.bits());
		for (unsigned int i = 0; i < _bits.size(); ++i) {
			_val[i] = u._val[i];
		}

		return *this;
	}

	Helib2Number operator==(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		mu += ni;
		mu += 1;
		return mu;
	}
	Helib2Number operator!=(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		mu += nu;
		return mu;
	}
		

	Helib2Number operator>(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		return mu;
	}
	Helib2Number operator<(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		return ni;
	}

	Helib2Number operator>=(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		ni += 1;
		return ni;
	}
	Helib2Number operator<=(const UnsignedWordHelib2Binary &b) const {
		Helib2Number mu;
		Helib2Number ni;
		compareTwoNumber(mu, ni, CtPtrs_VecCt(_val), CtPtrs_VecCt(b._val));
		mu += 1;
		return mu;
	}

	Helib2Number operator==(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator==(_b);
	}
	Helib2Number operator!=(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator!=(_b);
	}
	Helib2Number operator<(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator<(_b);
	}
	Helib2Number operator>(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator>(_b);
	}
	Helib2Number operator<=(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator<=(_b);
	}
	Helib2Number operator>=(long b) const {
		UnsignedWordHelib2Binary _b(b);
		return operator>=(_b);
	}

	int bits() const { return _val.size(); }
	void bits(int len) {
		int oldLen = bits();

//		for (int i = len; i < bits(); ++i) {
//			delete _val[i];
//		}
		 _bits.val(len);

		if (ALLOW_NEG && (oldLen > 0)) {
			for (size_t i = oldLen; i < len; ++i) {
				_val[i] = _val[oldLen - 1];
			}
		} else {
			for (size_t i = oldLen; i < len; ++i) {
				std::vector<long> bits(simd_factor, 0)
				keys()->encrypt(_val[i], bits);
			}
		}
	}

	int size() const { return _val.size(); }
	void set_bit(int i, const HELibNumber2 &b) {
		assert(i >= 0);
		assert(i < bits()); // TODO: in the future resize _val

		_val[i] = b.ctxt();
	}

	const HELibNumber2 &msb(size_t i = 0) const {
		assert(i < _val.size());
		return _val[_val.size() - i];
	}

	std::vector<long> to_vector() const {
		std::vector<long> ret(simd_factor(), 0);

		for (i_bit = 0; i_bit < bit_factor(); ++i_bit) {
			std::vector<long> bits;
			_keys->decrypt(bits, _bits);
			for (i_simd = 0; i_simd < simd_factor(); ++i_simd) {
				ret[i_simd] += bits[i_simd] << i_bit;
			}
		}
		return ret;
	}

	unsigned long to_int() const {
		std::vector<long> a = to_vector();
		return a[0];
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
//	friend std::ostream &operator<<(std::ostream &out, const UnsignedWordHelib2Binary<S,B> &z);
//	template<int S, class B>
//	friend std::istream &operator>>(std::istream &in, UnsignedWordHelib2Binary<S,B> &z);
//
//	static int static_in_range(int a) { return a & (((unsigned int)-1) >> (32 - MAX_BIT_NUM)); }
//	static int max_bit_num() { return MAX_BIT_NUM; }

};

#endif
