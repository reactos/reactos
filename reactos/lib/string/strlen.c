/*
 * $Id: strlen.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>
#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>

int strlen(const char* str)
{
  const char* s;

//  DPRINT1("strlen(%x)\n", str);
//  DPRINT1("%x\n", __builtin_return_address(0));
//  if (str == (char*)0x6418c4)
//  {
//      DPRINT1("%s\n", str);
//  }
  if (str == 0)
    return 0;
  for (s = str; *s; ++s);
  return s-str;
}

