#ifndef _CHANNEL_H
#define _CHANNEL_H


void Channel_UpdateQlinks();

HRESULT Channel_GetFolder(LPTSTR pszPath, int cchPath);
LPITEMIDLIST Channel_GetFolderPidl();
HRESULT ChannelBand_CreateInstance(IUnknown** ppunk);
HRESULT Channels_OpenBrowser(IWebBrowser2 **ppwb, BOOL fInPlace);

BOOL GetFirstUrl(TCHAR szURL[], DWORD cb);

#endif
