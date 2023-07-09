#ifndef __BMOBJECT_H__
#define __BMOBJECT_H__

class CBmObjSequence;

// Get*Prop return type
enum GPT
    {
    invalid,    // Not a known property or disabled
    valid,      // Value is correct
    ambiguous   // Multiple selection with different values
    };

// Selection Object
class CFileBuffer;

class CBitmapObj : public CObject
    {
    DECLARE_DYNCREATE( CBitmapObj )

    public:

    CBitmapObj();

    ~CBitmapObj();

    void Clear();

    void InformDependants( UINT idChange );
    void AddDependant    ( CBitmapObj* newDependant );
    void RemoveDependant ( CBitmapObj* oldDependant );

    // Notification Callback
    void OnInform( CBitmapObj* pChangedSlob, UINT idChange );

    // Property Management
    BOOL SetIntProp (UINT idProp, int val);
    GPT  GetIntProp (UINT idProp, int& val);
    BOOL SetSizeProp(UINT nPropID, const CSize& val);

    BOOL MakeEmpty       ();
    BOOL Import          ( LPCTSTR szFileName );
    BOOL Export          ( LPCTSTR szFileName );

    // Specify the type of header to put on a resource
    typedef enum _PBResType
    {
        rtFile,
        rtDIB,
        rtPaintOLEObj,
        rtPBrushOLEObj,
    } PBResType;

    BOOL ReadResource    ( LPBITMAPINFOHEADER lpbi );
    BOOL ReadResource    ( CFile* pfile, PBResType rtType = rtFile );
    BOOL WriteResource   ( CFile* pfile, PBResType rtType = rtFile );

#ifdef PCX_SUPPORT
    BOOL ReadPCX         ( CFile* pfile );
    BOOL WritePCX        ( CFile* pfile );
    BOOL PackBuff        ( CFileBuffer *FileBuffer, BYTE *PtrDib, int byteWidth);
#endif


    BOOL CreateImg       ();
    BOOL SaveResource    ( BOOL bClear = TRUE );
    void ReLoadImage     ( CPBDoc* pbDoc );
    void UndoAction      ( CBmObjSequence* pSeq, UINT nActionID );
    void DeleteUndoAction( CBmObjSequence* pSeq, UINT nActionID );
    BOOL FinishUndo      ( const CRect* pRect );

    // Resource data access helpers...
    BOOL    Alloc(); // m_lpvThing of size m_lMemSize
    void    Free();                 // m_lpvThing and set m_lMemSize to zero
    void    Zap();  // frees memory and zeros out the file position
                    // information - used to completely empty a resobject

    CString GetDefExtension(int iStringId=0);

    BOOL SetupForIcon( HBITMAP& hBitmap, HBITMAP& hMaskBitmap );

    // Load m_lpvThing with the resource data from the res file
    inline  LPVOID GetData() { ASSERT(m_lpvThing != NULL); return m_lpvThing; }
    inline  DWORD GetDataSize() const { return m_lMemSize; }
    inline  BOOL  IsDirty() const { return m_bDirty; }

    void SetDirty(BOOL bDirty = TRUE);

#ifdef  ICO_SUPPORT
    BOOL IsSaveIcon() { return(m_bSaveIcon); }
#endif

    struct IMG* m_pImg;

    // Properties...
    int  m_nWidth;
    int  m_nHeight;
    int  m_nColors;
    int  m_nSaveColors;
#ifdef ICO_SUPPORT
    BOOL m_bSaveIcon;
#endif

#ifdef PCX_SUPPORT
    BOOL m_bPCX;
#endif

    BOOL m_bCompressed;
    BOOL m_nShrink; // 0=crop, 1=shrink, 2=ask

    BOOL   m_bTempName;     // true if not save as m_strFileName yet
    BOOL   m_bDirty;        // true if changed

    LPVOID m_lpvThing;      // in memory resource (must be valid)
    DWORD  m_dwOffBits;     // offset of pixels in lpvThing; packed if 0
    LONG   m_lMemSize;      // size in bytes

    protected:

    CObList m_dependants;
    };

// Standard Slob Notifications
#define SN_DESTROY      0
#define SN_ALL          1

extern int mpncolorsbits[];

void PBGetDefDims(int& pnWidth, int& pnHeight);

#ifndef _WIN32
#define POINTS POINT
#endif

#endif // __BMOBJECT_H__
