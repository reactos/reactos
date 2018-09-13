inline ULONG AlignLong( ULONG nSrc )
{
   return( (nSrc+3)&(~0x3) );
}

