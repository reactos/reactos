/******************************Module*Header*******************************\
* Module Name: npipe.h
*
* Node stuff
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __npipe_h__
#define __npipe_h__

#include "sscommon.h"
#include "state.h"
#include "pipe.h"

class NORMAL_STATE;

class NORMAL_PIPE: public PIPE {
private:
public:
    NORMAL_STATE      *pNState;

    NORMAL_PIPE( STATE *state );
    void        Start();
    GLint       ChooseElbow( int oldDir, int newDir);
    void        DrawJoint( int newDir );
    void        Draw( ); //mf: could take param to draw n sections
    void        DrawStartCap( int newDir );
    void        DrawEndCap();
};

#endif // __npipe_h__
