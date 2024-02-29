#ifndef ___ZP____
#define ___ZP____

#include <assert.h>
#include <algorithm>
#include <istream>
#include <ostream>
#include <vector>
#include <functional>

#include <stdio.h>

#include <boost/multiprecision/cpp_int.hpp>
using boost::multiprecision::cpp_int;

class ZP;

class ZPKeys {
private:
	long _p;
	long _r;
	unsigned long _simd;

	static long power(long base, long exp, long p = -1) {
		cpp_int ret = 1;
		for (int i = 0; i < exp; ++i) {
			if (p == -1)
				ret *= base;
			else
				ret = (ret * base) % p;
		}
		return static_cast<long>(ret);
	}
public:
	ZPKeys() : _p(0), _r(0), _simd(0) {}
	ZPKeys(long p, long r, long s) : _p(p), _r(r), _simd(s) {}

	long p() const { return _p; }
	long r() const { return _r; }
	long get_ring_size() const { return power(_p, _r); }
	unsigned long simd_factor() const { return _simd; }

	void from_bigint(ZP &z, const cpp_int &c) const;
	void from_int(ZP &z, long c) const;
	ZP from_bigint(const cpp_int &c) const;
	ZP from_int(long c) const;
	long to_int(const ZP &z) const;

	void from_vector(ZP &z, const std::vector<long> &c) const;
	ZP from_vector(const std::vector<long> &c) const;
	std::vector<long> to_vector(const ZP &z) const;
};

class ZP {
private:
	static unsigned long _prev_simd_size;
	static long _prev_p;
	static long _prev_r;

	static long getPrevP() { return _prev_p; }
	static long getPrevR() { return _prev_r; }
	static unsigned long getPrevSimdSize() { return _prev_simd_size; }

	static std::function<long (void)> _getP;
	static std::function<long (void)> _getR;
	static std::function<unsigned long (void)> _getSimdSize;

	long _p;
	long _r;
	unsigned long _simd_size;
	long *_val;

	int _mul_depth;
	int _add_depth;

	long find_inv(long x, long p, long r) const;

	static long power(long base, long exp, long p = -1) {
		cpp_int ret = 1;
		for (int i = 0; i < exp; ++i) {
			if (p == -1)
				ret *= base;
			else
				ret = (ret * base) % p;
		}
		return static_cast<long>(ret);
	}

