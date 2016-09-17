/*
 * PROJECT:     ReactOS TXT to NLS Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/txt2nls/main.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "precomp.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    if (!nls_from_txt(argv[1], argv[2]))
        return 1;

    return 0;
}
