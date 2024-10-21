#ifndef _IT_COMMON_H
#define _IT_COMMON_H

#ifdef IT_ENABLE

#include "DelegateLib.h"
#define IT_PRIVATE_ACCESS public

#else

#define IT_PRIVATE_ACCESS

#endif // IT_ENABLE

#endif