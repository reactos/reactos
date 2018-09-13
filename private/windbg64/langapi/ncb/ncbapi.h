#ifndef __NCBAPI_H__
#define __NCBAPI_H__

HRESULT  OpenNcb (SZ szName, HTARGET hTarget, SZ szTarget, BOOL bOverwrite, Bsc ** ppBsc);
HRESULT  OpenNcb (PDB * ppdb, HTARGET hTarget, SZ szTarget, Bsc ** ppBsc);
HRESULT  OpenNcb (SZ szName, BOOL bOverwrite, NcbParseEx ** ppNcParse);
HRESULT  OpenNcb (PDB * ppdb, NcbParseEx **ppNcParse);

#endif // __NCBAPI_H__
