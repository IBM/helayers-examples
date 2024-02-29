#ifndef ___PACKED_BIT_ARRAY___
#define ___PACKED_BIT_ARRAY___

// Author: Hayim Shaul 2021.

// PackedBitArray.
// implements a number by its bit representation. Bit can be either ZP or HELibNumber, with p=2 or with bigger p.
// PackedBitArray is implemented to have a variable number of bits. The bits are packed in the slot of one or more ctxt.
//
// We also provide operations such as add/sub/cmp etc.


#include <vector>
#include <functional>
#include <stdexcept>
#include <string>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;


#include "utils.h"

template<class Bit, class BitKey>
class PackedBitArray {
private:
    const BitKey &_keys;
    int _bitSize;
    std::vector<Bit> _ctxt;

    void set(const boost::multiprecision::cpp_int &a, int minBits = -1);
    void genericCmpOneCtxt(const Bit &a, const Bit &b, Bit &eq, Bit &smaller, size_t bits);
    void isAllZeroes(Bit &out, const Bit &in);
public:
    enum {
        NO_MASK = 1
    };

    PackedBitArray(const BitKey &keys) : _keys(keys) { set(0); }
    PackedBitArray(long a, const BitKey &keys, int minBits = -1) : _keys(keys) { set(a, minBits); }
    PackedBitArray(const boost::multiprecision::cpp_int &a, const BitKey &keys, int minBits = -1) : _keys(keys) { set(a, minBits); }
    PackedBitArray(const PackedBitArray<Bit, BitKey> &a) : _keys(a._keys), _bitSize(a._bitSize), _ctxt(a._ctxt) {}
    PackedBitArray(const PackedBitArray<Bit, BitKey> &&a) : _keys(a._keys), _bitSize(a._bitSize), _ctxt(a._ctxt) {}

    void recrypt() {
        for (size_t i = 0; i < _ctxt.size(); ++i)
            _ctxt[i].recrypt();
    }

    const PackedBitArray<Bit, BitKey> &operator=(const PackedBitArray<Bit, BitKey> &p) { assert(&_keys == &p._keys); _bitSize=p._bitSize; _ctxt=p._ctxt; return *this; }
    const PackedBitArray<Bit, BitKey> &operator=(const PackedBitArray<Bit, BitKey> &&p) {
        assert(&_keys == &p._keys);
        _bitSize=p._bitSize;
        _ctxt=p._ctxt;
        return *this;
    }


    enum { DONT_GROW = 1 };
    int dontGrow() const { return DONT_GROW; }
    /// @brief set this=a+b 
    void add(const PackedBitArray<Bit, BitKey> &a, const PackedBitArray<Bit, BitKey> &b, int flags = 0);

    /// @brief set to the one's complement
    void onesComplement();

    /// @brief compare this with 'a'
    /// @param a what to compare with
    /// @param eq is set to 1 if this=a, 0 otherwise
    /// @param smaller if values are equal (*this==a) it is undefined, otherwise is set to 1 if *this<a, 0 otherwise
    void genericCmp(const PackedBitArray<Bit, BitKey> &a, Bit &eq, Bit &smaller, int from = 0, int to = -1);

    PackedBitArray<Bit, BitKey> operator^(const PackedBitArray<Bit,BitKey> &a) const {
        PackedBitArray<Bit, BitKey> ret(*this);
        ret ^= a;
        return ret;
    }
    PackedBitArray<Bit, BitKey> operator&(const PackedBitArray<Bit,BitKey> &a) const {
        PackedBitArray<Bit, BitKey> ret(*this);
        ret &= a;
        return ret;
    }
    PackedBitArray<Bit, BitKey> operator|(const PackedBitArray<Bit,BitKey> &a) const {
        PackedBitArray<Bit, BitKey> ret(*this);
        ret |= a;
        return ret;
    }

    void operator^=(const PackedBitArray<Bit,BitKey> &a);
    void operator&=(const PackedBitArray<Bit,BitKey> &a);
    void operator|=(const PackedBitArray<Bit,BitKey> &a);
    void mutuallyExclusiveOr(const PackedBitArray<Bit,BitKey> &a);
    
