/*
 * The builtin Date object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_date.c,v $
 * $Id: b_date.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"
#include "rentrant.h"

/* XXX TODO 15.13.3 -> */

/*
 * Types and definitions.
 */

#define GMT_DATE_FORMAT	"%a, %d %b %Y %H:%M:%S GMT"

#define MS_PER_SECOND	1000
#define MS_PER_MINUTE	(60 * MS_PER_SECOND)
#define MS_PER_HOUR	(60 * MS_PER_MINUTE)
#define MS_PER_DAY	(24 * MS_PER_HOUR)

/* Class context. */
struct date_ctx_st
{
  /* Static methods. */
  JSSymbol s_parse;

  /* Methods. */
  JSSymbol s_format;
  JSSymbol s_formatGMT;
  JSSymbol s_getDate;
  JSSymbol s_getDay;
  JSSymbol s_getHours;
  JSSymbol s_getMinutes;
  JSSymbol s_getMonth;
  JSSymbol s_getSeconds;
  JSSymbol s_getTime;
  JSSymbol s_getTimezoneOffset;
  JSSymbol s_getYear;
  JSSymbol s_setDate;
  JSSymbol s_setHours;
  JSSymbol s_setMinutes;
  JSSymbol s_setMonth;
  JSSymbol s_setSeconds;
  JSSymbol s_setTime;
  JSSymbol s_setYear;
  JSSymbol s_toGMTString;
  JSSymbol s_toLocaleString;
  JSSymbol s_UTC;
};

typedef struct date_ctx_st DateCtx;

/* Date instance context. */
struct date_instance_ctx_st
{
  time_t secs;
  struct tm localtime;
};

typedef struct date_instance_ctx_st DateInstanceCtx;

/*
 * Static functions.
 */

/* Global methods. */
void
MakeTime_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  if (args->u.vinteger != 4)
    {
      sprintf (vm->error, "MakeTime: illegal amount of argument");
      js_vm_error (vm);
    }
  if (!JS_IS_NUMBER (&args[1]) || !JS_IS_NUMBER (&args[2])
      || !JS_IS_NUMBER (&args[3]) || !JS_IS_NUMBER (&args[4]))
    {
      sprintf (vm->error, "MakeTime: illegal argument");
      js_vm_error (vm);
    }
  if (!JS_IS_FINITE (&args[1]) || !JS_IS_FINITE (&args[2])
      || !JS_IS_FINITE (&args[3]) || !JS_IS_FINITE (&args[4]))
    {
      result_return->type = JS_NAN;
    }
  else
    {
      JSInt32 hour, min, sec, ms;

      hour = js_vm_to_int32 (vm, &args[1]);
      min  = js_vm_to_int32 (vm, &args[2]);
      sec  = js_vm_to_int32 (vm, &args[3]);
      ms   = js_vm_to_int32 (vm, &args[4]);

      result_return->type = JS_FLOAT;
      result_return->u.vfloat = (hour * MS_PER_HOUR
				 + min * MS_PER_MINUTE
				 + sec * MS_PER_SECOND
				 + ms);
    }
}


void
MakeDay_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		       void *instance_context, JSNode *result_return,
		       JSNode *args)
{
  if (args->u.vinteger != 3)
    {
      sprintf (vm->error, "MakeDay: illegal amount of argument");
      js_vm_error (vm);
    }
  if (!JS_IS_NUMBER (&args[1]) || !JS_IS_NUMBER (&args[2])
      || !JS_IS_NUMBER (&args[3]))
    {
      sprintf (vm->error, "MakeDay: illegal argument");
      js_vm_error (vm);
    }
  if (!JS_IS_FINITE (&args[1]) || !JS_IS_FINITE (&args[2])
      || !JS_IS_FINITE (&args[3]))
    {
      result_return->type = JS_NAN;
    }
  else
    {
      JSInt32 year, month, day;

      year  = js_vm_to_int32 (vm, &args[1]);
      month = js_vm_to_int32 (vm, &args[2]);
      day   = js_vm_to_int32 (vm, &args[3]);

      sprintf (vm->error, "MakeDay: not implemented yet");
      js_vm_error (vm);
    }
}


