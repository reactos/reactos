/*-------------------------------------------------------------------------
 * FILENAME :    IMGCACHE.H
 * CREATED BY :  cdturner
 * DATE :        Jan. 3, 1996
 *
 * DESCRIPTION : Image List cache stuff.
 * 
 *-------------------------------------------------------------------------*/
 
#ifndef _IMGCACHE_H
#define _IMGCACHE_H

//////////////////////////////////////////////////////////////////////////////////////
#define DEF_MAX_THUMBNAILS  50
#define DEFSIZE_THUMBNAIL   120
#define DEFSIZE_BORDER      10
#define DEFSIZE_VERTBDR     30

//////////////////////////////////////////////////////////////////////////////////////
// a background task for grovelling the image cache to remove an item when
// the cache gets too big.....
// this will cause the items to have to be re-fetched from the disk....
class CThumbnailView;

HRESULT CImgCacheTidyup_Create( IImageCache * pCache, 
                                BOOL fMultiple,
                                HWND hWndListView,
                                int * piItemPos,
                                LPRUNNABLETASK * ppTask );

class CImgCacheTidyup : public IRunnableTask,
                        public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CImgCacheTidyup )
            COM_INTERFACE_ENTRY( IRunnableTask )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CImgCacheTidyup )
        
        STDMETHOD (Run)( );
        STDMETHOD (Kill)(BOOL fWait );
        STDMETHOD (Suspend)( );
        STDMETHOD (Resume)( );
        STDMETHOD_(ULONG, IsRunning)();

    protected:
        BOOL ImageIsInView( int iIndex );
        CImgCacheTidyup();
        ~CImgCacheTidyup();

        friend HRESULT CImgCacheTidyup_Create( IImageCache * pCache, 
                                               BOOL fMultiple,
                                               HWND hWndListView,
                                               int * piItemPos,
                                               LPRUNNABLETASK * ppTask );
        IImageCache * m_pCache;
        LONG m_lState;
        BOOL m_fMultiple;
        HWND m_hWndListView;
        int * m_piItemPos;
};
                    
#endif  //_IMGCACHE_H