    /// @brief shift this bit array toward the LSB
    /// @param n how many bits to shift
    /// @param flags bitwise-or or flags. Supported flags: NO_MASK if no need to mask when shifting
    void selfShiftLsb(int n, int flags = 0);

    /// @brief shift this bit array toward the LSB
    /// @param n how many bits to shift
    /// @param flags bitwise-or or flags. Supported flags: NO_MASK if no need to mask when shifting
    PackedBitArray<Bit,BitKey> shiftLsb(int n, int flags = 0) const {
        PackedBitArray<Bit,BitKey> ret(*this);
        ret.selfShiftLsb(n, flags);
        return ret;
    }
    void selfShiftMsb(int n, int maxBits = -1, int flags = 0);
    PackedBitArray<Bit,BitKey> shiftMsb(int n, int maxBits = -1, int flags = 0) const {
        PackedBitArray<Bit,BitKey> ret(*this);
        ret.selfShiftMsb(n, maxBits, flags);
        return ret;
    }
    void chompMsb();
    void cleanUnknowns();

    unsigned int toUnsignedInt(const BitKey &privKey) const;
    cpp_int toCppInt(const BitKey &privKey) const;

    /// @brief put in b aa ciphertext that holds the bit at position pos at slot 0
    /// if pos is negative, then it counts from the end.
    /// e.g.
    /// pos=0 means the LSB
    /// pos=-1 means the MSB
    Bit getBit(int pos) const;
    Bit msb() const { return getBit(-1); }
    Bit lsb() const { return getBit(0); }

    PackedBitArray<Bit,BitKey> multiplyAllCtxts(const Bit &b) const;

    std::string to_string(const BitKey &k) const;

    int getDepth() const {
        int d = 0;
        for (auto c : _ctxt) {
            d = std::max(d, c.mul_depth());
        }
        return d;
    }

    void save(std::ostream &s) const {
        for (auto &r : _ctxt)
            r.save(s);
    }
};

template<class Bit, class BitKey>
unsigned int PackedBitArray<Bit, BitKey>::toUnsignedInt(const BitKey &privKey) const {
    unsigned int ret = 0;
    std::vector<long> bits;
    if ((_bitSize % _keys.simd_factor()) != 0)
        bits = _ctxt[(_bitSize-1) / _keys.simd_factor()].to_vector();
    for (size_t i = _bitSize; i > 0; --i) {
        if ((i % _keys.simd_factor()) == 0)
            bits = _ctxt[(i-1) / _keys.simd_factor()].to_vector();
        ret *= 2;
        ret += bits[ (i-1) % _keys.simd_factor() ];        
    }
    return ret;
}

