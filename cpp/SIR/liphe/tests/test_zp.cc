#include <liphe/zp.h>
#include <liphe/unsigned_word.h>
#include <liphe/eq.h>
#include <liphe/binomial_tournament.h>
#include <liphe/first_non_zero.h>

#include "test_framework.h"

typedef ZP MyZP;
typedef ZPKeys MyZPKeys;

typedef UnsignedWord<ZP> MyUnsignedWord;

int main(int, char**) {
	MyZP::set_global_p(101);
	MyZPKeys keys(101, 1, 5);

	doTest2("cmp", test_packed_bit_array_cmp, MyZP, MyZPKeys, &keys, 1, -1);
	return 1;

//	doTest("operator + <ZP<127> >", test_add, MyZP, NULL, 1, -1);
//	doTest("operator - <ZP<127> >", test_sub, MyZP, NULL, 1, -1);
//	doTest("operator * <ZP<127> >", test_mul, MyZP, NULL, 1, -1);
//	doTest("euler_eq <ZP<127> >", test_euler_eq, MyZP, NULL, 1, -1);
//	doTest("BinomialTournament <ZP<127> >", test_binomial_tournament, MyZP, NULL, 1, -1);
//	doTest("FindFirstNonZero <ZP<127> >", test_find_first_in_array, MyZP, NULL, 1, -1);
//
//	doTest("operator < <UnsignedWord < ZP<127> >", test_bitwise_less_than, MyUnsignedWord, NULL, 100000, -1);
//	doTest("operator > <UnsignedWord < ZP<127> >", test_bitwise_more_than, MyUnsignedWord, NULL, 100000, -1);
//
//	MyZP::set_global_p(2);
//	doTest("operator + <UnsignedWord < ZP<127> >", test_add, MyUnsignedWord, NULL, 1, -1);
//	doTest("operator - <UnsignedWord < ZP<127> >", test_sub, MyUnsignedWord, NULL, 1, -1);
//	doTest("operator * <UnsignedWord < ZP<127> >", test_mul, MyUnsignedWord, NULL, 1, -1);
//
//	MyZP::set_global_p(101);
//	doTest("Polynomial < ZP<127> >", test_polynomial, MyZP, NULL, 1, -1);
}

