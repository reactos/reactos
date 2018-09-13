
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BROWSEUI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BROWSEUI_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef BROWSEUI_EXPORTS
#define BROWSEUI_API __declspec(dllexport)
#else
#define BROWSEUI_API __declspec(dllimport)
#endif

// This class is exported from the browseui.dll
class BROWSEUI_API CBrowseui {
public:
	CBrowseui(void);
	// TODO: add your methods here.
};

extern BROWSEUI_API int nBrowseui;

BROWSEUI_API int fnBrowseui(void);

