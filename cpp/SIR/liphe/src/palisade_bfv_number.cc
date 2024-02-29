#include "liphe/palisade_bfv_keys.h"
#include "liphe/palisade_bfv_number.h"

PalisadeBfvKeys *PalisadeBfvNumber::_prev_keys = NULL;
std::function<PalisadeBfvKeys *(void)> PalisadeBfvNumber::_getKeys = PalisadeBfvNumber::getPrevKeys;
