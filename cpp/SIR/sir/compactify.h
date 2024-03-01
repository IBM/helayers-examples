#ifndef __COMPACTIFY_H__
#define __COMPACTIFY_H__

#include "matrix.h"
#include "packed_matrix.h"
#include "packed_vector.h"
#include "server1.h"
#include "server2.h"

#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryPlaintext typename Types::BinaryPlaintext
#define BinaryCiphertext typename Types::BinaryCiphertext

template<class Types>
void Server1<Types>::compactify() {
    std::vector<long> &F = _feature_list[ _feature_list.size() - 1 ];
    int newSize = 0;
    for (size_t i = 0; i < _smallestFeatureChi.size(); ++i) {
        F[i] = 1 - _smallestFeatureChi[i];
        newSize += F[i];
    }

#   ifdef __DEBUG
    {
    Matrix<Plaintext> ADec(_A.cols(), _A.cols());
	_A.mat()[0].to_matrix(ADec, _plainKey);

	std::vector<Plaintext> bDec;
	_b.to_vector(bDec, _plainKey);

    std::cout << "Compacting:" << std::endl;
    std::cout << "A = " << std::endl << ADec << std::endl;
    std::cout << "b = ";
    for (auto b : bDec)
        std::cout << PRINT(b) << " ";
    std::cout << std::endl;
    std::cout << "F = ";
    for (auto f : F)
        std::cout << f << " ";
    std::cout << std::endl;
    }
#   endif


    // Create the full random R,r
    Matrix<Plaintext> fullR(_A.cols(), _A.cols());
    std::vector<Plaintext> fullr(_A.cols());

    for (size_t col = 0; col < _A.cols(); ++col) {
        for (size_t row = 0; row < _A.cols(); ++row) {
            std::vector<long> rand(_pubKey->simd_factor());
            for (size_t i = 0; i < rand.size(); ++i)
                rand[i] = random();
            fullR(col, row).from_vector(rand, _plainKey);
        }
    }

    for (size_t col = 0; col < _A.cols(); ++col) {
        std::vector<long> rand(_pubKey->simd_factor());
        for (size_t i = 0; i < rand.size(); ++i)
            rand[i] = random();
        fullr[col].from_vector(rand, _plainKey);
    }


    // Project full R,r and save them for later
    size_t projectedCol;

    _compactifyProjectedR.resize(newSize, newSize);
    _compactifyProjectedr.resize(newSize);

    projectedCol = 0;
    for (size_t col = 0; col < _A.cols(); ++col) {
        if (F[col] == 0)
            continue;
        size_t projectedRow = 0;
        for (size_t row = 0; row < _A.rows(); ++row) {
            if (F[row] == 0)
                continue;
            _compactifyProjectedR(projectedCol, projectedRow) = fullR(col, row);
            ++projectedRow;
        }
        ++projectedCol;
    }

    projectedCol = 0;
    for (size_t col = 0; col < _A.cols(); ++col) {
        if (F[col] == 0)
            continue;
        _compactifyProjectedr[projectedCol] = fullr[col];
        ++projectedCol;
    }


    // mask _A and _b
	PackedMatrixSet<Ciphertext> R;
	PackedVector<Ciphertext> r;

    R.init_left_matrix(fullR, _pubKey);
    r.init_vector(fullr, _pubKey);

    R += _A;
    r += _b;

#   ifdef __DEBUG
    {
    Matrix<Plaintext> ADec(_A.cols(), _A.cols());
	R.mat()[0].to_matrix(ADec, &plainKey);
    std::cout << "masked A = " << std::endl << ADec << std::endl;
    }
#   endif

    _communication_channel->send_A_to_server2_to_compact(R, r, F);
}

template<class Types>
void Server2<Types>::receive_A_to_compact(const PackedMatrixSet<Ciphertext> &encAPrime, const PackedVector<Ciphertext> &encbPrime, const std::vector<long> &F) {
    int newSize = 0;
    for (size_t i = 0; i < F.size(); ++i) {
        newSize += F[i];
    }

	Matrix<Plaintext> APrimeRotated;
	Matrix<Plaintext> APrime;
	std::vector<Plaintext> bPrime;

	encAPrime.mat()[0].to_matrix(APrimeRotated, _pubKey);
	encbPrime.to_vector(bPrime, _pubKey);

    APrime.resize(APrimeRotated.cols(), APrimeRotated.rows());
    for (size_t col = 0; col < APrime.cols(); ++col) {
        for (size_t row = 0; row < APrime.rows(); ++row) {
            APrime(col, row) = APrimeRotated((col - row + APrime.cols()) % APrime.cols(), row);
        }
    }

	Matrix<Plaintext> projectedAPrime;
	std::vector<Plaintext> projectedbPrime;

    projectedAPrime.resize(newSize, newSize);

    size_t projectedCol = 0;
    for (size_t col = 0; col < F.size(); ++col) {
        if (F[col] == 0)
            continue;
        size_t projectedRow = 0;
        for (size_t row = 0; row < F.size(); ++row) {
            if (F[row] == 0)
                continue;
            projectedAPrime(projectedCol, projectedRow) = APrime(col, row);
            ++projectedRow;
        }
        ++projectedCol;
    }

    projectedbPrime.resize(newSize);
    projectedCol = 0;
    for (size_t col = 0; col < F.size(); ++col) {
        if (F[col] == 0)
            continue;
        projectedbPrime[projectedCol] = bPrime[col];
        ++projectedCol;
    }

	PackedMatrixSet<Ciphertext> A_simd;
	PackedVector<Ciphertext> b_simd;

	A_simd.init_left_matrix(projectedAPrime, _pubKey);
	b_simd.init_vector(projectedbPrime, _pubKey);

#   ifdef __DEBUG
    {
    Matrix<Plaintext> ADec(A_simd.cols(), A_simd.cols());
	A_simd.mat()[0].to_matrix(ADec, _pubKey);
    std::cout << "masked compacted A = " << std::endl << ADec << std::endl;
    }
#   endif

    _communication_channel->send_compacted_A_to_server1(A_simd, b_simd);
}

template<class Types>
void Server1<Types>::receive_compacted_A(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b) {
    _A = A;
    _b = b;

	PackedMatrixSet<Ciphertext> R;
	PackedVector<Ciphertext> r;

    R.init_left_matrix(_compactifyProjectedR, _pubKey);
    r.init_vector(_compactifyProjectedr, _pubKey);

    _A -= R;
    _b -= r;

    _feature_list.push_back( std::vector<long>(_A.cols(), 1) );

#   ifdef __DEBUG
    Matrix<Plaintext> ADec(_A.cols(), _A.cols());
	_A.mat()[0].to_matrix(ADec, _pubKey);

	std::vector<Plaintext> bDec;
	_b.to_vector(bDec, _pubKey);

    std::cout << "Compacted:" << std::endl;
    std::cout << "A = " << std::endl << ADec << std::endl;
    std::cout << "b = ";
    for (auto b : bDec)
        std::cout << PRINT(b) << " ";
    std::cout << std::endl;
#   endif

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