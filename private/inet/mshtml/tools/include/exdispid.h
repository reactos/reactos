#ifndef EXDISPID_H_
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1995 - 1996 Microsoft Corporation. All Rights Reserved.
//
//  File: exdispid.h
//
//--------------------------------------------------------------------------


//
// Dispatch IDS for IExplorer Dispatch Events.
//
#define DISPID_BEFORENAVIGATE     100   // this is sent before navigation to give a chance to abort
#define DISPID_NAVIGATECOMPLETE   101   // in async, this is sent when we have enough to show
#define DISPID_STATUSTEXTCHANGE   102
#define DISPID_QUIT               103
#define DISPID_DOWNLOADCOMPLETE   104
#define DISPID_COMMANDSTATECHANGE 105
#define DISPID_DOWNLOADBEGIN      106
#define DISPID_NEWWINDOW          107   // sent when a new window should be created
#define DISPID_PROGRESSCHANGE     108   // sent when download progress is updated
#define DISPID_WINDOWMOVE         109   // sent when main window has been moved
#define DISPID_WINDOWRESIZE       110   // sent when main window has been sized
#define DISPID_WINDOWACTIVATE     111   // sent when main window has been activated
#define DISPID_PROPERTYCHANGE     112   // sent when the PutProperty method is called
#define DISPID_TITLECHANGE        113   // sent when the document title changes

#define DISPID_FRAMEBEFORENAVIGATE    200
#define DISPID_FRAMENAVIGATECOMPLETE  201
#define DISPID_FRAMENEWWINDOW         204


#define EXDISPID_H_
#endif // EXDISPID_H_
