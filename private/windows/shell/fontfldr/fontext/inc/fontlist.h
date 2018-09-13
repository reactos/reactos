/**********************************************************************
 * FontList.h  -- Manages a list of FontID objects via an array of
 *             FontVector objects.
 *
 **********************************************************************/

#if !defined(__FONTLIST_H__)
#define __FONTLIST_H__


#include "fontvect.h"

class CFontClass;

const int kDefaultVectSize = 50;   // Number of fonts in each CFontArray

class CFontList {
public:
    CFontList( int iSize, int iVectorSize = kDefaultVectSize );
    ~CFontList();
    
    int bInit();
    CFontList *Clone(void);
        
    //
    //  The real array functions.
    //

    int   iCount( void );
    int   bAdd( CFontClass * t );
    CFontClass *  poObjectAt( int idx );
    CFontClass *  poDetach( int idx );
    CFontClass *  poDetach( CFontClass * t );
    void  vDetachAll( );
    int   bDelete( int idx );
    int   bDelete( CFontClass * t );
    void  vDeleteAll( );
    int   iFind( CFontClass * t );
    void  ReleaseAll(void);
    void  AddRefAll(void);
    
private:
    CFontVector **  m_pData;
    int   m_iCount;         // Number of Fonts
    int   m_iVectorCount;   // Number of vectors allocated
    int   m_iVectorBounds;  // Total number of vector points
    int   m_iVectorSize;    // Number of fonts in each vector
};



/**********************************************************************
 * Some things you can do with a font list.
 */
HDROP hDropFromList( CFontList * poList );

#endif   // __FONTLIST_H__ 
