/* $Id: Settings.h,v 1.1 2001/04/16 05:03:29 narnaoud Exp $ */

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
  WORD GetPromptTextAttributes();
private:
  HRESULT Clean();
  LPTSTR m_pszPrompt;
  WORD m_wNormalTextAttributes;
  WORD m_wPromptTextAttributes;
};

#endif // #ifndef OPTIONS_H__a7382d2d_96b4_4472_974d_801281bd5327___INCLUDED