void
MakeDate_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  if (args->u.vinteger != 2)
    {
      sprintf (vm->error, "MakeDate: illegal amount of argument");
      js_vm_error (vm);
    }
  if (!JS_IS_NUMBER (&args[1]) || !JS_IS_NUMBER (&args[2]))
    {
      sprintf (vm->error, "MakeDate: illegal argument");
      js_vm_error (vm);
    }
  if (!JS_IS_FINITE (&args[1]) || !JS_IS_FINITE (&args[2]))
    {
      result_return->type = JS_NAN;
    }
  else
    {
      JSInt32 day;
      JSInt32 time;

      day  = js_vm_to_int32 (vm, &args[1]);
      time = js_vm_to_int32 (vm, &args[2]);

      result_return->type = JS_FLOAT;
      result_return->u.vfloat = (day * MS_PER_DAY + time);
    }
}


void
TimeClip_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "TimeClip: illegal amount of argument");
      js_vm_error (vm);
    }
  if (!JS_IS_NUMBER (&args[1]))
    {
      sprintf (vm->error, "TimeClip: illegal argument");
      js_vm_error (vm);
    }
  if (!JS_IS_FINITE (&args[1]))
    {
      result_return->type = JS_NAN;
    }
  else
    {
      result_return->type = JS_FLOAT;

      if (args[1].type == JS_INTEGER)
	result_return->u.vfloat = (double) args[1].u.vinteger;
      else
	result_return->u.vfloat = args[1].u.vfloat;

      if (result_return->u.vfloat > 8.64e15
	  || result_return->u.vfloat < -8.64e15)
	result_return->type = JS_NAN;
    }
}

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  DateCtx *ctx = builtin_info->obj_context;
  DateInstanceCtx *ictx = instance_context;

  /* The default return type is integer. */
  result_return->type = JS_INTEGER;

  /* Static methods. */
  if (method == ctx->s_parse)
    {
      goto not_implemented_yet;
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (ictx)
	goto date_to_string;
      else
	js_vm_make_static_string (vm, result_return, "Date", 4);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /* Methods. */

      if (method == ctx->s_format || method == ctx->s_formatGMT)
	{
	  struct tm tm_st;
	  struct tm *tm = &ictx->localtime;
	  char *fmt;
	  char *buf;
	  unsigned int buflen;

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  fmt = js_string_to_c_string (vm, &args[1]);

	  buflen = args[1].u.vstring->len * 2 + 1;
	  buf = js_malloc (vm, buflen);

	  if (method == ctx->s_formatGMT)
	    {
	      js_gmtime (&ictx->secs, &tm_st);
	      tm = &tm_st;
	    }

	  if (args[1].u.vstring->len == 0)
	    buf[0] = '\0';
	  else
	    {
	      while (strftime (buf, buflen, fmt, tm) == 0)
		{
		  /* Expand the buffer. */
		  buflen *= 2;
		  buf = js_realloc (vm, buf, buflen);
		}
	    }

	  js_vm_make_string (vm, result_return, buf, strlen (buf));

	  js_free (fmt);
	  js_free (buf);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getDate)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_mday;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getDay)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_wday;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getHours)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_hour;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getMinutes)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_min;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getMonth)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_mon;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getSeconds)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_sec;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getTime)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->type = JS_FLOAT;
	  result_return->u.vfloat = (double) ictx->secs * 1000;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getTimezoneOffset)
	goto not_implemented_yet;
      /* ***************************************************************** */
      else if (method == ctx->s_getYear)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->u.vinteger = ictx->localtime.tm_year;
	  if (ictx->localtime.tm_year >= 100
	      || ictx->localtime.tm_year < 0)
	    result_return->u.vinteger += 1900;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setDate)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (1 <= args[1].u.vinteger && args[1].u.vinteger <= 31)
	    {
	      ictx->localtime.tm_mday = args[1].u.vinteger;
	      ictx->secs = mktime (&ictx->localtime);
	    }
	  else
	    goto argument_range_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setHours)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (0 <= args[1].u.vinteger && args[1].u.vinteger <= 23)
	    {
	      ictx->localtime.tm_hour = args[1].u.vinteger;
	      ictx->secs = mktime (&ictx->localtime);
	    }
	  else
	    goto argument_range_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setMinutes)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (0 <= args[1].u.vinteger && args[1].u.vinteger <= 59)
	    {
	      ictx->localtime.tm_min = args[1].u.vinteger;
	      ictx->secs = mktime (&ictx->localtime);
	    }
	  else
	    goto argument_range_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setMonth)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (0 <= args[1].u.vinteger && args[1].u.vinteger <= 11)
	    {
	      ictx->localtime.tm_mon = args[1].u.vinteger;
	      ictx->secs = mktime (&ictx->localtime);
	    }
	  else
	    goto argument_range_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setSeconds)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  if (0 <= args[1].u.vinteger && args[1].u.vinteger <= 59)
	    {
	      ictx->localtime.tm_sec = args[1].u.vinteger;
	      ictx->secs = mktime (&ictx->localtime);
	    }
	  else
	    goto argument_range_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setTime)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type == JS_INTEGER)
	    ictx->secs = args[1].u.vinteger / 1000;
	  else if (args[1].type == JS_FLOAT)
	    ictx->secs = (long) (args[1].u.vfloat / 1000);
	  else
	    goto argument_type_error;

	  js_localtime (&ictx->secs, &ictx->localtime);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setYear)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  ictx->localtime.tm_year = args[1].u.vinteger;
	  if (args[1].u.vinteger < 0 || args[1].u.vinteger >= 100)
	    ictx->localtime.tm_year -= 1900;

	  ictx->secs = mktime (&ictx->localtime);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_toGMTString)
	{
	  struct tm tm_st;
	  char buf[1024];	/* This is enought. */

	  if (args->u.vinteger != 0)
	    goto argument_error;

	  js_gmtime (&ictx->secs, &tm_st);
	  strftime (buf, sizeof (buf), GMT_DATE_FORMAT, &tm_st);

	  js_vm_make_string (vm, result_return, buf, strlen (buf));
	}
      /* ***************************************************************** */
      else if (method == ctx->s_toLocaleString)
	{
	  char *cp;
	  char buf[1024];	/* This is enought */

	  if (args->u.vinteger != 0)
	    goto argument_error;

	date_to_string:

	  js_asctime (&ictx->localtime, buf, sizeof (buf));
	  cp = strchr (buf, '\n');
	  if (cp)
	    *cp = '\0';

	  js_vm_make_string (vm, result_return, buf, strlen (buf));
	}
      /* ***************************************************************** */
      else if (method == ctx->s_UTC)
	goto not_implemented_yet;
      /* ***************************************************************** */
      else
	return JS_PROPERTY_UNKNOWN;
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 not_implemented_yet:
  sprintf (vm->error, "Date.%s(): not implemented yet",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_error:
  sprintf (vm->error, "Date.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "Date.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_range_error:
  sprintf (vm->error, "Date.%s(): argument out of range",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/* Global method proc. */
static void
global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	       void *instance_context, JSNode *result_return,
	       JSNode *args)
{
  time_t secs;
  struct tm localtime;
  char buf[512];
  char *cp;

  if (args->u.vinteger > 7)
    {
      sprintf (vm->error, "Date(): illegal amount of arguments");
      js_vm_error (vm);
    }

  /*
   * We ignore our arguments and return the result of:
   * `new Date ().toString ()'.
   */

  secs = time (NULL);
  js_localtime (&secs, &localtime);
  js_asctime (&localtime, buf, sizeof (buf));

  cp = strchr (buf, '\n');
  if (cp)
    *cp = '\0';

  js_vm_make_string (vm, result_return, buf, strlen (buf));
}


/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}

/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  DateInstanceCtx *instance;
  time_t seconds = 0;		/* Initialized to keep compiler quiet. */

  instance = js_calloc (vm, 1, sizeof (*instance));

  if (args->u.vinteger == 0)
    {
      instance->secs = time (NULL);
      js_localtime (&instance->secs, &instance->localtime);
    }
  else if (args->u.vinteger == 1)
    goto not_implemented_yet;
  else if (args->u.vinteger == 3 || args->u.vinteger == 6)
    {
      int i;

      for (i = 0; i < args->u.vinteger; i++)
	if (args[i + 1].type != JS_INTEGER)
	  goto argument_type_error;

      /* Year. */
      instance->localtime.tm_year = args[1].u.vinteger;
      if (args[1].u.vinteger < 0 || args[1].u.vinteger >= 100)
	instance->localtime.tm_year -= 1900;

      /* Month. */
      if (0 <= args[2].u.vinteger && args[2].u.vinteger <= 11)
	instance->localtime.tm_mon = args[2].u.vinteger;
      else
	goto argument_range_error;

      /* Day. */
      if (1 <= args[3].u.vinteger && args[3].u.vinteger <= 31)
	instance->localtime.tm_mday = args[3].u.vinteger;
      else
	goto argument_range_error;

      if (args->u.vinteger == 6)
	{
	  /* Sync the localtime according to year, month and day. */
	  mktime (&instance->localtime);

	  /* Hours. */
	  if (0 <= args[4].u.vinteger && args[4].u.vinteger <= 23)
	    instance->localtime.tm_hour = args[4].u.vinteger;
	  else
	    goto argument_range_error;

	  /* Minutes. */
	  if (0 <= args[5].u.vinteger && args[5].u.vinteger <= 59)
	    instance->localtime.tm_min = args[5].u.vinteger;
	  else
	    goto argument_range_error;

	  /* Seconds. */
	  if (0 <= args[6].u.vinteger && args[6].u.vinteger <= 59)
	    instance->localtime.tm_sec = args[6].u.vinteger;
	  else
	    goto argument_range_error;
	}

      instance->secs = mktime (&instance->localtime);
    }
  else
    {
      js_free (instance);

      sprintf (vm->error, "new Date(): illegal amount of arguments");
      js_vm_error (vm);
    }

  js_vm_builtin_create (vm, result_return, builtin_info, instance);

  return;

  /*
   * Error handling.
   */

 not_implemented_yet:
  sprintf (vm->error, "new Date(%ld args): not implemented yet",
	   args->u.vinteger);
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "new Date(): illegal argument");
  js_vm_error (vm);

 argument_range_error:
  sprintf (vm->error, "new Date(): argument out of range");
  js_vm_error (vm);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  DateInstanceCtx *ictx = instance_context;

  if (ictx)
    js_free (ictx);
}


