typedef  unsigned int size_t;

void *
memcpy (char *to, char *from, size_t count);

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
void *
memcpy (char *to, char *from, size_t count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;

  return to;
}
