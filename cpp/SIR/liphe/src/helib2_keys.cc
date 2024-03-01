#include <any>

#include "liphe/helib2_keys.h"
#include "liphe/helib2_number.h"

void Helib2Keys::initKeys(const Params &p) {
	_params = p;
	_context = new helib::Context( helib::ContextBuilder<helib::BGV>()
					  .m(p.m)
					  .p(p.p)
					  .r(p.r)
                      .gens(p.gens)
                      .ords(p.ords)
					  .buildModChain(false)
					  .build() );
	_context->buildModChain(p.bits, p.c, p.bootstrappable, 0);
	_context->enableBootStrapping(helib::convert<NTL::Vec<long>>(p.mvec));

	_secretKey = new helib::SecKey(*_context);
	_secretKey->GenSecKey();

	helib::addSome1DMatrices(*_secretKey);
	helib::addFrbMatrices(*_secretKey);
	if (p.bootstrappable)
		_secretKey->genRecryptData();

	_publicKey = _secretKey;
	_ea = &_context->getEA();
}

helib::Ctxt Helib2Keys::encrypt(int b) const {
	helib::Ctxt r(*_publicKey);
	Helib2Keys::encrypt(r, b);
	return r;
}

void Helib2Keys::encrypt(helib::Ctxt &r, int b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	encode(ptxt, b);
	_publicKey->Encrypt(r, ptxt);
}

void Helib2Keys::recrypt(helib::Ctxt &r) const {
	_publicKey->thinReCrypt(r);
}

void Helib2Keys::encode(helib::Ptxt<helib::BGV> &r, long b) const {
	r = helib::Ptxt<helib::BGV>(*_context);
	for (size_t i = 0; i < r.size(); ++i)
		r[i] = b;
}

void Helib2Keys::encode(helib::Ptxt<helib::BGV> &r, const std::vector<long> &b) const {
	r = helib::Ptxt<helib::BGV>(*_context);
	assert(b.size() == r.size());
	for (size_t i = 0; i < r.size(); ++i)
		r[i] = b[i];
}

long Helib2Keys::decrypt(const helib::Ctxt &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	_secretKey->Decrypt(ptxt, b);
	return (long)ptxt[0];
}


helib::Ctxt Helib2Keys::encrypt(const std::vector<int> &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	for (size_t i = 0; i < std::min(ptxt.size(), b.size()); ++i)
		ptxt[i] = b[i];

	helib::Ctxt ctxt(*_publicKey);
	_publicKey->Encrypt(ctxt, ptxt);
	return ctxt;
}

helib::Ctxt Helib2Keys::encrypt(const std::vector<long> &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	for (size_t i = 0; i < std::min(ptxt.size(), b.size()); ++i)
		ptxt[i] = b[i];

	helib::Ctxt ctxt(*_publicKey);
	_publicKey->Encrypt(ctxt, ptxt);
	return ctxt;
}

void Helib2Keys::encrypt(helib::Ctxt &r, const std::vector<int> &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	for (size_t i = 0; i < std::min(ptxt.size(), b.size()); ++i)
		ptxt[i] = b[i];
	_publicKey->Encrypt(r, ptxt);
}

void Helib2Keys::encrypt(helib::Ctxt &r, const std::vector<long> &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	for (size_t i = 0; i < std::min(ptxt.size(), b.size()); ++i)
		ptxt[i] = b[i];
	_publicKey->Encrypt(r, ptxt);
}


void Helib2Keys::decrypt(std::vector<long> &out, const helib::Ctxt &b) const {
	helib::Ptxt<helib::BGV> ptxt(*_context);
	_secretKey->Decrypt(ptxt, b);
	out.resize(ptxt.size());
	for (size_t i = 0; i < std::min(ptxt.size(), ptxt.size()); ++i)
		out[i] = (long)ptxt[i];
}

void Helib2Keys::print(const helib::Ctxt &b) const {
//	PlaintextArray myDecrypt(*_ea);
//	_ea->decrypt(b, *_secretKey, myDecrypt);
//	helib::Ptxt<helib::BGV> myOutput;
//	_ea->encode(myOutput, myDecrypt);
//
//	std::cout << myOutput << std::endl;
	throw std::runtime_error("Not implemented");
}

void Helib2Keys::write_to_file(std::ostream &out) const {
	throw std::runtime_error("Not implemented");
}

void Helib2Keys::read_from_file(std::istream &in) {
	throw std::runtime_error("Not implemented");
}

Helib2Number Helib2Keys::from_vector(const std::vector<long> &v) const {
	return Helib2Number(encrypt(v), this);
}

Helib2Number Helib2Keys::from_int(long v) const {
	return from_vector(std::vector<long>(simd_factor(), v));
}

std::vector<long> Helib2Keys::to_vector(const Helib2Number &c) const {
	std::vector<long> v;
	decrypt(v, c.val());
	return v;
}
