#ifndef ___SCALED_RIDGE___
#define ___SCALED_RIDGE___

#include <omp.h>

#include "times.h"
#include "rational_reconstruction.h"
#include "packed_matrix.h"
#include "packed_vector.h"

#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryPlaintext typename Types::BinaryPlaintext
#define BinaryCiphertext typename Types::BinaryCiphertext

int lambda = 1;

float divide(cpp_int N, cpp_int a, cpp_int b) {
	float ret = 0;
	int resolution = 1000;
	if (a > N/2) {
		a = a - N;
	}
	a *= resolution;
	cpp_int retint = a/b;
	ret = static_cast<float>(retint) / resolution;
	return ret;
}


template<class P>
void reduceMatrix(Matrix<P> &out, const Matrix<P> &in, const std::vector<long> &F) {
	size_t colProj = 0;
	for (size_t col = 0; col < in.cols(); ++col) {
		if (F[col] == 0)
			continue;
		size_t rowProj = 0;
		for (size_t row = 0; row < in.rows(); ++row) {
			if (F[row] == 0)
				continue;
			out(colProj, rowProj) = in(col, row);
			++rowProj;
		}
		++colProj;
	}
}

template<class P>
void reduceVector(std::vector<P> &out, const std::vector<P> &in, const std::vector<long> &F) {
	size_t colProj = 0;
	for (size_t col = 0; col < in.size(); ++col) {
		if (F[col] == 0)
			continue;
		out[colProj] = in[col];
		++colProj;
	}
}

template<class P, class K>
void liftMatrix(Matrix<P> &out, const Matrix<P> &in, const std::vector<long> &F, const K &key) {
	size_t col = 0;
	for (size_t colLifted = 0; colLifted < out.cols(); ++colLifted) {
		size_t row = 0;
		for (size_t rowLifted = 0; rowLifted < out.rows(); ++rowLifted) {
			if ((F[colLifted] != 0) && (F[rowLifted] != 0)) {
				out(colLifted, rowLifted) = in(col, row);
				++row;
			} else
				out(colLifted, rowLifted).from_int(0, key);
		}
		if (F[colLifted] != 0)
			++col;
	}
}

template<class P, class K>
void liftVector(std::vector<P> &out, const std::vector<P> &in, const std::vector<long> &F, const K &key) {
	size_t col = 0;
	for (size_t colLifted = 0; colLifted < out.size(); ++colLifted) {
		if (F[colLifted] != 0) {
			out[colLifted] = in[col];
			++col;
		} else
			out[colLifted].from_int(0, key);
	}
}

template<class Types>
Plaintext DataSource<Types>::encode_number(float f, int resolution) const {
	Plaintext n;

	for (int i = 0; i < resolution; ++i)
		f *= 10;
	n.from_int(int(f), _plainKey);
	return n;
}

template<class Types>
void DataSource<Types>::encode_data(bool isDesignated, int resolution) {

	Matrix<float> floatA(_X.cols(), _X.cols());
	std::vector<float> floatb(_X.cols());

	Times::start(Times::Phase1Step2);

	for (unsigned int col = 0; col < _X.cols(); ++col)
		for (unsigned int row = 0; row < _X.cols(); ++row)
			if ((row == col) && isDesignated)
				floatA(col, row) = lambda;
			else
				floatA(col, row) = 0;

	for (unsigned int col = 0; col < _X.cols(); ++col)
		floatb[col] = 0;

	for (unsigned int i = 0; i < _X.rows(); ++i) {
		for (unsigned int col = 0; col < _X.cols(); ++col)
			for (unsigned int row = 0; row < _X.cols(); ++row)
				floatA(col, row) += _X(col,i) * _X(row,i);

		for (unsigned int col = 0; col < _X.cols(); ++col)
			floatb[col] += _X(col,i) * _y[i];
	}

	cout << "done computing floatA and floatb" << endl;

	Matrix<Plaintext> tempA(_X.cols(), _X.cols());
	std::vector<Plaintext> tempb(_X.cols());

	for (unsigned int col = 0; col < _X.cols(); ++col)
		for (unsigned int row = 0; row < _X.cols(); ++row)
			tempA(col, row) = encode_number(floatA(col, row), resolution);

	for (unsigned int col = 0; col < _X.cols(); ++col)
		tempb[col] = encode_number(floatb[col], resolution);

	A_simd = new PackedMatrixSet<Ciphertext>;
	b_simd = new PackedVector<Ciphertext>;
	A_simd->init_left_matrix(tempA, _pubKey, _plainKey);
	b_simd->init_vector(tempb, _pubKey, _plainKey);

	Times::end(Times::Phase1Step2);
}

