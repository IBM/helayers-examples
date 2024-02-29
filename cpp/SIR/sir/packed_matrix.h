#ifndef ___PACKED_MATRIX___
#define ___PACKED_MATRIX___

#include <sstream>

template<class Number>
class PackedVector;

template<class Number>
class PackedMatrix {
private:
	std::vector<Number> _ctxt;
	
	enum MatrixType { NONE, RIGHT, LEFT };
	MatrixType _matrixType;

	unsigned int _cols, _rows;
public:
	PackedMatrix() : _matrixType(NONE), _cols(0), _rows(0) {}

	PackedMatrix<Number> &operator=(const PackedMatrix<Number> &m) {
		_matrixType = m._matrixType;
		_cols = m._cols;
		_rows = m._rows;

		_ctxt.resize(m._ctxt.size());
		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] = m._ctxt[i];
		}
		return *this;
	}

	unsigned int cols() const { return _cols; }
	unsigned int rows() const { return _rows; }
	unsigned int simd_factor() const { return _ctxt[0].simd_factor(); }
	MatrixType matrixType() const { return _matrixType; }
	const std::vector<Number> &ctxt() const { return _ctxt; }

	// long to_int(int col, int row) const {
	// 	int simd_factor = _ctxt[0].simd_factor();
	// 	int i_item = col * _rows + row;
	// 	int i_ctxt = i_item / simd_factor;
	// 	std::vector<long> a = _ctxt[i_ctxt].to_vector();
	// 	return a[i_item - i_ctxt * simd_factor];
	// }

	template<class PrivKey>
	cpp_int to_bigint(int col, int row, const PrivKey *privKey) const {
		int simd_factor = _ctxt[0].simd_factor();
		int i_item = col * _rows + row;
		int i_ctxt = i_item / simd_factor;
		std::vector<cpp_int> a = _ctxt[i_ctxt].to_bigint_vector(privKey);
		return a[i_item - i_ctxt * simd_factor];
	}

	///@brief Convert this matrix.
	///@param o The matrix to convert to.
	///@param pubKey The public key to encode/encrypt the output matrix with.
	///@param privKey The private key to decrypt/decode this matrix.
	template<class Out, class PubKey, class PrivKey>
	void to_matrix(Matrix<Out> &o, const PubKey *pubKey, const PrivKey *privKey) const {
		o.resize(_cols, _rows);
		int simd_factor = _ctxt[0].simd_factor();
		int i_ctxt = -1;
		int i_slot = simd_factor;
		std::vector<cpp_int> data;

		for (unsigned int i_col = 0; i_col < _cols; ++i_col) {
			for (unsigned int i_row = 0; i_row < _rows; ++i_row) {
				if (i_slot == simd_factor) {
					i_slot = 0;
					++i_ctxt ;
					data = _ctxt[i_ctxt].to_bigint_vector(privKey);
				}

				o(i_col, i_row).from_bigint(data[i_slot], pubKey);
				++i_slot;
			}
		}
	}

	template<class PubKey>
	void init_matrix(int cols, int rows, unsigned int simd_factor, const PubKey *pubKey) {
		_cols = cols;
		_rows = rows;

//		unsigned int simd_factor = Number::static_simd_factor();
		_ctxt.resize((cols * rows + simd_factor - 1) / simd_factor);

		std::vector<long> zero(simd_factor);
		for (unsigned int i = 0; i < simd_factor; ++i)
			zero[i] = 0;

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i].from_vector(zero, pubKey);
		}
	}

	///@brief Initialize from a matrix m.
	///@param m The matrix to encode.
	///@param rot The rotation index.
	///@param pubKey The key with which to encrypt/encode this.
	///@param privKey The key with which to decrypt/decode m.
	template<class In, class PubKey, class PrivKey>
	void init_left_matrix(const Matrix<In> &m, int rot, const PubKey *pubKey, const PrivKey *privKey) {
		_cols = m.cols();
		_rows = m.rows();
		_matrixType = LEFT;

		unsigned int simd = m(0, 0).simd_factor();

		std::vector<cpp_int> serialMat(simd);
		_ctxt.resize((m.rows()*m.cols() + simd - 1) / simd);

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int i_col = 0; i_col < m.cols(); ++i_col) {
			for (unsigned int row = 0; row < m.rows(); ++row) {
				unsigned int col = (i_col + row + rot) % m.cols();
				serialMat[i_simd] = m(col, row).to_bigint(privKey);
				++i_simd;
				if (i_simd == simd) {
					i_simd = 0;
					_ctxt[i_ctxt].from_vector(serialMat, pubKey);
					++i_ctxt;
				}
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialMat[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialMat, pubKey);
//			std::cout << "after from_vector: " << _ctxt[i_ctxt] << std::endl;
		}
	}

	///@brief Initialize from a matrix m.
	///@param m The matrix to encode.
	///@param rot The rotation index.
	///@param pubKey The key with which to encrypt/encode this.
	///@param privKey The key with which to decrypt/decode m.
	template<class In, class PubKey, class PrivKey>
	void init_right_matrix(const Matrix<In> &m, int rot, const PubKey *pubKey, const PrivKey *privKey) {
		_cols = m.cols();
		_rows = m.rows();
		_matrixType = RIGHT;

		unsigned int simd = pubKey->simd_factor();

		std::vector<cpp_int> serialMat(simd);
		_ctxt.resize((m.rows()*m.cols() + simd - 1) / simd);

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int col = 0; col < m.cols(); ++col) {
			for (unsigned int i_row = 0; i_row < m.rows(); ++i_row) {
				unsigned int row = (i_row + col + rot) % m.rows();

				serialMat[i_simd] = m(col, row).to_bigint(privKey);
				++i_simd;
				if (i_simd == simd) {
					i_simd = 0;
					_ctxt[i_ctxt].from_vector(serialMat, pubKey);
					++i_ctxt;
				}
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialMat[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialMat, pubKey);
		}
	}

	///@brief Initialize from a matrix m.
	///@param m The matrix to encode.
	///@param pubKey The key with which to encrypt/encode this.
	///@param privKey The key with which to decrypt/decode m.
	template<class In, class PubKey, class PrivKey>
	void init_matrix(const Matrix<In> &m, const PubKey *pubKey, const PrivKey *privKey) {
		_cols = m.cols();
		_rows = m.rows();
		_matrixType = NONE;

		unsigned int simd = pubKey->simd_factor();

		std::vector<cpp_int> serialMat(simd);
		_ctxt.resize((m.rows()*m.cols() + simd - 1) / simd);

		unsigned int i_simd = 0;
		unsigned int i_ctxt = 0;
		for (unsigned int col = 0; col < m.cols(); ++col) {
			for (unsigned int row = 0; row < m.rows(); ++row) {
				serialMat[i_simd] = m(col, row).to_bigint(privKey);
				++i_simd;
				if (i_simd == simd) {
					i_simd = 0;
					_ctxt[i_ctxt].from_vector(serialMat, pubKey);
					++i_ctxt;
				}
			}
		}

		if (i_simd != 0) {
			while (i_simd < simd) {
				serialMat[i_simd] = 0;
				++i_simd;
			}
			_ctxt[i_ctxt].from_vector(serialMat, pubKey);
		}
	}

	void operator+=(const PackedMatrix<Number> &m) {
		assert(_matrixType == m._matrixType);
		assert(_cols == m._cols);
		assert(_rows == m._rows);
		assert(_ctxt.size() == m._ctxt.size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] += m._ctxt[i];
		}
	}

	void operator-=(const PackedMatrix<Number> &m) {
		assert(_matrixType == m._matrixType);
		assert(_cols == m._cols);
		assert(_rows == m._rows);
		assert(_ctxt.size() == m._ctxt.size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] -= m._ctxt[i];
		}
	}

	template<class Number2>
	void operator*=(const PackedMatrix<Number2> &m) {
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wenum-compare"
		assert(_matrixType == m.matrixType());
#		pragma GCC diagnostic pop
		assert(_cols == m.cols());
		assert(_rows == m.rows());
		assert(_ctxt.size() == m.ctxt().size());

		for (unsigned int i = 0; i < _ctxt.size(); ++i) {
			_ctxt[i] *= m.ctxt()[i];
		}
	}

	template<class PrivKey>
	std::string to_string(const PrivKey *privKey) const {
		std::stringstream ss;
		for (unsigned int row = 0; row < rows(); ++row) {
			for (unsigned int col = 0; col < cols(); ++col) {
				ss << to_bigint(col, row, privKey) << " ";
			}
			ss << std::endl;
		}
		return ss.str();
	}

	void save(ostream &s) const {
		for (auto &c : _ctxt)
			c.save(s);
	}

	template<class Out, class In1, class In2>
	friend void add_mul(PackedMatrix<Out> &out, const PackedMatrix<In1> &left, const PackedMatrix<In2> &right);

	template<class N>
	friend void add(PackedVector<N> &o, const PackedMatrix<N> &a, const PackedVector<N> &b);
};

