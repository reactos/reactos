/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RtlInitializeBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_RtlInitializeBitmap()
{
    RTL_BITMAP Bitmap;
    ULONG Buffer[5];

    Buffer[0] = 0x12345;
    Buffer[1] = 0x23456;
    Buffer[2] = 0x34567;
    Buffer[3] = 0x45678;
    Buffer[4] = 0x56789;

    RtlInitializeBitMap(&Bitmap, Buffer, 19);
    ok(Bitmap.Buffer == Buffer, "Buffer=%p\n", Bitmap.Buffer);
    ok(Bitmap.SizeOfBitMap == 19, "SizeOfBitMap=%ld\n", Bitmap.SizeOfBitMap);

    ok(Buffer[0] == 0x12345, "Buffer[0] == 0x%lx\n", Buffer[0]);
    ok(Buffer[1] == 0x23456, "Buffer[1] == 0x%lx\n", Buffer[1]);
    ok(Buffer[2] == 0x34567, "Buffer[2] == 0x%lx\n", Buffer[2]);
    ok(Buffer[3] == 0x45678, "Buffer[3] == 0x%lx\n", Buffer[3]);
    ok(Buffer[4] == 0x56789, "Buffer[4] == 0x%lx\n", Buffer[4]);

    RtlInitializeBitMap(&Bitmap, 0, -100);
    ok(Bitmap.Buffer == 0, "Buffer=%p\n", Bitmap.Buffer);
    ok(Bitmap.SizeOfBitMap == -100, "SizeOfBitMap=%ld\n", Bitmap.SizeOfBitMap);

}

START_TEST(RtlInitializeBitMap)
{
    Test_RtlInitializeBitmap();
}

