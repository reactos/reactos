/*
 * PROJECT:     ReactOS api tests (lsass.exe)
 * LICENSE:     GPLv2+ - See COPYING in the top level directory
 * PURPOSE:     shared definitions between exe (remote) and dll (real test)
 * PROGRAMMERS: Andreas Maier <staubim@quantentunnel.de> (2019)
 */

#ifndef _MSV1_0COMMON_H_
#define _MSV1_0COMMON_H_

/* name of pipes */
#define PIPE_NAME_CMD  L"\\\\.\\pipe\\msv1_0_apitest_cmd"
#define PIPE_NAME_OUT  L"\\\\.\\pipe\\msv1_0_apitest_out"

#endif