/*
 * Global functions.
 */

static struct
{
  char *name;
  JSBuiltinGlobalMethod method;
} global_methods[] =
{
  {"MakeTime",	MakeTime_global_method},
  {"MakeDay",	MakeDay_global_method},
  {"MakeDate",	MakeDate_global_method},
  {"TimeClip",	TimeClip_global_method},

  {NULL, NULL},
};

void
js_builtin_Date (JSVirtualMachine *vm)
{
  JSBuiltinInfo *info;
  DateCtx *ctx;
  JSNode *n;
  int i;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_format			= js_vm_intern (vm, "format");
  ctx->s_formatGMT		= js_vm_intern (vm, "formatGMT");
  ctx->s_getDate		= js_vm_intern (vm, "getDate");
  ctx->s_getDay			= js_vm_intern (vm, "getDay");
  ctx->s_getHours		= js_vm_intern (vm, "getHours");
  ctx->s_getMinutes		= js_vm_intern (vm, "getMinutes");
  ctx->s_getMonth		= js_vm_intern (vm, "getMonth");
  ctx->s_getSeconds		= js_vm_intern (vm, "getSeconds");
  ctx->s_getTime		= js_vm_intern (vm, "getTime");
  ctx->s_getTimezoneOffset	= js_vm_intern (vm, "getTimezoneOffset");
  ctx->s_getYear		= js_vm_intern (vm, "getYear");
  ctx->s_parse			= js_vm_intern (vm, "parse");
  ctx->s_setDate		= js_vm_intern (vm, "setDate");
  ctx->s_setHours		= js_vm_intern (vm, "setHours");
  ctx->s_setMinutes		= js_vm_intern (vm, "setMinutes");
  ctx->s_setMonth		= js_vm_intern (vm, "setMonth");
  ctx->s_setSeconds		= js_vm_intern (vm, "setSeconds");
  ctx->s_setTime		= js_vm_intern (vm, "setTime");
  ctx->s_setYear		= js_vm_intern (vm, "setYear");
  ctx->s_toGMTString		= js_vm_intern (vm, "toGMTString");
  ctx->s_toLocaleString		= js_vm_intern (vm, "toLocaleString");
  ctx->s_UTC			= js_vm_intern (vm, "UTC");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc		= method;
  info->global_method_proc	= global_method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->delete_proc		= delete_proc;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Date")];
  js_vm_builtin_create (vm, n, info, NULL);

  /* Global methods. */

  for (i = 0; global_methods[i].name; i++)
    {
      info = js_vm_builtin_info_create (vm);
      info->global_method_proc = global_methods[i].method;
      n = &vm->globals[js_vm_intern (vm, global_methods[i].name)];
      js_vm_builtin_create (vm, n, info, NULL);
    }
}
