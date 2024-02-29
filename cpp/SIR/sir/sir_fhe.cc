#include <omp.h>
#include <sys/file.h>
#include <vector>
#include <istream>
#include <fstream>
#include <sstream>
#include <boost/tokenizer.hpp>

#include <liphe/helib2_number.h>
#include <liphe/helib2_keys.h>
#include <liphe/seal_bfv_number.h>
#include <liphe/seal_bfv_keys.h>
#include <liphe/zp.h>
#include <liphe/unsigned_word.h>
#include <liphe/packed_bit_array.h>

#include "matrix.h"
#include "vector.h"
#include "packed_matrix.h"
#include "packed_vector.h"
#include "crt2.h"

#include "times.h"
#include "csv.h"


class SirTypes {
public:
	typedef CrtBundle<SealBfvKeys, SealBfvNumber> Ciphertext;
	typedef CrtBundle<ZPKeys, ZP> Plaintext;

	typedef CrtContainer<SealBfvKeys, SealBfvNumber> PrivKey;
	typedef CrtContainer<SealBfvKeys, SealBfvNumber> PubKey;
	typedef CrtContainer<ZPKeys, ZP> PlainKey;

	typedef Helib2Number BinaryBitCiphertext;
	typedef ZP BinaryBitPlaintext;

	typedef UnsignedWord<BinaryBitCiphertext> BinaryWordCiphertext;
	typedef UnsignedWord<BinaryBitPlaintext> BinaryWordPlaintext;

	typedef Helib2Keys PrivBinaryKey;
	typedef Helib2Keys PubBinaryKey;
	typedef ZPKeys PlainBinaryKey;

	typedef PackedBitArray<BinaryBitCiphertext, PubBinaryKey> PackedBitUnsignedWord;
};



SirTypes::PrivKey privKey;
SirTypes::PubKey pubKey;
SirTypes::PlainKey plainKey;


SirTypes::PrivBinaryKey privBinaryKey;
SirTypes::PubBinaryKey pubBinaryKey;
SirTypes::PlainBinaryKey plainBinaryKey;

const SirTypes::PubKey *pubKeyOfOperation(const SirTypes::PubKey *k1, const SirTypes::PubKey *k2) {
	assert(k1 == k2);
	return k1;
}

const SirTypes::PubKey *pubKeyOfOperation(const SirTypes::PlainKey *k1, const SirTypes::PubKey *k2) {
	return k2;
}

const SirTypes::PubKey *pubKeyOfOperation(const SirTypes::PubKey *k1, const SirTypes::PlainKey *k2) {
	return k1;
}

void convert(CrtDigit<SealBfvKeys, SealBfvNumber> &out, const CrtDigit<ZPKeys, ZP> &in, int i, const CrtContainer<SealBfvKeys, SealBfvNumber> *pubKey) {
	out = CrtDigit<SealBfvKeys,SealBfvNumber>(pubKey->key(i)->from_int( in.to_int() ), pubKey->key(i), pubKey);
}

CrtDigit<SealBfvKeys, SealBfvNumber> operator*(const CrtDigit<ZPKeys, ZP> &b, const CrtDigit<SealBfvKeys, SealBfvNumber> &a) {
//	bool isZero = true;
//	std::vector<long> vals = b.to_vector();
//	for (auto v : vals)
//		isZero &= (v != 0);
//
//	if (isZero) {
//		CrtDigit<SealBfvKeys, SealBfvNumber> aa(a);
//		aa -= a;
//		return aa;
//	} else
		return a*b;
}

cpp_int PRINT(const SirTypes::Plaintext &p) {
	return p.to_bigint(&plainKey);
}

cpp_int TO_INT(const SirTypes::Ciphertext &v) {
	return v.to_bigint(&privKey);
}

std::string TO_STRING(const PackedVector<SirTypes::Ciphertext> &v) {
	return v.to_string(&privKey);
}

std::string TO_STRING(const PackedMatrix<SirTypes::Ciphertext> &v) {
	return v.to_string(&privKey);
}

std::string TO_STRING(const PackedMatrixSet<SirTypes::Ciphertext> &v) {
	return v.get_matrix_as_str(&privKey);
}

