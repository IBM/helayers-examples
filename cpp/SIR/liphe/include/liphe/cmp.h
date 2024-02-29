#ifndef ___COMPARE_LIPHE__
#define ___COMPARE_LIPHE__

#include <map>
#include <liphe/polynomial.h>

template <class Number>
class CompareEuler {
private:
	const Number &_val;
public:
	CompareEuler(const Number &v) : _val(v) {}

	Number operator==(int b) const { return isZero_Euler(((b == 0) ? _val : (_val - b))); }
	Number operator!=(int b) const { return Number(1) - operator==(b); }
	Number operator<=(int b) const;
	Number operator<(int b) const;
	Number operator>=(int b) const;
	Number operator>(int b) const;
};

template <class Number>
class CompareNative {
private:
	const Number &_val;
public:
	CompareNative(const Number &v) : _val(v) {}

	Number operator==(int b) const { return _val == b; }
	Number operator<=(int b) const { return _val <= b; }
	Number operator<(int b) const { return _val < b; }
	Number operator>=(int b) const { return _val >= b; }
	Number operator>(int b) const { return _val > b; }


	Number operator==(Number &b) const { return _val == b; }
	Number operator<=(const Number &b) const { return _val <= b; }
	Number operator<(const Number &b) const { return _val < b; }
	Number operator>=(const Number &b) const { return _val >= b; }
	Number operator>(const Number &b) const { return _val > b; }
};

template <class Number>
class ComparePoly {
private:
	static std::map<std::pair<int ,int>, Polynomial<Number> > _smaller_poly;
	const Number &_val;

	Polynomial<Number> range_polynomial(int min, int max) const {
		int phi = ::phi(_val.p());
		Polynomial<Number> poly(0);
		for (int i = 1; i < _val.p()/2; ++i)
			poly += Polynomial<Number>(1) - (Polynomial<Number>(1, "-x")^phi);
		return poly;
	}

public:
	ComparePoly(const Number &v) : _val(v) {}

	Number operator<(const Number &b) const {
//		Polynomial<Number> poly = range_polynomial(1, _val.p()/2) % _val.p();
//		return poly.compute(_val -b);

		auto key = std::pair<int,int>(_val.get_ring_size(), _val.get_ring_size());
		auto poly = _smaller_poly.find(key);
		if (poly == _smaller_poly.end()) {
			int half = _val.get_ring_size() / 2;
			Polynomial<Number> new_poly = Polynomial<Number>::build_polynomial(_val.get_ring_size(), _val.get_ring_size(), [half](int x)->int{ return (x > half) ? 1 : 0; });

			auto add = std::pair<std::pair<int,int>, Polynomial<Number> >(key, new_poly);
			poly = _smaller_poly.insert(add).first;
		}

		return (*poly).second.compute(_val - b);
	}

	Number operator>(const Number &b) const {
		auto key = std::pair<int,int>(_val.get_ring_size(), _val.get_ring_size());
		auto poly = _smaller_poly.find(key);
		if (poly == _smaller_poly.end()) {
			int half = _val.get_ring_size() / 2;
			Polynomial<Number> new_poly = Polynomial<Number>::build_polynomial(_val.get_ring_size(), _val.get_ring_size(), [half](int x)->int{ return (x > half) ? 1 : 0; });

			auto add = std::pair<std::pair<int,int>, Polynomial<Number> >(key, new_poly);
			poly = _smaller_poly.insert(add).first;
		}

		return (*poly).second.compute(b -_val);
	}

	Number operator<(const int b) const {
		if (b > Number::get_global_ring_size())
			return Number(1);
		if (b <= 0)
			return Number(0);

		auto key = std::pair<int,int>(Number::get_global_ring_size(), b);
		auto poly = _smaller_poly.find(key);
		if (poly == _smaller_poly.end()) {
			Polynomial<Number> new_poly = Polynomial<Number>::build_polynomial(Number::get_global_ring_size(), Number::get_global_ring_size(), [b](int x)->int{ return (x < b) ? 1 : 0; });
			auto add = std::pair<std::pair<int,int>, Polynomial<Number> >(key, new_poly);
			poly = _smaller_poly.insert(add).first;
		}

		return (*poly).second.compute(_val);
	}

	Number operator>(const int b) const {
		if (b >= _val.get_ring_size() - 1)
			return Number(0);
		if (b <= 0)
			return Number(1);

		auto key = std::pair<int,int>(_val.get_ring_size(), b);
		auto poly = _smaller_poly.find(key);
		if (poly == _smaller_poly.end()) {
			Polynomial<Number> new_poly = Polynomial<Number>::build_polynomial(_val.get_ring_size(), _val.get_ring_size(), [b](int x)->int{ return (x > b) ? 1 : 0; });
			auto add = std::pair<std::pair<int,int>, Polynomial<Number> >(key, new_poly);
			poly = _smaller_poly.insert(add).first;
		}

		return (*poly).second.compute(_val);
	}
};

template<class Number>
std::map<std::pair<int,int>, Polynomial<Number> > ComparePoly<Number>::_smaller_poly;

#endif
