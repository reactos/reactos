/**************************************************************************
*
* Copyright 2016-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  FNET assert macros.
*
***************************************************************************/
#ifndef _FNET_ASSERT_H_
#define _FNET_ASSERT_H_

#define FNET_ASSERT_ERROR_MESSAGE   "ASSERT: %s: %s: %d"

#if FNET_CFG_ASSERT

/* Checks that the given condition is true, otherwise it prints error message
 * and stops the program execution.*/
#define FNET_ASSERT(condition)                                              \
    if (!(condition))                                                       \
    {                                                                       \
        fnet_println(FNET_ASSERT_ERROR_MESSAGE, __func__, __FILE__, __LINE__);    \
        while (1);                                                          \
    }
#else
#define FNET_ASSERT(condition)  ((void) 0)
#endif

/* Assert which is not possible to disable */
#define FNET_ASSERT_CRITICAL(condition)                                     \
    if (!(condition))                                                       \
    {                                                                       \
        fnet_println(FNET_ASSERT_ERROR_MESSAGE, __func__, __FILE__, __LINE__);    \
        while (1);                                                          \
    }
#endif  /* _FNET_ASSERT_H_ */

