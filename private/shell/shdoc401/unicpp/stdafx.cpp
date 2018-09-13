#include "stdafx.h"
#pragma hdrstop

// windowsx.h has a macro called SubclassWindow, which conflicts with
// ATL members with the same name.  This sucks.

#undef SubclassWindow

//-----------------------------------------------------------------------------
//
// statreg.cpp
//

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

//-----------------------------------------------------------------------------
//
// atlctl.cpp
//

// We shouldn't ever do this because we broke it in the interest of
// severing ties with OLEAut32.  DeskMovr doesn't need this.
#define OleCreatePropertyFrame(a,b,c,d,e,f,g,h,i,j,k) Bad_OCPF()

inline HRESULT Bad_OCPF(void) {
    ASSERT(FALSE);
    return OLEOBJ_S_CANNOT_DOVERB_NOW;
}

#include "atlctl.cpp"

#undef OleCreatePropertyFrame

// End of atlctl.cpp hackitude.

//-----------------------------------------------------------------------------
//
// atlimpl.cpp
//

// The use of ocscpy in atlimpl.cpp is not safe because it can overflow
// the target buffer.  Change it to a safe version.

#define ocscpy(dst, src) StrCpyNW(dst, src, ARRAYSIZE(dst))

#include "atlimpl.cpp"

#undef ocscpy

// End of atlimpl.cpp hackitude.

//-----------------------------------------------------------------------------
//
// atlwin.cpp
//

#include "atlwin.cpp"
