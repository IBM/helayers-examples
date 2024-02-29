#include <sys/file.h>
#include <vector>
#include <istream>
#include <fstream>
#include <sstream>
#include <boost/tokenizer.hpp>

//#include <zp.h>
#include <liphe/helib2_number.h>
#include <liphe/helib2_keys.h>
#include <liphe/zp.h>
#include <liphe/unsigned_word.h>
#include <liphe/packed_bit_array.h>

#include "matrix.h"
#include "vector.h"
#include "crt2.h"

#include "times.h"


class HelibTypes {
public:
	typedef CrtBundle<Helib2Keys, Helib2Number> Ciphertext;
	typedef CrtBundle<ZPKeys, ZP> Plaintext;

	typedef CrtContainer<Helib2Keys, Helib2Number> PrivKey;
	typedef CrtContainer<Helib2Keys, Helib2Number> PubKey;
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



HelibTypes::PrivKey privKey;
HelibTypes::PubKey pubKey;
HelibTypes::PlainKey plainKey;


HelibTypes::PrivBinaryKey privBinaryKey;
HelibTypes::PubBinaryKey pubBinaryKey;
HelibTypes::PlainBinaryKey plainBinaryKey;

const HelibTypes::PubKey *pubKeyOfOperation(const HelibTypes::PubKey *k1, const HelibTypes::PubKey *k2) {
	assert(k1 == k2);
	return k1;
}

const HelibTypes::PubKey *pubKeyOfOperation(const HelibTypes::PlainKey *k1, const HelibTypes::PubKey *k2) {
	return k2;
}

const HelibTypes::PubKey *pubKeyOfOperation(const HelibTypes::PubKey *k1, const HelibTypes::PlainKey *k2) {
	return k1;
}

void convert(CrtDigit<Helib2Keys, Helib2Number> &out, const CrtDigit<ZPKeys, ZP> &in, int i, const CrtContainer<Helib2Keys, Helib2Number> *pubKey) {
	out = CrtDigit<Helib2Keys,Helib2Number>(pubKey->key(i)->from_int( in.to_int() ), pubKey->key(i), pubKey);
}

CrtDigit<Helib2Keys, Helib2Number> operator*(const CrtDigit<ZPKeys, ZP> &b, const CrtDigit<Helib2Keys, Helib2Number> &a) { return a*b; }

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


unsigned int countLines(const std::string &fname) {
	std::ifstream inFile(fname); 

	if (!inFile)
		throw std::runtime_error(std::string("can't open file ") + fname);
//	std::string line;
//	int count = 0;
//	while (getline(inFile, line))
//		++count;
//
//	return count;
	return std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
}

std::vector<float> parseCSVLine(std::string line){
	using namespace boost;

	std::vector<float> vec;

	// Tokenizes the input string
	tokenizer<escaped_list_separator<char> > tk(line, escaped_list_separator<char> ('\\', ',', '\"'));
	for (auto i = tk.begin();  i!=tk.end();  ++i)
		vec.push_back(std::stof(*i));

	return vec;
}


// read a csv file given as x,y,c   where c=0,1 is the class of a point and (x,y) is the coordinates
template<class Num>
void read_csv_file(const std::string &fname, Matrix<Num> &m, std::vector<Num> &v) {
	std::istream *in;

	if (fname == "-") {
		in = &(std::cin);
	} else {
		in = new std::ifstream(fname);
	}

	std::string line;
	std::vector<float> l;
	int nlines = countLines(fname);

	std::getline(*in, line);	// first line is headers
	std::getline(*in, line);
	l = parseCSVLine(line);
	unsigned int dim = l.size() - 1;
	m.resize(dim, nlines-1);
	v.resize(nlines-1);
	int i_line = 0;
	while (l.size() > 0) {
		if (l.size() != dim+1) {
			throw std::runtime_error(std::string("Error while reading CSV file. Lines have different length"));
		}
		for (unsigned int col = 0; col < dim; ++col)
			m(col, i_line) = Num(int(l[col]));
		v[i_line] = Num(int(l[dim]));
		std::getline(*in, line);
		l = parseCSVLine(line);
		++i_line;
	}

	if (in != &(std::cin))
		delete in;
}


int myrand(int min, int max) {
	return min + (random() % (max - min));
}


int main(int argc, char **argv) {
	unsigned long p = 101;
	int pi = -1;
	int primeNumber = 1;
	std::string in("");
	int dim = 4;
	int lines = 20;
	long L = 5;
	long Lbin = 30;
	int m = 1024;
	int s = 2;

	for (int argc_i = 1; argc_i < argc; ++argc_i) {
		if (memcmp(argv[argc_i], "--L=", 4) == 0) {
			L = atoi(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--Lbin=", 7) == 0) {
			Lbin = atoi(argv[argc_i] + 7);
		} else if (memcmp(argv[argc_i], "--p=", 4) == 0) {
			p = atol(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--pi=", 5) == 0) {
			pi = atol(argv[argc_i] + 5);
		} else if (memcmp(argv[argc_i], "--n=", 4) == 0) {
			primeNumber = atoi(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--d=", 4) == 0) {
			dim = atoi(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--l=", 4) == 0) {
			lines = atoi(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--in=", 5) == 0) {
			in = std::string(argv[argc_i] + 5);
		} else if (memcmp(argv[argc_i], "--s=", 4) == 0) {
			s = atol(argv[argc_i] + 4);
		} else if (memcmp(argv[argc_i], "--m=", 4) == 0) {
			m = atol(argv[argc_i] + 4);
		} else
			cout << "param " << argv[argc_i] << " is unsupported" << endl;


		if (strcmp(argv[argc_i], "--help") == 0) {
			std::cout << "   --L=5 depth of key" << std::endl;
			std::cout << "   --Lbin=30 depth of bin key" << std::endl;
			std::cout << "   --p=101 first prime" << std::endl;
			std::cout << "   --n=1 number of primes for CRT" << std::endl;
			std::cout << "   --in= input file (blank means random)" << std::endl;
			std::cout << "   --d=4 dimension of model. If input is specified, d cannot be bigger than dimension of input" << std::endl;
			std::cout << "   --l=4 number of lines. If input is specified, l cannot be bigger than lines of input" << std::endl;
			std::cout << "   --s=4 number of features in the output model" << std::endl;
			std::cout << "   --m=1024 degree of cyclotomic polynomial" << std::endl;
			exit(1);
		}
	}

	cout << "Using parameters:" << endl
		<< "L = " << L << endl
		<< "Lbin = " << Lbin << endl
		<< "p = " << p << endl
		<< "pi = " << pi << endl
		<< "n = " << primeNumber << endl
		<< "d = " << dim << endl
		<< "l = " << lines << endl
		<< "in = " << in << endl
		<< "s = " << s << endl
		<< "m = " << m << endl;




	unsigned long prime;
	if (p < 100000)
		prime = Primes::find_prime_bigger_than(p-1);
	else
		prime = p;
	if (pi >= 0)
		prime = Primes::prime(pi);
	vector<long> primes;
	for (int i_prime = 0; i_prime < primeNumber; ++i_prime) {
		primes.push_back(prime);
		prime = Primes::find_prime_bigger_than(prime+1);
	}
#	pragma omp parallel for
	for (int i_prime = 0; i_prime < primeNumber; ++i_prime) {
		std::cout << "initing CRT key #" << i_prime << std::endl;
		Helib2Keys *heKeys = new Helib2Keys;

		Helib2Keys::Params params;
		params.p = primes[i_prime];
		params.r = 1;
		params.c = 2;
		params.m = m;
		params.bits = L * (log2(prime) + 1);

		heKeys->initKeys(params);

		ZPKeys *zpKeys = new ZPKeys(params.p, params.r, heKeys->simd_factor());

#		pragma omp critical
		{
			cout << "adding key with " << heKeys->simd_factor() << " slots and security level " << heKeys->securityLevel() << std::endl;
			privKey.add_key(heKeys);
			pubKey.add_key(heKeys);
			plainKey.add_key(zpKeys);
		}
	}

	printTimeStamp("after CRT key init");

	{
		std::cout << "initing binary key"  << std::endl;
		int p = Primes::find_prime_bigger_than(dim+1);

		Helib2Keys::Params params;
		params.p = p;
		params.r = 1;
		params.c = 2;
		params.m = m;
		params.bits = Lbin * (log2(p) + 1);

		cout << "Setting Bin p to " << params.p << endl;
		cout << "Setting BinBits to " << params.bits << endl;

		privBinaryKey.initKeys(params);
		pubBinaryKey = privBinaryKey;
		plainBinaryKey = ZPKeys(params.p, params.r, privBinaryKey.simd_factor());
	}
	printTimeStamp("after binary key init");


	Matrix<float> X;
	std::vector<float> y;
	std::vector<float> model(dim);

	if (in != std::string("")) {
		read_csv_file(in, X, y);
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



	std::vector<float> linRegModelFloat = sFIR<HelibTypes>(X, y, s, plainKey, pubKey);

	std::cout << "computed model = ";
	for (unsigned int i = 0; i < linRegModelFloat.size(); ++i) {
		std::cout << ((!i)?"":", ");
		std::cout << linRegModelFloat[i];
	}
	std::cout << std::endl;

	return 0;
}

