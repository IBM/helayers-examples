#include "liphe/helib_keys.h"

void HelibKeys::initKeys(long s, long R, long p, long r, long d, long c, long k, long w, long L, long chosen_m, const Vec<long> &gens, const Vec<long> &ords) {
//  long R=1;
//  long p=2;
//  long r=1;
//  long d=1;
//  long c=2;
//  long k=80;
//  long L=0;
//  long s=0;
//  long repeat=1;
//  long chosen_m=0;
//  Vec<long> gens;
//  Vec<long> ords;
//
//  long seed=0;

	_p = p;
	_r = r;

	if (L==0) { // determine L based on R,r
		L = 3*R+3;
		if (p>2 || r>1) { // add some more primes for each round
			long addPerRound = 2*ceil(log((double)p)*r*3)/(log(2.0)*FHE_p2Size) +1;
			L += R * addPerRound;
		}
	}
	_m = FindM(k, L, c, p, d, s, chosen_m, true);

	vector<long> gens1, ords1;
	convert(gens1, gens);
	convert(ords1, ords);

	_context = new FHEcontext(_m, p, r, gens1, ords1);
	buildModChain(*_context, L, c);
	_secretKey = new FHESecKey(*_context);
	_secretKey->GenSecKey(w); // A Hamming-weight-w secret key

	ZZX G;

	if (d == 0)
		G = _context->alMod.getFactorsOverZZ()[0];
	else
		G = makeIrredPoly(p, d); 

	cerr << "G = " << G << "\n";
	cerr << "generating key-switching matrices... ";
	addSome1DMatrices(*_secretKey); // compute key-switching matrices that we need
	cerr << "done\n";

//	FHESecKey *shadowPubKey = new FHESecKey(*_context);
//	shadowPubKey->GenSecKey(w);
//	addSome1DMatrices(*shadowPubKey); // compute key-switching matrices that we need
//	_publicKey = shadowPubKey;

	_publicKey = _secretKey;


	cerr << "computing masks and tables for rotation...";
	_ea = new EncryptedArray(*_context, G);

	cerr << "ea.size == " << _ea->size() << std::endl;
	cerr << "done\n";
}

