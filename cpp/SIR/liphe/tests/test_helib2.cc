#include <liphe/helib2_number.h>
#include <liphe/unsigned_word.h>
#include <liphe/eq.h>
#include <liphe/binomial_tournament.h>
#include <liphe/first_non_zero.h>

#include "test_framework.h"

typedef UnsignedWord<5, Helib2Number > MyUnsignedWord;

int main(int, char**) {
	Helib2Keys keys;

	Helib2Keys::Params params;
	params.set_security_param(80);
	params.p = 2;
	params.r = 1;

	keys.initKeys(params);
	Helib2Number::set_global_keys(&keys);

	doTest("operator + <Helib2Number>", test_add, Helib2Number, NULL, 1, -1);
	doTest("operator - <Helib2Number>", test_sub, Helib2Number, NULL, 1, -1);
	doTest("operator * <Helib2Number>", test_mul, Helib2Number, NULL, 1, -1);
	doTest("euler_eq <Helib2Number>", test_euler_eq, Helib2Number, NULL, 1, -1);
	doTest("BinomialTournament <Helib2Number>", test_binomial_tournament, Helib2Number, NULL, 1, -1);
	doTest("FindFirstNonZero <Helib2Number>", test_find_first_in_array, Helib2Number, NULL, 1, -1);


	doTest("operator + <UnsignedWord < Helib2Number >", test_add, MyUnsignedWord, NULL, 1, -1);
	doTest("operator - <UnsignedWord < Helib2Number >", test_sub, MyUnsignedWord, NULL, 1, -1);
	doTest("operator * <UnsignedWord < Helib2Number >", test_mul, MyUnsignedWord, NULL, 1, -1);
}

