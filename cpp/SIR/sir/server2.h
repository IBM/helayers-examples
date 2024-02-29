#ifndef ___SERVER2___
#define ___SERVER2___

#define PrivKey typename Types::PrivKey
#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PrivBinaryKey typename Types::PrivBinaryKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define BitWordCiphertext typename Types::BitWordCiphertext

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext

template<class Types>
class Server2 {
	const PrivKey *_privKey;
	const PubKey *_pubKey;
	const PlainKey *_plainKey;

	const PrivBinaryKey *_privBinaryKey;
	const PubBinaryKey *_pubBinaryKey;
	const PlainBinaryKey *_plainBinaryKey;

	PackedMatrix<Ciphertext> _EncAprime;
	PackedVector<Ciphertext> _Encbprime;
	std::vector<long> _F;

	//
	// members related to data preperation (merge and permute step)
	//
	Matrix<Plaintext> PermMat2;

	PackedVector<Ciphertext> _zPrime;
public:
	Server2() : _privKey(NULL), _pubKey(NULL), _plainKey(NULL), _privBinaryKey(NULL), _pubBinaryKey(NULL), _plainBinaryKey(NULL) {}

	Communication<Types> *_communication_channel;

	void setCommunicationChannel(Communication<Types> *c) { _communication_channel = c; }
	void setPrivKey(const PrivKey *p) { _privKey = p; }
	void setPubKey(const PubKey *p) { _pubKey = p; }
	void setPlainKey(const PlainKey *p) { _plainKey = p; }

	void setPrivBinaryKey(const PrivBinaryKey *p) { _privBinaryKey = p; }
	void setPubBinaryKey(const PubBinaryKey *p) { _pubBinaryKey = p; }
	void setPlainBinaryKey(const PlainBinaryKey *p) { _plainBinaryKey = p; }

	void receive_A_and_bfrom_server1(const PackedMatrix<Ciphertext> &A, const PackedVector<Ciphertext> b, const std::vector<long> &F) { _EncAprime = A; _Encbprime = b; _F = F; }

	void solve();

	// prepare data protcol
	void prepare_data_step2(const PackedMatrix<Ciphertext> &AP);
	void prepare_data_step4(PackedMatrix<Ciphertext> &AP);
	void prepare_data_step6(PackedMatrix<Ciphertext> &AP);
	void prepare_data_step8(PackedMatrix<Ciphertext> &AP);
	void prepare_data_step10(PackedMatrix<Ciphertext> &AP);
	void prepare_data_step12(PackedMatrix<Ciphertext> &AP);

	// binAbs protocol
	void receive_zPrime(const PackedVector<Ciphertext> &zPrime, const std::vector<long> &F) { _zPrime = zPrime; _F = F; }
	void binarize() const;

	// findSmallestFeatures protocol	
	void receive_ranks(const std::vector<BinaryBitCiphertext> &ranks, int k);
	void receiveWeightAllPairDiffs(std::vector<PackedVector<Ciphertext>> &diffEnc, const std::vector<long> &F);

	// compactify protocol
	void receive_A_to_compact(const PackedMatrixSet<Ciphertext> &encAPrime, const PackedVector<Ciphertext> &encbPrime, const std::vector<long> &F);
};

#undef PrivKey
#undef PubKey
#undef PlainKey
#undef PrivBinaryKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext

#endif