enum mValuesLegend { p = 0, phi_m,  m,    d, m1,  m2, m3,   g1,    g2,    g3,ord1,ord2,ord3, c_m };
static long mValues[][14] = { 
//{ p, phi(m),  m,    d, m1,  m2, m3,   g1,    g2,    g3,ord1,ord2,ord3, c_m}
  {  2,   600,  1023, 10, 11,  93,  0,   838,   584,    0, 10,  6,   0, 100}, // m=(3)*11*{31} m/phim(m)=1.7    C=24  D=2 E=1
  {  2,  1200,  1705, 20, 11, 155,  0,   156,   936,    0, 10,  6,   0, 100}, // m=(5)*11*{31} m/phim(m)=1.42   C=34  D=2 E=2
  {  2,  1728,  4095, 12,  7,  5, 117,  2341,  3277, 3641,  6,  4,   6, 100}, // m=(3^2)*5*7*{13} m/phim(m)=2.36 C=26 D=3 E=2
  {  2,  2304,  4641, 24,  7,  3, 221,  3979,  3095, 3760,  6,  2,  -8, 100}, // m=3*7*(13)*{17} :-( m/phim(m)=2.01 C=45 D=4 E=3
  {  2,  4096,  4369, 16, 17, 257,  0,   258,  4115,    0, 16,-16,   0, 100}, // m=17*(257) :-( m/phim(m)=1.06 C=61 D=3 E=4
  {  2, 12800, 17425, 40, 41, 425,  0,  5951,  8078,    0, 40, -8,   0, 100}, // m=(5^2)*{17}*41 m/phim(m)=1.36 C=93  D=3 E=3
  {  2, 15004, 15709, 22, 23, 683,  0,  4099, 13663,    0, 22, 31,   0, 100}, // m=23*(683) m/phim(m)=1.04      C=73  D=2 E=1
  {  2, 16384, 21845, 16, 17,   5,257,  8996, 17477, 21591, 16, 4, -16,1600}, // m=5*17*(257) :-( m/phim(m)=1.33 C=65 D=4 E=4
  {  2, 18000, 18631, 25, 31, 601,  0, 15627,  1334,    0, 30, 24,   0,  50}, // m=31*(601) m/phim(m)=1.03      C=77  D=2 E=0
  {  2, 18816, 24295, 28, 43, 565,  0, 16386, 16427,    0, 42, 16,   0, 100}, // m=(5)*43*{113} m/phim(m)=1.29  C=84  D=2 E=2
  {  2, 21168, 27305, 28, 43, 635,  0, 10796, 26059,    0, 42, 18,   0, 100}, // m=(5)*43*{127} m/phim(m)=1.28  C=86  D=2 E=2
  {  2, 23040, 28679, 24, 17,  7, 241, 15184,  4098,28204, 16,  6, -10,1000}, // m=7*17*(241) m/phim(m)=1.24    C=63  D=4 E=3
  {  2, 24000, 31775, 20, 41, 775,  0,  6976, 24806,    0, 40, 30,   0, 100}, // m=(5^2)*{31}*41 m/phim(m)=1.32 C=88  D=2 E=2
  {  2, 26400, 27311, 55, 31, 881,  0, 21145,  1830,    0, 30, 16,   0, 100}, // m=31*(881) m/phim(m)=1.03      C=99  D=2 E=0
  {  2, 31104, 35113, 36, 37, 949,  0, 16134,  8548,    0, 36, 24,   0, 400}, // m=(13)*37*{73} m/phim(m)=1.12  C=94  D=2 E=2
  {  2, 34848, 45655, 44, 23, 1985, 0, 33746, 27831,    0, 22, 36,   0, 100}, // m=(5)*23*{397} m/phim(m)=1.31  C=100 D=2 E=2
  {  2, 42336, 42799, 21, 127, 337, 0, 25276, 40133,    0,126, 16,   0,  20}, // m=127*(337) m/phim(m)=1.01     C=161 D=2 E=0
  {  2, 45360, 46063, 45, 73, 631,  0, 35337, 20222,    0, 72, 14,   0, 100}, // m=73*(631) m/phim(m)=1.01      C=129 D=2 E=0
  {  2, 46080, 53261, 24, 17, 13, 241, 43863, 28680,15913, 16, 12, -10, 100}, // m=13*17*(241) m/phim(m)=1.15   C=69  D=4 E=3
  {  2, 49500, 49981, 30, 151, 331, 0,  6952, 28540,    0,150, 11,   0, 100}, // m=151*(331) m/phim(m)=1        C=189 D=2 E=1
  {  2, 54000, 55831, 25, 31, 1801, 0, 19812, 50593,    0, 30, 72,   0, 100}, // m=31*(1801) m/phim(m)=1.03     C=125 D=2 E=0
  {  2, 60016, 60787, 22, 89, 683,  0,  2050, 58741,    0, 88, 31,   0, 200}, // m=89*(683) m/phim(m)=1.01      C=139 D=2 E=1

  { 17,   576,  1365, 12,  7,   3, 65,   976,   911,  463,  6,  2,   4, 100}, // m=3*(5)*7*{13} m/phim(m)=2.36  C=22  D=3
  { 17, 18000, 21917, 30, 101, 217, 0,  5860,  5455,    0, 100, 6,   0, 100}, // m=(7)*{31}*101 m/phim(m)=1.21  C=134 D=2 
  { 17, 30000, 34441, 30, 101, 341, 0,  2729, 31715,    0, 100, 10,  0, 100}, // m=(11)*{31}*101 m/phim(m)=1.14 C=138 D=2
  { 17, 40000, 45551, 40, 101, 451, 0, 19394,  7677,    0, 100, 10,  0,2000}, // m=(11)*{41}*101 m/phim(m)=1.13 C=148 D=2
  { 17, 46656, 52429, 36, 109, 481, 0, 46658,  5778,    0, 108, 12,  0, 100}, // m=(13)*{37}*109 m/phim(m)=1.12 C=154 D=2
  { 17, 54208, 59363, 44, 23, 2581, 0, 25811,  5199,    0, 22, 56,   0, 100}, // m=23*(29)*{89} m/phim(m)=1.09  C=120 D=2
  { 17, 70000, 78881, 10, 101, 781, 0, 67167, 58581,    0, 100, 70,  0, 100}, // m=(11)*{71}*101 m/phim(m)=1.12 C=178 D=2

  {127,   576,  1365, 12,  7,   3, 65,   976,   911,  463,  6,  2,   4, 100}, // m=3*(5)*7*{13} m/phim(m)=2.36   C=22  D=3
  {127,  1200,  1925, 20,  11, 175, 0,  1751,   199,    0, 10, 6,    0, 100}, //  m=(5^2)*{7}*11 m/phim(m)=1.6   C=34 D=2
  {127,  2160,  2821, 30,  13, 217, 0,   652,   222,    0, 12, 6,    0, 100}, // m=(7)*13*{31} m/phim(m)=1.3     C=46 D=2
  {127, 18816, 24295, 28, 43, 565,  0, 16386, 16427,    0, 42, 16,   0, 100}, // m=(5)*43*{113} m/phim(m)=1.29   C=84  D=2
  {127, 26112, 30277, 24, 17, 1781, 0, 14249, 10694,    0, 16, 68,   0, 100}, // m=(13)*17*{137} m/phim(m)=1.15  C=106 D=2
  {127, 31752, 32551, 14, 43,  757, 0,  7571, 28768,    0, 42, 54,   0, 100}, // m=43*(757) :-( m/phim(m)=1.02   C=161 D=3
  {127, 46656, 51319, 36, 37, 1387, 0, 48546, 24976,    0, 36, -36,  0, 200}, //m=(19)*37*{73}:-( m/phim(m)=1.09 C=141 D=3
  {127, 49392, 61103, 28, 43, 1421, 0,  1422, 14234,    0, 42, 42,   0,4000}, // m=(7^2)*{29}*43 m/phim(m)=1.23  C=110 D=2
  {127, 54400, 61787, 40, 41, 1507, 0, 30141, 46782,    0, 40, 34,   0, 100}, // m=(11)*41*{137} m/phim(m)=1.13  C=112 D=2
  {127, 72000, 77531, 30, 61, 1271, 0,  7627, 34344,    0, 60, 40,   0, 100}  // m=(31)*{41}*61 m/phim(m)=1.07   C=128 D=2
};
#define num_mValues (sizeof(mValues)/(13*sizeof(long)))

