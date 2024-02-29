#ifndef ___PACKED_VECTOR___
#define ___PACKED_VECTOR___

#include <boost/multiprecision/cpp_int.hpp>
using boost::multiprecision::cpp_int;

template<class Number>
class PackedVector {
private:
	std::vector<Number> _ctxt;
	unsigned int _size;
public:
	PackedVector() {}

	template<class Plaintext, class PubKey, class PrivKey>
	PackedVector(const std::vector<Plaintext> &v, const PubKey *pubKey, const PrivKey *privKey) { init_vector(v, pubKey, privKey); }

	template<class PubKey>
	PackedVector(const std::vector<cpp_int> &v, const PubKey *pubKey) { init_vector(v, pubKey); }

	template<class PubKey>
	void pubKey(const PubKey *pubKey) {
		for (auto &c : _ctxt) {
			c.setPubKey(pubKey);
		}
	}

	PackedVector<Number> &operator=(const PackedVector<Number> &v) {
		_size = v._size;
		_ctxt.resize(v._ctxt.size());
		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] = v._ctxt[i];
		}
		return *this;
	}

	unsigned int size() const { return _size; }
	const std::vector<Number> &ctxt() const { return _ctxt; }
	unsigned int simd_factor() const { return _ctxt[0].simd_factor(); }

	void resize(int s, unsigned int simd_factor) {
//		int simd_factor = Number::static_simd_factor();
		_size = s;
		_ctxt.resize( (s+simd_factor-1) / simd_factor );
	}

	size_t ctxt_size() const { return _ctxt.size(); }
	Number &ctxt(int i) { return _ctxt[i]; }
	const Number &ctxt(int i) const { return _ctxt[i]; }

	template<class PrivKey>
	cpp_int to_bigint(int i, const PrivKey *privKey) const {
		int simd_factor = _ctxt[0].simd_factor();
		int i_ctxt = i / simd_factor;
		std::vector<cpp_int> a = _ctxt[i_ctxt].to_bigint_vector(privKey);
		return a[i - i_ctxt * simd_factor];
	}

	template<class Out, class PubKey, class PrivKey>
	void to_vector(std::vector<Out> &o, const PubKey *pubKey, const PrivKey *privKey) const {
		o.resize(size());
		int simd_factor = _ctxt[0].simd_factor();
		int i_ctxt = -1;
		int i_slot = simd_factor;
		std::vector<cpp_int> data;

		for (unsigned int i = 0; i < size(); ++i) {
			if (i_slot == simd_factor) {
				i_slot = 0;
				++i_ctxt ;
				data = _ctxt[i_ctxt].to_bigint_vector(privKey);
			}

			o[i].from_bigint(data[i_slot], pubKey);
			++i_slot;
		}
	}

	template<class PrivKey>
	void to_bigint_vector(std::vector<cpp_int> &o, const PrivKey *privKey) const {
		o.resize(size());
		int simd_factor = _ctxt[0].simd_factor();
		int i_ctxt = -1;
		int i_slot = simd_factor;
		std::vector<cpp_int> data;

		for (unsigned int i = 0; i < size(); ++i) {
			if (i_slot == simd_factor) {
				i_slot = 0;
				++i_ctxt ;
				data = _ctxt[i_ctxt].to_bigint_vector(privKey, size() - i);
			}

			o[i] = data[i_slot];
			++i_slot;
		}
	}

	template<class Out, class PubKey>
	void to_bigint_vector(std::vector<Out> &o, const PubKey *pubKey) const {
		o.resize(size());
		int simd_factor = _ctxt[0].simd_factor();
		int i_ctxt = -1;
		int i_slot = simd_factor;
		std::vector<cpp_int> data;

		for (unsigned int i = 0; i < size(); ++i) {
			if (i_slot == simd_factor) {
				i_slot = 0;
				++i_ctxt ;
				data = _ctxt[i_ctxt].to_bigint_vector();
			}

			o[i].from_int(data[i_slot], pubKey);
			++i_slot;
		}
	}

	template<class PubKey>
	void init_vector(const std::vector<cpp_int> &m, const PubKey *pubKey) {
		assert(pubKey->simd_factor() > 0);
		unsigned int simd = pubKey->simd_factor();

		std::vector<cpp_int> serialVec(simd);
		_ctxt.resize((m.size() + simd - 1) / simd);
		_size = m.size();

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int i = 0; i < m.size(); ++i) {
			serialVec[i_simd] = m[i];
			++i_simd;
			if (i_simd == simd) {
				i_simd = 0;
				_ctxt[i_ctxt].from_vector(serialVec, pubKey);
				++i_ctxt;
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialVec[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialVec, pubKey);
		}
	}

	template<class Plaintext, class PubKey, class PrivKey>
	void init_vector(const std::vector<Plaintext> &m, const PubKey *pubKey, const PrivKey *privKey) {
		assert(m.size() > 0);
		unsigned int simd = m[0].simd_factor();

		std::vector<cpp_int> serialVec(simd);
		_ctxt.resize((m.size() + simd - 1) / simd);
		_size = m.size();

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int i = 0; i < m.size(); ++i) {
			serialVec[i_simd] = m[i].to_bigint(privKey);
			++i_simd;
			if (i_simd == simd) {
				i_simd = 0;
				_ctxt[i_ctxt].from_vector(serialVec, pubKey);
				++i_ctxt;
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialVec[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialVec, pubKey);
		}
	}

	template<class PubKey>
	void init_from_int_vector(const std::vector<long> &m, const PubKey *pubKey) {
		assert(m.size() > 0);
		unsigned int simd = pubKey->simd_factor();

		std::vector<long> serialVec(simd);
		_ctxt.resize((m.size() + simd - 1) / simd);
		_size = m.size();

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int i = 0; i < m.size(); ++i) {
			serialVec[i_simd] = m[i];
			++i_simd;
			if (i_simd == simd) {
				i_simd = 0;
				_ctxt[i_ctxt].from_vector(serialVec, pubKey);
				++i_ctxt;
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialVec[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialVec, pubKey);
		}
	}


	PackedVector<Number> operator-(const PackedVector<Number> &m) {
		PackedVector<Number> ret(*this);
		ret -= m;
		return ret;
	}

	PackedVector<Number> operator+(const PackedVector<Number> &m) {
		PackedVector<Number> ret(*this);
		ret += m;
		return ret;
	}

	PackedVector<Number> operator*(const Number &m) {
		PackedVector<Number> ret(*this);
		ret *= m;
		return ret;
	}

	void operator+=(const PackedVector<Number> &m) {
		assert(_size == m._size);
		assert(_ctxt.size() == m._ctxt.size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] += m._ctxt[i];
		}
	}

	void operator-=(const PackedVector<Number> &m) {
		assert(_size == m._size);
		assert(_ctxt.size() == m._ctxt.size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] -= m._ctxt[i];
		}
	}

	template<class Number2>
	void operator+=(const PackedVector<Number2> &m) {
		assert(_size == m.size());
		assert(_ctxt.size() == m.ctxt().size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] += m.ctxt()[i];
		}
	}

	template<class Number2>
	void operator*=(const PackedVector<Number2> &m) {
		assert(_size == m.size());
		assert(_ctxt.size() == m.ctxt().size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] *= m.ctxt()[i];
		}
	}

	template<class In>
	void operator*=(const In &m) {
		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] *= m;
		}
	}

	void shiftRight(int k) {
		if (_ctxt.size() != 1)
			throw std::runtime_error("PackedVector::rightShift when vector has more than 1 ciphertext is not implemented yet");
		_ctxt[0].shiftRight(k);
	}

	void shiftLeft(int k) {
		if (_ctxt.size() != 1)
			throw std::runtime_error("PackedVector::rightShift when vector has more than 1 ciphertext is not implemented yet");
		_ctxt[0].shiftLeft(k);
	}

	void msb(PackedVector<Number> &o) const {
		o._ctxt(_ctxt.size());
		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			o[i] = _ctxt.msb();
		}
	}

	template<class PrivKey>
	std::string to_string(const PrivKey *privKey) const {
		std::stringstream ss;
		for (unsigned int i = 0; i < size(); ++i) {
			ss << to_bigint(i, privKey) << " ";
		}
		return ss.str();
	}

	void save(ostream &s) const {
		for (auto &c : _ctxt)	
			c.save(s);
	}

	template<class N>
	friend void add(PackedVector<N> &o, const PackedMatrix<N> &a, const PackedVector<N> &b);

	template<class Out, class In1, class In2>
	friend void sub(PackedVector<Out> &o, const PackedVector<In1> &a, const PackedVector<In2> &b);

	template<class Out, class In1, class In2>
	void mul(PackedVector<Out> &o, const PackedVector<In1> &a, const PackedVector<In2> &b);
};


