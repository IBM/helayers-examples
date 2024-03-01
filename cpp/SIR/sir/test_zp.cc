#include <omp.h>
#include <vector>
#include <istream>
#include <fstream>
#include <algorithm>
#include <boost/tokenizer.hpp>

#include <liphe/zp.h>
#include <liphe/unsigned_word.h>
#include <liphe/packed_bit_array.h>

#include "matrix.h"
#include "vector.h"
#include "packed_matrix.h"
#include "packed_vector.h"
#include "crt2.h"
#include "csv.h"

class ZpTypes {
public:
	typedef CrtBundle<ZPKeys, ZP> Ciphertext;
	typedef CrtBundle<ZPKeys, ZP> Plaintext;

	typedef CrtContainer<ZPKeys, ZP> PrivKey;
	typedef CrtContainer<ZPKeys, ZP> PubKey;
	typedef CrtContainer<ZPKeys, ZP> PlainKey;

	typedef ZP BinaryBitCiphertext;
	typedef ZP BinaryBitPlaintext;

	typedef UnsignedWord<BinaryBitCiphertext> BinaryWordCiphertext;
	typedef UnsignedWord<BinaryBitPlaintext> BinaryWordPlaintext;

	typedef ZPKeys PrivBinaryKey;
	typedef ZPKeys PubBinaryKey;
	typedef ZPKeys PlainBinaryKey;

	typedef PackedBitArray<BinaryBitCiphertext, PubBinaryKey> PackedBitUnsignedWord;
};



ZpTypes::PrivKey privKey;
ZpTypes::PubKey &pubKey = privKey;
ZpTypes::PlainKey &plainKey = pubKey;


ZPKeys bitKeys(17, 2, 7000);
ZpTypes::PrivBinaryKey privBinaryKey = bitKeys;
ZpTypes::PubBinaryKey &pubBinaryKey = privBinaryKey;
ZpTypes::PlainBinaryKey &plainBinaryKey = privBinaryKey;

const ZpTypes::PubKey *pubKeyOfOperation(const ZpTypes::PubKey *k1, const ZpTypes::PubKey *k2) {
	assert(k1 == k2);
	return k1;
}


void convert(CrtDigit<ZPKeys, ZP> &out, const CrtDigit<ZPKeys, ZP> &in, int i, const CrtContainer<ZPKeys, ZP> *pubKey) {
	out = CrtDigit<ZPKeys,ZP>(pubKey->key(i)->from_int( in.to_int() ), pubKey->key(i), pubKey);
}

cpp_int PRINT(const ZpTypes::Plaintext &p) {
	return p.to_bigint(&privKey);
}

std::string TO_STRING(const ZpTypes::Plaintext &p) {
	stringstream ss;
	ss << PRINT(p);
	return ss.str();
}

std::string TO_STRING(const PackedVector<ZpTypes::Ciphertext> &v) {
	return v.to_string(&privKey);
}

std::string TO_STRING(const PackedMatrix<ZpTypes::Ciphertext> &v) {
	return v.to_string(&privKey);
}

std::string TO_STRING(const PackedMatrixSet<ZpTypes::Ciphertext> &v) {
	return v.get_matrix_as_str(&privKey);
}

std::string TO_STRING(const ZpTypes::PackedBitUnsignedWord &v) {
	stringstream ss;
	ss << v.toCppInt(privBinaryKey);
	return ss.str();
}

std::string TO_STRING(const ZpTypes::BinaryBitCiphertext &v) {
	stringstream ss;
	ss << v.to_int();
	return ss.str();
}

std::string TO_STRING(const Matrix<ZpTypes::Plaintext> &m) {
	stringstream ss;
	ss << "Matrix([";
	for (size_t i = 0; i < m.rows(); ++i) {
		if (i != 0)
			ss << ",";
		ss << "[";
		for (size_t j = 0; j < m.rows(); ++j) {
			if (j != 0)
				ss << ",";
			ss << TO_STRING(m(i,j));
		}
		ss << "]";
	}
	ss << "])";

	return ss.str();
}


#include "data_source.h"
#include "server1.h"
#include "server2.h"

#include "communication.h"

#include "sFir.h"

std::ostream &operator<<(std::ostream &out, std::vector<ZpTypes::Ciphertext> &v) {
	for (unsigned int i = 0; i < v.size(); ++i) {
		out << v[i].to_bigint(&privKey) << " ";
	}
	return out;
}




// #define PubKey typename Types::PubKey
// #define PlainKey typename Types::PlainKey
// #define BinaryPubKey typename Types::BinaryPubKey
// #define BinaryPlainKey typename Types::BinaryPlainKey

// #define Plaintext typename Types::Plaintext
// #define Ciphertext typename Types::Ciphertext
// #define BinaryPlaintext typename Types::BinaryPlaintext
// #define BinaryCiphertext typename Types::BinaryCiphertext



int myrand(int min, int max) {
	return min + (random() % (max - min));
}


