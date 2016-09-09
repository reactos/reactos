////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////


VOID
UDFSetModified(
    IN PVCB        Vcb
    )
{
    if(UDFInterlockedIncrement((PLONG)&(Vcb->Modified)) & 0x80000000)
        Vcb->Modified = 2;
} // end UDFSetModified()

VOID
UDFPreClrModified(
    IN PVCB        Vcb
    )
{
    Vcb->Modified = 1;
} // end UDFPreClrModified()

VOID
UDFClrModified(
    IN PVCB        Vcb
    )
{
    KdPrint(("ClrModified\n"));
    UDFInterlockedDecrement((PLONG)&(Vcb->Modified));
} // end UDFClrModified()

