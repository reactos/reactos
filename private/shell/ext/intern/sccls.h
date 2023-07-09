#ifndef SCCLS_H
#define SCCLS_H

STDAPI CExeDllColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut);
STDAPI CFShortcutMenu_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut, BOOL IsFS);
STDAPI CFolderInfoTip_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut);

#endif