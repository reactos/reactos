#ifdef BZ_DECOMPRESS_ONLY

int _stdcall DriverEntry( void *a, void *b )
{
  return 1;
}
#else
int _stdcall DllMain( unsigned long a, unsigned long b, unsigned long c )
{
  return 1;
}
#endif
void bz_internal_error ( int errcode )
{
  return;
}