template<class Number> 
class PackedMatrixSet {
private:
	std::vector<PackedMatrix<Number> > _mat;

	enum MatrixType { NONE, RIGHT, LEFT };
	MatrixType _matrixType;

	unsigned int _cols, _rows;
public:
	PackedMatrixSet() : _matrixType(NONE), _cols(0), _rows(0) {}

	size_t cols() const { return _cols; }
	size_t rows() const { return _rows; }
	MatrixType matrixType() const { return _matrixType; }
	const std::vector<PackedMatrix<Number>> &mat() const { return _mat; }
	unsigned int simd_factor() const { return _mat[0].simd_factor(); }

	template<class PrivKey>
	std::string get_matrix_as_str(const PrivKey *privKey) const {
		std::stringstream s;

		if (_matrixType == LEFT) {
			for (size_t i_row = 0; i_row < _rows; ++i_row) {
				for (size_t i_col = 0; i_col < _cols; ++i_col) {
					s << _mat[0].to_bigint((i_col - i_row) % _cols , i_row, privKey) << "  ";
				}
				s << std::endl;
			}
		} else {
			s << "printing this matrix type is not supported" << std::endl;
		}

		return s.str();
	}

	PackedMatrixSet<Number> &operator=(const PackedMatrixSet<Number> &m) {
		_matrixType = m._matrixType;
		_cols = m._cols;
		_rows = m._rows;

		_mat.resize(m._mat.size());
		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i] = m._mat[i];
		}
		return *this;
	}

	///@brief Initialize from a matrix m.
	///@param m The matrix to encode.
	///@param pubKey The key with which to encrypt/encode this.
	///@param privKey The key with which to decrypt/decode m.
	template<class In, class PubKey, class PrivKey>
	void init_left_matrix(const Matrix<In> &m, const PubKey *pubKey, const PrivKey *privKey) {
		_cols = m.cols();
		_rows = m.rows();
		_matrixType = LEFT;

		_mat.resize(_cols);
#pragma omp parallel for
		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i].init_left_matrix(m, i, pubKey, privKey);
		}

