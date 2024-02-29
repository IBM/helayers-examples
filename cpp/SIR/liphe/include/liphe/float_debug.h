#ifndef ___FLOAT_DEBUG___
#define ___FLOAT_DEBUG___

#include <algorithm>

class FloatDebug {
private:
	double _f;
	int _mul_depth;
	int _add_depth;
public:
	static FloatDebug encrypt(double f) { return FloatDebug(f); }
	static FloatDebug static_from_float(double d) { return FloatDebug(d); }
	float to_float() const { return _f; }
	void from_float(double d) { *this = FloatDebug(d); }


	FloatDebug(float f = 0) : _f(f), _mul_depth(0), _add_depth(0) {}
	FloatDebug(const FloatDebug &f) : _f(f._f), _mul_depth(f._mul_depth), _add_depth(f._add_depth) {}

	FloatDebug operator*(const FloatDebug &f) {
		FloatDebug ret(*this);
		ret *= f;
		return ret;
	}

	FloatDebug operator+(const FloatDebug &f) {
		FloatDebug ret(*this);
		ret += f;
		return ret;
	}

	FloatDebug operator-(const FloatDebug &f) {
		FloatDebug ret(*this);
		ret -= f;
		return ret;
	}

	FloatDebug &operator*=(const FloatDebug &f) {
		_f *= f._f;
		_mul_depth = std::max(_mul_depth, f._mul_depth) + 1;
		return *this;
	}

	FloatDebug &operator+=(const FloatDebug &f) {
		_f += f._f;
		_add_depth = std::max(_add_depth, f._add_depth) + 1;
		return *this;
	}

	FloatDebug &operator-=(const FloatDebug &f) {
		_f -= f._f;
		_add_depth = std::max(_add_depth, f._add_depth) + 1;
		return *this;
	}
};

#endif
