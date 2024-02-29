#ifndef __UTILS__
#define __UTILS__

inline bool is_power_of_2(unsigned long a) {
	while (a > 1) {
		if ((a & 1) == 1)
			return false;
		a /= 2;
	}
	return true;
}

#endif
