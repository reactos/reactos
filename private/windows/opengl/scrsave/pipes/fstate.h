/******************************Module*Header*******************************\
* Module Name: fstate.h
*
* FLEX_STATE
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __fstate_h__
#define __fstate_h__

#include "sscommon.h"
#include "state.h"
#include "pipe.h"

class PIPE;
class STATE;

class FLEX_STATE {
public:
    int             scheme;         // current drawing scheme (right now this
                                    // is a per-frame thing)
    BOOL            bTexture;       // mf: repetition
    FLEX_STATE( STATE *pState );
    PIPE*           NewPipe( STATE *pState );
    void            Reset();
    BOOL            OKToUseChase();
    int             GetMaxPipesPerFrame();
};

#endif // __fstate_h__
