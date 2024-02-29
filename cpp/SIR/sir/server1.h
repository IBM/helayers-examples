#ifndef ___SERVER1___
#define ___SERVER1___

#include <numeric>
//#include <liphe/unsigned_word.h>
#include <liphe/packed_bit_array.h>
#include "packed_matrix.h"
#include "packed_vector.h"

#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryWordPlaintext typename Types::BinaryWordPlaintext
#define BinaryWordCiphertext typename Types::BinaryWordCiphertext
#define BinaryBitCiphertext typename Types::BinaryBitCiphertext
#define PubBinaryKey typename Types::PubBinaryKey

#define PackedBitUnsignedWord typename Types::PackedBitUnsignedWord

template<class Types>
class Server1 {
private:
	const PubKey *_pubKey;
	const PlainKey *_plainKey;

	const PubBinaryKey *_pubBinaryKey;
	const PlainBinaryKey *_plainBinaryKey;

	// an indicator vector of the surviving features
	std::vector<long> _feature_list;

	//
	// members related to data preperation (merge and permute step)
	//
	Matrix<Plaintext> PermMat;
	Matrix<Plaintext> R1;
	Matrix<Plaintext> R2;
	Matrix<Plaintext> R3;
	Matrix<Plaintext> R4;
	Matrix<Plaintext> R5;
	Matrix<Plaintext> R6;
	Matrix<Plaintext> R7;


	//
	// members related to the scaled-ridge protocol
	//

	// _R and _r are the masking Matrix and vector used in the scaled_ridge step
	Matrix<Plaintext> _R;
	std::vector<Plaintext> _r;

	// _A and _b are the matrix and vector used in scaled ridge step
	PackedMatrixSet<Ciphertext> _A;
	PackedVector<Ciphertext> _b;

	// _scaledWMasked is the masked weight vector (scaled) as returned by server2
	std::vector< PackedVector<Ciphertext> > _scaledWMasked;
	// _det is the determinant of the masked version of _A as returned by server2
	Ciphertext _det;

	PackedVector<Ciphertext> _z;

	//
	// members related to the bin-abs protocol
	//

	// _z is the scaled weight vector (i.e. z = w * det(A)). It is also unmasked.
	// zPrime (not kept as a memeber) is a masking of _z
	// _zBinMasked is zPrime converted by Server2 to bits.
	//        Each bit is encrypted in a ciphertext and the SIMD dimension is used to keep the different values of _z[i].
	//        We use PackedVector because the number of features might exceed the SIMD factor.
	// _zBinMasked
	//
	// This is the packing of _zBin:
	//    _zBin[i] is the weight of the i-th feature.
	//       Each bit is stored in a different slot
	std::vector<cpp_int> _rBinAbs;
	std::vector<PackedBitUnsignedWord> _zBinMasked;
	std::vector<PackedBitUnsignedWord> _maskThreshold;
	std::vector<PackedBitUnsignedWord> _zBin;

	// how many features we need to remove
	int _k;
	// random masks for masking all distance pairs
	std::vector<std::vector<cpp_int>> _rSmallestFeatures1;
	std::vector<std::vector<cpp_int>> _rSmallestFeatures2;


	//
	// members related to the findSmallestFeatures protocol
	//

	std::vector<int> _randomPermutation;
	std::vector<long> _smallestFeatureChi;

	//
	// members related to the compactify protocol
	//

    Matrix<Plaintext> _compactifyProjectedR;
    std::vector<Plaintext> _compactifyProjectedr;



	Communication<Types> *_communication_channel;

	void recursiveSmearLsb(BinaryBitCiphertext &b, int &times, int &filled) const;
	BinaryBitCiphertext smearLsb(const BinaryBitCiphertext &b) const;

	void generateRandomPermutation();
	void permute(std::vector<BinaryBitCiphertext> &p);
	void unpermute(std::vector<long> &p);
public:
	Server1() : _pubKey(NULL), _plainKey(NULL), _pubBinaryKey(NULL), _plainBinaryKey(NULL) {}

	void setCommunicationChannel(Communication<Types> *c) { _communication_channel = c; }
	void setPubKey(const PubKey *p) { _pubKey = p; }
	void setPlainKey(const PlainKey *p) { _plainKey = p; }
	void setPubBinaryKey(const PubBinaryKey *p) { _pubBinaryKey = p; }
	void setPlainBinaryKey(const PlainBinaryKey *p) { _plainBinaryKey = p; }

	void init_feature_list() { _feature_list = std::vector<long>(_A.cols(), 1); }
	// size_t feature_number() const { size_t r=0; for (long i : _feature_list) r += i; return r; }
	size_t feature_number() const { return std::accumulate(_feature_list.begin(), _feature_list.end(), 0); }
	std::vector<long> features() const { return _feature_list; }


	void prepare_data();
	void prepare_data_step3(PackedMatrixSet<Ciphertext> &APR);
	void prepare_data_step5(PackedMatrixSet<Ciphertext> &APL);
	void prepare_data_step7(PackedMatrixSet<Ciphertext> &APL);
	void prepare_data_step9(PackedMatrixSet<Ciphertext> &APL);
	void prepare_data_step11(PackedMatrixSet<Ciphertext> &APL);
	void prepare_data_step13(PackedMatrixSet<Ciphertext> &APL);


	void receive_fraction_of_A_and_b(const PackedMatrixSet<Ciphertext> &a, const PackedVector<Ciphertext> &b);
	void receive_adj_det_from_server2(const std::vector< PackedVector<Ciphertext> > &scaledW, const Ciphertext &det) { _scaledWMasked = scaledW; _det = det; }

	void scaled_ridge_mask_and_send_to_server2();
	void scaled_ridge_unmask();

	void binAbs_mask_and_send_to_server2();
	void receive_zPrimeBits(const std::vector<PackedBitUnsignedWord> &zBinMasked, const std::vector<PackedBitUnsignedWord> &thresholdBits)
			{ _zBinMasked = zBinMasked; _maskThreshold = thresholdBits; }
	void binAbs_unmask();
	void binAbs_z();


	// findSmallestFeatures protocol	
	void findSmallestFeaturesOld(int k);
	void receive_rank_chi(const std::vector<long> &chi);
	void findSmallestFeaturesNew(int k);

	void receive_rMin_rMax(std::vector<PackedBitUnsignedWord> &rMin, std::vector<PackedBitUnsignedWord> &rmax, std::vector<BinaryBitCiphertext> &between, std::vector<std::pair<int,int>> &pairVec);


	// compactify protocol
	void compactify();
	void receive_compacted_A(const PackedMatrixSet<Ciphertext> &A, const PackedVector<Ciphertext> &b);


	void scaled_ridge();

	void linear_regression();
	void exhaustive_sparse_linear_regression();

	const Ciphertext &getDet() const { return _det; }
	const PackedVector<Ciphertext> &getZ() const { return _z; }
};


#undef PubKey
#undef PlainKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryWordPlaintext
#undef BinaryWordCiphertext

#undef PackedBitUnsignedWord

#endif
