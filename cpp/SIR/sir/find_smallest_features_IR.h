//  find_smallestFearutres_IR.h
//
//   Created on: Oct 12, 2020
//       Author: hayim

#ifndef __FIND_SMALLEST_FEATURES_IR_H__
#define __FIND_SMALLEST_FEATURES_IR_H__

#include <vector>
#include "server1.h"
#include "server2.h"
#include "communication.h"

#define Ciphertext typename Types::Ciphertext
#define Plaintext typename Types::Plaintext

#define BinaryWordPlaintext typename Types::BinaryWordPlaintext
#define BinaryWordCiphertext typename Types::BinaryWordCiphertext
#define BinaryBitPlaintext typename Types::BinaryBitPlaintext
#define BinaryBitCiphertext typename Types::BinaryBitCiphertext
#define PackedBitUnsignedWord typename Types::PackedBitUnsignedWord

int liftedIndex(const std::vector<long> &F, int i) {
    for (size_t j = 0; j < F.size(); ++j) {
        i -= F[j];
        if (i < 0)
            return j;
    }
    throw std::runtime_error("lifted index is out of bound");
    return -1;
}

template<class Types>
void Server1<Types>::generateRandomPermutation() {
    std::vector<long> reducedPerm(feature_number());
    for (size_t i = 0; i < reducedPerm.size(); ++i)
        reducedPerm[i] = i;
    std::random_shuffle(reducedPerm.begin(), reducedPerm.end());

    _randomPermutation.resize(_A.cols());
    int reducedI = 0;
    for (size_t i = 0; i < _randomPermutation.size(); ++i) {
        if (_feature_list[i] == 0) {
            _randomPermutation[i] = i;
        } else {
            _randomPermutation[i] = liftedIndex(_feature_list, reducedPerm[reducedI++]);
        }
    }
}

template<class Types>
void Server1<Types>::permute(std::vector<BinaryBitCiphertext> &p) {
    std::vector<BinaryBitCiphertext> ptemp;
    for (size_t i = 0; i < p.size(); ++i)
        ptemp.push_back(p[i]);
    
    for (size_t i = 0; i < p.size(); ++i)
        p[i] = ptemp[_randomPermutation[i]];
}

template<class Types>
void Server1<Types>::unpermute(std::vector<long> &p) {
    std::vector<long> ptemp;
    for (size_t i = 0; i < p.size(); ++i)
        ptemp.push_back(p[i]);
    
    for (size_t i = 0; i < p.size(); ++i)
        p[_randomPermutation[i]] = ptemp[i];
}


template<class Types>
void Server1<Types>::findSmallestFeaturesOld(int k) {
    if ((size_t)_pubBinaryKey->get_ring_size() <= _zBin.size()) {
        cout << "ring size = " << _pubBinaryKey->get_ring_size() << endl;
        cout << "zBin size = " << _zBin.size() << endl;
        throw std::runtime_error("Key for binary representation must have ring size bigger than number of features");
    }

    generateRandomPermutation();

    std::vector<BinaryBitCiphertext> rank;
    std::vector<long> zeroes(_pubBinaryKey->simd_factor(), 0);
    for (size_t i = 0; i < _zBin.size(); ++i) {
        rank.push_back(_pubBinaryKey->from_vector(zeroes));
    }

#   pragma omp parallel for
    for (size_t ij = 0; ij < _zBin.size()*(_zBin.size()-1)/2; ++ij) {
        // the value of i will be: 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, ...
        size_t i = (-1 + sqrt(1 + 8*ij)) / 2 + 1;
        // the value of j will be: 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, ...
        size_t j = ij - i*(i-1)/2;

        if ((_feature_list[i] == 0) || (_feature_list[j] == 0))
            continue;
        BinaryBitCiphertext eq;
        BinaryBitCiphertext smaller;
        _zBin[i].genericCmp(_zBin[j], eq, smaller);

#       pragma omp critical
        {
            rank[j] += smaller;
            rank[i] += -smaller + 1;
        }

#       ifdef __DEBUG
        std::cout << "After comparing rank[" << i << "] with rank[" << j << "]." << std::endl;
        int r;
        r = privBinaryKey.to_int(smaller);
        std::cout << " smaller = " << r << std::endl;
        r = privBinaryKey.to_int(rank[i]);
        std::cout << "rank[" << i << "] = " << r << std::endl;
        r = privBinaryKey.to_int(rank[j]);
        std::cout << "rank[" << j << "] = " << r << std::endl;
#       endif
    }


#   ifdef __DEBUG
    for (size_t i = 0; i < _zBin.size(); ++i) {
        int r = privBinaryKey.to_int(rank[i]);
        std::cout << "rank[" << i << "] = " << r << std::endl;
    }
#   endif

    permute(rank);

    _communication_channel->sendRanksToServer2(rank, k);
}

