#include <fstream>

#include <liphe/helib2_number.h>
#include <liphe/unsigned_word.h>
#include <liphe/eq.h>
#include <liphe/binomial_tournament.h>
#include <liphe/first_non_zero.h>

#include "test_framework.h"

int main(int, char**) {
	Helib2Keys keys;

	Helib2Keys::Params params;
	params.set_security_param(80);
	params.p = 101;
	params.p = 313916022213289L;
	//params.p = 1123;
	//params.p = 2543;
	//params.p = 2;
	params.r = 1;
	//params.L = 13*40;

	keys.initKeys(params);
	Helib2Number::set_global_keys(&keys);

	Helib2Number c(2);
	Helib2Number c2;
	clock_t start = clock();
	clock_t end = clock();
	int iter = 0;

	std::cout << "SlotCount = " << keys.nslots() << std::endl;

	for (int i = 0; i < 3; ++i) {
		std::cerr << "trying level " << i << std::endl;
		c *= c;
		std::cerr << "Done. c = " << c.to_int() << std::endl;
	}
	std::ofstream out("ctxt.bin");
	out << c.v();
	exit(1);

	while (end - start < 3*1000*1000) {
		++iter;
		c2 = c*c;
		end = clock();
	}
	std::cout << iter << " mult took " << (end - start) << std::endl;
	exit(1);
}

