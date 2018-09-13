/******************************Module*Header*******************************\
* Module Name: dlgdraw.hxx
*
* 
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __dlgdraw_hxx__
#define __dlgdraw_hxx__

#include "sscommon.h"
#include "sswindow.hxx"

/**************************************************************************\
* SS_TEX_BUTTON
*
* Texture wrapper for a dialog button.  The object's current texture is a
* pointer to a TEXTURE, so the caller must manage texture memory.
\**************************************************************************/

class SS_TEX_BUTTON {
public:
    SS_TEX_BUTTON( HWND hdlg, HWND hdlgBtn );
    ~SS_TEX_BUTTON();
    void    Draw();     // Use current texture
    void    Draw( TEXTURE *pTex );  // Use supplied texture
    void    SetTexture( TEXTURE *pTex );    // Set current texture
    void    Enable() { bEnabled = TRUE; };  // Set enabled state
    void    Disable() { bEnabled = FALSE; }; // Set disabled state
private:
    TEXTURE *pCurTex;          // Current texture for this button
    int     intensity;
    BOOL    bEnabled;
    PSSW    pssw;
    IPOINT2D origin;
    ISIZE size;

    void    InitGL();       // Sets texture params, etc.
};

#endif // __dlgdraw_hxx__
