#include "liphe/helib_keys.h"
#include "liphe/helib_number.h"

HelibKeys *HelibNumber::_prev_keys = NULL;
std::function<HelibKeys *(void)> HelibNumber::_getKeys = HelibNumber::getPrevKeys;