	void set_all(const cpp_int &a) { for (unsigned long i = 0; i < _simd_size; ++i) _val[i] = in_range(a); }
	void set_all(long a) { for (unsigned long i = 0; i < _simd_size; ++i) _val[i] = in_range(a); }
	void set_all(long *a) { for (unsigned long i = 0; i < _simd_size; ++i) _val[i] = in_range(a[i]); }
	void set_all(const std::vector<long> &a) {
		size_t m = std::min(a.size(), _simd_size);
		for (size_t i = 0; i < m; ++i) _val[i] = in_range(a[i]);
	}
		
public:
	ZP() : _p(_getP()), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; }
	ZP(long v) : _p(_getP()), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const cpp_int &v) : _p(_getP()), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const std::vector<long> &v) : _p(_getP()), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const std::vector<long> &v, long p, long r, long s) : _p(p), _r(r), _simd_size(s), _mul_depth(0), _add_depth(0) {
		_val = new long[_simd_size];
		set_all(v); }
	ZP(long v, long p) : _p(p), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(long v, long p, long r) : _p(p), _r(r), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(long v, long p, long r, long s) : _p(p), _r(r), _simd_size(s), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }

	ZP(const cpp_int &v, long p) : _p(p), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const cpp_int &v, long p, long r) : _p(p), _r(r), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const cpp_int &v, long p, long r, long s) : _p(p), _r(r), _simd_size(s), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }

	ZP(const ZPKeys *k, long v) : _p(k->p()), _r(k->r()), _simd_size(k->simd_factor()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(long *v, long p) : _p(p), _r(_getR()), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(long *v, long p, long r) : _p(p), _r(r), _simd_size(_getSimdSize()), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(long *v, long p, long r, long s) : _p(p), _r(r), _simd_size(s), _mul_depth(0), _add_depth(0) { _val = new long[_simd_size]; set_all(v); }
	ZP(const ZP &zp) : _p(zp._p), _r(zp._r), _simd_size(zp._simd_size), _mul_depth(zp._mul_depth), _add_depth(zp._add_depth) {
		_val = new long[_simd_size];
 		for (unsigned long i = 0; i < _simd_size; ++i)
			_val[i] = zp._val[i];
	}

	~ZP() { delete[] _val; }

	void set_keys(ZPKeys *k) { _p = k->p(); _r = k->r(); _simd_size = k->simd_factor(); }

	int in_range(int a) const { while (a < 0) a += get_ring_size(); return a % get_ring_size(); }
	long in_range(cpp_int a) const { while (a < 0) a += get_ring_size(); return static_cast<long>(a % get_ring_size()); }
	// static int static_in_range(int a) { while (a < 0) a += get_ring_size(); return a % get_ring_size(); }
	long to_int() const { return _val[0] % get_ring_size(); }
	std::vector<long> to_vector() const { return std::vector<long>(_val, _val + _simd_size); }
	ZP clone() { return ZP(_val, _p, _r); }

	void from_int(long i) { _val[0] = in_range(i); }
	static ZP static_from_int(long i) { return ZP(i); }
	void from_vector(const std::vector<long> &in) {
		for (unsigned int i = 0; i < std::min((unsigned long)_simd_size, in.size()); ++i)
			_val[i] = in_range(in[i]);
	}

	unsigned long simd_factor() const { return _simd_size; }
	void simd_factor(unsigned int s) { _prev_simd_size = _simd_size = s; }
	static void set_global_simd_factor(unsigned int a) { _prev_simd_size = a; }
	static unsigned long get_global_simd_factor() { return _prev_simd_size; }

	static void set_global_p(long p, long r = 1) { _prev_p = p; _prev_r = r; }
	static int global_p() { return _getP(); }
	static int get_global_ring_size() { return power(_getP(), _getR()); }
	void set_p(long p, long r = 1) { _p = _prev_p = p; _r = _prev_r = r; }
	long p() const { return _p; }
	long r() const { return _r; }
	long get_ring_size() const { return power(p(), r()); }
	long inv(long x) const { assert(r() == 1); return (p() <= 3) ? x : power(x, p() - 2, p()); }
	ZP inv() const {
		ZP ret(*this);
		for (unsigned int i = 0; i < _simd_size; ++i)
			ret._val[i] = inv(_val[i]);
		return ret;
	}
	long mod(long x) const { return ((x % get_ring_size()) + get_ring_size()) % get_ring_size(); }
	long mod(const cpp_int &x) const { return static_cast<long>(((x % get_ring_size()) + get_ring_size()) % get_ring_size()); }

	int add_depth() const { return _add_depth; }
	int mul_depth() const { return _mul_depth; }

	ZP &operator=(const ZP &b) {
		_p = b._p;
		_r = b._r;
		if (_simd_size != b._simd_size) {
			_simd_size = b._simd_size;
			if (_val != NULL)
				delete[] _val;
			_val = new long[_simd_size];
		}
		_mul_depth = b._mul_depth;
		_add_depth = b._add_depth;
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] = b._val[i];
		return *this;
	}

	void divide_by_p() {
		assert(_r > 1);
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] /= _p;
	}

	ZP shift_left(unsigned int step) const {
		ZP ret(*this);

		for (unsigned int i = 0; i < _simd_size; ++i)
			if (i + step < _simd_size)
				ret._val[i] = _val[i + step];
		return ret;
	}

	ZP shift_right(unsigned int step) const {
		ZP ret(*this);

		for (unsigned int i = 0; i < _simd_size; ++i)
			if (i >= step)
				ret._val[i] = _val[i - step];
		return ret;
	}

	ZP rotate_left(int step) const __attribute__((warn_unused_result)) {
		ZP ret(*this);

		for (unsigned int i = 0; i < _simd_size; ++i)
			ret._val[i] = _val[(i + step + _simd_size) % _simd_size];
		return ret;
	}
	void self_rotate_left(int step) { *this = this->rotate_left(step); }

	ZP rotate_right(int step) const __attribute__((warn_unused_result)) { return rotate_left(-step); }
	void self_rotate_right(int step) { *this = this->rotate_right(step); }

	ZP operator-() const {
		ZP zp(*this);
		for (unsigned int i = 0; i < _simd_size; ++i)
			zp._val[i] = -zp._val[i];
		return zp;
	}

	ZP operator-(long z) const { ZP zp(*this); zp -= z; return zp; }
	ZP operator+(long z) const { ZP zp(*this); zp += z; return zp; }
	ZP operator*(long z) const { ZP zp(*this); zp *= z; return zp; }

	void operator-=(long z) {
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] = mod(_val[i] - z);
		++_add_depth;
	}

	void operator+=(long z) {
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] = mod(_val[i] + z);
		++_add_depth;
	}

	void operator*=(long z) {
		for (unsigned int i = 0; i < _simd_size; ++i) {
			cpp_int temp = _val[i];
			temp *= z;
			_val[i] = mod(temp);
		}
		++_add_depth;
	}

	ZP operator-(const std::vector<long> &z) const { ZP zp(*this); zp -= z; return zp; }
	ZP operator+(const std::vector<long> &z) const { ZP zp(*this); zp += z; return zp; }
	ZP operator*(const std::vector<long> &z) const { ZP zp(*this); zp *= z; return zp; }

	void operator-=(const std::vector<long> & z) {
		unsigned int S = (_simd_size < z.size()) ? _simd_size : z.size();
		for (unsigned int i = 0; i < S; ++i)
			_val[i] = mod(_val[i] - z[i]);
		++_add_depth;
	}

	void operator+=(const std::vector<long> & z) {
		unsigned int S = (_simd_size < z.size()) ? _simd_size : z.size();
		for (unsigned int i = 0; i < S; ++i)
			_val[i] = mod(_val[i] + z[i]);
		++_add_depth;
	}

	void operator*=(const std::vector<long> & z) {
		unsigned int S = (_simd_size < z.size()) ? _simd_size : z.size();
		for (unsigned int i = 0; i < S; ++i) {
			cpp_int temp = _val[i];
			temp *= z[i];
			_val[i] = mod(temp);
		}
		++_add_depth;
	}


	ZP operator-(const ZP &z) const { ZP zp(*this); zp -= z; return zp; }
	ZP operator+(const ZP &z) const { ZP zp(*this); zp += z; return zp; }
	ZP operator*(const ZP &z) const { ZP zp(*this); zp *= z; return zp; }
	ZP operator!() const { ZP zp(*this); zp = ZP(1) - zp; return zp; }

	void operator-=(const ZP &z) {
		assert(_p == z._p);
		assert(_r == z._r);
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] = mod(_val[i] - z._val[i]);
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator+=(const ZP &z) {
		assert(_p == z._p);
		assert(_r == z._r);
		for (unsigned int i = 0; i < _simd_size; ++i)
			_val[i] = mod(_val[i] + z._val[i]);
		_mul_depth = std::max(_mul_depth, z._mul_depth);
		_add_depth = std::max(_add_depth, z._add_depth) + 1;
	}

	void operator*=(const ZP &z) {
		assert(_p == z._p);
		assert(_r == z._r);
		for (unsigned int i = 0; i < _simd_size; ++i) {
			cpp_int temp = _val[i];
			temp *= z._val[i];
			_val[i] = mod(temp);
		}
		_mul_depth = std::max(_mul_depth, z._mul_depth) + 1;
		_add_depth = std::max(_add_depth, z._add_depth);
	}

	template<class BITS>
	BITS to_digits() const {
		BITS ret;
		ret.set_bit_length(r());
		for (int i = 0; i < r(); ++i) {
			ZP bit;
			for (unsigned int s = 0; s < _simd_size; ++s) {
				bit._val[s] = (_val[s] / power(p(), i)) % p();
			}
			ret.set_bit(i, bit);
		}
		return ret;
	}

	void assert_co_prime(int a) const {
		assert(a != 0);
		assert(a != 1);

		for (unsigned int i = 0; i < _simd_size; ++i) {
			int b = _val[i];

			if (b > 1) {

				while (((b % a) != 0) && ((b % a) != 0)) {
					if (a > b)
						a -= b;
					else
						b -= a;
				}
				int gcd = (a < b) ? a : b;
				assert (gcd == 1);
			}
		}
	}