//		std::cout << "Packed left matrix:" << std::endl;
//		for (unsigned int i = 0; i < m.cols(); ++i) {
//			std::cout << "   Rotation " << i << ": " << std::endl << _mat[i] << std::endl;
//		}
	}

	///@brief Initialize from a matrix m.
	///@param m The matrix to encode.
	///@param pubKey The key with which to encrypt/encode this.
	///@param privKey The key with which to decrypt/decode m.
	template<class In, class PubKey, class PrivKey>
	void init_right_matrix(const Matrix<In> &m, const PubKey *pubKey, const PrivKey *privKey) {
		_cols = m.cols();
		_rows = m.rows();
		_matrixType = RIGHT;

		_mat.resize(_rows);
#pragma omp parallel for
		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i].init_right_matrix(m, i, pubKey, privKey);
		}

//		std::cout << "Packed right matrix:" << std::endl;
//		for (unsigned int i = 0; i < m.cols(); ++i) {
//			std::cout << "   Rotation " << i << ":" << std::endl << _mat[i] << std::endl;
//		}
	}

	template<class In, class PubKey, class PrivKey>
	void init_right_vector(const std::vector<In> &v, const PubKey *pubKey, const PrivKey *privKey) {
		Matrix<In> m(v.size(), v.size());

#pragma omp parallel for
		for (unsigned row = 0; row < v.size(); ++row) {
			m(0, row) = v[row];
#pragma omp parallel for
			for (unsigned col = 1; col < v.size(); ++col) {
				m(col, row).from_int(0, pubKey);
			}
		}

		init_right_matrix(m, pubKey, privKey);
	}

	template<class In>
	void operator*=(const PackedMatrixSet<In> &m) {
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wenum-compare"
		assert(_matrixType == m.matrixType());
#		pragma GCC diagnostic pop
		assert(_cols == m.cols());
		assert(_rows == m.rows());
		assert(_mat.size() == m.mat().size());

		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i] *= m.mat()[i];
		}
	}

	void operator+=(const PackedMatrixSet<Number> &m) {
		assert(_matrixType == m._matrixType);
		assert(_cols == m._cols);
		assert(_rows == m._rows);
		assert(_mat.size() == m._mat.size());

		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i] += m._mat[i];
		}
	}

	void operator-=(const PackedMatrixSet<Number> &m) {
		assert(_matrixType == m._matrixType);
		assert(_cols == m._cols);
		assert(_rows == m._rows);
		assert(_mat.size() == m._mat.size());

		for (unsigned int i = 0; i < _mat.size(); ++i) {
			_mat[i] -= m._mat[i];
		}
	}

	void save(ostream &s) const {
		for (auto &m : _mat)
			m.save(s);
	}

	template<class Out, class In1, class In2, class PubKey>
	friend void mul(PackedMatrix<Out> &out, const PackedMatrixSet<In1> &left, const PackedMatrixSet<In2> &right, const PubKey *pubKey);
};

