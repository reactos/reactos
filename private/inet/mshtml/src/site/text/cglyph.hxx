//+---------------------------------------------------------------------
//
//   File:      cglyph.hxx
//
//  Contents:   CGlyph Class which manages the information for glyphs and makes it available at rendering time.
//
//  Classes:    CGlyph
//              CGlyphTreeType
//              CList
//
//------------------------------------------------------------------------

#ifndef _CGLYPH_HXX_
#define _CGLYPH_HXX_

MtExtern(CGlyph)
MtExtern(CTreeObject)
MtExtern(CTreeList)
MtExtern(CGlyphInfoType)

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

class CList;
class CGlyph;

#define IFR(expr) {hr = THR(expr); if (FAILED(hr)) RRETURN1(hr, S_FALSE);}
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

#define pGTableType     CGlyphTreeType *
#define pCGlyphInfoType  CGlyphInfoType *
#define pIImgCtx        IImgCtx *

//
// Some default definitions
//
// Bitmap URLs for images related commands
#define DEFAULT_GLYPH_SIZE          20
#define DEFAULT_IMG_NAME            _T("unknown.gif")
#define DECIMALS                    _T("0123456789")
#define DEFAULT_XML_TAG_NAME        _T(" ")
#define MAX_GLYPH_STREAM_LENGTH     1000

//Some other handy definitions

#define NUM_INFO_LEVELS             4

#define NUM_STATE_ELEMS             3
#define NUM_ALIGN_ELEMS             4
#define NUM_ORIENT_ELEMS            5
#define NUM_POS_ELEMS               4

#define TREEDEPTH_STATE             0
#define TREEDEPTH_POSITIONINING     1
#define TREEDEPTH_ALIGNMENT         2
#define TREEDEPTH_ORIENTATION       3

#define COMPUTE                     -1


// Enumerated typedefs
typedef enum GLYPH_STATE_TYPE
{
    GST_COMPUTE     =   -1,
    GST_OPEN        =   0,
    GST_CLOSE       =   1,
    GST_DEFAULT     =   2,
};

typedef enum GLYPH_ALIGNMENT_TYPE
{
    GAT_COMPUTE     =   -1,
    GAT_LEFT        =   0,
    GAT_CENTER      =   1, 
    GAT_RIGHT       =   2, 
    GAT_DEFAULT     =   3
};

typedef enum GLYPH_POSITION_TYPE
{
    GPT_COMPUTE     =   -1,
    GPT_STATIC      =   0, 
    GPT_RELATIVE    =   1, 
    GPT_ABSOLUTE    =   2, 
    GPT_DEFAULT     =   3
};


typedef enum GLYPH_ORIENTATION_TYPE
{
    GOT_COMPUTE         =   -1,
    GOT_LEFT_TO_RIGHT   =   0, 
    GOT_RIGHT_TO_LEFT   =   1, 
    GOT_TOP_TO_BOTTOM   =   2, 
    GOT_BOTTOM_TO_TOP   =   3, 
    GOT_DEFAULT         =   4
};







// Class returned when a querry is made from rendering
class CGlyphRenderInfoType
{
public:

    IImgCtx *       pImageContext;
    LONG            width;
    LONG            height;
    LONG            offsetX;
    LONG            offsetY;

    inline BOOL HasInfo () 
    {
        return (pImageContext != NULL);
    }
};









static  int     s_levelSize [NUM_INFO_LEVELS];

class CGlyph    :   public CVoid
{
    class CGlyphTreeType;
    class CGlyphInfoType;
    class CTreeObject;
    class CTreeList;
    friend class CGlyphTreeType;  
    friend class CTreeObject;  
    friend class CTreeList;  

public:

    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CGlyph));

    CGlyph(CDoc * pDoc);

    HRESULT Init ();

    ~CGlyph ();    

    //
    // Information Entry/Deletion methods
    //

    HRESULT RemoveGlyphTableContents ();    //  Empties Glyph Table

    HRESULT ReplaceGlyphTableContents (
        BSTR inputStream                    //  IN  -Formatted BSTR according to defined syntax
        );

    HRESULT AddToGlyphTable (               
        BSTR inputStream                    //  IN  -Formatted BSTR according to defined syntax
        );

    HRESULT RemoveFromGlyphTable (
        BSTR inputStream                    //  IN  -Formatted BSTR according to defined syntax
        );

    HRESULT Exec(
        GUID * pguidCmdGroup,
        UINT idm,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);


    
    //
    // Information retrieval method
    //
    HRESULT GetTagInfo (
        CTreePos *              ptp,                //  IN  -Tree Position for which we want glyph information
        GLYPH_ALIGNMENT_TYPE    eAlign,             //  IN  -Alignment
        GLYPH_POSITION_TYPE     ePos,               //  IN  -Positioning
        GLYPH_ORIENTATION_TYPE  eOrientation,       //  IN  -Orientation
        void *                  invalidateInfo,     //  IN  -Invalidation information needed in the loading callback
        CGlyphRenderInfoType  *  ptagInfo            //  OUT -Always points to a struct. If no glyph info exists,
        );                                          //          pImageContext is set to NULL


