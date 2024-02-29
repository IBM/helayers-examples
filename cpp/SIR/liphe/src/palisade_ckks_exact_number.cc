#include "liphe/palisade_ckks_exact_keys.h"
#include "liphe/palisade_ckks_exact_number.h"

PalisadeCkksExactKeys *PalisadeCkksExactNumber::_prev_keys = NULL;
std::function<PalisadeCkksExactKeys *(void)> PalisadeCkksExactNumber::_getKeys = PalisadeCkksExactNumber::getPrevKeys;
bool PalisadeCkksExactNumber::_lazyRescale = false;