template<class Out, class In1, class In2>
void add_mul(PackedMatrix<Out> &out, const PackedMatrix<In1> &left, const PackedMatrix<In2> &right) {
	assert (out.cols() == left.cols());
	assert (out.cols() == right.cols());
	assert(left._ctxt.size() == right._ctxt.size());

	for (unsigned int i = 0; i < left._ctxt.size(); ++i) {
//		Out temp = left._ctxt[i] * right._ctxt[i];
//		std::cout << " mul = " << temp << std::endl;
		add_mul(out._ctxt[i], left._ctxt[i], right._ctxt[i]);
//		std::cout << " after adding = " << out._ctxt[i] << std::endl;
	}

//	std::cout << " after adding = " << std::endl << out << std::endl;
}

template<class Out, class In1, class In2, class PubKey>
void mul(PackedMatrix<Out> &out, const PackedMatrixSet<In1> &left, const PackedMatrixSet<In2> &right, const PubKey *pubKey) {
	assert(left.cols() == right.rows());
	assert(left._mat.size() == right._mat.size());
	assert(left.simd_factor() == right.simd_factor());

	out.init_matrix(left.cols(), right.rows(), left.simd_factor(), pubKey);

	for (unsigned int i = 0; i < right._mat.size(); ++i) {
//		std::cout << "   Adding  rotation " << i << std::endl;
		add_mul(out, left._mat[i], right._mat[i]);
	}
}

// template<class Number>
// std::ostream &operator<<(std::ostream &out, const PackedMatrix<Number> &m) {
// 	for (unsigned int row = 0; row < m.rows(); ++row) {
// 		for (unsigned int col = 0; col < m.cols(); ++col) {
// 			out << m.to_bigint(col, row) << " ";
// 		}
// 		out << std::endl;
// 	}
// 	return out;
// }

template<class Number>
std::ostream &operator<<(std::ostream &out, const PackedMatrixSet<Number> &m) {
	for (unsigned int i = 0; i < m.mat().size(); ++i) {
		out << "  rotation " << i << std::endl;
		out << m.mat()[i];
		out << std::endl;
	}
	return out;
}


#endif
