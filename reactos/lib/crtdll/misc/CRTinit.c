/*
 * CRTinit.c
 *
 * A dummy version of _CRT_INIT for MS compatibility. Programs, or more often
 * dlls, which use the static version of the MSVC run time are supposed to
 * call _CRT_INIT to initialize the run time library in DllMain. This does
 * not appear to be necessary when using crtdll or the dll versions of the
 * MSVC runtime, so the dummy call simply does nothing.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1 $
 * $Author: rex $
 * $Date: 1999/01/16 02:11:43 $
 *
 */

void
_CRT_INIT ()
{
}

