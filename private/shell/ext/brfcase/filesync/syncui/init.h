//
// init.h: Declares data, defines and struct types for DLL entry point
//          module.
//
//

#ifndef __INIT_H__
#define __INIT_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

#define CX_IMG      16
#define CY_IMG      16

// Indexes into our image list
//
#define IMAGE_FOLDER        0       // folder
#define IMAGE_OFOLDER       1       // open folder

#define MyGetTwinResult()           (g_tr)
#define MySetTwinResult(tr)         (g_tr = (tr))

/////////////////////////////////////////////////////  TYPEDEFS


/////////////////////////////////////////////////////  EXPORTED DATA

extern HINSTANCE   g_hinst;
extern TWINRESULT  g_tr;

extern HANDLE      g_hMutexDelay;

extern int g_cxIconSpacing;
extern int g_cyIconSpacing;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxIcon;
extern int g_cyIcon;
extern int g_cxIconMargin;
extern int g_cyIconMargin;
extern int g_cxLabelMargin;
extern int g_cyLabelSpace;
extern int g_cxMargin;

extern COLORREF g_clrHighlightText;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrWindow;

extern HBRUSH g_hbrHighlight;
extern HBRUSH g_hbrWindow;

extern TCHAR g_szDBName[];
extern TCHAR g_szDBNameShort[];

extern int g_cProcesses;
extern UINT g_cfBriefObj;

extern UINT g_uBreakFlags;       // Controls when to int 3
extern UINT g_uTraceFlags;       // Controls what trace messages are spewed
extern UINT g_uDumpFlags;        // Controls what structs get dumped

/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

#endif // __INIT_H__

