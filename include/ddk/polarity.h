/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 
 * PURPOSE:             
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#ifndef POLARITY_HEADERFILE_IS_INCLUDED
    #define POLARITY_HEADERFILE_IS_INCLUDED

    #ifdef USE_POLARITY
        #ifdef BUILDING_DLL
            #define POLARITY __declspec( dllexport )
        #else 
            #define POLARITY __declspec( dllimport )
        #endif
    #else
        #define POLARITY
    #endif
#endif