private:

    // Quick and lightweight implementation of a stack.
    class CList
    {
        public:

            CList ();

            ~CList ();

            HRESULT Push (
                void *      pushed
                );
    
            HRESULT Pop (
                void **     popped
                );
    
        private:
            // Internal for CList
            typedef struct XX_ListElemType
            {
                void *              elem;
                XX_ListElemType *     next;
            } ListElemType;

            ListElemType * _elemList;
    };

    

    


    // Basic info common to tags identified by name and ID
    typedef struct XX_BasicGlyphInfoType
    {
        GLYPH_STATE_TYPE        eState; 
        GLYPH_POSITION_TYPE     ePos;
        GLYPH_ALIGNMENT_TYPE    eAlign;
        GLYPH_ORIENTATION_TYPE  eOrient;
        PTCHAR                  pchImgURL;
        LONG                    width;
        LONG                    height;
        LONG                    offsetX;
        LONG                    offsetY;
    } BasicGlyphInfoType;



    typedef struct XX_XMLGlyphTableType
    {
        PTCHAR                  pchTagName; 
        BasicGlyphInfoType      basicInfo;
    } XMLGlyphTableType;


    typedef struct XX_IDGlyphTableType
    {
        ELEMENT_TAG             eTag; 
        BasicGlyphInfoType      basicInfo;
    } IDGlyphTableType;


    HRESULT GetEditResourceLibrary (
        HINSTANCE   *hResourceLibrary
        );

    HRESULT ConstructResourcePath (
        HINSTANCE   hResourceLibrary, 
        TCHAR szBuffer []
        );

    HRESULT ParseGlyphTable (
        BSTR                inputStream,            
        BOOL                addToTable  =   TRUE    
        );

    HRESULT AttemptToResolveTagName (
        PTCHAR              pchTagName, 
        ELEMENT_TAG &       eTag
        );

    HRESULT IDParse (
        PTCHAR &    pchInStream, 
        PTCHAR &    pchThisSection,
        BOOL        addToTable
        );

    HRESULT XMLParse (
        PTCHAR &            pchInStream, 
        PTCHAR &            pchThisSection, 
        BOOL                addToTable
        );

    BOOL NextIntSection (
        PTCHAR &    pchInStream, 
        LONG &      result
        );

    HRESULT GetThisSection (
        PTCHAR &    pchSectionBegin, 
        PTCHAR &    pchNewString
        );

    int CompareUpTo (
        PTCHAR  first, 
        PTCHAR  second, 
        int     numToComp
        );

    HRESULT ParseBasicInfo (
        PTCHAR & pchInStream, 
        BasicGlyphInfoType & gInfo
        );

    HRESULT InitGInfo (
        pCGlyphInfoType &        gInfo, 
        BasicGlyphInfoType &    basicInfo
        );

    HRESULT NewEntry(
        XMLGlyphTableType &     gTableElem, 
        BOOL                    addToTable
        );

    HRESULT NewEntry(
        IDGlyphTableType &      gTableElem,
        BOOL                    addToTable
        );
    
    HRESULT InsertIntoTable (
        CGlyphInfoType *         gInfo, 
        PTCHAR                  pchTagName, 
        GLYPH_STATE_TYPE        eState, 
        GLYPH_ALIGNMENT_TYPE    eAlign, 
        GLYPH_POSITION_TYPE     ePos, 
        GLYPH_ORIENTATION_TYPE  eOrient, 
        BOOL                    addToTable
        );

    HRESULT InsertIntoTable (
        CGlyphInfoType *         gInfo, 
        ELEMENT_TAG             eTag, 
        GLYPH_STATE_TYPE        eState, 
        GLYPH_ALIGNMENT_TYPE    eAlign, 
        GLYPH_POSITION_TYPE     ePos, 
        GLYPH_ORIENTATION_TYPE  eOrient, 
        BOOL                    addToTable
        );

    HRESULT GetImageContext (
        CGlyphInfoType *     gInfo, 
        void *              pvCallback, 
        pIImgCtx &          newImageContext
        );
    
    HRESULT GetXMLTagInfo (
        CTreePos *              ptp, 
        GLYPH_STATE_TYPE        eState, 
        GLYPH_ALIGNMENT_TYPE    eAlign, 
        GLYPH_POSITION_TYPE     ePos, 
        GLYPH_ORIENTATION_TYPE  eOrientation, 
        void *                  invalidateInfo, 
        CGlyphRenderInfoType  *  pTagInfo
        );

    HRESULT AttemptFinalDefault (
        CTreePos *              ptp, 
        pCGlyphInfoType &        gInfo, 
        GLYPH_STATE_TYPE        eState, 
        GLYPH_ALIGNMENT_TYPE    eAlign,
        GLYPH_POSITION_TYPE     ePos, 
        GLYPH_ORIENTATION_TYPE  eOrient
        );

    HRESULT  InitRenderTagInfo (
        CGlyphRenderInfoType *   renderTagInfo
        );

    static void CALLBACK OnImgCtxChange( 
        VOID *      pvImgCtx, 
        VOID *      pv 
        );

    HRESULT CompleteInfoProcessing (
        CGlyphInfoType *         localInfo, 
        CGlyphRenderInfoType  *  pRenderTagInfo, 
        void *                  invalidateInfo
        );
                    
    HRESULT AddSynthesizedRule (
        const TCHAR             imgName [], 
        ELEMENT_TAG             eTag, 
        GLYPH_STATE_TYPE        eState      = GST_DEFAULT, 
        GLYPH_POSITION_TYPE     ePos        = GPT_DEFAULT, 
        GLYPH_ALIGNMENT_TYPE    eAlign      = GAT_DEFAULT, 
        GLYPH_ORIENTATION_TYPE  eOrient     = GOT_DEFAULT,
        PTCHAR                  pchTagName  = NULL
        );
    
    
    CPtrBagCi <CGlyphTreeType *> *  _gHashTable;    //Info for tags that are unidentifiable by a tag ID
    CList *                         _XMLStack;      //Stack of entries into the hash table -- Used only for cleanup
    CGlyphTreeType *                _gIdentifiedTagArray [ETAG_LAST+1]; //Info for tags that can be resolved to an ID
    HINSTANCE                       _hEditResDLL;    
    CDoc *                          _pDoc;
    PTCHAR                          _pchBeginDelimiter;
    PTCHAR                          _pchEndDelimiter;
    PTCHAR                          _pchEndLineDelimiter;
    PTCHAR                          _pchDefaultImgURL;





