#ifndef PROMPT_H__d2442d41_b6b3_47b5_8cb8_e6c597e570b9__INCLUDED
#define PROMPT_H__d2442d41_b6b3_47b5_8cb8_e6c597e570b9__INCLUDED

#include "RegistryTree.h"
#include "Console.h"

class CPrompt
{
public:
  CPrompt(CRegistryTree& rTree, HRESULT& rhr);
  ~CPrompt();
  HRESULT SetPrompt(LPCTSTR pszPrompt);
  void ShowPrompt(CConsole &rConsole);
  static LPCTSTR GetDefaultPrompt();

private:
  CRegistryTree& m_rTree;
  LPTSTR m_pszPrompt;
};

#endif // #ifndef PROMPT_H__d2442d41_b6b3_47b5_8cb8_e6c597e570b9__INCLUDED
