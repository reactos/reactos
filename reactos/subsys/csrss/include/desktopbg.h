/* $Id: desktopbg.h,v 1.2 2004/01/11 17:31:15 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/destkopbg.h
 * PURPOSE:         CSRSS internal desktop background window interface
 */

#ifndef DESKTOPBG_H_INCLUDED
#define DESKTOPBG_H_INCLUDED

#include "api.h"

/* Api functions */
CSR_API(CsrCreateDesktop);
CSR_API(CsrShowDesktop);
CSR_API(CsrHideDesktop);

BOOL FASTCALL DtbgIsDesktopVisible(VOID);

#endif /* DESKTOPBG_H_INCLUDED */

/* EOF */

