// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include "stdio.h"

#if   ( _MSC_VER >= 800  )

#define try                             __try
#define except                          __except
#define finally                         __finally
#define leave                           __leave
#define endtry
#define gcc_volatile

#else

#include <pseh/pseh2.h>

#define try     _SEH2_TRY
#define except  _SEH2_EXCEPT
#define finally _SEH2_FINALLY
#define leave   _SEH2_LEAVE
#define endtry  _SEH2_END
#define abnormal_termination _abnormal_termination
#define gcc_volatile volatile

#endif
