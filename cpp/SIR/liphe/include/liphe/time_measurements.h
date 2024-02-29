#ifndef ___TIME_MEASUREMENTS___
#define ___TIME_MEASUREMENTS___

#include <assert.h>
#include <sys/time.h>
#include <sstream>
#include <ostream>
#include <iostream>

class TakeTimes {
	time_t _start;
	time_t _total;

	time_t _real_start;
	time_t _real_total;

public:
	TakeTimes() : _start(0), _total(0), _real_start(0), _real_total(0) {}

	void start() {
		assert(_start == 0);
		_start = clock();

		struct timeval tv;
		gettimeofday(&tv, NULL);

		assert(_real_start == 0);
		_real_start = tv.tv_sec*1000000 + tv.tv_usec;
	}

	std::string stats(const std::string &label) {
		std::stringstream str;

//		str << label << " took " << _total << " cpu and " << _real_total << " time";
//		if (_real_total > 0)
//			str << ", which is a parallelization factor of " << (0.01*((int) (_total / 10000) / _real_total));

		str << label << " took " << (0.01*(_total / 10000)) << " cpu sec and " << (0.01*(_real_total/10000)) << " sec in real time";
		if (_real_total > 0)
			str << ", which is a parallelization factor of " << (0.01*((int) (100 * _total  / _real_total)));

		str << std::endl;
		return str.str();
	}

	// deprecated
	std::string end(const std::string &label) {
		end();
		std::stringstream str;
		str << print(label) << std::endl;
		return str.str();
	}

	void end() {
		time_t add = clock() - _start;

		_total += add;
		_start = 0;

		struct timeval tv;
		gettimeofday(&tv, NULL);

		time_t real_add = tv.tv_sec*1000000 + tv.tv_usec - _real_start;

		_real_total += real_add;
		_real_start = 0;
	}

	std::string print(const std::string &label = "") const {
		std::stringstream str;
		str << label << " took " << (0.01*(_total / 10000)) << " cpu sec and " << (0.01*(_real_total/10000)) << " sec in real time";
		if (_real_total > 0)
			str << ", which is a parallelization factor of " << (0.01*((int) (100 * _total  / _real_total)));
		return str.str();
	}
};

inline std::ostream &operator<<(std::ostream &s, const TakeTimes &t) { s << t.print(); return s; }

class AutoTakeTimes {
private:
	std::string _s;
	TakeTimes _t;
public:
	AutoTakeTimes(const std::string &s) : _s(s) { _t.start(); }
	~AutoTakeTimes() { std::cout << _t.end(_s); }
};

#endif