template<class Types>
void Server2<Types>::receive_ranks(const std::vector<BinaryBitCiphertext> &ranks, int k) {
    std::vector<long> chi(ranks.size());

    cout << "Comparing ranks to " << k << endl;
    for (size_t i = 0; i < ranks.size(); ++i) {
        int r = _privBinaryKey->to_int(ranks[i]);
        chi[i] = (r < k) ? 1 : 0;
    }
    _communication_channel->send_smallest_features_to_server1(chi);
}

template<class Types>
void Server1<Types>::receive_rank_chi(const std::vector<long> &chi) {
    _smallestFeatureChi = chi;
    unpermute(_smallestFeatureChi);

    for (size_t i = 0; i < _feature_list.size(); ++i) {
        _feature_list[i] = 1 - _smallestFeatureChi[i];
#   ifdef __DEBUG
        std::cout << "Chi_rank[" << i << "] = " << _smallestFeatureChi[i] << std::endl;
#   endif
    }
}


///// The new protocol that avoid additions


template<class Types>
void Server1<Types>::findSmallestFeaturesNew(int k) {
    Times::start(Times::ComputePairDifferences);
    _k = k;
    _z *= _z;

    cpp_int ringSize = _pubKey->get_ring_size();
    _rSmallestFeatures1.resize(_z.size());
    _rSmallestFeatures2.resize(_z.size());

#   pragma omp parallel for
    for (size_t i = 0; i < _z.size(); ++i) {
        _rSmallestFeatures1[i].resize(_z.size());
        _rSmallestFeatures2[i].resize(_z.size());
    }

#   pragma omp parallel for collapse(2)
    for (size_t i = 0; i < _z.size(); ++i) {
        for (size_t j = 0; j < _z.size(); ++j) {
            _rSmallestFeatures1[i][j] = draw_big_int(ringSize);
            _rSmallestFeatures2[i][j] = draw_big_int(ringSize);
        }
    }

    printTimeStamp("after drawing random numbers");

    std::vector<PackedVector<Ciphertext>> diff(_z.size() - 1);
#pragma omp parallel for
    for (size_t i = 0; i < _z.size() - 1; ++i) {
        diff[i] = _z;
        // we split the random mask to two parts and add them at two different places
        // to make sure we mask everything even when we shift
        PackedVector<Plaintext> r1(_rSmallestFeatures1[i], _plainKey);
        PackedVector<Plaintext> r2(_rSmallestFeatures2[i], _plainKey);
        diff[i] += r1;

        diff[i].shiftLeft(i + 1);
        diff[i] -= _z;
        diff[i] += r2;
    }

    printTimeStamp("after computing diff of all pairs");
    Times::end(Times::ComputePairDifferences);

    // diff[i] slot j  holds   _z[j+i+1]-_z[j] + r1[i+j+1] + r2[j]

#ifdef __DEBUG
    cout << "At server1: " << endl;

    cout << "r1 = " << endl;
    for (size_t i = 0; i < _rSmallestFeatures1.size(); ++i) {
        for (size_t j = 0; j < _rSmallestFeatures1[i].size(); ++j) {
            cout << _rSmallestFeatures1[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    cout << "r2 = " << endl;
    for (size_t i = 0; i < _rSmallestFeatures2.size(); ++i) {
        for (size_t j = 0; j < _rSmallestFeatures2[i].size(); ++j) {
            cout << _rSmallestFeatures2[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    cout << "Masked diff matrix = " << endl;
    for (size_t i = 0; i < diff.size(); ++i)
        cout << TO_STRING(diff[i]) << endl;
    cout << endl;
#endif

    _communication_channel->sendDiffsToServer2(diff, _feature_list);
}

template<class Types>
void Server2<Types>::receiveWeightAllPairDiffs(vector<PackedVector<Ciphertext>> &diffEnc, const vector<long> &F) {
    Times::start(Times::ComputeThresholds);

    _F = F;

    cpp_int N = _pubKey->get_ring_size();
    cpp_int N2 = (N-1)/2 + 1;
	size_t logN = 0;
	while ((cpp_int(1) << logN) < N)
		++logN;

    std::vector<PackedBitUnsignedWord> rMinVec;
    std::vector<PackedBitUnsignedWord> rMaxVec;
    std::vector<BinaryBitCiphertext> betweenVec;
    std::vector<pair<int,int>> pairVec;

	PackedBitUnsignedWord zero(0, *_pubBinaryKey);
	BinaryBitCiphertext zeroBit(_pubBinaryKey, 0);

    size_t d = diffEnc.size() + 1; // diffEnc actually has d-1 comparisons
	for (unsigned int i = 0; i < d-1; ++i) {
        for (unsigned int j = 0; j < d - i - 1; ++j) {
            unsigned int a = j;
            unsigned int b = i+j+1;

            // skip a=d-1 and b=d-1 because this is the dimension simulating the bias
            if ((a == d-1) || (b == d-1))
                continue;
            if (_F[a] == 0)
                continue;
            if (_F[b] == 0)
                continue;

            rMinVec.push_back(zero);
            rMaxVec.push_back(zero);
            betweenVec.push_back(zeroBit);
            pairVec.push_back({i,j});
        }
	}

    std::vector<std::vector<cpp_int>> allDiffs(d-1);
#   pragma omp parallel for
    for (size_t i = 0; i < d - 1; ++i) {
	    diffEnc[i].to_bigint_vector(allDiffs[i], _privKey);
    }

#   pragma omp parallel for
    for (size_t ij = 0; ij < pairVec.size(); ++ij) {
        int i = pairVec[ij].first;
        int j = pairVec[ij].second;
        std::vector<cpp_int> &diff = allDiffs[i];
        // diff[i] slot j  holds   _z[j+i+1]-_z[j] + r1[i+j+1] + r2[j]

        // the minimal r for which dij is positive
        cpp_int rMin;
        // the maximal r for which dij is positive
        cpp_int rMax;
        // true if dij is positive if rMin < (dij + rij)  and  (dij + rij) < rMax
        // false if dij is positive if (dij + rij) < rMin  or  rMax < (dij + rij)
        long between;

        if (diff[j] < N2) {
            // the diff is positive if r = 0,1,2,...,d[i]   or   d[i]+ringSize/2, d[i]+ringSize/2+1, ... ringSize-1
            rMin = diff[j] + 1;
            rMax = diff[j] + N2;
            between = 0;
        } else {
            // the diff is positive if r = d[i]-ringSize/2, d[i]-ringSize/2+1, ...d[i]
            rMin = diff[j] - N2 + 1;
            rMax = diff[j] + 1;
            between = 1;
        }

        rMinVec[ij] = PackedBitUnsignedWord(rMin, *_pubBinaryKey, logN);
        rMaxVec[ij] = PackedBitUnsignedWord(rMax, *_pubBinaryKey, logN);
        betweenVec[ij] = BinaryBitCiphertext(_pubBinaryKey, between);

#ifdef __DEBUG
        int a = j;
        int b = i+j+1;
        cout << "comparing " << i << " and " << j << ":" << endl;
        cout << "   a = " << a << endl;
        cout << "   b = " << b << endl;
        cout << "   rmin = " << rMin << endl;
        cout << "   rmax = " << rMax << endl;
        cout << "   between = " << between << endl;
        cout << "   rminBin = " << TO_STRING(rMinVec[ij]) << endl;
        cout << "   rmaxBin = " << TO_STRING(rMaxVec[ij]) << endl;
#endif
    }

    // int count = 0;
    // for (size_t i = 0; i < d - 1; ++i) {
    //     std::vector<cpp_int> diff;
	//     diffEnc[i].to_bigint_vector(diff, _privKey);
    //     for (size_t j = 0; j < d - i - 1; ++j) {
    //         // diff[i] slot j  holds   _z[j+i+1]-_z[j] + r1[i+j+1] + r2[j]
    //         int a = j;
    //         int b = i+j+1;
    //         if (_F[a] == 0)
    //             continue;
    //         if (_F[b] == 0)
    //             continue;

    //         // the minimal r for which dij is positive
    //         cpp_int rMin;
    //         // the maximal r for which dij is positive
    //         cpp_int rMax;
    //         // true if dij is positive if rMin < (dij + rij)  and  (dij + rij) < rMax
    //         // false if dij is positive if (dij + rij) < rMin  or  rMax < (dij + rij)
    //         long between;

    //         if (diff[j] < N2) {
    //            // the diff is positive if r = 0,1,2,...,d[i]   or   d[i]+ringSize/2, d[i]+ringSize/2+1, ... ringSize-1
    //            rMin = diff[j] + 1;
    //            rMax = diff[j] + N2;
    //            between = 0;
    //         } else {
    //            // the diff is positive if r = d[i]-ringSize/2, d[i]-ringSize/2+1, ...d[i]
    //            rMin = diff[j] - N2 + 1;
    //            rMax = diff[j] + 1;
    //            between = 1;
    //         }

	// 	    rMinVec[count] = PackedBitUnsignedWord(rMin, *_pubBinaryKey, logN);
	// 	    rMaxVec[count] = PackedBitUnsignedWord(rMax, *_pubBinaryKey, logN);
    //         betweenVec[count] = BinaryBitCiphertext(_pubBinaryKey, between);
    //         pairVec[count].first = i;
    //         pairVec[count].second = j;
    //         ++count;
    //     }
    // }

#ifdef __DEBUG
    cout << endl << endl;
#endif
    
	printTimeStamp("after computing all thresholds");

    Times::end(Times::ComputeThresholds);
    _communication_channel->send_rMin_rMax_toServer1(rMinVec, rMaxVec, betweenVec, pairVec);
}

// receive rMin, rMax from Server2 and compute the rank of each feature
template<class Types>
void Server1<Types>::receive_rMin_rMax(vector<PackedBitUnsignedWord> &rMin, vector<PackedBitUnsignedWord> &rMax, vector<BinaryBitCiphertext> &between, vector<pair<int,int>> &pairVec) {
    Times::start(Times::ComputeRanks);

    std::vector<BinaryBitCiphertext> rank;
    std::vector<long> zeroes(_pubBinaryKey->simd_factor(), 0);

    size_t d = _z.size();

    for (size_t i = 0; i < d-1; ++i) {
        rank.push_back(_pubBinaryKey->from_vector(zeroes));
    }
    // _zBin[dim] is the weight of '1' (the bias). We must not compare to it and we should not eliminate it, to fix it to have rank d
    std::vector<long> ds(_pubBinaryKey->simd_factor(), _z.size()+1);
    rank.push_back(_pubBinaryKey->from_vector(ds));

#ifdef __DEBUG
    cout << "at server1 comparing the rs " << endl;
#endif

	printTimeStamp("after setting up rank vector");

    if (pairVec.size() < (size_t)omp_get_thread_num()*1.5)
	    omp_set_max_active_levels(2);
#pragma omp parallel for
    for (size_t ij = 0; ij < pairVec.size(); ++ij) {
        size_t i = pairVec[ij].first;
        size_t j = pairVec[ij].second;
        int a = j;
        int b = i+j+1;
        // diff[i] slot j  holds   _z[j+i+1]-_z[j] + r1[i+j+1] + r2[j]
        cpp_int rij = (_rSmallestFeatures1[i][j+i+1] + _rSmallestFeatures2[i][j]) % _pubKey->get_ring_size();

        PackedBitUnsignedWord rijEnc(rij, *_pubBinaryKey);

        BinaryBitCiphertext eqMin, bigger;
        BinaryBitCiphertext eqMax, smaller;

#pragma omp parallel for
        for (int qqq = 0; qqq < 2; ++qqq) {
            if (qqq == 0) {
                // check whether rMin < rij
                rMin[ij].genericCmp(rijEnc, eqMin, bigger);
            }
            if (qqq == 1) {
                // check whether rij < rMax
                rijEnc.genericCmp(rMax[ij], eqMax, smaller);
            }
        }
        bigger *= -eqMin+1;
        bigger += eqMin;
        smaller *= -eqMax+1;

        BinaryBitCiphertext positive = bigger*smaller*(-between[ij]+1) + (-(bigger*smaller)+1)*(between[ij]);

#ifdef __DEBUG
        cout << "i,j = " << i << ", " << j << endl;
        cout << "  a = " << a << endl;
        cout << "  b = " << b << endl;
        cout << "r_ij = " << TO_STRING(rijEnc) << endl;
        cout << " bigger = " << TO_STRING(bigger) << endl;
        cout << " smaller = " << TO_STRING(bigger) << endl;
        cout << endl;
#endif

#pragma omp critical
        {
            rank[a] += positive;
            rank[b] += -positive + 1;
        }
    }
	omp_set_max_active_levels(1);
	printTimeStamp("after computing all ranks");

#ifdef __DEBUG
    cout << "rank: ";
    for (size_t i = 0; i < rank.size(); ++i) {
        cout << rank[i].to_int() << ", ";
    }
    cout << endl;
#endif

    Times::end(Times::ComputeRanks);
    generateRandomPermutation();
    permute(rank);

	printTimeStamp("after permuting");

    _communication_channel->sendRanksToServer2(rank, _k);
}

#undef Ciphertext
#undef Plaintext
#undef BinaryWordPlaintext
#undef BinaryWordCiphertext
#undef BinaryBitPlaintext
#undef BinaryBitCiphertext
#undef PackedBitUnsignedWord
    
#endif
