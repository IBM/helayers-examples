#ifndef ___CARRAY_ITERATOR___
#define ___CARRAY_ITERATOR___

template<class X>
class CArrayIterator {
private:
	X *_x;
	int _size;

	int _location;

	void copy(const CArrayIterator &a) { _x = a._x; _size = a._size; _location = a._location; }
public:
	CArrayIterator(X *x, int s, int l = 0) : _x(x), _size(s), _location(l) {}
	CArrayIterator(const CArrayIterator &a) { copy(a); }

	CArrayIterator &operator=(const CArrayIterator &a) { copy(a); return *this; }

	void begin() { _location = 0; }
	void end() { _location = _size; }

	bool is_end() { return _location == _size; }
	bool is_begin() { return _location == 0; }

	void operator++() { ++_location; }

	bool operator==(const CArrayIterator &a) const {
		assert(_x == a._x);
		return _location == a._location;
	}
	bool operator!=(const CArrayIterator &a) const { return !operator==(a); }

	X &operator*() { return _x[_location]; }
	const X &operator*() const { return _x[_location]; }
};

#endif
