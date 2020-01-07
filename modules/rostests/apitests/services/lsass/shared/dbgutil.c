/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     debug utils
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>

void PrintHexDumpMax(
    IN int length,
    IN PBYTE buffer,
    IN int printmax)
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[512];
    int cbLine;

    if (length > printmax)
        length = printmax;

    for (index = 0; length;
         length -= count, buffer += count, index += count)
    {
        count = (length > 32) ? 32:length;

        snprintf(rgbLine, 512, "%4.4x  ",index);
        cbLine = 6;

        for (i = 0; i < count; i++)
        {
            rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[cbLine++] = ':';
            }
            else
            {
                rgbLine[cbLine++] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
        }

        rgbLine[cbLine++] = ' ';

        for (i = 0; i < count; i++)
        {
            if (buffer[i] < 32 || buffer[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = buffer[i];
            }
        }

        rgbLine[cbLine++] = 0;
        printf("%s\n", rgbLine);
    }
}

void PrintHexDump(
    IN DWORD length,
    IN PBYTE buffer)
{
    PrintHexDumpMax(length, buffer, 32);
}
