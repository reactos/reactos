/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isgraph
int isgraph(int c)
{
  return _isctype(c,_GRAPH);
}

#undef iswgraph
int iswgraph(int c)
{
	return iswctype(c,_GRAPH);
}
