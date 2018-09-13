// Copyright (C) 1997 Microsoft Corporation
// 
// Network ID Tab hook
// 
// 3-07-98 sburns



#include "sysdm.h"



HPROPSHEETPAGE
CreateNetIDPage(HINSTANCE hInst)
{
   TCHAR dll_name[MAX_PATH + 1] = {0};
   if (!::LoadString(hInst, IDS_NETID_DLL_NAME, dll_name, MAX_PATH))
   {
      return 0;
   }

   HPROPSHEETPAGE result = 0;
   HINSTANCE netid = ::LoadLibrary(dll_name);

   if (netid)
   {
      typedef HPROPSHEETPAGE (*CreateProc)();

      CreateProc proc =
         reinterpret_cast<CreateProc>(
            ::GetProcAddress(netid, "CreateNetIDPropertyPage"));

      if (proc)
      {
         result = proc();
      }
   }

   return result;
}

