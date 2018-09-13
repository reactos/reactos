// triedcid.h : Command IDs for TriEdit's IOleCommandTarget
// Copyright 1998-1999 Microsoft Corporation.  All rights reserved.

#ifndef __TRIEDCID_H__
#define __TRIEDCID_H__

// GUID_TriEditCommandGroup {2582F1C0-084E-11d1-9A0E-006097C9B344}

#define IDM_TRIED_NUDGE_ELEMENT             2   //[in,VT_BYREF VARIANT.byref=LPPOINT]
#define IDM_TRIED_SET_ALIGNMENT             3   //[in,VT_BYREF VARIANT.byref=LPPOINT]
#define IDM_TRIED_MAKE_ABSOLUTE             4
#define IDM_TRIED_LOCK_ELEMENT              5
#define IDM_TRIED_SEND_TO_BACK              6
#define IDM_TRIED_BRING_TO_FRONT            7
#define IDM_TRIED_SEND_BACKWARD             8
#define IDM_TRIED_BRING_FORWARD             9
#define IDM_TRIED_SEND_BELOW_TEXT          10 
#define IDM_TRIED_BRING_ABOVE_TEXT         11
#define IDM_TRIED_CONSTRAIN                12   //[in,VT_BOOL]
#define IDM_TRIED_ABSOLUTE_DROP_MODE       13   //[in,VT_BOOL]
#define IDM_TRIED_INSERTROW                14
#define IDM_TRIED_INSERTCOL                15
#define IDM_TRIED_DELETEROWS               16
#define IDM_TRIED_DELETECOLS               17
#define IDM_TRIED_MERGECELLS               18
#define IDM_TRIED_SPLITCELL                19
#define IDM_TRIED_INSERTCELL               20
#define IDM_TRIED_DELETECELLS              21
#define IDM_TRIED_INSERTTABLE              22   //[in, VT_ARRAY]
#define IDM_TRIED_ACTIVATEACTIVEXCONTROLS  23
#define IDM_TRIED_ACTIVATEAPPLETS          24
#define IDM_TRIED_ACTIVATEDTCS             25
#define IDM_TRIED_BACKCOLOR                26
#define IDM_TRIED_BLOCKFMT                 27
#define IDM_TRIED_BOLD                     28
#define IDM_TRIED_BROWSEMODE               29
#define IDM_TRIED_COPY                     30
#define IDM_TRIED_CUT                      31
#define IDM_TRIED_DELETE                   32
#define IDM_TRIED_EDITMODE                 33
#define IDM_TRIED_FIND                     34
#define IDM_TRIED_FONT                     35
#define IDM_TRIED_FONTNAME                 36
#define IDM_TRIED_FONTSIZE                 37
#define IDM_TRIED_FORECOLOR                38
#define IDM_TRIED_GETBLOCKFMTS             39
#define IDM_TRIED_HYPERLINK                40
#define IDM_TRIED_IMAGE                    41
#define IDM_TRIED_INDENT                   42
#define IDM_TRIED_ITALIC                   43
#define IDM_TRIED_JUSTIFYCENTER            44
#define IDM_TRIED_JUSTIFYLEFT              45
#define IDM_TRIED_JUSTIFYRIGHT             46
#define IDM_TRIED_ORDERLIST                47
#define IDM_TRIED_OUTDENT                  48
#define IDM_TRIED_PASTE                    50
#define IDM_TRIED_PRINT                    51
#define IDM_TRIED_REDO                     52
#define IDM_TRIED_REMOVEFORMAT             53
#define IDM_TRIED_SELECTALL                54
#define IDM_TRIED_SHOWBORDERS              55
#define IDM_TRIED_SHOWDETAILS              56
#define IDM_TRIED_UNDERLINE                57
#define IDM_TRIED_UNDO                     58
#define IDM_TRIED_UNLINK                   59
#define IDM_TRIED_UNORDERLIST              60
#define IDM_TRIED_DOVERB                   61

//WARNING WARNING WARNING!!! Don't forget to modify IDM_TRIED_LAST_CID
//when you add new Command IDs

#define IDM_TRIED_LAST_CID             IDM_TRIED_DOVERB

//These are old command IDs that are here for backward compatibility
//We don't recommend using these any more
#define IDM_TRIED_IS_1D_ELEMENT         0   //[out,VT_BOOL]
#define IDM_TRIED_IS_2D_ELEMENT         1   //[out,VT_BOOL]
#define IDM_TRIED_SEND_TO_FRONT         IDM_TRIED_BRING_TO_FRONT
#define IDM_TRIED_SEND_FORWARD          IDM_TRIED_BRING_FORWARD
#define IDM_TRIED_SEND_BEHIND_1D        IDM_TRIED_SEND_BELOW_TEXT
#define IDM_TRIED_SEND_FRONT_1D         IDM_TRIED_BRING_ABOVE_TEXT
#define IDM_TRIED_SET_2D_DROP_MODE      IDM_TRIED_ABSOLUTE_DROP_MODE

#endif //__TRIEDCID_H__
