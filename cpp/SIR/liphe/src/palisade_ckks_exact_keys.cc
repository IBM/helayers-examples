#include <assert.h>
#include "liphe/palisade_ckks_exact_keys.h"

lbcrypto::Ciphertext<lbcrypto::DCRTPoly> PalisadeCkksExactKeys::encrypt(double b) {
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> r;
	encrypt(r, b);
	return r;
}

void PalisadeCkksExactKeys::encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &r, double b) {
	std::vector<std::complex<double>> v(nslots(), std::complex<double>(b,0) );
	lbcrypto::Plaintext plaintext = _context->MakeCKKSPackedPlaintext(v);
	r = _context->Encrypt(_keyPair.publicKey, plaintext);
}

lbcrypto::Plaintext PalisadeCkksExactKeys::encode(double _b) {
	std::vector<std::complex<double>> b(nslots(), std::complex<double>(_b,0) );
	lbcrypto::Plaintext r = _context->MakeCKKSPackedPlaintext(b);
	return r;
}

lbcrypto::Plaintext PalisadeCkksExactKeys::encode(const std::vector<double> &_b) {
	std::vector<std::complex<double>> b(_b.size());
	for (size_t i = 0; i< _b.size(); ++i)
		b[i] = std::complex<double>(_b[i], 0);
	b.resize(nslots());
	lbcrypto::Plaintext r = _context->MakeCKKSPackedPlaintext(b);
	return r;
}

double PalisadeCkksExactKeys::decrypt(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	lbcrypto::Plaintext plaintext;
	_context->Decrypt(_keyPair.secretKey, b, &plaintext);
	std::vector<std::complex<double>> out = plaintext->GetCKKSPackedValue();

	return out[0].real();
}




lbcrypto::Ciphertext<lbcrypto::DCRTPoly> PalisadeCkksExactKeys::encrypt(const std::vector<double> &b) {
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> r;
	encrypt(r, b);
	return r;
}

void PalisadeCkksExactKeys::encrypt(lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &r, const std::vector<double> &b) {
	std::vector<std::complex<double>> v(nslots());
	for (size_t i = 0; i < nslots(); ++i) {
		if (i < b.size())
			v[i] = std::complex<double>(b[i], 0);
		else
			v[i] = std::complex<double>(0,0);
	}

	lbcrypto::Plaintext plaintext = _context->MakeCKKSPackedPlaintext(v);
	r = _context->Encrypt(_keyPair.publicKey, plaintext);
}


void PalisadeCkksExactKeys::decrypt(std::vector<double> &out, const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	lbcrypto::Plaintext plaintext;
	_context->Decrypt(_keyPair.secretKey, b, &plaintext);
	std::vector<std::complex<double>> v = plaintext->GetCKKSPackedValue();

	out.resize(v.size());

	for (size_t i = 0; i < out.size(); ++i)
		out[i] = v[i].real();
}

void PalisadeCkksExactKeys::print(const lbcrypto::Ciphertext<lbcrypto::DCRTPoly> &b) {
	long o = decrypt(b);

	std::cout << o << std::endl;
}

void PalisadeCkksExactKeys::write_to_file(std::ostream &out) {
	assert(false);
}

void PalisadeCkksExactKeys::read_from_file(std::istream &in) {
	assert(false);
}
