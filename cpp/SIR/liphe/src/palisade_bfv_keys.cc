#include <assert.h>
#include "liphe/palisade_bfv_keys.h"

lbcrypto::Ciphertext<lbcrypto::DCRTPoly> PalisadeBfvKeys::encrypt(int b) {
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> r;
	encrypt(r, b);
	return r;
}

void PalisadeBfvKeys::encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &r, int b) {
	std::vector<int64_t> v(nslots(), b);
	lbcrypto::Plaintext plaintext = _context->MakePackedPlaintext(v);
	r = _context->Encrypt(_keyPair.publicKey, plaintext);
}

lbcrypto::Plaintext PalisadeBfvKeys::encode(long _b) {
	std::vector<int64_t> b(nslots(), (int64_t)_b);
	lbcrypto::Plaintext r = _context->MakePackedPlaintext(b);
	return r;
}

lbcrypto::Plaintext PalisadeBfvKeys::encode(const std::vector<long> &_b) {
	std::vector<int64_t> b(_b.size());
	for (size_t i = 0; i< _b.size(); ++i)
		b[i] = _b[i];
	b.resize(nslots());
	lbcrypto::Plaintext r = _context->MakePackedPlaintext(b);
	return r;
}

long PalisadeBfvKeys::decrypt(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	lbcrypto::Plaintext plaintext;
	_context->Decrypt(_keyPair.secretKey, b, &plaintext);
	std::vector<int64_t> out = plaintext->GetPackedValue();

	return out[0];
}




lbcrypto::Ciphertext<lbcrypto::DCRTPoly> PalisadeBfvKeys::encrypt(const std::vector<int> &b) {
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> r;
	encrypt(r, b);
	return r;
}

lbcrypto::Ciphertext<lbcrypto::DCRTPoly> PalisadeBfvKeys::encrypt(const std::vector<long> &b) {
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> r;
	encrypt(r, b);
	return r;
}

void PalisadeBfvKeys::encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &r, const std::vector<int> &b) {
	std::vector<int64_t> v(nslots());
	for (size_t i = 0; i < nslots(); ++i) {
		if (i < b.size())
			v[i] = b[i];
		else
			v[i] = 0;
	}

	lbcrypto::Plaintext plaintext = _context->MakePackedPlaintext(v);
	r = _context->Encrypt(_keyPair.publicKey, plaintext);
}

void PalisadeBfvKeys::encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &r, const std::vector<long> &b) {
	std::vector<int64_t> v(nslots());
	for (size_t i = 0; i < nslots(); ++i) {
		if (i < b.size())
			v[i] = b[i];
		else
			v[i] = 0;
	}

	lbcrypto::Plaintext plaintext = _context->MakePackedPlaintext(v);
	r = _context->Encrypt(_keyPair.publicKey, plaintext);
}


void PalisadeBfvKeys::decrypt(std::vector<long> &out, const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	lbcrypto::Plaintext plaintext;
	_context->Decrypt(_keyPair.secretKey, b, &plaintext);
	std::vector<int64_t> v = plaintext->GetPackedValue();

	out.resize(v.size());

	for (size_t i = 0; i < out.size(); ++i)
		out[i] = v[i];
}

void PalisadeBfvKeys::print(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	long o = decrypt(b);

	std::cout << o << std::endl;
}

void PalisadeBfvKeys::write_to_file(std::ostream &out) {
	assert(false);
}

void PalisadeBfvKeys::read_from_file(std::istream &in) {
	assert(false);
}
