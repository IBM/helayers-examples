#ifndef ___FIRST_NON_ZERO__
#define ___FIRST_NON_ZERO__

#include <iostream>
#include <carray_iterator.h>
#include <binomial_tournament.h>

template<class Number, class Compare>
class BinarySearch {
public:
	// gets a binary array @array, 
//	static void searchFirst(const std::vector<Number> &array, IsZeroFunction *IsZero, std::vector<Number> &output) {
//		BinomialTournament>Number> bins;
//
//		for (int i = 0; i < output.size(); ++i)
//			output[i] = 0;
//
//		// No need to iterate over i == 0 because we are going to multiply everything by i==0
//		bins.add(array[0]);
//		for (int i = 1; i < size; ++i) {
//			prev = isZero(bin.unite_all(bin::mul));
//			int l = 0;
//			while ((1<<l) <= i) {
//				if (i & (1<<l))
//					output[l] += array[i] * prev * i;
//				++l;
//			}
//		}
//	}

	static void searchFirst(Number *array, int size, std::vector<Number> &output) {
		CArrayIterator<Number> begin(array, size, 0);
		CArrayIterator<Number> end(array, size, size);
		searchFirst(begin, end, output);
	}

	template<class Iterator>
	static void searchFirst(const Iterator &begin, const Iterator &end, std::vector<Number> &_output) {
		static int max_mul_depth_after_iRT = 0;
		static int max_mul_depth_after_first_isZero = 0;

		BinomialTournament<Number> bins(BinomialTournament<Number>::add);

		AddBinomialTournament<Number> *output = new AddBinomialTournament<Number>[_output.size()];

		int max_depth = 0;

		// we actually need log(end - begin) but iterators don't always support that operation
		// and _output.size() should be just that
		std::vector<Number*> compareCache(_output.size());
		for (int i = 0; i < compareCache.size(); ++i)
			compareCache[i] = NULL;

		// No need to iterate over i == 0 because we are going to multiply everything by i==0
		Iterator i = begin;
		bins.add_to_tournament(*i);
		++i;
		int count = 1;
		while (i != end) {
			BinomialTournament<Number> mul(BinomialTournament<Number>::mul);
			// on level 0 we do not need to use compare since it is already binary
			if (!bins.is_slot_empty(0))
				mul.add_to_tournament(Number(1) - bins.number(0));
			for (int level = 1; level < _output.size(); ++level) {
				if (!bins.is_slot_empty(level)) {
					if (compareCache[level] == NULL) {
						Number *iii = new Number(Compare(bins.number(level)) == 0);
						if (max_mul_depth_after_first_isZero < iii->mul_depth())
							max_mul_depth_after_first_isZero = iii->mul_depth();
						compareCache[level] = iii;
					}
					mul.add_to_tournament(*(compareCache[level]));
				} else {
					delete compareCache[level];
					compareCache[level] = NULL;
				}
			}
//			Number prev = mul.unite_all();
//			std::cout << "count=" << count << "  and prev=" << prev.to_int() << std::endl;
			mul.add_to_tournament(*i);
			Number add = mul.unite_all();

			if (max_mul_depth_after_iRT < add.mul_depth())
				max_mul_depth_after_iRT = add.mul_depth();

			for (int bit = 0; bit < _output.size(); ++bit) {
				if (count & (1 << bit)) {
					output[bit].add_to_tournament(add);
				}
			}
			bins.add_to_tournament(*i);

			++count;
			++i;
		}

		for (int i = 0; i < compareCache.size(); ++i) {
			if (compareCache[i] == NULL) {
				delete compareCache[i];
				compareCache[i] = NULL;
			}
		}



		for (int bit = 0; bit < _output.size(); ++bit) {
//std::cerr << "bit = " << bit << std::endl;
			if  (!output[bit].is_empty()) {
//				Number outbit = output[bit].unite_all();
//				std::cout << "output[" << bit << "] = " << outbit.to_int() << " => " << (Compare(outbit) != 0).to_int() << std::endl;
//				_output[bit] = (Compare(output[bit].unite_all()) != 0);
				_output[bit] = output[bit].unite_all();
			} else
				_output[bit] = 0;
		}

		std::cout << "max mul depth after first isZero = " << max_mul_depth_after_first_isZero << std::endl;
		std::cout << "max mul depth after iRT = " << max_mul_depth_after_iRT << std::endl;

		delete[] output;
	}
};

#endif
