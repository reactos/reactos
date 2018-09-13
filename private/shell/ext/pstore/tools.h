#include "resource.h"

typedef struct
   {
   int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   BYTE  bState;
   BYTE  bStyle;
   }MYTOOLINFO, *LPMYTOOLINFO;

extern MYTOOLINFO g_Buttons[];
