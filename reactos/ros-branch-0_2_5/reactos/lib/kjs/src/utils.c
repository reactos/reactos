/*
 * General utilites.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/utils.c,v $
 * $Id: utils.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Global variables.
 */

unsigned char js_latin1_tolower[256] =
{
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0x61, 0xe7,
 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7,
 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

unsigned char js_latin1_toupper[256] =
{
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
 0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xe6, 0xc7,
 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7,
 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xff,
};


/*
 * Global functions.
 */

void
js_vm_to_primitive (JSVirtualMachine *vm, const JSNode *n,
		    JSNode *result_return, JSNodeType preferred_type)
{
  JSNode args;

  switch (n->type)
    {
    case JS_UNDEFINED:
    case JS_NULL:
    case JS_BOOLEAN:
    case JS_INTEGER:
    case JS_FLOAT:
    case JS_NAN:
    case JS_STRING:
      JS_COPY (result_return, n);
      break;

    case JS_OBJECT:
      if (preferred_type == JS_STRING)
	{
	  if (js_vm_call_method (vm, (JSNode *) n, "toString", 0, &args)
	      && JS_IS_PRIMITIVE_VALUE (&vm->exec_result))
	    JS_COPY (result_return, &vm->exec_result);
	  else if (js_vm_call_method (vm, (JSNode *) n, "valueOf", 0, &args)
		   && JS_IS_PRIMITIVE_VALUE (&vm->exec_result))
	    JS_COPY (result_return, &vm->exec_result);
	  else
	    {
	      sprintf (vm->error, "ToPrimitive(): couldn't convert");
	      js_vm_error (vm);
	    }
	}
      else
	{
	  /* It must be, or it defaults to NUMBER. */
	  if (js_vm_call_method (vm, (JSNode *) n, "valueOf", 0, &args)
	      && JS_IS_PRIMITIVE_VALUE (&vm->exec_result))
	    JS_COPY (result_return, &vm->exec_result);
	  else
	    js_vm_to_string (vm, n, result_return);
	}
      break;

    case JS_BUILTIN:
      /* XXX ToPrimitive() for built-ins. */
      sprintf (vm->error, "ToPrimitive(): not implemented yet for built-ins");
      js_vm_error (vm);
      break;

    case JS_ARRAY:
    case JS_SYMBOL:
    case JS_FUNC:
    case JS_IPTR:
    default:
      sprintf (vm->error, "ToPrimitive(): couldn't convert (%d)", n->type);
      js_vm_error (vm);
      break;
    }
}


void
js_vm_to_string (JSVirtualMachine *vm, const JSNode *n, JSNode *result_return)
{
  const char *tostring;
  JSNode args;
  JSNode *nnoconst = (JSNode *) n; /* toString methods behave `constantly'
				      so no panic here. */

  /* Create empty arguments. */
  args.type = JS_INTEGER;
  args.u.vinteger = 0;

  switch (n->type)
    {
    case JS_UNDEFINED:
      tostring = "undefined";
      break;

    case JS_NULL:
      tostring = "null";
      break;

    case JS_BOOLEAN:
    case JS_INTEGER:
    case JS_FLOAT:
    case JS_NAN:
    case JS_STRING:
    case JS_ARRAY:
      (void) (*vm->prim[n->type]->method_proc) (vm, vm->prim[n->type],
						nnoconst, vm->syms.s_toString,
						result_return, &args);
      return;
      break;

    case JS_OBJECT:
      /* Try to call object's toString() method. */
      if (js_vm_call_method (vm, (JSNode *) n, "toString", 0, &args)
	  && vm->exec_result.type == JS_STRING)
	{
	  JS_COPY (result_return, &vm->exec_result);
	  return;
	}

      /* No match. */
      tostring = "object";
      break;

    case JS_SYMBOL:
      tostring = js_vm_symname (vm, n->u.vsymbol);
      break;

    case JS_BUILTIN:
      if (n->u.vbuiltin->info->method_proc)
	if ((*n->u.vbuiltin->info->method_proc) (
					vm,
					n->u.vbuiltin->info,
					n->u.vbuiltin->instance_context,
					vm->syms.s_toString, result_return,
					&args)
	  == JS_PROPERTY_FOUND)
	  return;

      /* Builtin didn't answer toString().  Let's use our default. */
      tostring = "builtin";
      break;

    case JS_FUNC:
      tostring = "function";
      break;

    case JS_IPTR:
      tostring = "pointer";
      break;

    default:
      tostring = "??? unknown type in js_vm_to_string() ???";
      break;
    }

  js_vm_make_static_string (vm, result_return, tostring, strlen (tostring));
}


void
js_vm_to_number (JSVirtualMachine *vm, const JSNode *n, JSNode *result_return)
{
  char *cp, *end;

  switch (n->type)
    {
    case JS_UNDEFINED:
      result_return->type = JS_NAN;
      break;

    case JS_NULL:
      result_return->type = JS_INTEGER;
      result_return->u.vinteger = 0;
      break;

    case JS_BOOLEAN:
      result_return->type = JS_INTEGER;
      result_return->u.vinteger = n->u.vboolean ? 1 : 0;
      break;

    case JS_INTEGER:
    case JS_FLOAT:
    case JS_NAN:
      JS_COPY (result_return, n);
      break;

    case JS_STRING:
      cp = js_string_to_c_string (vm, n);
      result_return->u.vinteger = strtol (cp, &end, 10);

      if (cp == end)
	{
	  int i;

	  /* It failed.  Check the `Infinity'. */

	  for (i = 0; cp[i] && JS_IS_STR_WHITE_SPACE_CHAR (cp[i]); i++)
	    ;

	  if (cp[i] && memcmp (cp + i, "Infinity", 8) == 0)
	    JS_MAKE_POSITIVE_INFINITY (result_return);
	  else
	    result_return->type = JS_NAN;
	}
      else
	{
	  if (*end == '.' || *end == 'e' || *end == 'E')
	    {
	      /* It is a float number. */
	      result_return->u.vfloat = strtod (cp, &end);
	      if (cp == end)
		/*  Couldn't parse. */
		result_return->type = JS_NAN;
	      else
		/* Success. */
		result_return->type = JS_FLOAT;
	    }
	  else
	    {
	      /* It is an integer. */
	      result_return->type = JS_INTEGER;
	    }
	}

      js_free (cp);
      break;

    case JS_ARRAY:
    case JS_OBJECT:
    case JS_BUILTIN:
      /* XXX Not implemented yet. */
      result_return->type = JS_NAN;
      break;

    case JS_SYMBOL:
    case JS_FUNC:
    case JS_IPTR:
    default:
      result_return->type = JS_NAN;
      break;
    }
}


void
js_vm_to_object (JSVirtualMachine *vm, const JSNode *n, JSNode *result_return)
{
  switch (n->type)
    {
    case JS_BOOLEAN:
    case JS_INTEGER:
    case JS_FLOAT:
    case JS_NAN:
    case JS_OBJECT:
      JS_COPY (result_return, n);
      break;

    case JS_STRING:
      js_vm_make_string (vm, result_return, n->u.vstring->data,
			 n->u.vstring->len);
      break;

    case JS_UNDEFINED:
    case JS_NULL:
    default:
      sprintf (vm->error, "ToObject(): illegal argument");
      js_vm_error (vm);
      break;
    }
}


JSInt32
js_vm_to_int32 (JSVirtualMachine *vm, JSNode *n)
{
  JSNode intermediate;
  JSInt32 result;

  js_vm_to_number (vm, n, &intermediate);

  switch (intermediate.type)
    {
    case JS_INTEGER:
      result = (JSInt32) intermediate.u.vinteger;
      break;

    case JS_FLOAT:
      if (JS_IS_POSITIVE_INFINITY (&intermediate)
	  || JS_IS_NEGATIVE_INFINITY (&intermediate))
	result = 0;
      else
	result = (JSInt32) intermediate.u.vfloat;
      break;

    case JS_NAN:
    default:
      result = 0;
      break;
    }

  return result;
}


int
js_vm_to_boolean (JSVirtualMachine *vm, JSNode *n)
{
  int result;

  switch (n->type)
    {
    case JS_BOOLEAN:
      result = n->u.vboolean;
      break;

    case JS_INTEGER:
      result = n->u.vinteger != 0;
      break;

    case JS_FLOAT:
      result = n->u.vfloat != 0;
      break;

    case JS_STRING:
      result = n->u.vstring->len > 0;
      break;

    case JS_OBJECT:
      result = 1;
      break;

    case JS_UNDEFINED:
    case JS_NULL:
    case JS_NAN:
    default:
      result = 0;
      break;
    }

  return result;
}
