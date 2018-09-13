
/* pngerror.c - stub functions for i/o and memory allocation

   libpng 1.0 beta 2 - version 0.88
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   January 25, 1996

   This file provides a location for all error handling.  Users which
   need special error handling are expected to write replacement functions
   and use png_set_message_fn() to use those functions.  See the instructions
   at each function. */

#ifdef NEVER
#define PNG_INTERNAL
#include "png.h"
#endif

#include "headers.h"

/* This function is called whenever there is a fatal error.  This function
   should not be changed.  If there is a need to handle errors differently,
   you should supply a replacement error function and use png_set_message_fn()
   to replace the error function at run-time. */
void
png_error(png_structp png_ptr, png_const_charp message)
{
   if (png_ptr->error_fn)
      (*(png_ptr->error_fn))(png_ptr, message);

   /* if the following returns or doesn't exist, use the default function,
      which will not return */
   png_default_error(png_ptr, message);
}

/* This function is called whenever there is a non-fatal error.  This function
   should not be changed.  If there is a need to handle warnings differently,
   you should supply a replacement warning function and use
   png_set_message_fn() to replace the warning function at run-time. */
void
png_warning(png_structp png_ptr, png_const_charp message)
{
   if (png_ptr->warning_fn)
      (*(png_ptr->warning_fn))(png_ptr, message);
   else
      png_default_warning(png_ptr, message);
}

/* This is the default error handling function.  Note that replacements for
   this function MUST NOT RETURN, or the program will likely crash.  This
   function is used by default, or if the program supplies NULL for the
   error function pointer in png_set_message_fn(). */
void
png_default_error(png_structp png_ptr, png_const_charp message)
{
#ifndef PNG_NO_STDIO
    OutputDebugStringA( "libpng error: " );
    OutputDebugStringA( message );
    OutputDebugStringA( "\n" );
#endif

#ifdef USE_FAR_KEYWORD
   {
      jmp_buf jmpbuf;
      png_memcpy(jmpbuf,png_ptr->jmpbuf,sizeof(jmp_buf));
      longjmp(jmpbuf, 1);
   }
#else
   longjmp(png_ptr->jmpbuf, 1);
#endif
}

/* This function is called when there is a warning, but the library thinks
   it can continue anyway.  Replacement functions don't have to do anything
   here if you don't want to.  In the default configuration, png_ptr is
   not used, but it is passed in case it may be useful. */
void
png_default_warning(png_structp png_ptr, png_const_charp message)
{
   if (!png_ptr)
      return;

#ifndef PNG_NO_STDIO
    OutputDebugStringA( "libpng warning: " );
    OutputDebugStringA( message );
    OutputDebugStringA( "\n" );
#endif
}

/* This function is called when the application wants to use another method
   of handling errors and warnings.  Note that the error function MUST NOT
   return to the calling routine or serious problems will occur. The error
   return method used in the default routine calls
   longjmp(png_ptr->jmpbuf, 1) */
void
png_set_message_fn(png_structp png_ptr, png_voidp msg_ptr, png_msg_ptr error_fn,
   png_msg_ptr warning_fn)
{
   png_ptr->msg_ptr = msg_ptr;

   png_ptr->error_fn = error_fn;
   png_ptr->warning_fn = warning_fn;
}


/* This function returns a pointer to the msg_ptr associated with the user
   functions.  The application should free any memory associated with this
   pointer before png_write_destroy and png_read_destroy are called. */
png_voidp
png_get_msg_ptr(png_structp png_ptr)
{
   return png_ptr->msg_ptr;
}