template<class Types>
void DataSource<Types>::send_A_and_b_to_server1() {
	_communication_channel->send_A_and_b_to_server1(*A_simd, *b_simd);
	delete A_simd;
	A_simd = NULL;
	delete b_simd;
	b_simd = NULL;
}

template<class Types>
void Server1<Types>::receive_fraction_of_A_and_b(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b) {
	Times::start(Times::Phase1Step3);
	if (_A.cols() == 0) {
		_A = A;
		_b = b;
	} else {
		_A += A;
		_b += b;
	}
	Times::end(Times::Phase1Step3);
}


template<class Types>
void Server1<Types>::scaled_ridge_mask_and_send_to_server2() {
//	std::cout << "server1 masking:  A = " << std::endl << _A << std::endl;
//	std::cout << "server1 masking: A_simd = " << std::endl << _A_simd << std::endl;

//	std::cout << "server1 masking:  b = " << _b << std::endl;
//	std::cout << "server1 masking: b_simd = " << _b_simd << std::endl;

	_R.resize(_A.cols(), _A.rows());
	_r.resize(_b.size());

	PackedMatrix<Ciphertext> Aprime;
	PackedVector<Ciphertext> bprime;

	draw(_R, _feature_list, _plainKey);
	draw(_r, _feature_list, _plainKey);

	PackedMatrixSet<Plaintext> R_simd;
	R_simd.init_right_matrix(_R, _plainKey, _plainKey);

	printTimeStamp("after initing R");

	PackedMatrixSet<Plaintext> r_simd;
	r_simd.init_right_vector(_r, _plainKey, _plainKey);

	printTimeStamp("after initing r");

	PackedMatrix<Ciphertext> Ar_simd;

	Times::start(Times::Phase2Step1);

	PackedMatrixSet<Plaintext> AReduceMask;
	PackedVector<Plaintext> bReduceMask;

	// delete the rows and cols that are not in the feature list
	{
		Matrix<Plaintext> tempAReduceMask(_A.cols(), _A.cols());
		std::vector<Plaintext> tempbReduceMask(_A.cols());

#		pragma omp parallel for collapse(2)
		for (unsigned int col = 0; col < _A.cols(); ++col)
			for (unsigned int row = 0; row < _A.cols(); ++row)
				if ((_feature_list[col] != 0) && (_feature_list[row] != 0))
					tempAReduceMask(col, row).from_int(1, _plainKey);
				else
					tempAReduceMask(col, row).from_int(0, _plainKey);

#		pragma omp parallel for
		for (unsigned int col = 0; col < _A.cols(); ++col)
			if (_feature_list[col] != 0)
				tempbReduceMask[col].from_int(1, _plainKey);
			else
				tempbReduceMask[col].from_int(0, _plainKey);

		AReduceMask.init_left_matrix(tempAReduceMask, _plainKey, _plainKey);
		bReduceMask.init_vector(tempbReduceMask, _plainKey, _plainKey);
	}
	printTimeStamp("after initing zero mask for A and b");

	PackedMatrixSet<Ciphertext> AReduced;
	PackedVector<Ciphertext> bReduced;

	AReduced = _A;
	AReduced *= AReduceMask;

	bReduced = _b;
	bReduced *= bReduceMask;

	printTimeStamp("after zero masking A and b");

	mul(Aprime, AReduced, R_simd, _pubKey);

	mul(Ar_simd, AReduced, r_simd, _pubKey);
	add(bprime, Ar_simd, bReduced);
	printTimeStamp("after masking A and b");

	Times::end(Times::Phase2Step1);

#	ifdef __DEBUG
	std::cout << "R = " << std::endl << _R << std::endl;
	std::cout << "r = " << std::endl;
	for (auto r : _r)
		std::cout << PRINT(r) << " ";
	std::cout << std::endl << std::endl;

	std::cout << "A' = AR = " << std::endl << TO_STRING(Aprime) << std::endl;
	std::cout << "Ar = " << std::endl << TO_STRING(Ar_simd) << std::endl;
	std::cout << "b' = bprime = " << std::endl << TO_STRING(bprime) << std::endl << std::endl;
#	endif

	_communication_channel->send_Aprime_and_bprime_to_server2(Aprime, bprime, _feature_list);
}

