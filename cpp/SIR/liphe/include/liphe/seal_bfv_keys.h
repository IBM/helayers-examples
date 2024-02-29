#ifndef __SEAL_BFV_KEYS__
#define __SEAL_BFV_KEYS__

#include <seal/seal.h>

class SealBfvNumber;

class SealBfvKeys {
private:
	unsigned long _plaintextModulus;
	std::shared_ptr<seal::EncryptionParameters> _params;
	std::shared_ptr<seal::SEALContext> _context;
	std::shared_ptr<seal::SecretKey> _secretKey;
	std::shared_ptr<seal::PublicKey> _publicKey;
	std::shared_ptr<seal::Encryptor> _encryptor;
	std::shared_ptr<seal::Decryptor> _decryptor;
	std::shared_ptr<seal::BatchEncoder> _encoder;
	std::shared_ptr<seal::GaloisKeys> _galoisKeys;
	std::shared_ptr<seal::RelinKeys> _relinKeys;
	std::shared_ptr<seal::Evaluator> _evaluator;
public:
	SealBfvKeys() {}
	SealBfvKeys(const SealBfvKeys &a) : _params(a._params), _context(a._context), _secretKey(a._secretKey), _publicKey(a._publicKey), _encryptor(a._encryptor), _decryptor(a._decryptor) {} 

	SealBfvKeys &operator=(const SealBfvKeys &a) { _params=a._params; _context=a._context; _secretKey=a._secretKey; _publicKey=a._publicKey; _encryptor=a._encryptor; _decryptor=a._decryptor; return *this; } 

	// m must be a power of 2
	// c is a vector setting the bit sizes of the primes in the coef mod-chain
	// p much satisfy   p = 1 mod 2*m
	void initKeys(unsigned long p, int m, std::vector<int> c) {
		_params = std::make_shared<seal::EncryptionParameters>(seal::scheme_type::bfv);
		_params->set_poly_modulus_degree(m);
		_params->set_coeff_modulus(seal::CoeffModulus::Create(m, c));
		_params->set_plain_modulus(p);
		_plaintextModulus = p;

		// parms.set_poly_modulus_degree(poly_modulus_degree);
    	// parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    	// parms.set_plain_modulus(PlainModulus::Batching(poly_modulus_degree, 20));

		_context = std::make_shared<seal::SEALContext>(*_params, true, seal::sec_level_type::tc128);
		seal::KeyGenerator keygen(*_context);
		_secretKey = std::make_shared<seal::SecretKey>();
		*_secretKey = keygen.secret_key();
		_publicKey = std::make_shared<seal::PublicKey>();
		keygen.create_public_key(*_publicKey);
		_encoder = std::make_shared<seal::BatchEncoder>(*_context);
		_encryptor = std::make_shared<seal::Encryptor>(*_context, *_publicKey);
		_evaluator = std::make_shared<seal::Evaluator>(*_context);
	 	_decryptor = std::make_shared<seal::Decryptor>(*_context, *_secretKey);
		_galoisKeys = std::make_shared<seal::GaloisKeys>();
		keygen.create_galois_keys(*_galoisKeys);
		_relinKeys = std::make_shared<seal::RelinKeys>();
    	keygen.create_relin_keys(*_relinKeys);

		// std::vector<uint64_t> m1;
		// m1.resize(simd_factor());
		// for (size_t i = 0; i < m1.size(); ++i)
		// 	m1[i] = i;
		// seal::Plaintext ptxt1;
		// seal::Ciphertext ctxt;
		// _encoder->encode(m1, ptxt1);
		// _encryptor->encrypt(ptxt1, ctxt);

		// rotate(ctxt, 1);

		// seal::Plaintext ptxt2;
		// std::vector<uint64_t> m2;
		// _decryptor->decrypt(ctxt, ptxt2);
		// _encoder->decode(ptxt2, m2);
		// std::cout << m2[0] << ", " << m2[1] << std::endl;
	}

	long p() const { return _plaintextModulus; }
	long m() const { return _params->poly_modulus_degree(); }

	long get_ring_size() const { return p(); }

	long securityLevel() const { return 128; }

	int nslots() const { return _encoder->slot_count(); }
	unsigned long simd_factor() const { return nslots(); }

	seal::PublicKey &publicKey() { return *_publicKey; }
	const seal::PublicKey &publicKey() const { return *_publicKey; }
	const seal::Evaluator &eval() const { return *_evaluator; }
	void relinearize_inplace(seal::Ciphertext &c) const { eval().relinearize_inplace(c, *_relinKeys); }

	seal::Ciphertext encrypt(int b) const;
	void encrypt(seal::Ciphertext &c, int b) const;
	long decrypt(const seal::Ciphertext &b) const;

	seal::Plaintext encode(long z) const;
	seal::Plaintext encode(std::vector<long> &z) const;

	seal::Ciphertext encrypt(const std::vector<long> &b) const;
	seal::Ciphertext encrypt(const std::vector<int> &b) const;
	void encrypt(seal::Ciphertext &c, const std::vector<long> &b) const;
	void encrypt(seal::Ciphertext &c, const std::vector<int> &b) const;
	void decrypt(std::vector<long> &, const seal::Ciphertext &b) const;

	SealBfvNumber from_vector(const std::vector<long> &v) const;
	SealBfvNumber from_int(long v) const;
	std::vector<long> to_vector(const SealBfvNumber &v) const;
	long to_int(const SealBfvNumber &v) const { return to_vector(v)[0]; }

	void encode(seal::Plaintext &r, long b) const;
	void encode(seal::Plaintext &r, const std::vector<long> &b) const;

	void write_to_file(std::ostream& s) const;
	void read_from_file(std::istream& s);

	void print(const seal::Ciphertext &b) const;

	void rotate(seal::Ciphertext &a, int step) const { _evaluator->rotate_rows_inplace(a, step, *_galoisKeys); }
};

#endif
