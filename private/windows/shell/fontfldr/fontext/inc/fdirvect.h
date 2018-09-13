/**********************************************************************
 * FDirVect.h  -- Font directory vector. An indirect array of 
 *       CFontDir objects.
 *
 **********************************************************************/

#if !defined(__FDIRVECT_H__)

#define __FDIRVECT_H__

#include "vecttmpl.h"

// ********************************************************************
// Forward Declarations
//
class CFontDir;

typedef CIVector<CFontDir> CFDirVect;

class CFDirVector : public CFDirVect
{
public:
    CFDirVector( int iSize ) : CFDirVect(iSize) {}
    CFontDir *fdFindDir( LPTSTR lpPath, int iLen, BOOL bAdd = FALSE );
} ;


#endif   // __FDIRVECT_H__ 