template<class Types>
void Server2<Types>::solve() {
	Matrix<Plaintext> AprimeLifted;
	std::vector<Plaintext> bprimeLifted;

	Times::start(Times::Phase2Step2a);
	_EncAprime.to_matrix(AprimeLifted, &plainKey, &privKey);
	_Encbprime.to_vector(bprimeLifted, &plainKey, &privKey);
	Times::end(Times::Phase2Step2a);

	printTimeStamp("after decrypting A' and b'");

	int projectedDim = 0;
	for (long i : _F) projectedDim += i;
	Matrix<Plaintext> Aprime(projectedDim, projectedDim);
	std::vector<Plaintext> bprime(projectedDim);

	reduceMatrix(Aprime, AprimeLifted, _F);
	reduceVector(bprime, bprimeLifted, _F);

	printTimeStamp("after reducing A' and b'");

#	ifdef __DEBUG
	std::cout << "A' = " << Aprime << std::endl;
	std::cout << "b' = ";
	for (auto b : bprime)
		std::cout << PRINT(b) << " ";
	std::cout << std::endl;
#	endif

	Times::start(Times::Phase2Step2b);

	// compute adjoint and determiniant
	Matrix<Plaintext> AprimeAdjReduced;
	Plaintext AprimeDet;
	Aprime.compute_adjugate_determinant(AprimeAdjReduced, AprimeDet, plainKey);

	std::vector<Plaintext> AprimeAdj_times_bprimeReduced;
	mul(AprimeAdj_times_bprimeReduced, AprimeAdjReduced, bprime, &plainKey);

	printTimeStamp("after computing Adj and det");

	// lift the reduced matrix and vector	
	Matrix<Plaintext> AprimeAdj(AprimeLifted.cols(), AprimeLifted.rows());
	std::vector<Plaintext> AprimeAdj_times_bprime(AprimeLifted.cols());
	liftMatrix(AprimeAdj, AprimeAdjReduced, _F, _plainKey);
	liftVector(AprimeAdj_times_bprime, AprimeAdj_times_bprimeReduced, _F, _plainKey);

	printTimeStamp("after lifting Adj and det");

	std::vector< PackedVector<Ciphertext> > AprimeAdj_times_bprime_Enc;
	Ciphertext AprimeDetEnc(*_pubKey, AprimeDet);

	size_t dim = bprimeLifted.size();
	AprimeAdj_times_bprime_Enc.resize(dim);
	for (size_t i = 0; i < dim; ++i) {
		std::vector<Plaintext> temp_v;
		temp_v.resize(dim);
		for (size_t j = 0; j < dim; ++j)
			temp_v[j] = AprimeAdj_times_bprime[i];
		AprimeAdj_times_bprime_Enc[i].init_vector(temp_v, _pubKey, _plainKey);
	}

	printTimeStamp("after encrypting masked model");

	Times::end(Times::Phase2Step2b);

#	ifdef __DEBUG
	std::cout << "Det(A') = " << PRINT(AprimeDet) << std::endl << std::endl;
	std::cout << "Adj(A') = " << AprimeAdj << std::endl;
	std::cout << "Adj(A') * b' = ";
	for (auto b : AprimeAdj_times_bprime)
		std::cout << PRINT(b) << " ";
	std::cout << std::endl << std::endl;
	std::cout << " Enc( Adj(A') * b' ) = ";
	for (auto b : AprimeAdj_times_bprime_Enc)
		std::cout << TO_STRING(b) << std::endl;
	std::cout << std::endl;
#	endif

	_communication_channel->send_Adj_Det_to_server1(AprimeAdj_times_bprime_Enc, AprimeDetEnc);
}

