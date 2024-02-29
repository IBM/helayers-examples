#include <any>

#include "liphe/seal_bfv_keys.h"
#include "liphe/seal_bfv_number.h"

seal::Ciphertext SealBfvKeys::encrypt(int b) const {
	seal::Ciphertext r;
	SealBfvKeys::encrypt(r, b);
	return r;
}

void SealBfvKeys::encrypt(seal::Ciphertext &r, int b) const {
	seal::Plaintext ptxt;
	encode(ptxt, b);
	_encryptor->encrypt(ptxt, r);
}

void SealBfvKeys::encode(seal::Plaintext &r, long b) const {
	std::vector<uint64_t> bb(simd_factor());
	for (size_t i = 0; i < bb.size(); ++i)
		bb[i] = b;
	_encoder->encode(bb, r);
}

void SealBfvKeys::encode(seal::Plaintext &r, const std::vector<long> &b) const {
	assert(b.size() == simd_factor());
	std::vector<uint64_t> bb(b.size());
	for (size_t i = 0; i < bb.size(); ++i)
		bb[i] = b[i];
	_encoder->encode(bb, r);
}

long SealBfvKeys::decrypt(const seal::Ciphertext &b) const {
	seal::Plaintext ptxt;
	_decryptor->decrypt(b, ptxt);
	return (long)ptxt[0];
}


seal::Ciphertext SealBfvKeys::encrypt(const std::vector<int> &b) const {
	seal::Plaintext ptxt;
	// for (size_t i = 0; i < std::min(simd_factor(), b.size()); ++i)
	// 	ptxt[i] = b[i];
	std::vector<long> bb(b.size());
	for (size_t i = 0; i < b.size(); ++i)
		bb[i] = b[i];
	_encoder->encode(bb, ptxt);

	seal::Ciphertext ctxt;
	_encryptor->encrypt(ptxt, ctxt);
	return ctxt;
}

seal::Ciphertext SealBfvKeys::encrypt(const std::vector<long> &b) const {
	seal::Ciphertext r;
	encrypt(r, b);
	return r;
}

void SealBfvKeys::encrypt(seal::Ciphertext &r, const std::vector<int> &b) const {
	seal::Plaintext ptxt;
	std::vector<long> bb(b.size());
	for (size_t i = 0; i < b.size(); ++i)
		bb[i] = b[i];
	_encoder->encode(bb, ptxt);
	_encryptor->encrypt(ptxt, r);
}

void SealBfvKeys::encrypt(seal::Ciphertext &r, const std::vector<long> &b) const {
	seal::Plaintext ptxt;
	std::vector<uint64_t> bb(b.size());
	for (size_t i = 0; i < bb.size(); ++i)
		bb[i] = b[i];
	_encoder->encode(bb, ptxt);
	_encryptor->encrypt(ptxt, r);
}


void SealBfvKeys::decrypt(std::vector<long> &out, const seal::Ciphertext &b) const {
	seal::Plaintext ptxt;
	std::vector<uint64_t> out2;
	_decryptor->decrypt(b, ptxt);
    _encoder->decode(ptxt, out2);
	out.resize(out2.size());
	for (size_t i = 0; i < out.size(); ++i)
		out[i] = out2[i];
}

void SealBfvKeys::print(const seal::Ciphertext &b) const {
//	PlaintextArray myDecrypt(*_ea);
//	_ea->decrypt(b, *_secretKey, myDecrypt);
//	helib::Ptxt<helib::BGV> myOutput;
//	_ea->encode(myOutput, myDecrypt);
//
//	std::cout << myOutput << std::endl;
	throw std::runtime_error("Not implemented");
}

void SealBfvKeys::write_to_file(std::ostream &out) const {
	throw std::runtime_error("Not implemented");
}

void SealBfvKeys::read_from_file(std::istream &in) {
	throw std::runtime_error("Not implemented");
}

SealBfvNumber SealBfvKeys::from_vector(const std::vector<long> &v) const {
	return SealBfvNumber(encrypt(v), this);
}

SealBfvNumber SealBfvKeys::from_int(long v) const {
	return from_vector(std::vector<long>(simd_factor(), v));
}

std::vector<long> SealBfvKeys::to_vector(const SealBfvNumber &c) const {
	std::vector<long> v;
	decrypt(v, c.val());
	return v;
}