void HelibKeys::initKeys_with_recrypt(long s, long p, long r, long d, long c, long k, long w, long L, long chosen_m) {
////  long p=2;
////  long r=1;
////  long d=1;
////  long c=2;
////  long k=80;
////  long L=0;
////  long s=0;
////  long repeat=1;
////  long chosen_m=0;
////  Vec<long> gens;
////  Vec<long> ords;
////
////  long seed=0;
//
//	_p = p;
//	_r = r;
//
//	long m = FindM(k, L, c, p, d, s, chosen_m, true);
//
//	int best_line = -1;
//	for (int i = 0; i < num_mValues; ++i) {
//		if (mValues[i][mValuesLegend::m] > m) {
//			if ((best_line == -1) || (mValues[best_line][mValuesLegend::m] > mValues[i][mValuesLegend::m]))
//				best_line = i;
//		}
//	}
//	if (best_line == -1) {
//		std::cerr << "I do not know how to choose parameters for m=" << m << std::endl;
//		exit(1);
//	}
//
//	vector<long> gens1, ords1;
//	Vec<long> mvec;
//
//	append(mvec, mValues[idx][4]);
//	if (mValues[idx][5]>1) append(mvec, mValues[idx][5]);
//	if (mValues[idx][6]>1) append(mvec, mValues[idx][6]);
//	gens.push_back(mValues[idx][7]);
//	if (mValues[idx][8]>1) gens.push_back(mValues[idx][8]);
//	if (mValues[idx][9]>1) gens.push_back(mValues[idx][9]);
//	ords.push_back(mValues[idx][10]);
//	if (abs(mValues[idx][11])>1) ords.push_back(mValues[idx][11]);
//	if (abs(mValues[idx][12])>1) ords.push_back(mValues[idx][12]);
//
//	setDryRun(false);
//	_context = new FHEcontext(m, p, r, gens, ords);
//	_context->bitsPerLevel = 23; // (B)
//	buildModChain(*_context, L, c);
//	_context->makeBootstrappable(mvec, /*t=*/0, false);
//	_context->rcData.skHwt = 0;
//	setDryRun(true);
//
//	_secretKey = new FHESecKey(*_context);
//	_secretKey->GenSecKey(w); // A Hamming-weight-w secret key
//
//	ZZX G;
//
//	if (d == 0)
//		G = _context->alMod.getFactorsOverZZ()[0];
//	else
//		G = makeIrredPoly(p, d); 
//
//	cerr << "G = " << G << "\n";
//	cerr << "generating key-switching matrices... ";
//	addSome1DMatrices(*_secretKey); // compute key-switching matrices that we need
//	cerr << "done\n";
//
////	FHESecKey *shadowPubKey = new FHESecKey(*_context);
////	shadowPubKey->GenSecKey(w);
////	addSome1DMatrices(*shadowPubKey); // compute key-switching matrices that we need
////	_publicKey = shadowPubKey;
//
//	_publicKey = _secretKey;
//
//
//	cerr << "computing masks and tables for rotation...";
//	_ea = new EncryptedArray(*_context, G);
//
//	cerr << "ea.size == " << _ea->size() << std::endl;
//	cerr << "done\n";
}

