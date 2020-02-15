#include "version.h"

#define __BDATE__      "2020-02-15 17:27:26"
#define __BVERSION__   "1.0.002"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
