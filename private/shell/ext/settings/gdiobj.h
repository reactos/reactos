#ifndef INCLUDED_GDIOBJ_H
#define INCLUDED_GDIOBJ_H

#ifndef STRICT
#define STRICT  // Needed so HDC is a different type than HWND.
#endif

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

//
// These classes are not intended to be a full functional class library
// like you would find with MFC.  My intent was to have a simple set of
// wrapper classes that will ensure the proper cleanup of GDI objects
// used in the settings UI.
//
class CGDIObject
{
    public:
        CGDIObject(HANDLE handle = NULL);
        virtual ~CGDIObject(VOID);

        class Exception { };

    protected:
        HANDLE m_handle;
};


class CFont : public CGDIObject
{
    public:
        CFont(VOID) { }
        CFont(const LOGFONT& lf);
        ~CFont(VOID) { }

        CFont& operator = (const LOGFONT& lf);

        operator HFONT() const
            { return (HFONT)m_handle; }

        BOOL GetLogFont(LPLOGFONT plf) const;

    private:
        CFont(const CFont& rhs);
        CFont& operator = (const CFont& rhs);
};



class CDC : public CGDIObject
{
    public:
        CDC(HWND hwnd = NULL);
        CDC(HDC hdc);
        virtual ~CDC(VOID);

        operator HDC() const
            { return (HDC)m_handle; }

    private:
        class RealDC : public CGDIObject
        {
            public:
                RealDC(HANDLE& refHandle)
                    : m_refHandle(refHandle) { }
                virtual ~RealDC(VOID) { };

            protected:
                HANDLE& m_refHandle;

            private:
                RealDC(const RealDC& rhs);
                RealDC& operator = (const RealDC& rhs);
        };

        class WindowDC : public RealDC
        {
            public:
                WindowDC(HANDLE& refHandle, HWND hwnd = NULL);
                ~WindowDC(VOID);

            private:
                HWND m_hwnd;

                WindowDC(const WindowDC& rhs);
                WindowDC& operator = (const WindowDC& rhs);
        };

        class CompatibleDC : public RealDC
        {
            public:
                CompatibleDC(HANDLE& refHandle, HDC hdc = NULL);
                ~CompatibleDC(VOID);

            private:
                CompatibleDC(const CompatibleDC& rhs);
                CompatibleDC& operator = (const CompatibleDC& rhs);
        };

        RealDC *m_pRealDC;

        CDC(const CDC& rhs);
        CDC& operator = (const CDC& rhs);
};


class CDIB : public CGDIObject
{
    public:
        CDIB(VOID);
        CDIB(HINSTANCE hInstance, LPCTSTR pszResource);
        ~CDIB(VOID);

        operator HBITMAP() const
            { return (HBITMAP)m_handle; }

        operator HPALETTE() const
            { return (HPALETTE)m_hPalette; }

        VOID GetRect(LPRECT prc);
        VOID GetBitmapInfo(LPBITMAP pbm);
        VOID Load(HINSTANCE, LPCTSTR pszResource);

    private:
        HPALETTE m_hPalette;
        BITMAP   m_bitmap;

        CDIB(const CDIB& rhs);
        CDIB& operator = (const CDIB& rhs);
        HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString, HPALETTE *lphPalette);
        HPALETTE CreateDIBPalette(LPBITMAPINFO lpbmi, LPINT lpiNumColors);
};

class CBrush : public CGDIObject
{
    public:
        CBrush(VOID);
        CBrush(COLORREF clrRGB);

        operator HBRUSH() const
            { return (HBRUSH)m_handle; }

        CBrush& operator = (COLORREF clrRGB);

    private:
        CBrush(const CBrush& rhs);
        CBrush& operator = (const CBrush& rhs);
};


#endif // INCLUDED_GDI_OBJECTS

