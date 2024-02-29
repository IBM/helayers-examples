#include <liphe/helib_number.h>
#include <liphe/unsigned_word.h>
#include <liphe/eq.h>
#include <liphe/binomial_tournament.h>
#include <liphe/first_non_zero.h>

#include "test_framework.h"

typedef UnsignedWord<5, HelibNumber > MyUnsignedWord;

int main(int, char**) {
	HelibKeys keys;

	long R = 1;
	long p = 11;
	long r = 1;
	long d = 1;
	long c = 2;
	long k = 80;
	long L = 54;
	long s = 0;
	long chosen_m = 0;
	Vec<long> gens;
	Vec<long> ords;

	keys.initKeys(s, R, p, r, d, c, k, 64, L, chosen_m, gens, ords);
	HelibNumber::set_global_keys(&keys);

	HelibNumber ctxt;
	HelibNumber ctxt2;
	clock_t start = clock();
	clock_t end = clock();
	int iter = 0;

	while (end - start < 3*1000*1000) {
		++iter;
		ctxt2 = ctxt*ctxt;
		end = clock();
	}

	std::cout << iter << " mult took " << (end - start) << std::endl;
	exit(1);


	skipDoTest("operator + <HelibNumber>", test_add, HelibNumber, NULL, 1, -1);
	skipDoTest("operator - <HelibNumber>", test_sub, HelibNumber, NULL, 1, -1);
	skipDoTest("operator * <HelibNumber>", test_mul, HelibNumber, NULL, 1, -1);
	skipDoTest("euler_eq <HelibNumber>", test_euler_eq, HelibNumber, NULL, 1, -1);
	skipDoTest("BinomialTournament <HelibNumber>", test_binomial_tournament, HelibNumber, NULL, 1, -1);
	skipDoTest("FindFirstNonZero <HelibNumber>", test_find_first_in_array, HelibNumber, NULL, 1, -1);


	doTest("operator + <UnsignedWord < HelibNumber >", test_add, MyUnsignedWord, NULL, 1, -1);
	doTest("operator - <UnsignedWord < HelibNumber >", test_sub, MyUnsignedWord, NULL, 1, -1);
	doTest("operator * <UnsignedWord < HelibNumber >", test_mul, MyUnsignedWord, NULL, 1, -1);
}

