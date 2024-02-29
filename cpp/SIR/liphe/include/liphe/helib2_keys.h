#ifndef __HELIB2_KEYS__
#define __HELIB2_KEYS__

#include <any>

#include <helib/helib.h>

class Helib2Number;

class Helib2Keys {
public:
	struct Params {
		// plaintext base
		long p;

		// Hensel lifting
		long r;

		// cyclotomic polynomial - defines phi(m)
		long m;

		// Number of bits of the modulus chain.
		long bits = 500;

		// Number of columns of Key-Switching matrix (typically 2 or 3).
		long c = 2;

		// Factorisation of m required for bootstrapping.
		//std::vector<long> mvec = {7, 5, 9, 13};
		std::vector<long> mvec;

		// Generating set of Zm* group.
		//std::vector<long> gens = {2341, 3277, 911};
		std::vector<long> gens;

		// Orders of the previous generators.
		//std::vector<long> ords = {6, 4, 6};
		std::vector<long> ords;

		bool bootstrappable = false;



		Params() {}
		Params(const Params &p) { operator=(p); }

		Params &operator=(const Params &prm) {
			p = prm.p;
			r = prm.r;
			m = prm.m;
			bits = prm.bits;
			c = prm.c;
			mvec = prm.mvec;
			gens = prm.gens;
			ords = prm.ords;
			bootstrappable = prm.bootstrappable;

			return *this;
		}
	};

private:
	helib::PubKey *_publicKey;
	helib::SecKey *_secretKey;
	const helib::EncryptedArray *_ea;
	helib::Context *_context;

	Params _params;
public:
	Helib2Keys() : _publicKey(NULL), _secretKey(NULL), _ea(NULL), _context(NULL)  {}
	Helib2Keys(const Helib2Keys &h) : _publicKey(h._publicKey), _secretKey(h._secretKey), _ea(h._ea), _context(h._context), _params(h._params) {}
	Helib2Keys(helib::PubKey *pub, helib::SecKey *sec, helib::EncryptedArray *ea, helib::Context *ctx)  { setKeys(pub, sec, ea, ctx); }

	void initKeys(const Params &p);

	void setKeys(helib::PubKey *pub, helib::SecKey *sec, helib::EncryptedArray *ea, helib::Context *ctx)
		{ _publicKey = pub; _secretKey = sec; _ea = ea; _context = ctx; }

	long &p() { return _params.p; }
	long &r() { return _params.r; }
	long &m() { return _params.m; }

	long p() const { return _params.p; }
	long r() const { return _params.r; }
	long m() const { return _params.m; }

	long get_ring_size() const {
		long ret = 1;
		for (int i = 0; i < r(); ++i)
			ret *= p();
		return ret;
	}

	long securityLevel() const { return _context->securityLevel(); }

	int nslots() const { return _ea->size(); }
	unsigned long simd_factor() const { return nslots(); }

	helib::PubKey &publicKey() { return *_publicKey; }
	const helib::PubKey &publicKey() const { return *_publicKey; }
	const helib::EncryptedArray &ea() { return *_ea; }



	helib::Ctxt encrypt(int b) const;
	void encrypt(helib::Ctxt &c, int b) const;
	void recrypt(helib::Ctxt &r) const;
	long decrypt(const helib::Ctxt &b) const;

	helib::Ptxt<helib::BGV> encode(long z) const;
	helib::Ptxt<helib::BGV> encode(std::vector<long> &z) const;

	helib::Ctxt encrypt(const std::vector<long> &b) const;
	helib::Ctxt encrypt(const std::vector<int> &b) const;
	void encrypt(helib::Ctxt &c, const std::vector<long> &b) const;
	void encrypt(helib::Ctxt &c, const std::vector<int> &b) const;
	void decrypt(std::vector<long> &, const helib::Ctxt &b) const;

	Helib2Number from_vector(const std::vector<long> &v) const;
	Helib2Number from_int(long v) const;
	std::vector<long> to_vector(const Helib2Number &v) const;
	long to_int(const Helib2Number &v) const { return to_vector(v)[0]; }

	void encode(helib::Ptxt<helib::BGV> &r, long b) const;
	void encode(helib::Ptxt<helib::BGV> &r, const std::vector<long> &b) const;

	void write_to_file(std::ostream& s) const;
	void read_from_file(std::istream& s);

	void print(const helib::Ctxt &b) const;

	void rotate(helib::Ctxt &a, int step) const { _ea->rotate(a, step); }
	void shift(helib::Ctxt &a, int step) const { _ea->shift(a, step); }
};

#endif