//	bool operator>(const ZP &z) const { assert(_p == z._p); return _val[0] > z._val[0]; }
//	bool operator<(const ZP &z) const { assert(_p == z._p); return _val[0] < z._val[0]; }
//	bool operator!=(const ZP &z) const { assert(_p == z._p); return _val[0] != z._val[0]; }

	void reduceNoiseLevel() {}
	void recrypt() {}

	void save(std::ostream &s) const {
		s << "ZP" << std::endl;
		for (size_t i = 0; i < _simd_size; ++i)
			s << _val[i];
		s << std::endl;
	}

	friend std::ostream &operator<<(std::ostream &out, const ZP &z);
	friend std::istream &operator>>(std::istream &out, ZP &z);
};

inline void ZPKeys::from_int(ZP &z, long c) const {
	z = ZP(c, _p, _r, _simd);
}

inline void ZPKeys::from_bigint(ZP &z, const cpp_int &c) const {
	z = ZP(c, _p, _r, _simd);
}

inline ZP ZPKeys::from_int(long c) const {
	ZP ret;
	from_int(ret, c);
	return ret;
}

inline ZP ZPKeys::from_bigint(const cpp_int &c) const {
	ZP ret;
	from_bigint(ret, c);
	return ret;
}

inline long ZPKeys::to_int(const ZP &z) const {
	return z.to_int();
}

inline void ZPKeys::from_vector(ZP &z, const std::vector<long> &c) const {
	z = ZP(c, _p, _r, _simd);
}

inline ZP ZPKeys::from_vector(const std::vector<long> &c) const {
	ZP ret(c, _p, _r, _simd);
	return ret;
}

inline std::vector<long> ZPKeys::to_vector(const ZP &z) const {
	return z.to_vector();
}

inline std::ostream &operator<<(std::ostream &out, const ZP &z) {

	out
		<< z._p << " "
		<< z._r << " ";

	out << z.simd_factor() << " ";
	for (unsigned int i = 0; i < z.simd_factor(); ++i)
		out << z._val[i] << " ";

	out
		<< z._mul_depth << " "
		<< z._add_depth << " ";

	return out;
}

inline std::istream &operator>>(std::istream &in, ZP &z) {
	in
		>> z._p
		>> z._r;

	unsigned int simd_size;
	in >> simd_size;
	assert(simd_size == z.simd_factor());
	for (unsigned int i = 0; i < z.simd_factor(); ++i)
		in >> z._val[i];

	in
		>> z._mul_depth
		>> z._add_depth;

	return in;
}

#endif
