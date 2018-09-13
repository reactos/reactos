
/* pngio.c - stub functions for i/o and memory allocation

   libpng 1.0 beta 2 - version 0.88
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   January 25, 1996

   This file provides a location for all input/output.  Users which need
   special handling are expected to write functions which have the same
   arguments as these, and perform similar functions, but possibly have
   different I/O methods.  Note that you shouldn't change these functions,
   but rather write replacement functions and then change them at run
   time with png_set_write_fn(...) or png_set_read_fn(...), etc */

#ifdef NEVER
#define PNG_INTERNAL
#define COBJMACROS
#include "png.h"
#endif

#include "headers.h"

/* Write the data to whatever output you are using.  The default routine
   writes to a file pointer.  Note that this routine sometimes gets called
   with very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered writes.  This should never be asked
   to write more then 64K on a 16 bit machine.  The cast to png_size_t is
   there to quiet warnings of certain compilers. */

void
png_write_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
   if (png_ptr->write_data_fn)
      (*(png_ptr->write_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL write function");
}

/* This is the function which does the actual writing of data.  If you are
   not writing to a standard C stream, you should create a replacement
   write_data function and use it at run time with png_set_write_fn(), rather
   than changing the library. */
#ifndef USE_FAR_KEYWORD
void
png_default_write_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    png_uint_32 check = 0;

   IStream_Write( png_ptr->pstream, data, (png_size_t)length, &check );
   if (check != length)
   {
      png_error(png_ptr, "Write Error");
   }
}
#else
/* this is the model-independent version. Since the standard I/O library
   can't handle far buffers in the medium and small models, we have to copy
   the data.
*/

#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)

#ifdef _MSC_VER
/* for FP_OFF */
#include <dos.h>
#endif

void
png_default_write_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
   png_uint_32 check;
   png_byte *n_data;

   /* Check if data really is near. If so, use usual code. */
#ifdef _MSC_VER
   /* do it this way just to quiet warning */
   FP_OFF(n_data) = FP_OFF(data);
   if (FP_SEG(n_data) == FP_SEG(data))
#else
   /* this works in MSC also but with lost segment warning */
   n_data = (png_byte *)data;
   if ((png_bytep)n_data == data)
#endif
   {
      check = fwrite(n_data, 1, (png_size_t)length, png_ptr->fp);
   }
   else
   {
      png_byte buf[NEAR_BUF_SIZE];
      png_size_t written, remaining, err;
      check = 0;
      remaining = (png_size_t)length;
      do
      {
         written = MIN(NEAR_BUF_SIZE, remaining);
         png_memcpy(buf, data, written); /* copy far buffer to near buffer */
         err = fwrite(buf, 1, written, png_ptr->fp);
         if (err != written)
            break;
         else
            check += err;
         data += written;
         remaining -= written;
      }
      while (remaining != 0);
   }
   if (check != length)
   {
      png_error(png_ptr, "Write Error");
   }
}

#endif

/* Read the data from whatever input you are using.  The default routine
   reads from a file pointer.  Note that this routine sometimes gets called
   with very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered reads.  This should never be asked
   to read more then 64K on a 16 bit machine.  The cast to png_size_t is
   there to quiet some compilers */
void
png_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   if (png_ptr->read_mode == PNG_READ_PUSH_MODE)
   {
      png_push_fill_buffer(png_ptr, data, length);
   }
   else
#endif
   {
      if (png_ptr->read_data_fn)
         (*(png_ptr->read_data_fn))(png_ptr, data, length);
      else
         png_error(png_ptr, "Call to NULL read function");
   }
}

/* This is the function which does the actual reading of data.  If you are
   not reading from a standard C stream, you should create a replacement
   read_data function and use it at run time with png_set_read_fn(), rather
   than changing the library. */
#ifndef USE_FAR_KEYWORD
void
png_default_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
  png_uint_32 check = 0;

   IStream_Read( png_ptr->pstream, data, (size_t)length, &check );
   if (check != length)
   {
      png_error(png_ptr, "Read Error");
   }
}
#else
void
png_default_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
   png_uint_32 check;
   png_byte *n_data;

   /* Check if data really is near. If so, use usual code. */
#ifdef _MSC_VER
   /* do it this way just to quiet warning */
   FP_OFF(n_data) = FP_OFF(data);
   if (FP_SEG(n_data) == FP_SEG(data))
#else
   /* this works in MSC also but with lost segment warning */
   n_data = (png_byte *)data;
   if ((png_bytep)n_data == data)
