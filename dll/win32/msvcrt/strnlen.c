/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT
 * PURPOSE:     Implementation of strnlen and wcsnlen
 * COPYRIGHT:   Copyright 2024 ReactOS Project
 */

#include <precomp.h>

/*
 * @implemented
 */
size_t __cdecl strnlen(const char *str, size_t maxlen)
{
    size_t len;
    
    if (!str)
        return 0;
    
    for (len = 0; len < maxlen && str[len]; len++)
        ;
    
    return len;
}

/*
 * @implemented
 */
size_t __cdecl wcsnlen(const wchar_t *str, size_t maxlen)
{
    size_t len;
    
    if (!str)
        return 0;
    
    for (len = 0; len < maxlen && str[len]; len++)
        ;
    
    return len;
}