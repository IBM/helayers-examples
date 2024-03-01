#include <iostream>
#include <time.h>
#include "times.h"

std::mutex Times::m;
std::map<std::string, clock_t> Times::cpuTime;
std::map<std::string, time_t> Times::realTime;
std::map<std::string, int> Times::depthCount;

static time_t startingTime = time(NULL);

void printTimeStamp(const std::string &s) {
	std::cout << "Time at '" << s << "' " << (time(NULL) - startingTime) << std::endl;
}