//+-----------------------------------------------------------------------  
// This is really really cool little class. It is basically a tree where 
// various depths can have different number of children. All that is 
// needed to modify the supported depth and number of children at a depth 
// is to modify NUM_INFO_LEVELS to the desired depth, the array 
// 's_levelSize' needs to be set to the number of children needed at each 
// depth, and finally, the method 'TransormInfoToArray' and 
// 'ComputeLevelIndex' needs to be changed to make the correct 
// initalization. What more, we can also modify the traversal of the tree, 
// giving a different priority to various elements by changing the depth 
// of a given info type. For example, if we wanted Positioning information 
// to have a higher priority than orientation information, all that we 
// need to do is flip NUM_ALIGN_ELEMS with NUM_POS_ELEMS in the setting of 
// s_levelSize, and flip ePos and eOrient in the 'TransormInfoToArray' 
// method and modify 'ComputeLevelIndex' in the same manner.
//+-----------------------------------------------------------------------


    class CTreeObject
    {
    public:

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeObject));

        virtual ~CTreeObject () {}
        
        virtual BOOL IsInfoType () {return TRUE;}
    };

    class CTreeList : public CTreeObject
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeList));

        CTreeList (LONG numObjects);
        
        ~CTreeList ();

        BOOL IsInfoType () 
        { 
            return FALSE; 
        }

        CTreeObject * & operator[](int index);

    private:   
        CTreeObject **   _nextLevel;
        LONG            _numObjects;
    };

    class CGlyphInfoType : public CTreeObject
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeObject));

        ~CGlyphInfoType ();

        BOOL IsInfoType () { return TRUE; }

        IImgCtx *           pImageContext;
        PTCHAR              pchImgURL;
        LONG                width;
        LONG                height;
        LONG                offsetX;
        LONG                offsetY;
    };


    class CGlyphTreeType
    {
        friend class CGlyph;

        public:

            CGlyphTreeType ();
    
            ~CGlyphTreeType ();
    
            HRESULT AddRule (
                CGlyphInfoType *         gInfo, 
                GLYPH_STATE_TYPE        eState, 
                GLYPH_ALIGNMENT_TYPE    eAlign, 
                GLYPH_POSITION_TYPE     ePos, 
                GLYPH_ORIENTATION_TYPE  eOrient,
                BOOL                    addToTable,
                CGlyph *                glyphTable
                );

            HRESULT GetGlyphInfo (
                CTreePos *              ptp,
                pCGlyphInfoType &        gInfo, 
                GLYPH_STATE_TYPE        eState, 
                GLYPH_ALIGNMENT_TYPE    eAlign, 
                GLYPH_POSITION_TYPE     ePos, 
                GLYPH_ORIENTATION_TYPE  eOrient
                );

        private:

            HRESULT TransformInfoToArray (
                GLYPH_STATE_TYPE        eState, 
                GLYPH_ALIGNMENT_TYPE    eAlign, 
                GLYPH_POSITION_TYPE     ePos, 
                GLYPH_ORIENTATION_TYPE  eOrient, 
                int                     indexArray []
                );

            HRESULT
            ComputeLevelIndex (
                CTreePos *      ptp, 
                int             indexArray [], 
                int             levelCount
                );
                                     
            HRESULT InsertIntoTree (
                CGlyphInfoType *     gInfo, 
                int                 index [],
                BOOL                addToTable, 
                CGlyph *            glyphTable
                );

            HRESULT GetFromTree (
                CTreePos *          ptp, 
                pCGlyphInfoType &    gInfo, 
                int                 index []
                );

    
            CTreeList * _infoTree;
    };

};

#endif


