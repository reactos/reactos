#ifndef __HEXEDIT_H
#define __HEXEDIT_H

#define HEX_EDIT_CLASS_NAME _T("HexEdit32")

ATOM STDCALL
RegisterHexEditorClass(HINSTANCE hInstance);
BOOL STDCALL
UnregisterHexEditorClass(HINSTANCE hInstance);

/* styles */
#define HES_READONLY	(0x800)
#define HES_LOWERCASE	(0x10)
#define HES_UPPERCASE	(0x8)	

#endif /* __HEXEDIT_H */