#endif
   {
      check = fread(n_data, 1, (size_t)length, png_ptr->fp);
   }
   else
   {
      png_byte buf[NEAR_BUF_SIZE];
      png_size_t read, remaining, err;
      check = 0;
      remaining = (png_size_t)length;
      do
      {
         read = MIN(NEAR_BUF_SIZE, remaining);
         err = fread(buf, 1, read, png_ptr->fp);
         png_memcpy(data, buf, read); /* copy far buffer to near buffer */
         if(err != read)
            break;
         else
            check += err;
         data += read;
         remaining -= read;
      }
      while (remaining != 0);
   }
   if (check != length)
   {
      png_error(png_ptr, "read Error");
   }
}
#endif

/* This function is called to output any data pending writing (normally
   to disk.  After png_flush is called, there should be no data pending
   writing in any buffers. */
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
void
png_flush(png_structp png_ptr)
{
   if (png_ptr->output_flush_fn)
      (*(png_ptr->output_flush_fn))(png_ptr);
}

void
png_default_flush(png_structp png_ptr)
{
    if (png_ptr->pstream)
        png_ptr->pstream->Commit( STGC_DEFAULT );
}
#endif

/* This function allows the application to supply new output functions for
   libpng if standard C streams aren't being used.

   This function takes as its arguments:
   png_ptr       - pointer to a png output data structure
   io_ptr        - pointer to user supplied structure containing info about
                   the output functions.  May be NULL.
   write_data_fn - pointer to a new output function which takes as its
                   arguments a pointer to a png_struct, a pointer to
                   data to be written, and a 32-bit unsigned int which is
                   the number of bytes to be written.  The new write
                   function should call png_error(png_ptr, "Error msg")
                   to exit and output any fatal error messages.
   flush_data_fn - pointer to a new flush function which takes as its
                   arguments a pointer to a png_struct.  After a call to
                   the flush function, there should be no data in any buffers
                   or pending transmission.  If the output method doesn't do
                   any buffering of ouput, a function prototype must still be
                   supplied although it doesn't have to do anything.  If
                   PNG_WRITE_FLUSH_SUPPORTED is not defined at libpng compile
                   time, output_flush_fn will be ignored, although it must be
                   supplied for compatibility. */
void
png_set_write_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
   png_ptr->io_ptr = io_ptr;

   if (write_data_fn)
      png_ptr->write_data_fn = write_data_fn;
   else
      png_ptr->write_data_fn = png_default_write_data;

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   if (output_flush_fn)
      png_ptr->output_flush_fn = output_flush_fn;
   else
      png_ptr->output_flush_fn = png_default_flush;
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

   /* It is an error to read while writing a png file */
   png_ptr->read_data_fn = NULL;
}


/* This function allows the application to supply a new input function
   for libpng if standard C streams aren't being used.

   This function takes as its arguments:
   png_ptr      - pointer to a png input data structure
   io_ptr       - pointer to user supplied structure containing info about
                  the input functions.  May be NULL.
   read_data_fn - pointer to a new input function which takes as it's
                  arguments a pointer to a png_struct, a pointer to
                  a location where input data can be stored, and a 32-bit
                  unsigned int which is the number of bytes to be read.
                  To exit and output any fatal error messages the new write
                  function should call png_error(png_ptr, "Error msg"). */
void
png_set_read_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn)
{
   png_ptr->io_ptr = io_ptr;

   if (read_data_fn)
      png_ptr->read_data_fn = read_data_fn;
   else
      png_ptr->read_data_fn = png_default_read_data;

   /* It is an error to write to a read device */
   png_ptr->write_data_fn = NULL;

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_ptr->output_flush_fn = NULL;
#endif /* PNG_WRITE_FLUSH_SUPPORTED */
}


/* This function returns a pointer to the io_ptr associated with the user
   functions.  The application should free any memory associated with this
   pointer before png_write_destroy and png_read_destroy are called. */
png_voidp
png_get_io_ptr(png_structp png_ptr)
{
   return png_ptr->io_ptr;
}

/* Initialize the default input/output functions for the png file.  If you
   change the read, or write routines, you can call either png_set_read_fn()
   or png_set_write_fn() instead of png_init_io(). */
void
png_init_io(png_structp png_ptr, LPSTREAM pstream )
{
    // would do a reference swap here, but need to find a place to do the release
   png_ptr->pstream = pstream;

   png_ptr->read_data_fn = png_default_read_data;
   png_ptr->write_data_fn = png_default_write_data;
#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_ptr->output_flush_fn = png_default_flush;
#endif
}

