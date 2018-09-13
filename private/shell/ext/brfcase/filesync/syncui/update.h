//
// update.h: Declares data, defines and struct types for twin creation
//          module.
//
//

#ifndef __UPDATE_H__
#define __UPDATE_H__

// Flags for Upd_DoModal
#define UF_SELECTION    0x0001
#define UF_ALL          0x0002

int PUBLIC Upd_DoModal(HWND hwndOwner, CBS * pcbs, LPCTSTR pszList, UINT cFiles, UINT uFlags);

#endif // __UPDATE_H__

