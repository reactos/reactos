/* $Id: desktopbg.h 47315 2010-05-23 00:51:29Z jmorlan $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/destkopbg.h
 * PURPOSE:         CSRSS internal desktop background window interface
 */

#pragma once

#include "api.h"

/* Api functions */
CSR_API(CsrCreateDesktop);
CSR_API(CsrShowDesktop);
CSR_API(CsrHideDesktop);
CSR_API(CsrRegisterSystemClasses);

BOOL FASTCALL DtbgIsDesktopVisible(VOID);

/* EOF */
