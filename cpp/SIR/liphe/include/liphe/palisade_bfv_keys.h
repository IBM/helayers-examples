#ifndef __PALISADE_BFV_KEYS__
#define __PALISADE_BFV_KEYS__

#include <memory>

#include <palisade.h>

class PalisadeBfvKeys {
private:
	lbcrypto::LPKeyPair<lbcrypto::DCRTPoly> _keyPair;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> _context;

	int _plaintextMod;
	unsigned int _nSlots;
	lbcrypto::SecurityLevel _sec;
public:
	PalisadeBfvKeys() {}
	PalisadeBfvKeys(const PalisadeBfvKeys &h) : _keyPair(h._keyPair), _context(h._context), _plaintextMod(h._plaintextMod), _nSlots(h._nSlots), _sec(h._sec) {}

	void initKeys(int plaintextMod, unsigned int depth, int slots, const std::vector<int> &rotations, const lbcrypto::SecurityLevel &sec = lbcrypto::HEStd_128_classic) {
		_plaintextMod = plaintextMod;
		_sec = sec;

		double sigma = 3.2;
		_context = lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::genCryptoContextBFVrns(plaintextMod, sec, sigma, 0, depth, 0, OPTIMIZED);

		//Enable features that you wish to use
		_context->Enable(ENCRYPTION);
		_context->Enable(SHE);

		_keyPair = _context->KeyGen();

		// Generate the relinearization key
		_context->EvalMultKeyGen(_keyPair.secretKey);

		// Generate the rotation evaluation keys
		_context->EvalAtIndexKeyGen(_keyPair.secretKey, rotations);
	}

	long p() const { return _plaintextMod; }
	long r() const { return 1; }
	long m() const { return -1; }

//	long secrityLevel() const { return _context->securityLevel(); }

	size_t nslots() const { return _context->GetRingDimension(); }
	size_t simd_factor() const { return nslots(); }

//	FHEPubKey &publicKey() { return *_publicKey; }
	const lbcrypto::CryptoContext<lbcrypto::DCRTPoly> &context() { return _context; }
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> encrypt(int b);
	void encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &c, int b);
	long decrypt(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

	lbcrypto::Plaintext encode(long z);
	lbcrypto::Plaintext encode(const std::vector<long> &z);

	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> encrypt(const std::vector<long> &b);
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> encrypt(const std::vector<int> &b);
	void encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &c, const std::vector<long> &b);
	void encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &c, const std::vector<int> &b);
	void decrypt(std::vector<long> &, const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

//	void encode(NTL::ZZX &r, long b);
//	void encode(NTL::ZZX &r, const std::vector<long> &b);

	void write_to_file(std::ostream& s);
	void read_from_file(std::istream& s);

	void print(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b);

	void rotate(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &a, int step) { a = _context->EvalAtIndex(a, step); }
//	void shift(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &a, int step) { _ea->shift(a, step); }
};

#endif
