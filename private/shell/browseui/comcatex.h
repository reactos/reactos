#ifndef __COMCATEX_H__
#define __COMCATEX_H__

#include <comcat.h>

//-------------------------------------------------------------------------//
//  Retrieves cache-aware enumerator over classes which require or 
//  implement the specified component catagory(ies).
//  See docs on ICatInformation::EnumClassesOfCategories() for more information
//  on arguments and usage.
STDMETHODIMP SHEnumClassesOfCategories(
      ULONG cImplemented,       //Number of category IDs in the rgcatidImpl array
      CATID rgcatidImpl[],      //Array of category identifiers
      ULONG cRequired,          //Number of category IDs in the rgcatidReq array
      CATID rgcatidReq[],       //Array of category identifiers
      IEnumGUID** ppenumGUID ) ;//Address to receive a pointer to an IEnumGUID interface

//-------------------------------------------------------------------------//
//  Determines whether a cache exists for the indicated CATID.
//  If bImplementing is TRUE, the function checks for a cache of
//  implementing classes; otherwise the function checks for a cache of
//  requiring classes.  Returns S_OK if the cache exists, S_FALSE if
//  it does not exist, or an error indicating a failure occurred.
STDMETHODIMP SHDoesComCatCacheExist( REFCATID refcatid, BOOL bImplementing ) ;

//-------------------------------------------------------------------------//
//  Caches implementing and/or requiring classes for the specified categories
//  See docs on ICatInformation::EnumClassesOfCategories() for more information
//  on arguments.
STDMETHODIMP SHWriteClassesOfCategories( 
      ULONG cImplemented,       //Number of category IDs in the rgcatidImpl array
      CATID rgcatidImpl[],      //Array of category identifiers
      ULONG cRequired,          //Number of category IDs in the rgcatidReq array
      CATID rgcatidReq[],       //Array of category identifiers
      BOOL  bForceUpdate,       //TRUE: Unconditionally update the cache; 
                                //otherwise create cache iif doesn't exist.
      BOOL  bWait ) ;           //If FALSE, the function returns immediately and the
                                //   caching occurs asynchronously; otherwise
                                //   the function returns only after the caching
                                //   operation has completed.

#endif __COMCATEX_H__