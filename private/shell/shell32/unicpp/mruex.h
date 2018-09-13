#ifndef __MRUEX_H__
#define __MRUEX_H__

HANDLE CreateMRUListEx(LPMRUINFO lpmi);
HANDLE CreateMRUListLazyEx(LPMRUINFO lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot);
void FreeMRUListEx(HANDLE hMRU);
int AddMRUDataEx(HANDLE hMRU, const void FAR *lpData, UINT cbData);

#endif // __MRUEX_H__

