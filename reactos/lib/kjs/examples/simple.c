/*
 * An example how to embed the JavaScript interpreter to your program.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/examples/simple.c,v $
 * $Id: simple.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <js.h>

/*
 * Static functions.
 */

/* I/O function for the standard error stream. */
int
io_stderr (void *context, unsigned char *buffer, unsigned int amount)
{
  return fwrite (buffer, 1, amount, stderr);
}


/* The `hello' command. */
JSMethodResult
hello_proc (void *context, JSInterpPtr interp, int argc, JSType *argv,
	    JSType *result_return, char *error_return)
{
  char *msg = context;

  printf ("%s\n", msg);

  return JS_OK;
}

/* The `add' command. */
JSMethodResult
add_proc (void *context, JSInterpPtr interp, int argc, JSType *argv,
	  JSType *result_return, char *error_return)
{
  int i;
  double d = 0.0;
  int doubles = 0;

  for (i = 0; i < argc; i++)
    {
      JSType *arg = &argv[i];

      switch (arg->type)
	{
	case JS_TYPE_INTEGER:
	  d += (double) arg->u.i;
	  break;

	case JS_TYPE_DOUBLE:
	  d += arg->u.d;
	  doubles++;
	  break;

	default:
	  sprintf (error_return, "add: illegal argument");
	  return JS_ERROR;
	  break;
	}
    }

  if (doubles == 0)
    {
      result_return->type = JS_TYPE_INTEGER;
      result_return->u.i = (long) d;
    }
  else
    {
      result_return->type = JS_TYPE_DOUBLE;
      result_return->u.d = d;
    }

  return JS_OK;
}

/* Methods for the `Hello' class. */

/* The instance context for Hello objects. */
struct hello_ctx_st
{
  char msg[1024];
};

typedef struct hello_ctx_st HelloCtx;

/* Method `Hello.show()'. */
static JSMethodResult
hello_show (JSClassPtr cls, void *instance_context, JSInterpPtr interp,
	    int argc, JSType *argv, JSType *result_return, char *error_return)
{
  HelloCtx *ictx = instance_context;
  char *msg;

  if (ictx)
    msg = ictx->msg;
  else
    msg = js_class_context (cls);

  printf ("%s\n", msg);

  return JS_OK;
}

/* Method `Hello.fail()'. */
static JSMethodResult
hello_fail (JSClassPtr cls, void *instance_context, JSInterpPtr interp,
	    int argc, JSType *argv, JSType *result_return, char *error_return)
{
  strcpy (error_return, "My mission is to fail!");

  return JS_ERROR;
}

/* Method `Hello.investigate(obj)'. */
static JSMethodResult
hello_investigate (JSClassPtr cls, void *instance_context, JSInterpPtr interp,
		   int argc, JSType *argv, JSType *result_return,
		   char *error_return)
{
  HelloCtx *others_ictx;

  if (argc != 1)
    {
      strcpy (error_return, "illegal amount of arguments");
      return JS_ERROR;
    }
  if (!js_isa (interp, &argv[0], js_lookup_class (interp, "Hello"),
	       (void **) &others_ictx))
    {
      strcpy (error_return, "illegal argument");
      return JS_ERROR;
    }

  printf ("Hello.investigate(): my argument's message is \"%s\"\n",
	  others_ictx->msg);

  return JS_OK;
}

/* Take a copy of us. */
static JSMethodResult
hello_copy (JSClassPtr cls, void *instance_context, JSInterpPtr interp,
	    int argc, JSType *argv, JSType *result_return,
	    char *error_return)
{
  HelloCtx *ictx = instance_context;
  HelloCtx *nictx;
  JSClassPtr rcls;

  rcls = js_lookup_class (interp, "Hello");
  if (cls != rcls)
    {
      sprintf (error_return, "internal class error");
      return JS_ERROR;
    }

  nictx = malloc (sizeof (*nictx));
  if (ictx)
    strcpy (nictx->msg, ictx->msg);
  else
    strcpy (nictx->msg, "Hello, world!!!!!");

  js_instantiate_class (interp, cls, nictx, free, result_return);

  return JS_OK;
}