template<class Bit, class BitKey>
cpp_int PackedBitArray<Bit, BitKey>::toCppInt(const BitKey &privKey) const {
    cpp_int ret = 0;
    std::vector<long> bits;
    if ((_bitSize % _keys.simd_factor()) != 0)
        bits = _ctxt[(_bitSize-1) / _keys.simd_factor()].to_vector();
    for (size_t i = _bitSize; i > 0; --i) {
        if ((i % _keys.simd_factor()) == 0)
            bits = _ctxt[(i-1) / _keys.simd_factor()].to_vector();
        ret *= 2;
        ret += bits[ (i-1) % _keys.simd_factor() ];        
    }
    return ret;
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::operator^=(const PackedBitArray<Bit, BitKey> &a) {
    int ctxts = (_bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();
    int aCtxts = (a._bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();

    // Deal with ctxt that are full in 'this' and in 'a'
    // assuming the ctxts are padded with zeroes then that ctxt is handled the same
    for (int i = 0; i < std::min(ctxts, aCtxts); ++i) {
        if (_keys.get_ring_size() == 2) {
            _ctxt[i] += a._ctxt[i];
        } else {
            _ctxt[i] -= a._ctxt[i];
            _ctxt[i] *= _ctxt[i];
        }
    }

    // deal with the tail
    if (ctxts < aCtxts)
        _ctxt.resize(aCtxts);
    for (int i = ctxts; i < aCtxts; ++i)
        _ctxt[i] = a._ctxt[i];
    _bitSize = std::max(_bitSize, a._bitSize);
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::operator|=(const PackedBitArray<Bit, BitKey> &a) {
    int ctxts = (_bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();
    int aCtxts = (a._bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();

    // Deal with ctxt that are full in 'this' and in 'a'
    // assuming the ctxts are padded with zeroes then that ctxt is handled the same
    for (size_t i = 0; i < std::min(ctxts, aCtxts); ++i) {
        _ctxt[i] += a._ctxt[i] - _ctxt[i]*a._ctxt[i];
    }

    // deal with the tail
    if (ctxts < aCtxts)
        _ctxt.resize(aCtxts);
    for (size_t i = ctxts; i < aCtxts; ++i)
        _ctxt[i] = a._ctxt[i];
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::operator&=(const PackedBitArray<Bit, BitKey> &a) {
    int ctxts = (_bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();
    int aCtxts = (a._bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();

    // Deal with ctxt that are full in 'this' and in 'a'
    // assuming the ctxts are padded with zeroes then that ctxt is handled the same
    for (int i = 0; i < std::min(ctxts, aCtxts); ++i) {
        _ctxt[i] *= a._ctxt[i];
    }
    if (a._bitSize < _bitSize) {
        _bitSize = std::min(_bitSize, a._bitSize);
        _ctxt.resize(std::min(ctxts, aCtxts));
        cleanUnknowns();
    }

}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::mutuallyExclusiveOr(const PackedBitArray<Bit, BitKey> &a) {
    int ctxts = (_bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();
    int aCtxts = (a._bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();

    // Deal with ctxt that are full in 'this' and in 'a'
    // assuming the ctxts are padded with zeroes then that ctxt is handled the same
    for (int i = 0; i < std::min(ctxts, aCtxts); ++i) {
        _ctxt[i] += a._ctxt[i];
    }

    // deal with the tail
    if (ctxts < aCtxts)
        _ctxt.resize(aCtxts);
    _bitSize = std::max(_bitSize, a._bitSize);
    for (int i = ctxts; i < aCtxts; ++i)
        _ctxt[i] = a._ctxt[i];
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::cleanUnknowns() {
    if ((_bitSize % _keys.simd_factor()) == 0)
        return;

    size_t bit = _bitSize % _keys.simd_factor();
    size_t c = (_bitSize + _keys.simd_factor() - 1) / _keys.simd_factor();

    std::vector<long> maskInt(_keys.simd_factor(), 0);
    for (size_t i = 0; i < bit; ++i) {
        maskInt[i] = 1;
    }
    Bit mask = _keys.from_vector(maskInt);

    _ctxt[c - 1] *= mask;
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::selfShiftLsb(int a, int flags) {
    if (_bitSize <= a) {
        _bitSize = 0;
        _ctxt.resize(0);
        return;
    }

    // size_t ctxts = _ctxt.size();
    size_t newCtxts = (_bitSize - a + _keys.simd_factor() - 1) / _keys.simd_factor();
    int ctxtOffset = a / _keys.simd_factor();
    int bitOffset = a % _keys.simd_factor();

    if (((flags & NO_MASK) == NO_MASK) && (_ctxt.size() == 1)) {
        _ctxt[0].self_rotate_left(bitOffset);
        return;
    }

    if (bitOffset == 0) {
        for (size_t i = newCtxts; i > 0; --i)
            _ctxt[i - 1] = _ctxt[i - 1 + ctxtOffset];
        _bitSize -= a;
        _ctxt.resize(newCtxts);
        return;
    }

    std::vector<long> msbMaskInt(_keys.simd_factor(), 0);
    std::vector<long> lsbMaskInt(_keys.simd_factor(), 1);
    for (size_t i = _keys.simd_factor() - bitOffset; i < _keys.simd_factor(); ++i) {
        msbMaskInt[i] = 1;
        lsbMaskInt[i] = 0;
    }
    Bit msbMask = _keys.from_vector(msbMaskInt);
    Bit lsbMask = _keys.from_vector(lsbMaskInt);

    for (size_t i = 0; i < newCtxts; ++i) {
        if (ctxtOffset > 0)
            _ctxt[i] = _ctxt[i + ctxtOffset];
        _ctxt[i].self_rotate_left(bitOffset);
        _ctxt[i] *= lsbMask;
        if (i + ctxtOffset + 1 < _ctxt.size()) {
            Bit temp(_ctxt[i + ctxtOffset + 1]);
            temp.self_rotate_right(_keys.simd_factor() -  bitOffset);
            temp *= msbMask;
            _ctxt[i] += temp;
        }
    }

    _bitSize -= a;
    if (_ctxt.size() > newCtxts)
        _ctxt.resize(newCtxts);
    cleanUnknowns();
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::chompMsb() {
    if (_bitSize == 0)
        return;

    --_bitSize;
    std::vector<long> v(_keys.simd_factor(), 0);
    for (unsigned int i = 0; i < (_bitSize % _keys.simd_factor()); ++i)
        v[i] = 1;
    if ((_bitSize % _keys.simd_factor()) == 0)
        _ctxt[_bitSize / _keys.simd_factor()] = _keys.from_vector(v);
    else
        _ctxt[_bitSize / _keys.simd_factor()] *= _keys.from_vector(v);
}


template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::selfShiftMsb(int a, int maxBits, int flags) {
    if (_bitSize == 0)
        return;

    int ctxts = _ctxt.size();
    int newBits = _bitSize + a;
    if ((maxBits > -1) && (newBits > maxBits))
        newBits = maxBits;
    int newCtxts = (newBits + _keys.simd_factor() - 1) / _keys.simd_factor();
    int ctxtOffset = a / _keys.simd_factor();
    int bitOffset = a % _keys.simd_factor();

    _ctxt.resize(newCtxts);
    _bitSize += a;
    if (bitOffset == 0) {
        for (int i = newCtxts-1; i >= ctxtOffset; --i)
            _ctxt[i] = _ctxt[i - ctxtOffset];
        for (int i = ctxtOffset-1; i >= 0; --i)
            _ctxt[i] = _keys.from_int(0);
        return;
    }

    if (((flags & NO_MASK) == NO_MASK) && (_ctxt.size() == 1)) {
        _ctxt[0].self_rotate_right(bitOffset);
        return;
    }

    std::vector<long> msbMaskInt(_keys.simd_factor(), 1);
    std::vector<long> lsbMaskInt(_keys.simd_factor(), 0);
    for (int i = 0; i < bitOffset; ++i) {
        msbMaskInt[i] = 0;
        lsbMaskInt[i] = 1;
    }
    Bit msbMask = _keys.from_vector(msbMaskInt);
    Bit lsbMask = _keys.from_vector(lsbMaskInt);

    for (int i = newCtxts - 1; i >= 1+ctxtOffset; --i) {
        if (i - ctxtOffset < ctxts) {
            if (ctxtOffset > 0)
                _ctxt[i] = _ctxt[i - ctxtOffset];
            _ctxt[i].self_rotate_right(bitOffset);
            _ctxt[i] *= msbMask;
            Bit temp(_ctxt[i - ctxtOffset - 1]);
            temp.self_rotate_left(_keys.simd_factor() - bitOffset);
            temp *= lsbMask;
            _ctxt[i] += temp;
        } else {
            _ctxt[i] = _ctxt[i - ctxtOffset - 1];
            _ctxt[i].self_rotate_left(_keys.simd_factor() - bitOffset);
            _ctxt[i] *= lsbMask;
        }
    }

    if (ctxtOffset != 0) {
        _ctxt[ctxtOffset] = _ctxt[0];
    }
    _ctxt[ctxtOffset].self_rotate_right(bitOffset);
    _ctxt[ctxtOffset] *= msbMask;

    for (int i = ctxtOffset - 1; i >= 0; --i) {
        _ctxt[i] = _keys.from_int(0);
    }
    cleanUnknowns();
}


template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::set(const boost::multiprecision::cpp_int &_a, int minBits) {
    boost::multiprecision::cpp_int a(_a);
    _bitSize = 0;
    std::vector<long> bits;
    while ((a != 0) || (_bitSize < minBits)) {
        int b = a % 2;
        bits.push_back(b);
        a = a >> 1;
        ++_bitSize;

        if (bits.size() == _keys.simd_factor()) {
            _ctxt.push_back( _keys.from_vector(bits));
            bits.clear();
        }
    }
    if (bits.size() != 0) {
        while (bits.size() < _keys.simd_factor())
            bits.push_back(0);
        _ctxt.push_back( _keys.from_vector(bits));
    }
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::genericCmpOneCtxt(const Bit &a, const Bit &b, Bit &eq, Bit &smallerEq, size_t bitsEnd) {
    size_t bits = 1;

    if (_keys.get_ring_size() == 2)
        eq = a + b + 1;
    else
        eq = (a-b)*(b-a) + 1; // 1 - (a-b)^2

    // smaller = b * (-a + 1);
    smallerEq = b;
    Bit &smaller = smallerEq;

    while (bits < bitsEnd) {
        Bit eqRot = eq.rotate_left(bits);
        Bit smallerRot = smaller.rotate_left(bits);

        if ((!is_power_of_2(_keys.simd_factor())) && (_keys.simd_factor() < (size_t)2*_bitSize)) {
            std::vector<long> mask(_keys.simd_factor(), 1);
            std::vector<long> invMask(_keys.simd_factor(), 0);
            for (size_t i = 0; i < bits; ++i) {
                mask[_keys.simd_factor() - i - 1] = 0;
                invMask[_keys.simd_factor() - i - 1] = 1;
            }
            Bit maskEnc = _keys.from_vector(mask);
            Bit invMaskEnc = _keys.from_vector(invMask);

            // if slotCount is not a power of 2, we need to set the bits coming in from the MSB to 1. We need that for the correct computation of smaller
            eqRot *= maskEnc;
            eqRot += invMaskEnc;
            smallerRot *= maskEnc;
        }

        // Version 1.0
        smaller = eqRot*smaller + (-eqRot+1)*smaller.rotate_left(bits);
        // Version 2.0
        // smaller = eqRot*(smaller - smallerRot) + smallerRot;
        // Version 3.0
        // smaller -= smallerRot;
        // smaller *= eqRot;
        // smaller += smallerRot;
        eq *= eqRot;
        bits *= 2;
    }

    // eq.self_rotate_left(_keys.simd_factor() - 1);
    // smaller.self_rotate_left(_keys.simd_factor() - 1);
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::isAllZeroes(Bit &out, const Bit &in) {
    out = in;
    out = -out + 1;
    for (size_t bits = 1; bits < _keys.simd_factor(); bits *= 2) {
        out *= out.rotate_left(bits);
    }
}


template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::genericCmp(const PackedBitArray<Bit, BitKey> &a, Bit &eq, Bit &smaller, int from, int to) {
    if (from < 0)
        from = 0;
    if (to < 0)
        to = std::max(_ctxt.size(), a._ctxt.size()) - 1;

    if (from < to) {
        int mid = (from + to) / 2;

        Bit tmpEq;
        Bit tmpSmaller;

        genericCmp(a, tmpEq, smaller, from, mid);
        genericCmp(a, eq, tmpSmaller, mid + 1, to);
        // smaller = eq* smaller + (-eq+1)*tmpSmaller;
        smaller = eq * (smaller - tmpSmaller) + tmpSmaller;

        tmpSmaller *= (-eq + 1);
        eq *= tmpEq;
    } else {
        if (((size_t)from < _ctxt.size()) && ((size_t)from < a._ctxt.size())) {
            int simd = _ctxt[0].simd_factor();
            size_t bits = (std::max(_bitSize, a._bitSize) - from*simd) % simd;
            genericCmpOneCtxt(_ctxt[from], a._ctxt[from], eq, smaller, bits);
        } else if ((size_t)from < _ctxt.size()) {
            isAllZeroes(eq, _ctxt[from]);
            smaller = _keys.from_int(0);
        } else if ((size_t)from < a._ctxt.size()) {
            isAllZeroes(eq, a._ctxt[from]);
            smaller = -eq + 1;
        }
    }
}


template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::add(const PackedBitArray<Bit, BitKey> &a, const PackedBitArray<Bit, BitKey> &b, int flags) {
    if ((this == &a) || (this == &b))
        throw std::runtime_error("PackedBitArray::add : target must be different than args");

    if (a._bitSize == 0) {
        *this = b;
        return;
    }
    if (b._bitSize == 0) {
        *this = a;
        return;
    }

    int shiftFlags = 0;
    if ((int)a._ctxt[0].simd_factor() > (int)2* std::max(a._bitSize, b._bitSize))
	    shiftFlags = NO_MASK;

    // std::function<Bit(const Bit &, const Bit &)> myXor;
    // std::function<Bit(const Bit &, const Bit &)> myAnd;
    // std::function<Bit(const Bit &, const Bit &)> myOr;

    // typedef PackedBitArray<Bit, BitKey> PBA;

    // myXor = [](const PBA &x, const PBA &y){ return x^y; };
    // myAnd = [](const PBA &x, const PBA &y){ return x*y; };
    // myOr = [](const PBA &x, const PBA &y){ return x+y-x*y; };


    // g_i = a_i and b_i   - generating carry at location i
    // h_i = a_i myXor b_i   - output of half-addr at location i
    // g_i and h_i are mutually inclusive so (h_i + g_i) = (h_i myXor g_i)
    //
    // c_i = g_i + h_i*g_{i-1} + h_i*h_{i-1}*g_{i-2} ...    - a carry at location i
    //
    // H_i^j = h_i * h_{i-1} * h{i-2} * ... * h_{i-j}
    //
    // H_i^0 = h_i
    // H_i^j = H_i^{floor{k}}* H_{i-floor{k}-1}^{ceil{k}}     k={j-1}/2
    // H_i^j = H_i^{floor{k}}* (H_i^{ceil{k}} << (floor{k}-1))     k={j-1}/2
    //
    // c_i = g_i + H_i^0*g_{i-1} + H_i^1*g_{i-2} ...
    int max = std::max(a._bitSize, b._bitSize);
    // int min = std::min(a._bitSize, b._bitSize);

    PackedBitArray<Bit, BitKey> g = a & b;

    std::vector<PackedBitArray<Bit, BitKey>> H;
    for (int i = 0; i < max - 1; ++i)
        H.push_back(PackedBitArray<Bit,BitKey>(_keys));

    if (H.size() > 0) {
        H[0] = a ^ b;
        H[0].selfShiftLsb(1, shiftFlags);
    }

    for (unsigned int jp2 = 1; jp2 < H.size(); jp2 *= 2) {
        unsigned int loopend = std::min((unsigned int)2*jp2, (unsigned int)H.size());
        // std::cout << "---------" << std::endl;
#       pragma omp parallel for
        for (unsigned int j = jp2; j < loopend; ++j) {
            int floor_k = j - jp2;
            int ceil_k = jp2-1;

            // std::cout << "H[" << j << "] = H[" << ceil_k << "] & H[" << floor_k << "].shift(" << (ceil_k+1) << ")" << std::endl;
            // PackedBitArray<Bit, BitKey> temp(H[floor_k].shiftLsb(ceil_k + 1));
            H[j] = H[ceil_k] & H[floor_k].shiftLsb(ceil_k + 1, shiftFlags);

            // j = 1   a_i & a_{i-1}
            // H[1] = H[0] & H[0].shiftLsb(1);
            // j = 2   a_i & a_{i+1} & a_{i+2}
            // H[2] = H[1] & H[0].shiftLsb(2);
            // j = 3   a_i & a_{i+1} & a_{i+2} & a_{i+3}
            // H[3] = H[1] & H[1].shiftLsb(2);
            // j = 4   a_i & a_{i+1} & a_{i+2} & a_{i+3} & a_{i+4}
            // H[4] = H[2] & H[1].shiftLsb(3);
            //
            // j = 2n   a_i & a_{i+1} & a_{i+2} & a_{i+3} & a_{i+4} ... & a_{i+2n-1} & a_{i+2n}
            // H[2n] = H[n] & H[n-1].shiftLsb(n+1)
            //
            // j = 2n+1   a_i & a_{i+1} & a_{i+2} & a_{i+3} & a_{i+4} ... & a_{i+2n-1} & a_{i+2n+1}
            // H[2n+1] = H[n] & H[n].shiftLsb(n+1)
        }
    }

    if ((shiftFlags & NO_MASK) == NO_MASK) {
#       pragma omp parallel for
        for (size_t i = 0; i < H.size(); ++i)
            H[i].cleanUnknowns();
    }

    PackedBitArray<Bit, BitKey> carry = g;
#   pragma omp parallel for
    for (unsigned int i = 0; i < H.size(); ++i) {
        PackedBitArray<Bit, BitKey> temp = H[i] & g;
        temp.selfShiftMsb(i + 1, shiftFlags, shiftFlags);
#       pragma omp critical
        carry.mutuallyExclusiveOr(temp);
    }

    carry.selfShiftMsb(1, shiftFlags);

    *this = a;
    *this ^= b;
    *this ^= carry;

    if ((shiftFlags & NO_MASK) == NO_MASK)
        cleanUnknowns();

    if (((flags & DONT_GROW) == DONT_GROW))
        chompMsb();
}

template<class Bit, class BitKey>
void PackedBitArray<Bit, BitKey>::onesComplement() {
    std::vector<long> mask(_keys.simd_factor(), 1);
    Bit ones = _keys.from_vector(mask);
    for (size_t i = 0; i < _ctxt.size(); ++i) {
        _ctxt[i] = ones - _ctxt[i];
    }
}

template<class Bit, class BitKey>
Bit PackedBitArray<Bit, BitKey>::getBit(int pos) const {
    if (pos < 0)
        pos = _bitSize + pos;

    if ((pos < 0) || (pos >= _bitSize))
        throw std::runtime_error("PackedBitArray::getBit: position is out of range ");

    Bit b = _ctxt[pos / _keys.simd_factor()];
    pos %= _keys.simd_factor();
    if (pos != 0)
        b.self_rotate_left(pos);
    
    return b;
}

template<class Bit, class BitKey>
PackedBitArray<Bit,BitKey> PackedBitArray<Bit,BitKey>::multiplyAllCtxts(const Bit &b) const {
    PackedBitArray<Bit, BitKey> ret = *this;
    for (size_t i = 0; i < ret._ctxt.size(); ++i)
        ret._ctxt[i] *= b;
    return ret;
}

template<class Bit, class BitKey>
std::string PackedBitArray<Bit,BitKey>::to_string(const BitKey &k) const {
    std::string s;
    cpp_int val = toCppInt(k);

    for (int i = 0; i < _bitSize; ++i) {
        if ((val % 2) == 1) {
            s = std::string("1") + s;
            val -= 1;
        } else
            s = std::string("0") + s;
        val /= 2;
    }
    return s;
}

#endif

#if 0

#include  "zp.h"

void testAdd(const ZPKeys &bitKeys, int minBits = -1) {
    int maxDepth = 0;

    // for (int plainA = 64; plainA < 128; ++plainA) {
    for (int plainA = 1; plainA < 128; ++plainA) {
    for (int plainB = 1; plainB < 128; ++plainB) {
        std::cout << "checking " << plainA << " + " << plainB << std::endl;
        PackedBitArray<ZP, ZPKeys> a(plainA, bitKeys, minBits);
        PackedBitArray<ZP, ZPKeys> b(plainB, bitKeys, minBits);


        PackedBitArray<ZP, ZPKeys> c(bitKeys);
        // c.add(a, b, PackedBitArray<ZP, ZPKeys>::DONT_GROW);
        c.add(a, b);

        if (c.getDepth() > maxDepth) {
            maxDepth = c.getDepth();
            std::cout << "depth of addition: " << maxDepth << std::endl;
        }

        int plainC = c.toUnsignedInt(bitKeys);

        // if (plainC != ((plainA + plainB) % 128))
        if (plainC != plainA + plainB)
            throw std::runtime_error(std::string("Error adding ") + std::to_string(plainA) + " " + std::to_string(plainB) + " != " + std::to_string(plainC));
    }
    }
}

void testCmp(const ZPKeys &bitKeys, int minBits = -1) {
    int maxEqDepth = 0;
    int maxSmallerDepth = 0;

    for (int plainA = 0; plainA < 512; ++plainA) {
    for (int plainB = 0; plainB < 512; ++plainB) {
        std::cout << "checking " << plainA << " =? " << plainB << std::endl;
        PackedBitArray<ZP, ZPKeys> a(plainA, bitKeys, minBits);
        PackedBitArray<ZP, ZPKeys> b(plainB, bitKeys, minBits);

        ZP eq;
        ZP smaller;

        a.genericCmp(b, eq, smaller);

        if (eq.mul_depth() > maxEqDepth) {
            maxEqDepth = eq.mul_depth();
            std::cout << "depth of eq: " << maxEqDepth << std::endl;
        }

        if (smaller.mul_depth() > maxSmallerDepth) {
            maxSmallerDepth = smaller.mul_depth();
            std::cout << "depth of smaller: " << maxSmallerDepth << std::endl;
        }

        int plainEq = eq.to_int();
        int plainSmaller = smaller.to_int();

        if (((plainEq == 1) ^ (plainA == plainB)) != 0)
            throw std::runtime_error(std::string("Error comparing ") + std::to_string(plainA) + " == " + std::to_string(plainB) + " != " + std::to_string(plainEq));
        if (plainA != plainB) {
            if (((plainSmaller == 1) ^ (plainA < plainB)) != 0)
                throw std::runtime_error(std::string("Error comparing ") + std::to_string(plainA) + " < " + std::to_string(plainB) + " != " + std::to_string(plainSmaller));
        }
    }
    }
}

int main(int, char **) {
    // testAdd( ZPKeys(2, 16, 7000), 2000 );
    // testAdd( ZPKeys(2, 1, 4096), 2000 );
    // testAdd( ZPKeys(2, 1, 4096), 2000 );
    // // // testAdd( ZPKeys(5, 1, 2) ); // doesn't work
    // testAdd( ZPKeys(5, 1, 4096), 2000 );
    // testAdd( ZPKeys(5, 1, 4096), 2000 );
    // return 1;

    // testAdd( ZPKeys(2, 1, 4) );
    // testAdd( ZPKeys(2, 1, 3) );
    // testAdd( ZPKeys(2, 1, 50) );
    // // // testAdd( ZPKeys(5, 1, 2) ); // doesn't work
    // testAdd( ZPKeys(5, 1, 3) );
    // testAdd( ZPKeys(5, 1, 50) );

    //testCmp( ZPKeys(2, 16, 7000), 2000 ); // works
    //testCmp( ZPKeys(2, 1, 4096), 2000 ); // works
    //testCmp( ZPKeys(2, 1, 4096), 2000 ); // works
    //testCmp( ZPKeys(2, 1, 4096), 2000 ); // works
    //testCmp( ZPKeys(5, 1, 4096), 2000 ); // works
    //testCmp( ZPKeys(5, 1, 4096), 2000 ); // works
    //testCmp( ZPKeys(5, 1, 4096), 2000 );
    //testCmp( ZPKeys(5, 1, 4096), 2000 ); // works

    testCmp( ZPKeys(5, 1, 100), 9 );
    //testCmp( ZPKeys(2, 1, 4) ); // works
    //testCmp( ZPKeys(2, 1, 3) ); // works
    //testCmp( ZPKeys(2, 1, 50) ); // works
    //testCmp( ZPKeys(2, 1, 64) ); // works
    //testCmp( ZPKeys(5, 1, 4) ); // works
    //testCmp( ZPKeys(5, 1, 3) ); // works
    //testCmp( ZPKeys(5, 1, 50) );
    //testCmp( ZPKeys(5, 1, 64) ); // works
}


// g++ -g -I..  packed_bit_array.cc ../../src/zp.cc


#endif
