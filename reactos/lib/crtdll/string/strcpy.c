/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
char *
strcpy(char *to, const char *from);

char *
strcpy(char *to, const char *from)
{
  char *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
}
