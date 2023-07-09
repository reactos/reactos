#ifndef _FADETSK_H
#define _FADETSK_H

#include <runtask.h>
extern const GUID TASKID_Fader;

#define FADE_BEGIN          0x00000001
#define FADE_END            0x00000002

typedef void (*PFNFADESCREENRECT)(DWORD dwFadeState, LPVOID pvParam); // Called after the Fade has begun

class CFadeTask : public CRunnableTask
{
public:
    // IRunnableTask methods (override)
    virtual STDMETHODIMP RunInitRT(void);

    CFadeTask();
    void _StopFade();
    BOOL FadeRect(PRECT prc, PFNFADESCREENRECT pfn, LPVOID pvParam);

private:
    virtual ~CFadeTask();

    HWND        _hwndFader;
    RECT        _rect;
    PFNFADESCREENRECT _pfn;
    LPVOID      _pvParam;
    HDC         _hdcFade;
    HBITMAP     _hbm;
    HBITMAP     _hbmOld;
};

#endif
