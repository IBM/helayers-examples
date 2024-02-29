#include "liphe/palisade_ckks_keys.h"
#include "liphe/palisade_ckks_number.h"

PalisadeCkksKeys *PalisadeCkksNumber::_prev_keys = NULL;
std::function<PalisadeCkksKeys *(void)> PalisadeCkksNumber::_getKeys = PalisadeCkksNumber::getPrevKeys;
