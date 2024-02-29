//  abs_bits_protocol.h
//
//   Created on: Oct 12, 2020
//       Author: hayim

#ifndef ABS_BITS_PROTOCOL_H_
#define ABS_BITS_PROTOCOL_H_

//#include <liphe/unsigned_word.h>

#define PrivKey typename Types::PrivKey
#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey

#define PrivBinaryKey typename Types::PrivBinaryKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryWordPlaintext typename Types::BinaryWordPlaintext
#define BinaryWordCiphertext typename Types::BinaryWordCiphertext
#define BinaryBitPlaintext typename Types::BinaryBitPlaintext
#define BinaryBitCiphertext typename Types::BinaryBitCiphertext
#define PackedBitUnsignedWord typename Types::PackedBitUnsignedWord

cpp_int draw_big_int(cpp_int mod) {
	size_t modBits = 0;
	while ((cpp_int(1) << modBits) < mod)
		++modBits;

	while (1) {
		cpp_int ret(0);
		for (size_t i = 0; i < modBits; ++i) {
			ret *= 2;
			if ((rand() % 2) == 1)
				ret += 1;
		}
		if (ret < mod)
			return ret;
	}
}

template<class Types>
void Server1<Types>::binAbs_mask_and_send_to_server2() {
	_rBinAbs.resize(_z.size());

#	ifdef __DEBUG
	{
		std::vector<cpp_int> zPlaintext;
		_z.to_bigint_vector(zPlaintext, _pubKey);
		int Q = _pubKey->get_ring_size();
		for (size_t i = 0; i< zPlaintext.size(); ++i) {
			if (zPlaintext[i] < Q/2) {
				std::cout << "z[" << i << "] = " << zPlaintext[i] << std::endl;
			} else {
				std::cout << "z[" << i << "] = " << zPlaintext[i] << "(-" << (Q-zPlaintext[i]) << ")" << std::endl;
			}
		}
	}
#	endif


	// make sure the LSB of _z and r are 0.
	// We do that by multiplying _z by 2, and by subtracting (r mod 2) from r

	cpp_int ringSize = _pubKey->get_ring_size();
	size_t ringSizeBits = 0;
	while ((cpp_int(1) << ringSizeBits) < ringSize)
		++ringSizeBits;

	std::cout << "ringSize = " << ringSize << "   bits = " << ringSizeBits << std::endl;

	_rBinAbs.resize(_z.size());
	for (size_t i = 0; i < _rBinAbs.size(); ++i) {
		_rBinAbs[i] = draw_big_int(ringSize);
	}

	printTimeStamp("after drawing big ints");

	PackedVector<Plaintext> rBinAbsPacked(_rBinAbs, _plainKey);

	PackedVector<Ciphertext> zprime;

	zprime = _z;
	zprime += rBinAbsPacked;

	printTimeStamp("after computing zprime");

#	ifdef __DEBUG
	std::cout << "r = " << std::endl;
	for (const auto &a : _rBinAbs) {
		std::cout << a << ", ";
	}
	std::cout << std::endl << std::endl;
	std::cout << "z' = zprime = " << std::endl << TO_STRING(zprime) << std::endl << std::endl;
#	endif

	_communication_channel->send_zprime_to_server2(zprime, _feature_list);
}


template<class Types>
void Server2<Types>::binarize() const {
	std::vector<cpp_int> zPrimePlaintext;
	{
	TimeGuard g("binarize - decrypting");
	_zPrime.to_bigint_vector(zPrimePlaintext, _privKey);
	}

	cpp_int Q = _privKey->get_ring_size();
	cpp_int N = 2;
	int logN = 1;
	while (N < Q) {
		++logN;
		N *= 2;
	}
	N *= 2;
	++logN;

	std::vector<cpp_int> thresholdPlaintext(zPrimePlaintext.size());
	for (size_t i = 0; i < thresholdPlaintext.size(); ++i) {
		if (zPrimePlaintext[i] > Q/2) {
			thresholdPlaintext[i] = zPrimePlaintext[i] - Q/2 - 1;
			zPrimePlaintext[i] = N + zPrimePlaintext[i] - Q;
		} else
			thresholdPlaintext[i] = zPrimePlaintext[i] + Q/2;
	}

	std::vector<PackedBitUnsignedWord> zPrimeBits;
	std::vector<PackedBitUnsignedWord> thresholdBits;
	PackedBitUnsignedWord zero(0, *_pubBinaryKey);

	for (unsigned int i = 0; i < zPrimePlaintext.size(); ++i) {
		zPrimeBits.push_back(zero);
		thresholdBits.push_back(zero);
	}

	{
	TimeGuard g("binarize - encrypt");
#	pragma omp parallel for
	for (unsigned int i = 0; i < zPrimePlaintext.size(); ++i) {
		if (_F[i] == 0)
			continue;
		// Pad zPrimePlaintext with 0 by multiplying them by 2.
		// We need that to perrofm sub more effifiently. See more in the PADDING remark
		zPrimeBits[i] = PackedBitUnsignedWord(zPrimePlaintext[i]*2, *_pubBinaryKey, logN+1);
		thresholdBits[i] = PackedBitUnsignedWord(thresholdPlaintext[i], *_pubBinaryKey, logN+1);
	}
	}

	_communication_channel->send_zPrimeBits_to_server1(zPrimeBits, thresholdBits);
}