/* The getter function for the `Hello.msg' property. */
static JSMethodResult
hello_msg (JSClassPtr cls, void *instance_context, JSInterpPtr interp,
	   int setp, JSType *value, char *error_return)
{
  HelloCtx *ictx = instance_context;
  char *msg;

  if (ictx)
    msg = ictx->msg;
  else
    msg = js_class_context (cls);

  js_type_make_string (interp, value, msg, strlen (msg));
}


static JSMethodResult
hello_constructor (JSClassPtr cls, JSInterpPtr interp, int argc, JSType *argv,
		   void **ictx_return, JSFreeProc *ictx_destructor_return,
		   char *error_return)
{
  HelloCtx *ictx;
  int len;

  if (argc != 1)
    {
      strcpy (error_return, "wront amount of arguments");
      return JS_ERROR;
    }
  if (argv[0].type != JS_TYPE_STRING)
    {
      strcpy (error_return, "illegal argument");
      return JS_ERROR;
    }

  ictx = calloc (1, sizeof (*ictx));
  if (ictx == NULL)
    {
      strcpy (error_return, "out of memory");
      return JS_ERROR;
    }

  len = argv[0].u.s->len;
  if (len + 1 > sizeof (ictx->msg))
    len = sizeof (ictx->msg) - 1;

  memcpy (ictx->msg, argv[0].u.s->data, len);
  ictx->msg[len] = '\0';

  *ictx_return = ictx;
  *ictx_destructor_return = free;

  return JS_OK;
}


/*
 * Global functions.
 */

int
main (int argc, char *argv[])
{
  JSInterpPtr interp;
  JSInterpOptions options;
  JSClassPtr cls;

  int i;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s FILE...\n", argv[0]);
      exit (1);
    }

  /* Create one interpreter. */

  js_init_default_options (&options);

  options.verbose = 2;
  options.s_stderr = io_stderr;
  options.s_context = NULL;

  interp = js_create_interp (&options);

  /* Create our extensions. */
  js_create_global_method (interp, "hello", hello_proc,
			   "(global method) Hello, world!", NULL);
  js_create_global_method (interp, "add", add_proc, NULL, NULL);

  /* Create our class. */
  cls = js_class_create ("(Hello) Hello, world!", NULL, 0,
			 hello_constructor);

  js_class_define_method (cls, "show", JS_CF_STATIC, hello_show);
  js_class_define_method (cls, "fail", JS_CF_STATIC, hello_fail);
  js_class_define_method (cls, "investigate", JS_CF_STATIC, hello_investigate);
  js_class_define_method (cls, "copy", 0, hello_copy);

  js_class_define_property (cls, "msg", JS_CF_STATIC | JS_CF_IMMUTABLE,
			    hello_msg);

  js_define_class (interp, cls, "Hello");

  /* Evaluate all argument files. */
  for (i = 1; i < argc; i++)
    if (!js_eval_file (interp, argv[i]))
      fprintf (stderr, "evaluation of file `%s' failed: %s\n",
	       argv[i], js_error_message (interp));

  /* Call our add command with the js_apply() function. */
  {
    JSType argv[3];

    argv[0].type = JS_TYPE_INTEGER;
    argv[0].u.i = 42;

    argv[1].type = JS_TYPE_INTEGER;
    argv[1].u.i = 72;

    if (!js_apply (interp, "add", 2, argv))
      {
	fprintf (stderr, "js_apply() failed: %s\n", js_error_message (interp));
	exit (1);
      }

    js_result (interp, &argv[2]);

    if (argv[2].type != JS_TYPE_INTEGER)
      {
	fprintf (stderr, "add() returned wrong type!\n");
	exit (1);
      }

    printf ("%ld + %ld = %ld\n", argv[0].u.i, argv[1].u.i, argv[2].u.i);
  }

  /* Cleanup. */
  js_destroy_interp (interp);

  return 1;
}
