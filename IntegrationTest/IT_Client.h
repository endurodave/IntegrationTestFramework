#ifndef _IT_CLIENT_H
#define _IT_CLIENT_H

// Client helper include file used within production code

#ifdef IT_ENABLE

// Allow production code to use delegate library
#include "DelegateLib.h"

// Allow integration tests to access production code private members
#define IT_PRIVATE_ACCESS public

#else

#define IT_PRIVATE_ACCESS

#endif // IT_ENABLE

#endif