// We filled *filled* bits and still need to multiply the 1s by *times* times
template<class Types>
void Server1<Types>::recursiveSmearLsb(BinaryBitCiphertext &b, int &times, int &filled) const {
	if (times <= 1)
		return;

	// int flags = Types::PackedBitUnsignedWord::NO_MASK;
	// if (2*filled > _pubBinaryKey->simd_factor())
	// 	flags = 0;

	if ((times % 2) == 0) {
		BinaryBitCiphertext temp = b;
		temp.self_rotate_left(filled);
		b += temp;
		times /= 2;
		filled *= 2;
		recursiveSmearLsb(b, times, filled);
	} else {
		BinaryBitCiphertext temp = b;
		int tempFilled = filled;
		times -= 1;
		recursiveSmearLsb(b, times, filled);
		temp.self_rotate_left(filled);
		filled += tempFilled;
		b += temp;
	}
}

template<class Types>
BinaryBitCiphertext Server1<Types>::smearLsb(const BinaryBitCiphertext &b) const {
	std::vector<long> m(_pubBinaryKey->simd_factor(), 0);
	m[0] = 1;
	BinaryBitCiphertext mEnc = _pubBinaryKey->from_vector(m);
	mEnc *= b;
	int times = _pubBinaryKey->simd_factor();
	int filled = 1;
	recursiveSmearLsb(mEnc, times, filled);
	return mEnc;
}

