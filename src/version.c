#include "version.h"

#define __BDATE__      "2020-02-15 14:37:01"
#define __BVERSION__   "1.0.001"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
