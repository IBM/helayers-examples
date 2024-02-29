#ifndef ___RATIONAL_RECONSTRUCTION___
#define ___RATIONAL_RECONSTRUCTION___

#include <stdexcept>
#include <algorithm>

inline int findGcd(int x, int y) {
	if (x > y)
		std::swap(x, y);

	assert(x >= 0);

	while ((y % x) != 0) {
		y = y % x;
		std::swap(x, y);
	}

	return x;
}

inline void rational_reconstruction(int &num, int &denom, int quontient, int m) {
	int u1(1), u2(0), u3(m);
	int v1(0), v2(1), v3(quontient);

    while (sqrt(m/2) <= v3) {
        int q = int(u3/v3);
        int r1((u1-q*v1)%m), r2((u2-q*v2)%m), r3((u3-q*v3)%m);
//        int r1((u1-q*v1)), r2((u2-q*v2)), r3((u3-q*v3));
		u1=v1; u2=v2; u3=v3;
		v1=r1; v2=r2; v3=r3;
		if (v2 > sqrt(m/2)) {
			std::cout << "Error in rational reconstruction. Ring size is too small" << std::endl;
//			throw std::runtime_error(std::string("Error in rational reconstruction. Ring size is too small"));
		}
	}

	num = v3;
	denom = v2;
	return;

//	while (v2 < 0)
//		v2 += m;
//	while (v3 < 0)
//		v3 += m;
//
//	int gcd = findGcd(v3, v2);
//	num = v3 / gcd;
//	denom = v2 / gcd;
}

#endif