int main(int argc, char **argv) {
	//omp_set_max_active_levels(4);

	unsigned long p = 271;
	int numberOfPrimes = 1;
	std::string in("");
	int lines = 20;
	int dim = 4;
	int s = 2;
	int resolution = 0;
	int maxDim = -1;
	int iter = -1;

	Communication<ZpTypes>::setDump(false);
	for (int argc_i = 0; argc_i < argc; ++argc_i) {
		if (memcmp(argv[argc_i], "--s=", 4) == 0)
			s = atoi(argv[argc_i] + 4);
		if (memcmp(argv[argc_i], "--r=", 4) == 0)
			resolution = atol(argv[argc_i] + 4);
		if (memcmp(argv[argc_i], "--p=", 4) == 0)
			p = atol(argv[argc_i] + 4);
		if (memcmp(argv[argc_i], "--primeNumber=", 4) == 0)
			numberOfPrimes = atoi(argv[argc_i] + 14);
		if (memcmp(argv[argc_i], "--in=", 5) == 0)
			in = std::string(argv[argc_i] + 5);
		if (memcmp(argv[argc_i], "--d=", 4) == 0)
			dim = atoi(argv[argc_i] + 4);
		if (memcmp(argv[argc_i], "--maxd=", 7) == 0)
			maxDim = atoi(argv[argc_i] + 7);
		if (memcmp(argv[argc_i], "--iter=", 7) == 0)
			iter = atoi(argv[argc_i] + 7);
		if (memcmp(argv[argc_i], "--dump", 6) == 0)
			Communication<ZpTypes>::setDump();
		if (memcmp(argv[argc_i], "--l=", 4) == 0)
			lines = atoi(argv[argc_i] + 4);

		if (strcmp(argv[argc_i], "--help") == 0) {
			std::cout << "   --p=101 first prime" << std::endl;
			std::cout << "   --n=1 number of primes for CRT" << std::endl;
			std::cout << "   --in= input file (blank means random)" << std::endl;
			std::cout << "   --d=4 dimension of model. If input is specified, d cannot be bigger than dimension of input" << std::endl;
			std::cout << "   --maxd=-1 when reading a csv specify the max number of features" << std::endl;
			std::cout << "   --iter=-1 max number of iterations to run" << std::endl;
			std::cout << "   --l=4 number of lines. If input is specified, l cannot be bigger than lines of input" << std::endl;
			std::cout << "   --s=4 sparsity. How how many features should the model consider" << std::endl;
			std::cout << "   --r=4 resolution. How how many decimal digits to take from the input" << std::endl;
			std::cout << "   --dump dump the communication into files" << std::endl;
			exit(1);
		}
	}

	unsigned long prime;
	// if (p < 100000)
		prime = Primes::find_prime_bigger_than(p-1);
	// else
	// 	prime = p;
	for (int i_prime = 0; i_prime < numberOfPrimes; ++i_prime) {
		cout << "Adding a key with ring size = " << prime << endl;
		ZPKeys *zpKeys = new ZPKeys(prime, 1, 4000);

		pubKey.add_key(zpKeys);

		prime = Primes::find_prime_bigger_than(prime+1);
	}

	pubKey.init_inversion_keys();


	// test CRT
	{
		// CrtBundle<ZPKeys, ZP> m(pubKey, 2);
		// for (int i = 0; i < 20; ++i) {
		// 	m += m;
		// 	cpp_int d = m.to_bigint(&privKey);
		// 	cout << "2^" << (i+2) << " = " << d << endl;
		// }
		// cpp_int x = 2;
		// cpp_int y = 3;
		// int res = 2;
		// for (int i = 0; i < 100; ++i) {
		// 	x = 2*x;
		// 	res = (res * 2) % 3;
		// }
		// cout << "x mod 3 = " << (x % y) << endl;
		// cout << "res mod 3 = " << res << endl;
		// exit(1);
	}

	// test compute adjugate+determinant
	// {
	// 	Matrix<ZpTypes::Plaintext> A(3,3);

	// 	int _A[3][3] = {{1,2,3}, {5,6,7}, {10,2,34}};

	// 	// test A=I
	// 	for (int i = 0; i < 3; ++i) {
	// 		for (int j = 0; j < 3; ++j) {
	// 			A(i,j) = ZpTypes::Plaintext(plainKey, _A[i][j]);
	// 			// if (i != j)
	// 			// 	A(i,j) = ZpTypes::Plaintext(plainKey, 0);
	// 			// else if (i == 0)
	// 			// 	A(i,j) = ZpTypes::Plaintext(plainKey, 2);
	// 			// else
	// 			// 	A(i,j) = ZpTypes::Plaintext(plainKey, 1);
	// 		}
	// 	}

	// 	Matrix<ZpTypes::Plaintext> adj;
	// 	ZpTypes::Plaintext det;
	// 	A.compute_adjugate_determinant(adj, det, plainKey);

	// 	cout << "A = " << TO_STRING(A) << endl;
	// 	cout << "Adj(A) = " << TO_STRING(adj) << endl;
	// 	cout << "det(A) = " << TO_STRING(det) << endl;

	// 	exit(0);
	// }

	Matrix<float> X;
	std::vector<float> y;

	if (in != std::string("")) {
		read_csv_file(in, X, y, maxDim);
	} else {
		std::vector<float> model(dim);
		std::cout << "The real model is:";
		for (int i_col = 0; i_col < dim; ++i_col) {
			model[i_col] = myrand(0,5);
			// model[i_col] = (myrand(0,5) + 1) * (((i_col % 2) == 0) ? 1 : -1);
			std::cout << " " << model[i_col];
		}
		std::cout << std::endl;

		X.resize(dim, lines);
		for (unsigned int i = 0; i < X.cols(); ++i) {
			for (unsigned int j = 0; j < X.rows(); ++j) {
				X(i,j) = myrand(0,10);
			}
		}

		y.resize(lines);
		for (unsigned int j = 0; j < X.rows(); ++j) {
			y[j] = 0;
			for (unsigned int i = 0; i < X.cols(); ++i) {
				y[j] += X(i,j) * model[i];
			}
		}
	}

	std::vector<float> linRegModelFloat = sFIR<ZpTypes>(X, y, s, resolution, plainKey, pubKey, iter);

	std::cout << "computed model = ";
	for (unsigned int i = 0; i < linRegModelFloat.size(); ++i) {
		std::cout << ((!i)?"":", ");
		std::cout << linRegModelFloat[i];
	}
	std::cout << std::endl;

	Times::print(std::cout);

	return 0;
}

