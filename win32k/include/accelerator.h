#ifndef _WIN32K_ACCELERATOR_H
#define _WIN32K_ACCELERATOR_H

#include <include/winsta.h>
#include <include/window.h>

typedef struct _ACCELERATOR_TABLE
{
   union
   {
      USER_OBJECT_HDR hdr;
      struct
      {
         /*---------- USER_OBJECT_HDR --------------*/
         HACCEL hSelf; /* want typesafe handle */
         LONG refs_placeholder;
         BYTE flags_placeholder;
         /*---------- USER_OBJECT_HDR --------------*/

         int Count;
         LPACCEL Table;
      };

   };
} ACCELERATOR_TABLE, *PACCELERATOR_TABLE;

#endif /* _WIN32K_ACCELERATOR_H */