std::string TO_STRING(const SirTypes::PackedBitUnsignedWord &v) {
	stringstream ss;
	ss << v.toCppInt(privBinaryKey);
	return ss.str();
}

std::string TO_STRING(const SirTypes::BinaryBitCiphertext &v) {
	stringstream ss;
	ss << v.to_int();
	return ss.str();
}




#include "data_source.h"
#include "server1.h"
#include "server2.h"
#include "communication.h"
#include "sFir.h"

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

long findPrime1Mod2m(long p, long m) {
	// p = Primes::find_prime_bigger_than(p - 1);
	// while ((p % (2*m)) != 1) {
	// 	p = Primes::find_prime_bigger_than(p + 1);
	// }
	return (seal::PlainModulus::Batching(m, 17)).value();
}

void mytest(SealBfvKeys *k) {
	SealBfvNumber c;
	vector<long> a({ 0, 1, 2 });
	vector<long> b;

	c = k->from_vector(a);
	for (int i = 0; i < 30; ++i) {
		b = k->to_vector(c);
		cout << "At iteration " << i << ": " << b[0] << " " << b[1] << " " << b[2] << " " << endl;
		if (b[0] != 0)
			break;
		c *= c;
	}
	b = k->to_vector(c);
	cout << b[0] << " " << b[1] << " " << b[2] << " " << endl;
	exit(1);
}

bool arg_int(int &i, const char *name, const char *argv) {
	std::string fullName = string("--") + name + "=";

	if (memcmp(argv, fullName.c_str(), fullName.size()) == 0) {
		i = atoi(argv + fullName.size());
		return true;
	}
	return false;
}

bool arg_string(string &i, const char *name, const char *argv) {
	std::string fullName = string("--") + name + "=";

	if (memcmp(argv, fullName.c_str(), fullName.size()) == 0) {
		i = string(argv + fullName.size());
		return true;
	}
	return false;
}

