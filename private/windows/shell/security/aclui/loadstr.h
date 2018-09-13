
#ifndef LOADSTR_H_INCLUDED
#define LOADSTR_H_INCLUDED

#include <malloc.h>

UINT SizeofStringResource(HINSTANCE hInstance, UINT idStr);

inline TCHAR* _LoadResourceStringHelper_(HINSTANCE hModule, UINT id, void* pBuffer, int cchBuffer)
{
    if (pBuffer)
        LoadString(hModule, id, (TCHAR*) pBuffer, cchBuffer);

    return (TCHAR*) pBuffer;
}

#define USES_LOAD_STRING int _iStrLength_
#define LOAD_STRING(hModule, id) \
	_LoadResourceStringHelper_((hModule), (id), \
        ((_iStrLength_ = SizeofStringResource((hModule), (id))) == 0) \
        ? NULL : _alloca((_iStrLength_ + 1) * sizeof TCHAR), \
        _iStrLength_ + 1)

#endif // !LOADSTR_H_INCLUDED