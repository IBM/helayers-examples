#ifndef __PALISADE_CKKS_EXACT_KEYS__
#define __PALISADE_CKKS_EXACT_KEYS__

// This class uses the ckks implementation in palisade with exact rescaling

#include <memory>

#include <palisade.h>

class PalisadeCkksExactKeys {
public:
	struct Params {
		uint32_t multDepth;
		uint32_t scaleFactorBits;
		uint32_t slotNum;
		lbcrypto::SecurityLevel securityLevel;
	};

private:
	lbcrypto::LPKeyPair<lbcrypto::DCRTPoly> _keyPair;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> _context;

	unsigned int _nSlots;
	lbcrypto::SecurityLevel _sec;
public:
	PalisadeCkksExactKeys() {}
	PalisadeCkksExactKeys(const PalisadeCkksExactKeys &h) : _keyPair(h._keyPair), _context(h._context), _nSlots(h._nSlots), _sec(h._sec) {}

	void initKeys(int multDepth, int scaleFactorBits, int slots, const std::vector<int> &rotations, const lbcrypto::SecurityLevel &sec = lbcrypto::HEStd_128_classic) {
		_sec = sec;

		lbcrypto::RescalingTechnique rsTech = lbcrypto::EXACTRESCALE;
		uint32_t ringDimension = 0; // 0 means the library will choose it based on securityLevel

		lbcrypto::KeySwitchTechnique ksTech = lbcrypto::BV;
		uint32_t dnum = 0;
		/*
	 	* This controls how many multiplications are possible without rescaling.
	 	* The number of multiplications (depth) is maxDepth - 1.
	 	* This is useful for an optimization technique called lazy
	 	* re-linearization (only applicable in APPROXRESCALE, as
	 	* EXACTRESCALE implements automatic rescaling).
	 	*/
		uint32_t maxDepth = 2;
		// This is the size of the first modulus
		uint32_t firstModSize = 60;
		/*
	 	* The relinearization window is only used in BV key switching and
	 	* it allows us to perform digit decomposition at a finer granularity.
	 	* Under normal circumstances, digit decomposition is what we call
	 	* RNS decomposition, i.e., each digit is roughly the size of the
	 	* qi's that comprise the ciphertext modulus Q. When using BV, in
	 	* certain cases like having to perform rotations without any
	 	* preceding multiplication, we need to have smaller digits to prevent
	 	* noise from corrupting the result. In this case, using relinWin = 10
	 	* does the trick. Users are encouraged to set this to 0 (i.e., RNS
	 	* decomposition) and see how the results are incorrect.
	 	*/
		uint32_t relinWin = 10;
		MODE mode = OPTIMIZED; // Using ternary distribution

		_context = lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::genCryptoContextCKKS(multDepth, scaleFactorBits, slots, sec, ringDimension, rsTech, ksTech, dnum, maxDepth, firstModSize, relinWin, mode);
		_context->Enable(ENCRYPTION);
		_context->Enable(SHE);
		_context->Enable(LEVELEDSHE);

		std::cout << "CKKS scheme is using ring dimension " << _context->GetRingDimension() << std::endl << std::endl;

		_keyPair = _context->KeyGen();
		_context->EvalMultKeyGen(_keyPair.secretKey);

		_context->EvalAtIndexKeyGen(_keyPair.secretKey, rotations);
	}

//	long secrityLevel() const { return _context->securityLevel(); }

	size_t nslots() const { return _context->GetRingDimension() / 2; }
	size_t simd_factor() const { return nslots(); }

//	FHEPubKey &publicKey() { return *_publicKey; }
	const lbcrypto::CryptoContext<lbcrypto::DCRTPoly> &context() { return _context; }
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> encrypt(double b);
	void encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &c, double b);
	double decrypt(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

	lbcrypto::Plaintext encode(double z);
	lbcrypto::Plaintext encode(const std::vector<double> &z);

	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> encrypt(const std::vector<double> &b);
	void encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &c, const std::vector<double> &b);
	void decrypt(std::vector<double> &, const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

//	void encode(NTL::ZZX &r, long b);
//	void encode(NTL::ZZX &r, const std::vector<long> &b);

	void write_to_file(std::ostream& s);
	void read_from_file(std::istream& s);

	void print(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

	void rotate(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &a, int step) { a = _context->EvalAtIndex(a, step); }
//	void shift(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &a, int step) { _ea->shift(a, step); }
};

#endif
