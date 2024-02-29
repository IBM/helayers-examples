#ifndef ___LINEAR_REGRESSION___
#define ___LINEAR_REGRESSION___



#include <omp.h>
#include <set>

#include "times.h"
#include "rational_reconstruction.h"
#include "packed_matrix.h"
#include "packed_vector.h"
//#include "crt.h"

#include "scaled_ridge.h"
#include "abs_bits_protocol.h"
#include "find_smallest_features_IR.h"
#include "prepare_data.h"

// Plaintext = crt<long>
// Ciphertext = crt<HElib>
//
// PlaintextBits = 
// Ciphertext =

#define PrivKey typename Types::PrivKey
#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryPlaintext typename Types::BinaryPlaintext
#define BinaryCiphertext typename Types::BinaryCiphertext

template<class Types>
std::vector<float> sFIR(const Matrix<float> &X, const std::vector<float> &y, size_t s, int resolution, const PlainKey &plainKey, const PubKey &pubKey, int iter) {
#	ifdef __DEBUG
	std::cout << "Running sFIR with ring size = " << pubKey.get_ring_size() << std::endl;
#	endif

	// omp_set_nested(4);

	TimeGuard sirTimeGuard(Times::EntireSir);

	Server1<Types> server1;
	Server2<Types> server2;
	std::vector< DataSource<Types> > dataSource(DATA_SOURCE_NUMBER);

	server1.setPubKey(&pubKey);
	server1.setPlainKey(&plainKey);

	server1.setPubBinaryKey(&pubBinaryKey);
	server1.setPlainBinaryKey(&plainBinaryKey);

	server2.setPrivKey(&privKey);
	server2.setPubKey(&pubKey);
	server2.setPlainKey(&plainKey);

	server2.setPrivBinaryKey(&privBinaryKey);
	server2.setPubBinaryKey(&pubBinaryKey);
	server2.setPlainBinaryKey(&plainBinaryKey);

	Communication<Types> communication(dataSource, server1, server2);

	Times::start(Times::DataEncoding);

#pragma omp parallel for
	for (int i_dataSource = 0; i_dataSource < DATA_SOURCE_NUMBER; ++i_dataSource) {
		DataSource<Types> d;
		d.setCommunicationChannel(&communication);
		d.setPubKey(&pubKey);
		d.setPlainKey(&plainKey);

		int rowsPerSource = (X.rows() + DATA_SOURCE_NUMBER - 1) / DATA_SOURCE_NUMBER;

		Matrix<float> encX;
		std::vector<float> ency;

		unsigned int start = i_dataSource * rowsPerSource;
		unsigned int end = std::min(X.rows() - 1, start + rowsPerSource);

		encX.resize(X.cols(), end - start);
		ency.resize(end - start);

		for (unsigned int j = start; j < end; ++j) {
			ency[j - start] = y[j];
			for (unsigned int i = 0; i < X.cols(); ++i) {
				encX(i, j - start) = X(i,j);
			}
		}

		d.set_data(encX, ency);
		d.encode_data(i_dataSource == 0, resolution);
#pragma omp critical
		{
			d.send_A_and_b_to_server1();
		}
	}
	Times::end(Times::DataEncoding);

	printTimeStamp("after encoding data");

	server1.init_feature_list();

	Times::start("prepare data");
	server1.prepare_data();
	Times::end("prepare data");

	printTimeStamp("after preparing data");

	int iteration = 0;
	while (server1.feature_number() > s) {
		++iteration;

		// engage in the scaled_ridge protocol with server2.
		// Input (of server1): _A, _b _R, _r
		// Output (for server1): _z the scaled weight vector
		string iterationName = string("Iteration ") + std::to_string(iteration);

		cout << "Starting " << iterationName << endl;
		cout << "Feature list of " << iterationName << " = ";
		for (auto i : server1.features()) cout << i << " ";
		cout << endl;

		TimeGuard g(Times::SirSingleIteration + iterationName);
		{
			TimeGuard g2(Times::ScaledRidgeAllIterations);
			TimeGuard g(Times::ScaledRidgeSingleIteration + iterationName);

			server1.scaled_ridge();
		}

		printTimeStamp("after scaled ridge");

		if (server1.feature_number() > 100000) {
//			// engage in the binAbs protocol with server2.
//			// Input (of server1): _z a packed vector of weights
//			// Output (for server1): _a_binAbs a packed vector of bits.
//			{
//				TimeGuard g(std::string("Iteration ") + std::to_string(iteration) + " binAbs");
//				server1.binAbs_z();
//			}
//
//			{
//				TimeGuard g(std::string("Iteration ") + std::to_string(iteration) + " find smallest Feature (large)");
//				if (server1.feature_number() > 2*s) {
//					server1.findSmallestFeaturesOld(server1.feature_number() / 10);
//				} else {
//					server1.findSmallestFeaturesOld(1);
//				}
//			}
		} else {
			TimeGuard g1(Times::FindSmallestFeatureSingleIteration + iterationName);
			TimeGuard g2(Times::FindSmallestFeatureAllIterations);

			if (server1.feature_number() > 2*s) {
				// remove 10% of featureNum (rounded up)
				server1.findSmallestFeaturesNew((server1.feature_number()+9) / 10);
			} else {
				server1.findSmallestFeaturesNew(1);
			}
		}

		if ((iter != -1) && (iter == iteration)) {
			cout << "reached max number of specified iteration. Stopping SIR" << endl;
			break;
		}
	}

#ifdef __DEBUG
		cout << "Feature list of last = ";
		for (auto i : server1.features()) cout << i << " ";
		cout << endl;
#endif

	// last scaled ridge to recover the weights
	{
		TimeGuard g(Times::SirSingleIteration + "last");
		{
			TimeGuard g2(Times::ScaledRidgeAllIterations);
			TimeGuard g(Times::ScaledRidgeSingleIteration + "last");

			server1.scaled_ridge();
		}
	}

	std::vector<cpp_int> zint;
	server1.getZ().to_bigint_vector(zint, &privKey);

	cpp_int detint;
	detint = server1.getDet().to_bigint(&privKey);
#ifdef __DEBUG
	std::cout << "final determinant = " << detint << std::endl;
	std::cout << "final z = " << zint[0] << ", " << zint[1] << ", " << zint[2] << ", " << zint[3] << std::endl;
#endif

	std::vector<float> ret(server1.getZ().size());

	for (unsigned int i = 0; i < zint.size(); ++i) {
		ret[i] = divide(plainKey.get_ring_size(), zint[i], detint);
	}

#ifdef __DEBUG
	std::cout << "final w = " << ret[0] << ", " << ret[1] << ", " << ret[2] << ", " << ret[3] << std::endl;
#endif

	return ret;
}

#undef PrivKey
#undef PubKey
#undef PlainKey
#undef PrivBinaryKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryPlaintext
#undef BinaryCiphertext

#endif
