#include "liphe/zp.h"

unsigned long ZP::_prev_simd_size = 20;

long ZP::_prev_p = 2;

long ZP::_prev_r = 1;

std::function<long(void)> ZP::_getR = ZP::getPrevR;

std::function<long(void)> ZP::_getP = ZP::getPrevP;

std::function<unsigned long(void)> ZP::_getSimdSize = ZP::getPrevSimdSize;

