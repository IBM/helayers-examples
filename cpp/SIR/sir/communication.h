#ifndef ___COMMUNICATION___
#define ___COMMUNICATION___

#include <sstream>

#define PrivKey typename Types::PrivKey
#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryWordPlaintext typename Types::BinaryWordPlaintext
#define BinaryWordCiphertext typename Types::BinaryWordCiphertext

#define PackedBitUnsignedWord typename Types::PackedBitUnsignedWord


template<class Types>
class Communication {
private:
	std::vector< DataSource<Types> > &_dataSource;
	Server1<Types> &_server1;
	Server2<Types> &_server2;
	static bool dumpCommunication;

	string genName(const string &s, int i) {
		stringstream ss;
		ss << s << i;
		return ss.str();
	}

public:
	Communication(std::vector< DataSource<Types> > &d, Server1<Types> &s1, Server2<Types> &s2) : _dataSource(d), _server1(s1), _server2(s2) {
		for (unsigned int i = 0; i < _dataSource.size(); ++i)
			_dataSource[i].setCommunicationChannel(this);
		_server1.setCommunicationChannel(this);
		_server2.setCommunicationChannel(this);
	}

	static void setDump(bool s = true) { dumpCommunication = s; }
	
	// prepare the data (permute)
	void prepare_data_step2(const PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step2(AP); }
	void prepare_data_step4(PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step4(AP); }
	void prepare_data_step6(PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step6(AP); }
	void prepare_data_step8(PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step8(AP); }
	void prepare_data_step10(PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step10(AP); }
	void prepare_data_step12(PackedMatrix<Ciphertext> &AP) { _server2.prepare_data_step12(AP); }

	void prepare_data_step3(PackedMatrixSet<Ciphertext> &APR) { _server1.prepare_data_step3(APR); }
	void prepare_data_step5(PackedMatrixSet<Ciphertext> &APL) { _server1.prepare_data_step5(APL); }
	void prepare_data_step7(PackedMatrixSet<Ciphertext> &APL) { _server1.prepare_data_step7(APL); }
	void prepare_data_step9(PackedMatrixSet<Ciphertext> &APL) { _server1.prepare_data_step9(APL); }
	void prepare_data_step11(PackedMatrixSet<Ciphertext> &APL) { _server1.prepare_data_step11(APL); }
	void prepare_data_step13(PackedMatrixSet<Ciphertext> &APL) { _server1.prepare_data_step13(APL); }

	// ScaledRidge protocol
	void send_A_and_b_to_server1(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_A_b_DO_to_S1", i));
			A.save(f);
			b.save(f);
			++i;
		}
		_server1.receive_fraction_of_A_and_b(A, b);
	}

	void send_Aprime_and_bprime_to_server2(const PackedMatrix<Ciphertext> &A, const PackedVector<Ciphertext> &b, const std::vector<long> &F) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_A_b_S1_to_S2", i));
			A.save(f);
			b.save(f);
			++i;
		}
 		_server2.receive_A_and_bfrom_server1(A, b, F);
	}

	void send_w_to_DataSource(const std::vector<Plaintext> &v) {
		for (unsigned int i = 0; i < _dataSource.size(); ++i)
			_dataSource[i].receive_w_from_server1(v);
	}

	void send_Adj_Det_to_server1(const std::vector< PackedVector<Ciphertext> > &a, const Ciphertext &det) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_a_det_S2_to_S1", i));
			for (auto &aa : a)
				aa.save(f);
			det.save(f);
			++i;
		}

		_server1.receive_adj_det_from_server2(a, det);
	}

	void server2_solve() { _server2.solve(); }

	// binAbs protocol
	void send_zprime_to_server2(const PackedVector<Ciphertext> &zPrime, const std::vector<long> &F) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_z_S1_to_S2", i));
			zPrime.save(f);
			++i;
		}

		_server2.receive_zPrime(zPrime, F);
	}

	void server2_binarize() { _server2.binarize(); }

	void send_zPrimeBits_to_server1(const std::vector<PackedBitUnsignedWord> &zBinMasked,
				const std::vector<PackedBitUnsignedWord> &thresoldBits) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_zBits_S2_to_S1", i));
			zBinMasked.save(f);
			thresoldBits.save(f);
			++i;
		}

		_server1.receive_zPrimeBits(zBinMasked, thresoldBits);
	}

	// findSmallestFeatures protocol	
	void sendRanksToServer2(const std::vector<BinaryBitCiphertext> &ranks, int k) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_ranks_S1_to_S2", i));
			for (auto &r : ranks)
				r.save(f);
			++i;
		}

		_server2.receive_ranks(ranks, k);
	}

    void send_smallest_features_to_server1(const std::vector<long> &chi) { _server1.receive_rank_chi(chi); }
    void send_rMin_rMax_toServer1(std::vector<PackedBitUnsignedWord> &rMinVec, std::vector<PackedBitUnsignedWord> &rMaxVec, std::vector<BinaryBitCiphertext> &betweenVec, std::vector<std::pair<int,int>> &pairVec) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_threshold_S2_to_S1", i));
			for (auto &r : rMinVec)
				r.save(f);
			for (auto &r : rMaxVec)
				r.save(f);
			for (auto &r : betweenVec)
				r.save(f);
			++i;
		}

		_server1.receive_rMin_rMax(rMinVec, rMaxVec, betweenVec, pairVec);
	}

    void sendDiffsToServer2(std::vector<PackedVector<Ciphertext>> &diff, const std::vector<long> &F) {
		static int i = 1;
		if (dumpCommunication) {
			ofstream f(genName("send_diffs_S1_to_S2", i));
			for (auto &ff : diff)
				ff.save(f);
			++i;
		}

		_server2.receiveWeightAllPairDiffs(diff, F);
	}

	// compactify protocol
    void send_A_to_server2_to_compact(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b, const std::vector<long> &F) { _server2.receive_A_to_compact(A, b, F); }
    void send_compacted_A_to_server1(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b) { _server1.receive_compacted_A(A, b); }
};

template<class Types>
bool Communication<Types>::dumpCommunication = false;

#undef PrivKey
#undef PubKey
#undef PlainKey
#undef PrivBinaryKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryWordPlaintext
#undef BinaryWordCiphertext

#undef PackedBitUnsignedWord

#endif
