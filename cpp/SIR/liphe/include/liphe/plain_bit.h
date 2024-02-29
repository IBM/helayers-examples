#ifndef ___PLAIN_BIT___
#define ___PLAIN_BIT___

#include <iostream>

// Author: Hayim Shaul 2016.

// PlainBit.
// implements a plain bit. all operations are performed on plain bits, and return plain bits

class PlainBit {
private:
	int _bit;
public:
	PlainBit() : _bit(0) {}
	PlainBit(int b) : _bit(!!b) {}
	PlainBit(const PlainBit &b) : _bit(b._bit) {}

	PlainBit &operator=(const PlainBit &b) { _bit = b._bit; return *this; }
	void operator+=(const PlainBit &b) { _bit ^= b._bit; }
	void operator-=(const PlainBit &b) { operator+=(b); }
	void operator*=(const PlainBit &b) { _bit *= b._bit; }
	void operator|=(const PlainBit &b) { _bit = _bit | b._bit; }

	PlainBit operator+(const PlainBit &b) const { PlainBit c(*this); c+=b; return c; }
	PlainBit operator-(const PlainBit &b) const { return operator+(b); }
	PlainBit operator*(const PlainBit &b) const { PlainBit c(*this); c*=b; return c; }

	// using deMorgan
//	PlainBit operator|(const PlainBit &b) const { PlainBit c; c = !((!*this)*(!b)); return c; }
	// using ???
	PlainBit operator|(const PlainBit &b) const { PlainBit c; c = (*this) + b + (*this)*b; return c; }

	void neg() { _bit = 1 - _bit; }
	PlainBit operator!() const { PlainBit c(*this); c.neg(); return c; }

	int get() const { return _bit; }
};


inline std::ostream &operator<<(std::ostream &out, const PlainBit &b) {
	out << b.get();
	return out;
}

#endif

