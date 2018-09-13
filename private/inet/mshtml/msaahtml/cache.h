//================================================================================
//		File:	CACHE.H
//		Date: 	3/20/98
//		Desc:	Defines the CCache class, which is a simple data cache manager.
//				The cache does not manage the actual storage of the data, but rather
//				the data's state (dirty or clean).  
//================================================================================

#ifndef __CACHE_H__
#define __CACHE_H__


//================================================================================
// Includes
//================================================================================

//================================================================================
// Defines
//================================================================================

//================================================================================
// Class forwards
//================================================================================

//================================================================================
// CCache class definition.
//================================================================================

class CCache
{

public:

    CCache(void)                    { m_dwCache = 0L; } 

    BOOL    IsDirty( DWORD dwMask ) { return (dwMask & m_dwCache); }
    void    Dirty( DWORD dwMask )   { m_dwCache |= dwMask; }
    void    Clean( DWORD dwMask )   { m_dwCache &= ~dwMask; }
    void    CleanAll( void )        { m_dwCache = 0L; } 

protected:

    DWORD	m_dwCache;
};

#endif	// __CACHE_H__