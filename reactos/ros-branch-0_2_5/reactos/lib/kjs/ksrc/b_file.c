/*
 * The builtin File object.
 * Copyright (c) 1998-1999 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/b_file.c,v $
 * $Id: b_file.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Static methods.
 *
 *   byteToString (BYTE) => string
 * + chmod (string, int) => boolean
 * + lstat (PATH) => array / boolean
 * + remove (PATH) => boolean
 * + rename (FROM, TO) => boolean
 * + stat (PATH) => array / boolean
 *   stringToByte (STRING) => number
 *
 * Methods:
 *
 *   open (MODE) => boolean
 *   close () => boolean
 *   setPosition (POSITION [, WHENCE]) => boolean
 *   getPosition () => integer
 *   eof () => boolean
 *   read (SIZE) => string
 *   readln () => string
 *   readByte () => integer
 *   write (STRING) => boolean
 *   writeln (STRING) => boolean
 *   writeByte (INTEGER) => boolean
 * + ungetByte (BYTE) => boolean
 *   flush () => boolean
 *   getLength () => integer
 *   exists () => boolean
 *   error () => integer
 *   clearError () => true
 *
 * Properties:
 *
 *   autoFlush	boolean		mutable
 *   bufferSize	integer		mutable
 */

#include "jsint.h"

#include <sys/stat.h>

/*
 * Types and definitions.
 */

#define INSECURE()		\
  do {				\
    if (secure_mode)		\
      goto insecure_feature;	\
  } while (0)

/* Class context. */
struct file_ctx_st
{
  /* Static methods. */
  JSSymbol s_byteToString;
  JSSymbol s_chmod;
  JSSymbol s_lstat;
  JSSymbol s_remove;
  JSSymbol s_rename;
  JSSymbol s_stat;
  JSSymbol s_stringToByte;

  /* Methods */
  JSSymbol s_open;
  JSSymbol s_close;
  JSSymbol s_setPosition;
  JSSymbol s_getPosition;
  JSSymbol s_eof;
  JSSymbol s_read;
  JSSymbol s_readln;
  JSSymbol s_readByte;
  JSSymbol s_write;
  JSSymbol s_writeln;
  JSSymbol s_writeByte;
  JSSymbol s_ungetByte;
  JSSymbol s_flush;
  JSSymbol s_getLength;
  JSSymbol s_exists;
  JSSymbol s_error;
  JSSymbol s_clearError;

  /* Properties. */
  JSSymbol s_autoFlush;
  JSSymbol s_bufferSize;
};

typedef struct file_ctx_st FileCtx;

/* Instance context. */
struct file_instance_ctx_st
{
  /* Flags. */
  unsigned int dont_close : 1;

  char *path;
  JSIOStream *stream;

  /* Needed for the delete_proc. */
  JSVirtualMachine *vm;
};

typedef struct file_instance_ctx_st FileInstanceCtx;


