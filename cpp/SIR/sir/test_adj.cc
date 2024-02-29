#include <liphe/zp.h>
#include "matrix.h"

int main(int, char**) {
	ZP::set_global_p(11);
	Matrix<ZP> test(3,3);

	int rand  = 1;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			test(i, j) = rand;
			rand = (rand + i + 1) % 7;
			std::cout << rand << " ";
		}
	}

	std::cout << "test = " << test << std::endl;

	Matrix<ZP> inv1 = test.inverse();
	std::cout << "inv1 = " << inv1 << std::endl;

	Matrix<ZP> adj = test.adjoint();
//	std::cout << "adj = " << adj << std::endl;

	ZP det = test.determinant();
//	std::cout << "det = " << det.to_int() << std::endl;

	std::cout << "inv2 = " << (adj * det.inv()) << std::endl;
}