// Add a M-type vector to a packed vector
template<class Number>
void add(PackedVector<Number> &o, const PackedMatrix<Number> &a, const PackedVector<Number> &b) {
	assert(b.size() == a.cols());
	assert(b.simd_factor() == a.simd_factor());

	o.resize(b.size(), b.simd_factor());
	for (unsigned int i = 0; i < o._ctxt.size(); ++i)
		o._ctxt[i] = a._ctxt[i] + b._ctxt[i];
}

template<class Out, class In1, class In2>
void sub(PackedVector<Out> &o, const PackedVector<In1> &a, const PackedVector<In2> &b) {
	assert(b.size() == a.size());
	assert(b.simd_factor() == a.simd_factor());

	o.resize(a.size(), a.simd_factor());
	for (unsigned int i = 0; i < o._ctxt.size(); ++i)
		o._ctxt[i] = a._ctxt[i] - b._ctxt[i];
}

// perform: o = a*b  where the multiplication is element-wise
template<class Out, class In1, class In2>
void mulConst(PackedVector<Out> &o, const PackedVector<In1> &a, const In2 &b) {
	o.resize(a.size(), a.simd_factor());
	for (unsigned int i = 0; i < o.ctxt_size(); ++i) {
		mul(o.ctxt(i), a.ctxt(i), b);
	}
}

