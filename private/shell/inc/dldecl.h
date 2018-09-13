#ifndef _DLDECL_H_
#define _DLDECL_H_

// Delay load declarations

// The following are defined to get the DECLSPEC_IMPORT stuff right.
// Since we have identically-named local functions that thunk to the
// real function, we need to correct the dll linkage.

#ifdef DL_OLEAUT32
#define _OLEAUT32_
#endif

#ifdef DL_OLE32
#define _OLE32_
#endif

#endif // _DLDECL_H_