Ctxt HelibKeys::encrypt(int b) {
	Ctxt r(*_publicKey);
	HelibKeys::encrypt(r, b);
	return r;
}

void HelibKeys::encrypt(Ctxt &r, int b) {
//	NewPlaintextArray myPlain(*_ea);
//	_ea->decode(myPlain, ZZX(b));
//	_ea->encrypt(r, *_publicKey, myPlain);

	std::vector<long> _b(nslots(), b);
	_ea->encrypt(r, *_publicKey, _b);
}

void HelibKeys::encode(ZZX &r, long b) {
	std::vector<long> _b(nslots(), b);
	encode(r, _b);
}

void HelibKeys::encode(ZZX &r, const std::vector<long> &b) {
	_ea->encode(r, b);
}

long HelibKeys::decrypt(const Ctxt &b) {
	std::vector<long> out;
	_ea->decrypt(b, *_secretKey, out);
	return out[0];

//	NewPlaintextArray myDecrypt(*_ea);
//	_ea->decrypt(b, *_secretKey, myDecrypt);
//	ZZX myOutput;
//	_ea->encode(myOutput, myDecrypt);
//
//	if (myOutput == ZZX(0))
//		return 0;
//
//	return to_long(myOutput[0]);
}




Ctxt HelibKeys::encrypt(const std::vector<int> &b) {
	std::vector<long> _b(b.size());
	for (unsigned int i = 0; i < b.size(); ++i)
		_b[i] = b[i];
	return encrypt(_b);
}

Ctxt HelibKeys::encrypt(const std::vector<long> &b) {
	Ctxt r(*_publicKey);
	std::vector<long> _b(b);
	_b.resize(_ea->size(), 0);
	_ea->encrypt(r, *_publicKey, _b);
	return r;
}

void HelibKeys::encrypt(Ctxt &r, const std::vector<int> &b) {
	std::vector<long> _b(b.size());
	for (unsigned int i = 0; i < b.size(); ++i)
		_b[i] = b[i];
	encrypt(r, _b);
}

void HelibKeys::encrypt(Ctxt &r, const std::vector<long> &b) {
	std::vector<long> _b(b);
	_b.resize(_ea->size(), 0);
	_ea->encrypt(r, *_publicKey, _b);
}


void HelibKeys::decrypt(std::vector<long> &out, const Ctxt &b) {
	_ea->decrypt(b, *_secretKey, out);
}

void HelibKeys::print(const Ctxt &b) {
	NewPlaintextArray myDecrypt(*_ea);
	_ea->decrypt(b, *_secretKey, myDecrypt);
	ZZX myOutput;
	_ea->encode(myOutput, myDecrypt);

	std::cout << myOutput << std::endl;
//	if (myOutput == ZZX(0))
//		return 0;
//
//	return to_long(myOutput[0]);
}

void HelibKeys::write_to_file(std::ostream &out) {
	writeContextBase(out, *_context);
	out << *_context << endl;
	out << *_secretKey << endl;
}

void HelibKeys::read_from_file(std::istream &in) {
	unsigned long m;
	std::vector<long> gens, ords;

	unsigned long p, r;

	readContextBase(in, m, p, r, gens, ords);
	_p = p; _r = r;
	_context = new FHEcontext(m, p, r, gens, ords);
	in >> *_context;

	_secretKey = new FHESecKey(*_context);
	in >> *_secretKey;

	_publicKey = _secretKey;

//	ZZX G;
//	G = makeIrredPoly(p, 1); 
	_ea = new EncryptedArray(*_context);
}