/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  FileCtx *ctx = builtin_info->obj_context;
  FileInstanceCtx *ictx = instance_context;
  char buf[256];
  long int li = 0;
  int i = 0;
  /* char *cp; */
  int secure_mode = vm->security & JS_VM_SECURE_FILE;

  /* The default result is false. */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 0;

  /*
   * Static methods.
   */
  if (method == ctx->s_byteToString)
    {
      if (args->u.vinteger != 1)
	goto argument_error;

      i = -1;
      if (args[1].type == JS_INTEGER)
	{
	  i = args[1].u.vinteger;
	  if (i < 0 || i > 255)
	    i = -1;
	}

      js_vm_make_string (vm, result_return, NULL, 1);

      if (i < 0)
	result_return->u.vstring->len = 0;
      else
	result_return->u.vstring->data[0] = i;
    }
  /* ********************************************************************** */
  else if (method == ctx->s_chmod)
    {
#if 0
      INSECURE ();

      if (args->u.vinteger != 2)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;
      if (args[2].type != JS_INTEGER)
	goto argument_type_error;

      result_return->type= JS_BOOLEAN;

      cp = js_string_to_c_string (vm, &args[1]);
      result_return->u.vboolean = (chmod (cp, args[2].u.vinteger) == 0);
      js_free (cp);
#endif
    }
  /* ********************************************************************** */
  else if (method == ctx->s_lstat || method == ctx->s_stat)
    {
#if 0
      char *path;
      struct stat stat_st;
      int result;

      INSECURE ();

      if (args->u.vinteger != 1)
	goto argument_error;

      path = js_string_to_c_string (vm, &args[1]);

#if HAVE_LSTAT
      if (method == ctx->s_lstat)
	result = lstat (path, &stat_st);
      else
#endif /* HAVE_LSTAT */
	result = stat (path, &stat_st);

      js_free (path);

      if (result >= 0)
	{
	  JSNode *node;

	  /* Success. */
	  js_vm_make_array (vm, result_return, 13);
	  node = result_return->u.varray->data;

	  /* dev */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_dev;
	  node++;

	  /* ino */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_ino;
	  node++;

	  /* mode */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_mode;
	  node++;

	  /* nlink */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_nlink;
	  node++;

	  /* uid */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_uid;
	  node++;

	  /* gid */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_gid;
	  node++;

	  /* rdev */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_rdev;
	  node++;

	  /* size */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_size;
	  node++;

	  /* atime */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_atime;
	  node++;

	  /* mtime */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_mtime;
	  node++;

	  /* ctime */
	  node->type = JS_INTEGER;
	  node->u.vinteger = stat_st.st_ctime;
	  node++;

	  /* blksize */
	  node->type = JS_INTEGER;
#if HAVE_STAT_ST_ST_BLKSIZE
	  node->u.vinteger = stat_st.st_blksize;
#else /* not HAVE_STAT_ST_ST_BLKSIZE */
	  node->u.vinteger = 0;
#endif /* not HAVE_STAT_ST_ST_BLKSIZE */
	  node++;

	  /* blocks */
	  node->type = JS_INTEGER;
#if HAVE_STAT_ST_ST_BLOCKS
	  node->u.vinteger = stat_st.st_blocks;
#else /* not HAVE_STAT_ST_ST_BLOCKS */
	  node->u.vinteger = 0;
#endif /* not HAVE_STAT_ST_ST_BLOCKS */
	}
#endif
    }
  /* ********************************************************************** */
  else if (method == ctx->s_remove)
    {
#if 0
      char *path;

      INSECURE ();

      if (args->u.vinteger != 1)
	goto argument_error;

      if (args[1].type != JS_STRING)
	goto argument_type_error;

      path = js_string_to_c_string (vm, &args[1]);
      i = remove (path);
      js_free (path);

      result_return->u.vboolean = (i == 0);
#endif
    }
  /* ********************************************************************** */
  else if (method == ctx->s_rename)
    {
#if 0
      char *path1;
      char *path2;

      INSECURE ();

      if (args->u.vinteger != 2)
	goto argument_error;

      if (args[1].type != JS_STRING || args[2].type != JS_STRING)
	goto argument_type_error;

      path1 = js_string_to_c_string (vm, &args[1]);
      path2 = js_string_to_c_string (vm, &args[2]);

      i = rename (path1, path2);

      js_free (path1);
      js_free (path2);

      result_return->u.vboolean = (i == 0);
#endif
    }
  /* ********************************************************************** */
  else if (method == ctx->s_stringToByte)
    {
      if (args->u.vinteger != 1)
	goto argument_error;

      result_return->type = JS_INTEGER;

      if (args[1].type == JS_STRING && args[1].u.vstring->len > 0)
	result_return->u.vinteger = args[i].u.vstring->data[0];
      else
	result_return->u.vinteger = 0;
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (ictx)
	js_vm_make_string (vm, result_return, ictx->path, strlen (ictx->path));
      else
	js_vm_make_static_string (vm, result_return, "File", 4);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /*
       * Instance methods.
       */

      if (method == ctx->s_open)
	{
	  int readp = 0;
	  int writep = 0;

	  INSECURE ();

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_STRING
	      || args[1].u.vstring->len == 0
	      || args[1].u.vstring->len > 3)
	    goto argument_type_error;

	  i = args[1].u.vstring->len;
	  memcpy (buf, args[1].u.vstring->data, i);

	  if (buf[i - 1] != 'b')
	    buf[i++] = 'b';
	  buf[i] = '\0';

	  /* Check that the mode is valid. */
	  if (strcmp (buf, "rb") == 0)
	    readp = 1;
	  else if (strcmp (buf, "wb") == 0)
	    writep = 1;
	  else if (strcmp (buf, "ab") == 0)
	    writep = 1;
	  else if (strcmp (buf, "r+b") == 0)
	    readp = writep = 1;
	  else if (strcmp (buf, "w+b") == 0)
	    readp = writep = 1;
	  else if (strcmp (buf, "a+b") == 0)
	    readp = writep = 1;
	  else
	    {
	      sprintf (vm->error, "File.%s(): illegal open mode \"%s\"",
		       js_vm_symname (vm, method), buf);
	      js_vm_error (vm);
	    }

	  if (ictx->stream == NULL)
	    {
#if 0
	      /* Do open. */
	      JS_VM_ALLOCATE_FD (vm, "File.open()");
	      ictx->stream = js_iostream_file (fopen (ictx->path, buf), readp,
					       writep, 1);
	      if (ictx->stream == NULL)
		JS_VM_FREE_FD (vm);
	      else
		result_return->u.vboolean = 1;
#endif
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_close)
	{
	  if (ictx->stream != NULL)
	    {
	      int result = 0;

	      if (!ictx->dont_close)
		{
		  result = js_iostream_close (ictx->stream);
		  JS_VM_FREE_FD (vm);
		}

	      ictx->stream = NULL;
	      result_return->u.vboolean = result >= 0;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setPosition)
	{
	  if (args->u.vinteger == 1)
	    {
	      if (args[1].type != JS_INTEGER)
		goto argument_type_error;
	      li = args[1].u.vinteger;
	      i = SEEK_SET;
	    }
	  else if (args->u.vinteger == 2)
	    {
	      if (args[2].type == JS_INTEGER)
		{
		  switch (args[2].u.vinteger)
		    {
		    case 1:
		      i = SEEK_CUR;
		      break;

		    case 2:
		      i = SEEK_END;
		      break;

		    default:
		      i = SEEK_SET;
		      break;
		    }
		}
	      else
		i = SEEK_SET;
	    }
	  else
	    goto argument_error;

	  if (ictx->stream && js_iostream_seek (ictx->stream, li, i) >= 0)
	    result_return->u.vboolean = 1;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getPosition)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->type = JS_INTEGER;
	  if (ictx->stream == NULL)
	    result_return->u.vinteger = -1;
	  else
	    result_return->u.vinteger
	      = js_iostream_get_position (ictx->stream);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_eof)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->stream != NULL)
	    result_return->u.vboolean = ictx->stream->at_eof;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_read)
	{
	  size_t got;
	  char *buffer;

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER || args[1].u.vinteger < 0)
	    goto argument_type_error;

	  if (ictx->stream != NULL)
	    {
	      buffer = js_vm_alloc (vm, args[1].u.vinteger + 1);

	      got = js_iostream_read (ictx->stream, buffer,
				      args[1].u.vinteger);
	      if (got < 0)
		got = 0;

	      js_vm_make_static_string (vm, result_return, buffer, got);
	      result_return->u.vstring->staticp = 0;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_readln)
	{
	  /* int ch; */
	  unsigned int bufpos = 0;
	  unsigned int buflen = 0;
	  char *buffer = NULL;

	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->stream != NULL)
	    {
	      /* Flush all buffered output data. */
	      js_iostream_flush (ictx->stream);

	      while (1)
		{
		  /* Process all the data we have in the buffer. */
		  for (; ictx->stream->bufpos < ictx->stream->data_in_buf
			 && (ictx->stream->buffer[ictx->stream->bufpos]
			     != '\n');
		       ictx->stream->bufpos++)
		    {
		      if (bufpos >= buflen)
			{
			  buflen += 1024;
			  buffer = js_vm_realloc (vm, buffer, buflen);
			}
		      buffer[bufpos++]
			= ictx->stream->buffer[ictx->stream->bufpos];
		    }

		  if (ictx->stream->bufpos >= ictx->stream->data_in_buf)
		    {
		      /* int result; */

		      /* Read past the buffer. */
		      if (ictx->stream->at_eof)
			/* EOF seen. */
			break;

		      /* Read more data. */
		      js_iostream_fill_buffer (ictx->stream);
		    }
		  else
		    {
		      /* Got it.  Skip the newline character. */
		      ictx->stream->bufpos++;
		      break;
		    }
		}

	      /* Remove '\r' characters. */
	      while (bufpos > 0)
		if (buffer[bufpos - 1] == '\r')
		  bufpos--;
		else
		  break;

	      if (buffer == NULL)
		/* An empty string.  Allocate one byte. */
		buffer = js_vm_alloc (vm, 1);

	      /*
	       * Use the data we already had.  In maximum, it has only
	       * 1023 bytes overhead.
	       */
	      js_vm_make_static_string (vm, result_return, buffer, bufpos);
	      result_return->u.vstring->staticp = 0;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_readByte)
	{
	  result_return->type = JS_INTEGER;
	  if (ictx->stream == NULL)
	    result_return->u.vinteger = -1;
	  else
	    {
	    retry:
	      if (ictx->stream->bufpos < ictx->stream->data_in_buf)
		result_return->u.vinteger
		  = ictx->stream->buffer[ictx->stream->bufpos++];
	      else
		{
		  if (ictx->stream->at_eof)
		    result_return->u.vinteger = -1;
		  else
		    {
		      js_iostream_fill_buffer (ictx->stream);
		      goto retry;
		    }
		}
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_write || method == ctx->s_writeln)
	{
	  size_t wrote;
	  int autoflush;

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  if (ictx->stream != NULL)
	    {
	      autoflush = ictx->stream->autoflush;
	      ictx->stream->autoflush = 0;

	      wrote = js_iostream_write (ictx->stream,
					 args[1].u.vstring->data,
					 args[1].u.vstring->len);
	      if (wrote == args[1].u.vstring->len)
		{
		  /* Success. */
		  result_return->u.vboolean = 1;

		  if (method == ctx->s_writeln)
		    if (js_iostream_write (ictx->stream,
					   JS_HOST_LINE_BREAK,
					   JS_HOST_LINE_BREAK_LEN) < 0)
		      /* No, it was not a success. */
		      result_return->u.vboolean = 0;
		}

	      ictx->stream->autoflush = autoflush;
	      if (autoflush)
		js_iostream_flush (ictx->stream);
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_writeByte)
	{
	  unsigned char buf[1];

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  buf[0] = args[1].u.vinteger;

	  if (ictx->stream != NULL)
	    result_return->u.vboolean
	      = js_iostream_write (ictx->stream, buf, 1) >= 0;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_ungetByte)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (ictx->stream != NULL)
	    result_return->u.vboolean
	      = js_iostream_unget (ictx->stream, args[1].u.vinteger);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_flush)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->stream != NULL && js_iostream_flush (ictx->stream) >= 0)
	    result_return->u.vboolean = 1;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getLength)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  /* The default error code is an integer -1. */
	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = -1;

	  if (ictx->stream != NULL)
	    result_return->u.vinteger
	      = js_iostream_get_length (ictx->stream);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_exists)
	{
#if 0
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->stream)
	    {
	      /* Since we have opened the file, it must exist. */
	      result_return->u.vboolean = 1;
	    }
	  else
	    {
	      struct stat stat_st;

	      if (stat (ictx->path, &stat_st) >= 0)
		result_return->u.vboolean = 1;
	    }
