#ifndef I_NDUMP_HXX_
#define I_NDUMP_HXX_
#pragma INCMSG("--- Beg 'ndump.hxx'")

//+--------------------------------------------------------------------------
//
// MACROS
//
//+--------------------------------------------------------------------------

extern char g_achClassBegin[];
extern char g_achPropBegin [];

#define DUMP_PROPERTY(pName, pValue)    dc.PrintLevelBlanks (); \
                                        dc.Printf(pStr, pName); \
                                        dc << pValue;           \
                                        dc.PrintNewLine ();


#define DUMP_FIRST_PROPERTY(pClassName, pName, pValue)                  \
                                    char *pStr = g_achClassBegin;       \
                                    dc.PrintLevelBlanks ();             \
                                    dc.Printf(pStr, pClassName, pName); \
                                    dc << pValue;                       \
                                    dc.PrintNewLine ();                 \
                                    pStr = g_achPropBegin;


/////////////////////////////////////////////////////////////////////////////
// Diagnostic dumping

class CDumpContext
{
public:
//  CDumpContext(CFile* pFile);
    CDumpContext() { _iObjectLevel = 0; m_pFile = NULL; m_nDepth = 0; }
    CDumpContext(const CDumpContext&) { _iObjectLevel = 0; m_pFile = NULL; m_nDepth = 0;}

// Attributes
    int GetDepth() const;      // 0 => this object, 1 => children objects
    void SetDepth(int nNewDepth);

// Operations
    CDumpContext& operator<<(LPCTSTR lpsz);
#ifdef _UNICODE
    CDumpContext& operator<<(LPCSTR lpsz);  // automatically widened
#else
#ifndef _MAC
//    CDumpContext& operator<<(LPCWSTR lpsz); // automatically thinned
#endif
#endif
    CDumpContext& operator<<(const void* lp);
    CDumpContext& operator<<(BYTE by);
    CDumpContext& operator<<(WORD w);
    CDumpContext& operator<<(UINT u);
    CDumpContext& operator<<(LONG l);
    CDumpContext& operator<<(DWORD dw);
    CDumpContext& operator<<(float f);
    CDumpContext& operator<<(double d);
    CDumpContext& operator<<(int n);
    CDumpContext& operator<<(RECTL& rcl);

    void HexDump(LPCTSTR lpszLine, BYTE* pby, int nBytes, int nWidth);
    void Flush();

    void IncLevel ();               // increment hierarchy level while printing object hierarchy
    void DecLevel ();               
    void PrintLevelBlanks ();
    void Printf(char *pvOutput, ...);
    void PrintNewLine ();

// Implementation
protected:
    // dump context objects cannot be copied or assigned
    void OutputString(LPCTSTR lpsz);

    int m_nDepth;
    int _iObjectLevel;              // object hierarchy level

public:
    // CFile* m_pFile;
    char * m_pFile;
};

#if DBG == 1
#       ifndef IS_VALID
#           define IS_VALID(this) ((this)->IsValidObject())
#           define DUMP(this,dc)  ((this)->Dump(dc))
#           define NDUMP(this)    ((this)->Dump(nautilusDump))
#       endif
#   define DEBUG_METHODS                  \
    virtual BOOL IsValidObject();         \
    virtual void Dump(CDumpContext &dc);  \
    virtual char*GetClassName();
#else _DEBUG
#   ifndef IS_VALID
#       define IS_VALID(this)   (1)
#       define DUMP(this, dc)   (1)
#       define NDUMP(this)      (1)
#   endif
#   define DEBUG_METHODS
#endif DBG == 1

#pragma INCMSG("--- End 'ndump.hxx'")
#else
#pragma INCMSG("*** Dup 'ndump.hxx'")
#endif