template<class Types>
void Server1<Types>::binAbs_unmask() {

	std::vector<cpp_int> Q_mask(_rBinAbs.size());
	cpp_int Q = _pubKey->get_ring_size();
	cpp_int N = cpp_int(2);
	while (N < Q)
		N *= 2;
	N *= 2;


	for (unsigned int i = 0; i < _rBinAbs.size(); ++i) {
		PackedBitUnsignedWord zero(*_pubBinaryKey);
		_zBin.push_back(zero);
	}

#	pragma omp parallel for
	for (unsigned int i = 0; i < _rBinAbs.size(); ++i) {
		if (_feature_list[i] == 0) {
			continue;
		}

#		ifdef __DEBUG
		// std::cout << "zBin'[" << i << "] = " << _zBinMasked[i].to_string(*_pubBinaryKey) << std::endl;
#		endif

		BinaryBitCiphertext isWrap;
		BinaryBitCiphertext isNoWrap;

		{
			TimeGuard g("binarize - compute isWrap");
			BinaryBitCiphertext eq;
			BinaryBitCiphertext smaller;
			PackedBitUnsignedWord maskEnc(_rBinAbs[i], *_pubBinaryKey);
			_maskThreshold[i].genericCmp(maskEnc, eq, smaller);
			isWrap = (-eq + 1)*smaller;
			// isWrap.recrypt();
			isNoWrap = -isWrap;
			isNoWrap += 1;
		}


		PackedBitUnsignedWord zOnesComplement = _zBinMasked[i];
		zOnesComplement.onesComplement();

		PackedBitUnsignedWord z1(*_pubBinaryKey);
		PackedBitUnsignedWord z2(*_pubBinaryKey);
		PackedBitUnsignedWord z3(*_pubBinaryKey);
		PackedBitUnsignedWord z4(*_pubBinaryKey);

#		pragma omp parallel for
		for (int z = 1; z <= 4; ++z) {
			if (z == 1)
			{
				TimeGuard g("binarize - compute z1");
				// we need to pad _r by 0 (by multiplying by 2) because _zBinMasked is also padded.
				// The padding allows us to compute r-z mroe efficiently. See PADDING remark
				cpp_int maskMinus = (N - _rBinAbs[i]) * 2;
				PackedBitUnsignedWord maskMinusEnc(maskMinus, *_pubBinaryKey);
				z1.add(_zBinMasked[i], maskMinusEnc, z1.dontGrow());
				// z1.recrypt();
			}

			// (PADDING remark) We assume all _z are padded with 0. With this assumption we can compute _r-_z more easily (with +/-1 difference).
			// specifically then we have:
			// _r+1+onesComplement(_z) = _r-_z +/- 1
			if (z == 2)
			{
				TimeGuard g("binarize - compute z2");
				cpp_int mask = _rBinAbs[i]*2 + 1;
				PackedBitUnsignedWord maskEnc(mask, *_pubBinaryKey);
				z2.add(zOnesComplement, maskEnc, z1.dontGrow());
				// z2.recrypt();
			}

			if (z == 3)
			{
				TimeGuard g("binarize - compute z3");
				// See padding remarks as above
				cpp_int mask = (Q - _rBinAbs[i]) * 2;
				PackedBitUnsignedWord maskEnc(mask, *_pubBinaryKey);
				z3.add(_zBinMasked[i], maskEnc, z1.dontGrow());
				// z3.recrypt();
			}

			if (z == 4)
			{
				TimeGuard g("binarize - compute z4");
				// See padding remarks as above
				// we also need to add 1 to make it into 2s complement
				cpp_int maskMinus = (N - Q + _rBinAbs[i])*2 + 1;
				PackedBitUnsignedWord maskMinusEnc(maskMinus, *_pubBinaryKey);
				z4.add(zOnesComplement, maskMinusEnc, z1.dontGrow());
				// z4.recrypt();
			}
		}

#		ifdef __DEBUG
		// std::cout << "z1 = " << z1.to_string(*_pubBinaryKey) << " = " << z1.toUnsignedInt(*_pubBinaryKey) << std::endl;
		// std::cout << "z2 = " << z2.to_string(*_pubBinaryKey) << " = " << z2.toUnsignedInt(*_pubBinaryKey) << std::endl;
		// std::cout << "z3 = " << z3.to_string(*_pubBinaryKey) << " = " << z3.toUnsignedInt(*_pubBinaryKey) << std::endl;
		// std::cout << "z4 = " << z4.to_string(*_pubBinaryKey) << " = " << z4.toUnsignedInt(*_pubBinaryKey) << std::endl;
		// std::vector<long> isWrapPlaintext = isWrap.to_vector();
		// std::vector<long> isNoWrapPlaintext = isNoWrap.to_vector();
		// std::vector<long> z1Msb = z1.msb().to_vector();
		// std::vector<long> z2Msb = z2.msb().to_vector();
		// std::vector<long> z3Msb = z3.msb().to_vector();
		// std::vector<long> z4Msb = z4.msb().to_vector();

		// std::cout << "isWrap = " << isWrapPlaintext[0] << isWrapPlaintext[1] << isWrapPlaintext[2] << isWrapPlaintext[3] << std::endl;
		// std::cout << "isNoWrap = " << isNoWrapPlaintext[0] << isNoWrapPlaintext[1] << isNoWrapPlaintext[2] << isNoWrapPlaintext[3] << std::endl;
		// std::cout << "z1Msb = " << z1Msb[0] << z1Msb[1] << z1Msb[2] << z1Msb[3] << std::endl;
		// std::cout << "z2Msb = " << z2Msb[0] << z2Msb[1] << z2Msb[2] << z2Msb[3] << std::endl;
		// std::cout << "z3Msb = " << z3Msb[0] << z3Msb[1] << z3Msb[2] << z3Msb[3] << std::endl;
		// std::cout << "z4Msb = " << z4Msb[0] << z4Msb[1] << z4Msb[2] << z4Msb[3] << std::endl;
#		endif

		{
			TimeGuard g("binarize - compute zBin");
			_zBin[i] =  z1.multiplyAllCtxts(smearLsb((-z1.msb()+1)*(isNoWrap))) ^
					z2.multiplyAllCtxts(smearLsb((-z2.msb()+1)*(isNoWrap))) ^
					z3.multiplyAllCtxts(smearLsb((-z3.msb()+1)*(isWrap))) ^
					z4.multiplyAllCtxts(smearLsb((-z4.msb()+1)*(isWrap))) ;

			_zBin[i].recrypt();
		}

#		ifdef __DEBUG
			std::cout << "|zBin[" << i << "]| = " << _zBin[i].toUnsignedInt(*_pubBinaryKey) << std::endl;
#		endif

	}
}

// A protocol to encode the each element in the vector z as a vector of bits.
// Input: z - a packed vector
// Output -
template<class Types>
void Server1<Types>::binAbs_z() {
	{
		TimeGuard g("binarize - mask");
		binAbs_mask_and_send_to_server2();
	}
	{
		TimeGuard g("binarize - binarization");
		_communication_channel->server2_binarize();
	}
	{
		TimeGuard g("binarize - unmask");
		binAbs_unmask();
	}
}

#undef PrivKey
#undef PubKey
#undef PlainKey

#undef PrivBinaryKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryWordPlaintext
#undef BinaryWordCiphertext
#undef BinaryBitPlaintext
#undef BinaryBitCiphertext

#undef PackedBitUnsignedWord

#endif /* ABS_BITS_PROTOCOL_H_ */
