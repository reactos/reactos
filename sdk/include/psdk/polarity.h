/*
 * COPYRIGHT:            This file is in the public domain.
 * PROJECT:              ReactOS kernel
 * FILE:                 include/psdk/polarity.h
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
