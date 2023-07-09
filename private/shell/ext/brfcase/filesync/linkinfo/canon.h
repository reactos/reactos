/*
 * canon.h - Canonical path manipulation module description.
 */


/* Constants
 ************/

#define MAX_NETRESOURCE_LEN      (2 * MAX_PATH_LEN)


/* Types
 ********/

/* NETRESOURCE buffer */

typedef union _netresourcebuf
{
   NETRESOURCE nr;

   BYTE rgbyte[MAX_NETRESOURCE_LEN];
}
NETRESOURCEBUF;
DECLARE_STANDARD_TYPES(NETRESOURCEBUF);

