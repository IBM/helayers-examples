#include <liphe/palisade_bfv_number.h>
#include <liphe/unsigned_word.h>
#include <liphe/eq.h>
#include <liphe/binomial_tournament.h>
#include <liphe/first_non_zero.h>

#include "test_framework.h"

typedef UnsignedWord<5, PalisadeBfvNumber > MyUnsignedWord;

int main(int, char**) {
	PalisadeBfvKeys keys;

	keys.initKeys(101, 3, 10, std::vector<int>());
	PalisadeBfvNumber::set_global_keys(&keys);

	doTest("operator + <PalisadeBfvNumber>", test_add, PalisadeBfvNumber, NULL, 1, -1);
	doTest("operator - <PalisadeBfvNumber>", test_sub, PalisadeBfvNumber, NULL, 1, -1);
	doTest("operator * <PalisadeBfvNumber>", test_mul, PalisadeBfvNumber, NULL, 1, -1);
	skipDoTest("euler_eq <PalisadeBfvNumber>", test_euler_eq, PalisadeBfvNumber, NULL, 1, -1);
	doTest("BinomialTournament <PalisadeBfvNumber>", test_binomial_tournament, PalisadeBfvNumber, NULL, 1, -1);
	skipDoTest("FindFirstNonZero <PalisadeBfvNumber>", test_find_first_in_array, PalisadeBfvNumber, NULL, 1, -1);


	doTest("operator + <UnsignedWord < PalisadeBfvNumber >", test_add, MyUnsignedWord, NULL, 1, -1);
	doTest("operator - <UnsignedWord < PalisadeBfvNumber >", test_sub, MyUnsignedWord, NULL, 1, -1);
	doTest("operator * <UnsignedWord < PalisadeBfvNumber >", test_mul, MyUnsignedWord, NULL, 1, -1);
}


