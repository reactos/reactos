#ifndef INCLUDED_GLOBALS_H
#define INCLUDED_GLOBALS_H

extern HINSTANCE g_hInstance;
extern DWORD g_dwTlsIndex;


//
// This structure holds all the global data used by the app.
//
class PerThreadGlobals
{
    public:
        INT DialogBaseUnitsX;          // For du<->pixel conversion.
        INT DialogBaseUnitsY;          // For du<->pixel conversion.
        INT cxSmallIcon;               // Icon sizes.
        INT cySmallIcon;               // ...
        INT cxLargeIcon;               // ...
        INT cyLargeIcon;               // ...
        INT cxVertScrollBar;           // Width of a vertical scroll bar.
        INT iLastItemHit;              // Index of last LV item hit.
        BOOL bListviewSingleClick;     // Use single-click selection.
        BOOL bWaitCursor;              // Should display wait cursor?
        HWND hwndTopicList;            // Hwnd of topic list view object.
        CFont hfontTextNormal;         // Font used for unselected topic titles.
        CFont hfontTextHighlight;      // ... for selected topic titles.
        CFont hfontBanner;             // ... for banner text.
        HICON hiconApplicationSm;      // Icons used.
        HICON hiconApplicationLg;      // ...
        HACCEL haccelKeyboard;         // Keyboard accelerators.
        HCURSOR hcursorHand;           // Hand cursor.
        HCURSOR hcursorWait;           // Hourglass cursor.
        CDIB rgdibTopicSpot[2];        // Topic menu item "spot" bitmaps.
        CDIB dibBanner;                // Header banner watermark bitmap.
        COLORREF clrTopicTextNormal;   // Text colors.
        COLORREF clrTopicTextHighlight;
        COLORREF clrTopicTextBackground;
        SettingsFolder Folder;
        SETTINGS_FOLDER_TOPIC *pICurrentTopic;
        PTRAYPROPSHEETCALLBACK pfnTaskbarPropSheetCallback;
        CDC dcMem;                     // For Blt'ing bitmaps.
                                       // Place last so it's deleted last.

        PerThreadGlobals(VOID);
        ~PerThreadGlobals(VOID);

        VOID OnSysColorChange(VOID);
        VOID OnDisplayChange(VOID);

    private:
        PerThreadGlobals(const PerThreadGlobals& rhs);
        PerThreadGlobals& operator = (const PerThreadGlobals& rhs);
};


//
// Function for returning a reference to the per-thread-storage.
// Use them like this:
//
//   PTG.cxSmallIcon = 16;
//
//   x = PTG.cySmallIcon;
//
inline PerThreadGlobals& PerThdGlbl(VOID)
{
    Assert(TLS_OUT_OF_INDEXES != g_dwTlsIndex);
    Assert(NULL != TlsGetValue(g_dwTlsIndex));
    return *((PerThreadGlobals *)TlsGetValue(g_dwTlsIndex));
}

#define PTG  PerThdGlbl()

#endif // INCLUDED_GLOBALS_H
