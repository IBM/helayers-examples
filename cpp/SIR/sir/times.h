#ifndef ___TIMES___
#define ___TIMES___

#include <stdexcept>
#include <iostream>
#include <string>
#include <time.h>
#include <map>
#include <mutex>

using namespace std;

// Phase I
//    Step 1: key generation
//    Step 2: local computation. Compute compute outer product x_i x_i^T and encrypt it
//    Step 3: dataset merge. Add all outer products
//
// Phase II
//    Step 1: Data masking
//    Step 2: Masked model computation
//    Step 3: Model reconstruction
class Times {
private:
	static std::map<std::string, clock_t> cpuTime;
	static std::map<std::string, time_t> realTime;
	static std::map<std::string, int> depthCount;
	static std::mutex m;
public:
	inline static const std::string KeyGeneration = "Key Generation";
	inline static const std::string EntireSir = "Entire SIR";
	inline static const std::string DataEncoding = "Data Encoding";

	inline static const std::string ScaledRidgeAllIterations = "Scaled Ridge (all iterations)";
	inline static const std::string ScaledRidgeSingleIteration = "Scaled Ridge (single iteration) ";

	inline static const std::string FindSmallestFeatureAllIterations = "Find Smallest Feature(s) (all iterations)";
	inline static const std::string FindSmallestFeatureSingleIteration = "Find Smallest Feature(s) (single iteration) ";

	inline static const std::string SirSingleIteration = "SIR (single iteration) ";
	inline static const std::string ComputePairDifferences = "Compute pair differences";
	inline static const std::string ComputeRanks = "Compute ranks";
	inline static const std::string ComputeThresholds = "Compute thresholds";




	inline static const std::string Phase1Step1 = "Phase1 Step1";
	inline static const std::string Phase1Step2 = "Phase1 Step2";
	inline static const std::string Phase1Step3 = "Phase1 Step3";
	inline static const std::string Phase2Step1 = "Phase2 Step1";
	inline static const std::string Phase2Step2a = "Phase2 Step2a";
	inline static const std::string Phase2Step2b = "Phase2 Step2b";
	inline static const std::string Phase2Step3 = "Phase2 Step3";

	static void start(const std::string &s) {
		std::lock_guard<std::mutex> g(m);

		if (cpuTime.find(s) == cpuTime.end()) {
			cpuTime.insert({s, -clock()});
			realTime.insert({s, -time(NULL)});
			depthCount.insert({s, 1});
		} else {
			cpuTime[s] -= clock();
			realTime[s] -= time(NULL);
			depthCount[s] += 1;
		}
	}

	static void end(const std::string &s) {
		std::lock_guard<std::mutex> g(m);
		if (cpuTime.find(s) == cpuTime.end()) {
			throw std::runtime_error("Unknown time measurement");
		} else {
			cpuTime[s] += clock();
			realTime[s] += time(NULL);
			depthCount[s] -= 1;
		}
	}

	static void print(std::ostream &out) {
		for (auto i = cpuTime.begin(); i != cpuTime.end(); ++i) {
			if (depthCount[i->first] != 0)
				continue;
			clock_t cc = i->second / CLOCKS_PER_SEC;
			time_t tt = realTime[i->first];
			int ratio = 0;
			if (tt > 0)
				ratio = (100*cc/tt);

			out << i->first << " took " << cc << " CPU sec.  and " << tt << " real world sec " << " parallelism ratio of " << ratio << "%" << std::endl;
		}
	}




	// static float time_phase1_step1() { return ((float)phase1_step1 / 1000000); }
	// static float time_phase1_step2() { return ((float)phase1_step2 / 1000000); }
	// static float time_phase1_step3() { return ((float)phase1_step3 / 1000000); }

	// static float time_phase2_step1() { return ((float)phase2_step1 / 1000000); }
	// static float time_phase2_step2a() { return ((float)phase2_step2a / 1000000); }
	// static float time_phase2_step2b() { return ((float)phase2_step2b / 1000000); }
	// static float time_phase2_step3() { return ((float)phase2_step3 / 1000000); }

	// static float time_phase2_step2() { return time_phase2_step2a() + time_phase2_step2b(); }

	// static float time_server1() { return time_phase1_step3() + time_phase2_step1() + time_phase2_step3(); }
	// static float time_server12() { return time_server1() + time_phase2_step2(); }

};

void printTimeStamp(const std::string &s);

class TimeGuard {
private:
	std::string name;
	clock_t c;
	time_t t;
public:
	TimeGuard(const std::string &s) : name(s), c(clock()), t(time(NULL)) { Times::start(s); }
	~TimeGuard() {
		Times::end(name);
		// clock_t cc = (clock() - c) / CLOCKS_PER_SEC;
		// time_t tt = time(NULL) - t;
		// int ratio = 0;
		// if (tt > 0)
		// 	ratio = (100*cc/tt);

		// cout << name << " took " << cc << " CPU sec.  and " << tt << " real world sec " << " which is a ratio of " << ratio << "%" << endl;
	}
};

#endif

