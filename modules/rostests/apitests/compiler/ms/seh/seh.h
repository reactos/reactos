// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include "stdio.h"

#if   ( _MSC_VER >= 800  )

#define try                             __try
#define except                          __except
#define finally                         __finally
#define leave                           __leave

#endif