int main(int argc, char **argv) {
	//omp_set_max_active_levels(4);

	int primeNumber = 1;
	int primeBits = 30;
	int sealChainLength = 4;
	int sealChainBits = 20;
	std::string in("");
	int dim = 4;
	int lines = 20;
	int s = 2;
	int resolution = 0;
	int maxDim = -1;
	int iter = -1;

	int mSeal = 4*1024;

	int dump = 0;
	for (int argc_i = 1; argc_i < argc; ++argc_i) {
		if (!arg_int(mSeal, "mSeal", argv[argc_i]) &&
			!arg_int(primeNumber, "primeNumber", argv[argc_i]) &&
			!arg_int(primeBits, "primeBits", argv[argc_i]) &&
			!arg_int(sealChainLength, "sealChainLength", argv[argc_i]) &&
			!arg_int(sealChainBits, "sealChainBits", argv[argc_i]) &&
			!arg_int(dim, "d", argv[argc_i]) &&
			!arg_int(maxDim, "maxd", argv[argc_i]) &&
			!arg_int(iter, "iter", argv[argc_i]) &&
			!arg_int(lines, "l", argv[argc_i]) &&
			!arg_int(s, "s", argv[argc_i]) &&
			!arg_int(resolution, "r", argv[argc_i]) &&
			!arg_int(dump, "dump", argv[argc_i]) &&
			!arg_string(in, "in", argv[argc_i]) &&
			!arg_int(DATA_SOURCE_NUMBER, "do", argv[argc_i]) &&
			true
		)
			cout << "param " << argv[argc_i] << " is unsupported" << endl;



		if (strcmp(argv[argc_i], "--help") == 0) {
			std::cout << "   --mSeal=32768 cyclotomic polynomial degree for Seal" << std::endl;
			std::cout << "   --primeNumber=1 number of primes in Seal CRT" << std::endl;
			std::cout << "   --primeBits=59 number of bits in each prime in Seal CRT" << std::endl;
			std::cout << "   --sealChainLength=29 Number of chain elements for seal" << std::endl;
			std::cout << "   --sealChainBits=29 Number bits in each chain element" << std::endl;
			std::cout << "   --d=4 dimension of model. If input is specified, d cannot be bigger than dimension of input" << std::endl;
			std::cout << "   --maxd=-1 when reading a csv specify the max number of features" << std::endl;
			std::cout << "   --iter=-1 max number of iterations to run" << std::endl;
			std::cout << "   --l=20 number of lines. If input is specified, lines cannot be bigger than lines of input" << std::endl;
			std::cout << "   --s=4 number of features in the output model" << std::endl;
			std::cout << "   --in= input file (blank means random)" << std::endl;
			std::cout << "   --r= how many decimal digits to consider (default is 0)" << std::endl;
			std::cout << "   --dump dump the communiation into files" << std::endl;
			std::cout << "   --do=10 number of data owners" << std::endl;
			exit(1);
		}
	}

	cout << "Using parameters:" << endl
		<< "mSeal = " << mSeal << endl
		<< "primeNumber = " << primeNumber << endl
		<< "primeBits = " << primeBits << endl
		<< "sealChainLength = " << sealChainLength << endl
		<< "sealChainBits = " << sealChainBits << endl
		<< "d = " << dim << endl
		<< "l = " << lines << endl
		<< "s = " << s << endl
		<< "in = " << in << endl
		<< "do = " << DATA_SOURCE_NUMBER << endl
		<< "resolution = " << resolution << endl;

	Communication<SirTypes>::setDump(dump != 0);

	Matrix<float> X;
	std::vector<float> y;
	std::vector<float> model(dim);

	if (in != std::string("")) {
		read_csv_file(in, X, y, maxDim);
	} else {
		std::cout << "The real model is:";
		for (int i_col = 0; i_col < dim; ++i_col) {
			model[i_col] = myrand(0,5);
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

	cout << "x[0,0] = " << X(0,0) << endl;



	Times::start(Times::KeyGeneration);
	std::vector<seal::Modulus> sealMods =  seal::PlainModulus::Batching(mSeal, vector<int>(primeNumber, primeBits));

#	pragma omp parallel for
	for (int i_prime = 0; i_prime < primeNumber + 1; ++i_prime) {
		if (i_prime < primeNumber) {
			std::cout << "initing CRT key #" << i_prime << std::endl;
			SealBfvKeys *heKeys = new SealBfvKeys;

			// vector<int> c({30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30});
			 vector<int> c(sealChainLength, sealChainBits);
			long p = sealMods[i_prime].value();

			heKeys->initKeys(p, mSeal, c);

			ZPKeys *zpKeys = new ZPKeys(p, 1, heKeys->simd_factor());

#			pragma omp critical
			{
				cout << "adding key with plaintext modulo " << p << " and " << heKeys->simd_factor() << " slots and security level " << heKeys->securityLevel() << std::endl;
				privKey.add_key(heKeys);
				pubKey.add_key(heKeys);
				plainKey.add_key(zpKeys);
			}
		} else {
			std::cout << "initing binary key"  << std::endl;

			Helib2Keys::Params params;
			params.p = 17;
			params.r = 2;
			params.c = 64;
			params.m = 78881;
			params.bits = 1000;
			params.mvec = {101, 781};
			params.gens = {67167, 58581};
			params.ords = {100, 70};
			params.bootstrappable = true;

			privBinaryKey.initKeys(params);

			cout << "Init binary key with " << privBinaryKey.simd_factor() << " slots" << endl;
			pubBinaryKey = privBinaryKey;
			plainBinaryKey = ZPKeys(params.p, params.r, privBinaryKey.simd_factor());
		}
	}
	plainKey.init_inversion_keys();	
	privKey.init_inversion_keys();	
	pubKey.init_inversion_keys();	
	Times::end(Times::KeyGeneration);

	printTimeStamp("after initing keys");

	// SirTypes::Ciphertext c;
	// c.from_int(1, &pubKey);
	// for (int i = 0; i < 10; ++i) {
	// 	cout << i << " = " << TO_INT(c) << endl;
	// 	c *= 2;
	// }

	Times::start(Times::EntireSir);
	std::vector<float> linRegModelFloat = sFIR<SirTypes>(X, y, s, resolution, plainKey, pubKey, iter);
	Times::end(Times::EntireSir);

	std::cout << "computed model = ";
	for (unsigned int i = 0; i < linRegModelFloat.size(); ++i) {
		std::cout << ((!i)?"":", ");
		std::cout << linRegModelFloat[i];
	}
	std::cout << std::endl;

	Times::print(cout);

	return 0;
}