#endif
	}
      /* ***************************************************************** */
      else if (method == ctx->s_error)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->type = JS_INTEGER;
	  if (ictx->stream == NULL)
	    result_return->u.vinteger = -1;
	  else
	    result_return->u.vinteger = ictx->stream->error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_clearError)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->stream != NULL)
	    {
	      ictx->stream->error = 0;
	      result_return->u.vboolean = 1;
	    }
	}
      /* ***************************************************************** */
      else
	return JS_PROPERTY_UNKNOWN;
    }
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "File.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "File.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 insecure_feature:
  sprintf (vm->error, "File.%s(): not allowed in secure mode",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}

/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  FileCtx *ctx = builtin_info->obj_context;
  FileInstanceCtx *ictx = instance_context;

  if (ictx)
    {
      /* Instance properties. */
      if (property == ctx->s_autoFlush)
	{
	  if (ictx->stream == NULL)
	    goto not_open;

	  if (set)
	    {
	      if (node->type != JS_BOOLEAN)
		goto argument_type_error;

	      ictx->stream->autoflush = node->u.vboolean;
	    }
	  else
	    {
	      node->type = JS_BOOLEAN;
	      node->u.vboolean = ictx->stream->autoflush;
	    }
	}
      /* ***************************************************************** */
      else if (property == ctx->s_bufferSize)
	{
	  if (ictx->stream == NULL)
	    goto not_open;

	  if (set)
	    {
	      unsigned char *buf;
	      unsigned int len;

	      if (node->type != JS_INTEGER)
		goto argument_type_error;

	      js_iostream_flush (ictx->stream);

	      len = node->u.vinteger;
	      buf = js_realloc (vm, ictx->stream->buffer, len);

	      ictx->stream->buflen = len;
	      ictx->stream->buffer = buf;
	    }
	  else
	    {
	      node->type = JS_INTEGER;
	      node->u.vinteger = ictx->stream->buflen;
	    }
	}
      /* ***************************************************************** */
      else
	{
	  if (!set)
	    node->type = JS_UNDEFINED;

	  return JS_PROPERTY_UNKNOWN;
	}
    }
  else
    {
      if (!set)
	node->type = JS_UNDEFINED;

      return JS_PROPERTY_UNKNOWN;
    }

  return JS_PROPERTY_FOUND;


  /* Error handling. */

 argument_type_error:
  sprintf (vm->error, "File.%s: illegal value",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

 not_open:
  sprintf (vm->error, "File.%s: the stream is not opened",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}


/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  FileInstanceCtx *instance;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "new File(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type != JS_STRING)
    {
      sprintf (vm->error, "new File(): illegal argument");
      js_vm_error (vm);
    }

  instance = js_calloc (vm, 1, sizeof (*instance));
  instance->path = js_string_to_c_string (vm, &args[1]);
  instance->vm = vm;

  js_vm_builtin_create (vm, result_return, builtin_info, instance);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  FileInstanceCtx *ictx = instance_context;

  if (ictx)
    {
      if (ictx->stream)
	{
	  if (!ictx->dont_close)
	    {
	      js_iostream_close (ictx->stream);
	      JS_VM_FREE_FD (ictx->vm);
	    }

	  ictx->stream = NULL;
	}

      js_free (ictx->path);
      js_free (ictx);
    }
}

