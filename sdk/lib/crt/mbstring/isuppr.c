/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * PURPOSE:
 * PROGRAMER:
 * UPDATE HISTORY:
 *              12/04/99: Ariadne Created
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>

/*
 * @implemented
 */
int _ismbcupper( unsigned int c )
{
    return ((c) >= 0x8260 && (c) <= 0x8279);
}
