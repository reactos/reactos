/* $Id: Settings.h 15091 2005-05-07 21:24:31Z sedwards $ */

#ifndef OPTIONS_H__a7382d2d_96b4_4472_974d_801281bd5327___INCLUDED
#define OPTIONS_H__a7382d2d_96b4_4472_974d_801281bd5327___INCLUDED

class CSettings
{
public:
  CSettings();
  ~CSettings();
  HRESULT Load(LPCTSTR pszLoadKey);
  HRESULT Store(LPCTSTR pszStoreKey);
  LPCTSTR GetPrompt();
  WORD GetNormalTextAttributes();
  WORD GetCommandTextAttributes();
private:
  HRESULT Clean();
  LPTSTR m_pszPrompt;
  WORD m_wNormalTextAttributes;
  WORD m_wCommandTextAttributes;
};

#endif // #ifndef OPTIONS_H__a7382d2d_96b4_4472_974d_801281bd5327___INCLUDED