/*
 * Global functions.
 */

void
js_builtin_File (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;
  FileCtx *ctx;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_byteToString	= js_vm_intern (vm, "byteToString");
  ctx->s_chmod		= js_vm_intern (vm, "chmod");
  ctx->s_lstat		= js_vm_intern (vm, "lstat");
  ctx->s_remove		= js_vm_intern (vm, "remove");
  ctx->s_rename		= js_vm_intern (vm, "rename");
  ctx->s_stat		= js_vm_intern (vm, "stat");
  ctx->s_stringToByte	= js_vm_intern (vm, "stringToByte");

  ctx->s_open		= js_vm_intern (vm, "open");
  ctx->s_close		= js_vm_intern (vm, "close");
  ctx->s_setPosition	= js_vm_intern (vm, "setPosition");
  ctx->s_getPosition	= js_vm_intern (vm, "getPosition");
  ctx->s_eof		= js_vm_intern (vm, "eof");
  ctx->s_read		= js_vm_intern (vm, "read");
  ctx->s_readln		= js_vm_intern (vm, "readln");
  ctx->s_readByte	= js_vm_intern (vm, "readByte");
  ctx->s_write		= js_vm_intern (vm, "write");
  ctx->s_writeln	= js_vm_intern (vm, "writeln");
  ctx->s_writeByte	= js_vm_intern (vm, "writeByte");
  ctx->s_ungetByte	= js_vm_intern (vm, "ungetByte");
  ctx->s_flush		= js_vm_intern (vm, "flush");
  ctx->s_getLength	= js_vm_intern (vm, "getLength");
  ctx->s_exists		= js_vm_intern (vm, "exists");
  ctx->s_error		= js_vm_intern (vm, "error");
  ctx->s_clearError	= js_vm_intern (vm, "clearError");

  ctx->s_autoFlush	= js_vm_intern (vm, "autoFlush");
  ctx->s_bufferSize	= js_vm_intern (vm, "bufferSize");


  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc 		= method;
  info->property_proc 		= property;
  info->new_proc 		= new_proc;
  info->delete_proc 		= delete_proc;
  info->obj_context 		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "File")];
  js_vm_builtin_create (vm, n, info, NULL);
}


void
js_builtin_File_new (JSVirtualMachine *vm, JSNode *result_return,
		     char *path, JSIOStream *stream, int dont_close)
{
  JSNode *n;
  FileInstanceCtx *ictx;

  /* Lookup our context. */
  n = &vm->globals[js_vm_intern (vm, "File")];

  /* Create a file instance. */
  ictx = js_calloc (vm, 1, sizeof (*ictx));
  ictx->path = js_strdup (vm, path);
  ictx->stream = stream;
  ictx->dont_close = dont_close;
  ictx->vm = vm;

  /* Create the builtin. */
  js_vm_builtin_create (vm, result_return, n->u.vbuiltin->info, ictx);
}