// perform: o = a*b  where the multiplication is element-wise
template<class Out, class In1, class In2>
void mulConst(PackedVector<Out> &o, const In2 &b, const PackedVector<In1> &a) { mulConst(o, a, b); }


// perform: o = a*b  where the multiplication is element-wise
template<class Out, class In1, class In2>
void mul(PackedVector<Out> &o, const PackedVector<In1> &a, const PackedVector<In2> &b) {
	assert(a.size() == b.size());

	o.resize(b.size());
	for (unsigned int i = 0; i < o.ctxt_size(); ++i)
		o.ctxt(i) = a.ctxt(i) * b.ctxt(i);
}

// perform: o += a*b  where the multiplication is element-wise
template<class Out, class In1, class In2>
void add_mul(PackedVector<Out> &o, const PackedVector<In1> &a, const PackedVector<In2> &b) {
	assert(a.size() == b.size());
	assert(a.size() == o.size());

	for (unsigned int i = 0; i < o.ctxt_size(); ++i)
		add_mul(o.ctxt(i), a.ctxt(i), b.ctxt(i));
}

template<class Out, class In1, class In2, class OutPubKey, class MatrixPubKey>
void mul(PackedVector<Out> &out, Matrix<In1> &m, const std::vector< PackedVector<In2> > &v, const OutPubKey *outPubKey, const MatrixPubKey *matrixPubKey) {
	assert(m.cols() == m.rows());
	assert(m.cols() == v.size());

	size_t dim = m.rows();

	std::vector<long> zeroes(m.rows(), 0);
	out.init_from_int_vector(zeroes, outPubKey);

	for (size_t i = 0; i < dim; ++i) {
		std::vector<In1> rowv(dim);
		for (size_t i_row = 0; i_row < dim; ++i_row)
			rowv[i_row] = m(i, i_row);

		// TODO: Recently added matrixPubKey as the privKey with which rowv was encoded
		// this works because rowv and matrixPubKey are Plaintext
		PackedVector<In1> row(rowv, matrixPubKey, matrixPubKey);
		add_mul(out, row, v[i]);
	}
}

// template<class Number>
// std::ostream &operator<<(std::ostream &out, const PackedVector<Number> &m) {
// 	for (unsigned int i = 0; i < m.size(); ++i) {
// 		out << m.to_int(i) << " ";
// 	}
// 	return out;
// }

template<class Number>
std::ostream &operator<<(std::ostream &out, const std::vector< PackedVector<Number> > &m) {
	for (unsigned int i = 0; i < m.size(); ++i) {
		out << "(" << TO_STRING(m[i]) << ") ";
	}
	return out;
}


#endif
