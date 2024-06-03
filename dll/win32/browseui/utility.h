#pragma once

void *operator new(size_t size);
void operator delete(void *p);

HRESULT SHELL_GetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl);
BOOL SHELL_IsEqualAbsoluteID(PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b);
BOOL SHELL_IsVerb(IContextMenu *pcm, UINT_PTR idCmd, LPCWSTR Verb);