template<class Types>
void Server1<Types>::scaled_ridge_unmask() {
	Times::start(Times::Phase2Step3);

	int reducedDim = feature_number();
	Matrix<Plaintext> RReduced(reducedDim, reducedDim);
	Matrix<Plaintext> adj;
	reduceMatrix(RReduced, _R, _feature_list);
	Plaintext Rdet;
	RReduced.compute_adjugate_determinant(adj, Rdet, plainKey);

	PackedVector<Ciphertext> scaledW;
	// mul(scaledW, _R, _scaledWMasked, _pubKey);
	mul(scaledW, _R, _scaledWMasked, _pubKey, &plainKey);

	printTimeStamp("after computing scaledW");

	PackedVector<Plaintext> r(_r, _plainKey, _plainKey);
	PackedVector<Ciphertext> r_det;
	r_det.pubKey(_pubKey);
	mulConst(r_det, r, _det);

	sub(_z, scaledW, r_det);

	Plaintext invRdet = inverse(Rdet);

#	ifdef __DEBUG
	std::cout << "scaledWMasked = Adj(A')b' = " << _scaledWMasked << std::endl;
	std::cout << "scaledW = R (Adj(A')b') = " << TO_STRING(scaledW) << std::endl;  // this is wrong

	std::cout << "Det(R) = " << PRINT(Rdet) << std::endl;
	std::cout << "inverse of Det(R) = " << PRINT(inverse(Rdet)) << std::endl;
	std::cout << "r * det(A') = " << TO_STRING(r_det) << std::endl;

	std::cout << "R (Adj(A')b') - r det(A') = " << TO_STRING(_z) << std::endl;

	std::cout << "Det(R) = " << PRINT(Rdet) << endl;
	std::cout << "Det(R)^{-1} = " << PRINT(invRdet) << endl;
#	endif

	_z *=  invRdet;
	_det *= invRdet;

	printTimeStamp("after computing z");

#ifdef __DEBUG
	std::cout << "scaled-z = (R (Adj(A')b') - r det(A')) Det(R)^{-1} = " << TO_STRING(_z) << std::endl;
#endif

#if defined(__DEBUG) || defined(__DEBUGW)
	std::vector<cpp_int> zint;
	_z.to_bigint_vector(zint, &privKey);

	cpp_int detint;
	detint = _det.to_bigint(&privKey);

	cout << "w = ";
	for (unsigned int i = 0; i < zint.size(); ++i) {
		if (i > 0)
			cout << ", ";
		cout << divide(plainKey.get_ring_size() , zint[i], detint);
	}
	cout << endl;
#endif

	Times::end(Times::Phase2Step3);
}


//template<class Plaintext, class Ciphertext>
//void DataSource<Plaintext, Ciphertext>::decrypt(std::vector<Plaintext> &w) {
//	w.resize(_Encw.size());
//
//	Times::start(Times::phase2Step3);
//	for (unsigned int i = 0; i < w.size(); ++i) {
//		w[i] = _Encw[i].to_int();
//	}
//	Times::end(Times::Phase2Step3);
//}





template<class Types>
void Server1<Types>::scaled_ridge() {
	// Debug
#ifdef __DEBUG
	std::cout << "Performing scaled ridge" << std::endl;
	std::cout << "A = " << std::endl << TO_STRING(_A) << std::endl;
	// vector<cpp_int> bDec;
	// _b.to_bigint_vector(bDec, _plainKey);
	// std::cout << "b = " << std::endl;
	// for (auto bb : bDec) 
	// 	std::cout << bb << " ";

	std::cout << "b = " << TO_STRING(_b) << std::endl << std::endl;
#endif

	scaled_ridge_mask_and_send_to_server2();
	_communication_channel->server2_solve();
	scaled_ridge_unmask();

#ifdef __DEBUG
	std::cout << "After performing scaled ridge" << std::endl;
	std::cout << "z = A^{-1} * b * det(A) = " << TO_STRING(_z) << std::endl;
#endif

	// Times::print(std::cout);
}



// Code taken from https://stackoverflow.com/questions/30495102/iterate-through-different-subset-of-size-k/30518940#30518940
template<typename BidiIter, typename CBidiIter, typename Compare = std::less<typename BidiIter::value_type> >
int next_comb(BidiIter first, BidiIter last, CBidiIter /* first_value */, CBidiIter last_value, Compare comp = Compare()) {
	// 1. Find the rightmost value which could be advanced, if any
	auto p = last;
	while (p != first && !comp(*(p - 1), *--last_value))
		--p;
	if (p == first)
		return false;

	// 2. Find the smallest value which is greater than the selected value
	for (--p; comp(*p, *(last_value - 1)); --last_value) {
	}

	// 3. Overwrite the suffix of the subset with the lexicographically smallest sequence starting with the new value
	while (p != last)
		*p++ = *last_value++;
	return true;
}


template<class Types>
void Server1<Types>::exhaustive_sparse_linear_regression() {
//	int k = 10;
//
//	std::vector<int> values(X.size() + 1);
//	for (unsigned int i = 0; i < X.size(); ++i)
//		values[i] = i;
//
//		// Since that's sorted, the first subset is just the first k values */
//	std::vector<int> subset(values.cbegin(), values.cbegin() + k);
//
//		// Print each combination
//	do {
//		for (auto const& v : subset) std::cout << v << ' ';
//		std::cout << '\n';
//	} while (next_comb(subset.begin(),  subset.end(), values.cbegin(), values.cend()));
//
//
//
//
//
//	std::vector<Ciphertext> Encw;
//
//	mask(_X, _y);
//	_communication_channel->server2_solve();
//	unmask(_X, _y, Encw);
//
//	_communication_channel->send_w_to_DataSource(Encw);
}



#undef PubKey
#undef PlainKey
#undef PubBinaryKey
#undef PriveBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryPlaintext
#undef BinaryCiphertext

#endif
