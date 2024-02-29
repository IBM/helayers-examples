#ifndef ___HEAAN_NUMBER___
#define ___HEAAN_NUMBER___

#include <heaan_keys.h>

class HeaanNumber {
private:
	static HeaanKeys *_prev_keys;
	HeaanKeys *_keys;
	Ciphertext _val;

	int _mul_depth;
	int _add_depth;

public:
	static void set_global_keys(HeaanKeys *k) { _prev_keys = k; }

	HeaanNumber() : _keys(_prev_keys), _mul_depth(0), _add_depth(0) {}
	HeaanNumber(double v) : _keys(_prev_keys), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, v); }
//	HeaanNumber(const std::vector<long> &v) : _keys(_prev_keys), _val(_keys->publicKey()), _mul_depth(0), _add_depth(0) { _keys->encrypt(_val, v); print("allocating"); }
	HeaanNumber(const HeaanNumber &n) : _keys(n._keys), _val(n._val), _mul_depth(n._mul_depth), _add_depth(n._add_depth) {}
	HeaanNumber(const Ciphertext &n) : _keys(_prev_keys), _val(n), _mul_depth(0), _add_depth(0) {}

	~HeaanNumber() { std::cerr << "destructing " << this << std::endl; }

	int mul_depth() const { return _mul_depth; }
	int add_depth() const { return _add_depth; }

	void from_float(double i)  {
			_keys->encrypt(_val, i);
		}

	static HeaanNumber static_from_float(double i) {
			HeaanNumber ret;
			_prev_keys->encrypt(ret._val, i);
			return ret;
		}


	double to_float() const {
		return _keys->decrypt(_val);
//		complex<double>* m = _keys->scheme()->decrypt(_keys->secretKey(), &_val);
//		double ret = m->real();
//		delete[] m;
//		return ret;
	}


	void operator=(const HeaanNumber &f) {
		_val = f._val;
	}

	HeaanNumber operator*(const HeaanNumber &f) {
		HeaanNumber ret(*this);
		ret *= f;
		return ret;
	}

	HeaanNumber operator+(const HeaanNumber &f) {
		HeaanNumber ret(*this);
		ret += f;
		return ret;
	}

	HeaanNumber operator-(const HeaanNumber &f) {
		HeaanNumber ret(*this);
		ret -= f;
		return ret;
	}

	HeaanNumber &operator*=(HeaanNumber &f) {
		// HEAAN operators do not get const arguments :(
//		HeaanNumber f(_f);
		if (&f.val == &_val)
			_keys->scheme()->imultAndEqual(_val);
		else
			_keys->scheme()->imult(_val, f._val);
		_keys->scheme()->reScaleByAndEqual(_val, _keys->logP());
		_mul_depth = std::max(_mul_depth, f._mul_depth) + 1;
		return *this;
	}

	HeaanNumber &operator*=(const double _f) {
		// HEAAN operators do not get const arguments :(
		_keys->scheme()->imultByConst(_val, _f, _keys->logP());
		_keys->scheme()->reScaleByAndEqual(_val, _keys->logP());
		_mul_depth += 1;
		return *this;
	}


	HeaanNumber &operator+=(const HeaanNumber &_f) {
		// HEAAN operators do not get const arguments :(
		HeaanNumber f(_f);

		while (mul_depth() < f.mul_depth())
			increase_mul_depth();
		while (mul_depth() > f.mul_depth())
			f.increase_mul_depth();

		_keys->scheme()->iadd(_val, f._val);
		_add_depth = std::max(_add_depth, f._add_depth) + 1;
		return *this;
	}

	void increase_mul_depth() {
		static HeaanNumber l(1);
		operator*=(l);
	}

	HeaanNumber &operator-=(const HeaanNumber &_f) {
		// HEAAN operators do not get const arguments :(
		HeaanNumber f(_f);

		while (mul_depth() < f.mul_depth())
			increase_mul_depth();
		while (mul_depth() > f.mul_depth())
			f.increase_mul_depth();

		_keys->scheme()->isub(_val, f._val);
		_add_depth = std::max(_add_depth, f._add_depth) + 1;
		return *this;
	}

};

#endif

