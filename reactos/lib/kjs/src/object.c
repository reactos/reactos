/*
 * User object handling.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/object.c,v $
 * $Id: object.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

#define HASH_SIZE 128

/*
 * Prototypes for static functions.
 */

static void hash_create (JSVirtualMachine *vm, JSObject *obj);

static void hash_insert (JSVirtualMachine *vm, JSObject *obj, const char *name,
			 unsigned int name_len, int pos);

static void hash_delete (JSVirtualMachine *vm, JSObject *obj, const char *name,
			 unsigned int name_len);

static int hash_lookup (JSObject *obj, char *name, unsigned int name_len);


/*
 * Global functions.
 */

JSObject *
js_vm_object_new (JSVirtualMachine *vm)
{
  JSObject *obj;

  obj = js_vm_alloc (vm, sizeof (*obj));
  obj->hash = NULL;
  obj->num_props = 0;
  obj->props = NULL;

  return obj;
}


void
js_vm_object_mark (JSObject *obj)
{
  int i;
  unsigned int num_objects;

  if (obj == NULL)
    return;

 tail_recursive:

  if (!js_vm_mark_ptr (obj))
    /* This object has already been marked.  Nothing to do here. */
    return;

  js_vm_mark_ptr (obj->props);

  /* Mark property hash. */
  if (obj->hash)
    {
      JSObjectPropHashBucket *b;
      int i;

      js_vm_mark_ptr (obj->hash);
      js_vm_mark_ptr (obj->hash_lengths);

      for (i = 0; i < HASH_SIZE; i++)
	for (b = obj->hash[i]; b; b = b->next)
	  {
	    js_vm_mark_ptr (b);
	    js_vm_mark_ptr (b->data);
	  }
    }

  /* Mark all non-object properties. */
  num_objects = 0;
  for (i = 0; i < obj->num_props; i++)
    {
      if (obj->props[i].value.type == JS_OBJECT)
	{
	  if (!js_vm_is_marked_ptr (obj->props[i].value.u.vobject))
	    num_objects++;
	}
      else
	js_vm_mark (&obj->props[i].value);
    }

  /* And finally, mark all objects we have left. */
  if (num_objects > 0)
    {
      /* Find the objects. */
      for (i = 0; i < obj->num_props; i++)
	if (obj->props[i].value.type == JS_OBJECT
	    && !js_vm_is_marked_ptr (obj->props[i].value.u.vobject))
	  {
	    if (num_objects == 1)
	      {
		/*
		 * Hahaa, this is the only non-marked object.  We can
		 * do a tail-recursion optimization.
		 */
		obj = obj->props[i].value.u.vobject;
		goto tail_recursive;
	      }

	    /* Just mark it. */
	    js_vm_mark (&obj->props[i].value);
	  }
    }
}


int
js_vm_object_load_property (JSVirtualMachine *vm, JSObject *obj,
			    JSSymbol prop, JSNode *value_return)
{
  unsigned int ui;
  JSSymbol link_sym = vm->syms.s___proto__;
  JSObject *link_obj = NULL;

follow_link:

  /* Check if we know this property. */
  for (ui = 0; ui < obj->num_props; ui++)
    if (obj->props[ui].name == prop)
      {
	JS_COPY (value_return, &obj->props[ui].value);
	return JS_PROPERTY_FOUND;
      }
    else if (obj->props[ui].name == link_sym
	     && obj->props[ui].value.type == JS_OBJECT)
      link_obj = obj->props[ui].value.u.vobject;

  /* Undefined so far. */
  if (link_obj)
    {
      /* Follow the link. */
      obj = link_obj;

      link_obj = NULL;
      goto follow_link;
    }

  /* Undefined.  Make it undef. */
  value_return->type = JS_UNDEFINED;
  return JS_PROPERTY_UNKNOWN;
}


void
js_vm_object_store_property (JSVirtualMachine *vm, JSObject *obj,
			     JSSymbol prop, JSNode *val)
{
  unsigned int ui;
  JSSymbol free_slot = JS_SYMBOL_NULL;

  /* Check if we already know this property. */
  for (ui = 0; ui < obj->num_props; ui++)
    if (obj->props[ui].name == prop)
      {
	JS_COPY (&obj->props[ui].value, val);
	return;
      }
    else if (obj->props[ui].name == JS_SYMBOL_NULL)
      free_slot = ui;

  /* Must create a new property. */

  if (free_slot == JS_SYMBOL_NULL)
    {
      /* Expand our array of properties. */
      obj->props = js_vm_realloc (vm, obj->props,
				  (obj->num_props + 1) * sizeof (JSProperty));
      free_slot = obj->num_props++;
    }

  obj->props[free_slot].name = prop;
  obj->props[free_slot].attributes = 0;
  JS_COPY (&obj->props[free_slot].value, val);

  /* Insert it to the hash (if the hash has been created). */
  if (obj->hash)
    {
      const char *name;

      name = js_vm_symname (vm, prop);
      hash_insert (vm, obj, name, strlen (name), free_slot);
    }
}


void
js_vm_object_delete_property (JSVirtualMachine *vm, JSObject *obj,
			      JSSymbol prop)
{
  unsigned int ui;

  /* Check if we already know this property. */
  for (ui = 0; ui < obj->num_props; ui++)
    if (obj->props[ui].name == prop)
      {
	/* Found, remove it from our list of properties. */
	obj->props[ui].name = JS_SYMBOL_NULL;
	obj->props[ui].value.type = JS_UNDEFINED;

	/* Remove its name from the hash (if present). */
	if (obj->hash)
	  {
	    const char *name = js_vm_symname (vm, prop);
	    hash_delete (vm, obj, name, strlen (name));
	  }

	/* All done here. */
	return;
      }
}


void
js_vm_object_load_array (JSVirtualMachine *vm, JSObject *obj, JSNode *sel,
			 JSNode *value_return)
{
  if (sel->type == JS_INTEGER)
    {
      if (sel->u.vinteger < 0 || sel->u.vinteger >= obj->num_props)
	value_return->type = JS_UNDEFINED;
      else
	JS_COPY (value_return, &obj->props[sel->u.vinteger].value);
    }
  else if (sel->type == JS_STRING)
    {
      int pos;

      if (obj->hash == NULL)
	hash_create (vm, obj);

      pos = hash_lookup (obj, (char *) sel->u.vstring->data,
			 sel->u.vstring->len);
      if (pos < 0)
	value_return->type = JS_UNDEFINED;
      else
	JS_COPY (value_return, &obj->props[pos].value);
    }
  else
    {
      sprintf (vm->error, "load_property: illegal array index");
      js_vm_error (vm);
    }
}


void
js_vm_object_store_array (JSVirtualMachine *vm, JSObject *obj, JSNode *sel,
			  JSNode *value)
{
  if (sel->type == JS_INTEGER)
    {
      if (sel->u.vinteger < 0)
	{
	  sprintf (vm->error, "store_array: array index can't be nagative");
	  js_vm_error (vm);
	}
      if (sel->u.vinteger >= obj->num_props)
	{
	  /* Expand properties. */
	  obj->props = js_vm_realloc (vm, obj->props,
				      (sel->u.vinteger + 1)
				      * sizeof (JSProperty));

	  /* Init the possible gap. */
	  for (; obj->num_props <= sel->u.vinteger; obj->num_props++)
	    {
	      obj->props[obj->num_props].name = 0;
	      obj->props[obj->num_props].attributes = 0;
	      obj->props[obj->num_props].value.type = JS_UNDEFINED;
	    }
	}

      JS_COPY (&obj->props[sel->u.vinteger].value, value);
    }
  else if (sel->type == JS_STRING)
    {
      int pos;

      if (obj->hash == NULL)
	hash_create (vm, obj);

      pos = hash_lookup (obj, (char *) sel->u.vstring->data,
			 sel->u.vstring->len);
      if (pos < 0)
	{
	  /* It is undefined, define it. */
	  obj->props = js_vm_realloc (vm, obj->props,
				      (obj->num_props + 1)
				      * sizeof (JSProperty));

	  /*
	   * XXX if <sel> is a valid symbol, intern it and set symbol's
	   * name below.
	   */
	  obj->props[obj->num_props].name = JS_SYMBOL_NULL;
	  obj->props[obj->num_props].attributes = 0;
	  JS_COPY (&obj->props[obj->num_props].value, value);

	  hash_insert (vm, obj, (char *) sel->u.vstring->data,
		       sel->u.vstring->len, obj->num_props);

	  obj->num_props++;
	}
      else
	JS_COPY (&obj->props[pos].value, value);
    }
}


void
js_vm_object_delete_array (JSVirtualMachine *vm, JSObject *obj, JSNode *sel)
{
  if (sel->type == JS_INTEGER)
    {
      if (0 <= sel->u.vinteger && sel->u.vinteger < obj->num_props)
	{
	  JSSymbol sym;

	  sym = obj->props[sel->u.vinteger].name;
	  obj->props[sel->u.vinteger].name = JS_SYMBOL_NULL;
	  obj->props[sel->u.vinteger].value.type = JS_UNDEFINED;

	  /* Remove its name from the hash (if present and it is not NULL). */
	  if (sym != JS_SYMBOL_NULL && obj->hash)
	    {
	      const char *name = js_vm_symname (vm, sym);
	      hash_delete (vm, obj, name, strlen (name));
	    }
	}
    }
  else if (sel->type == JS_STRING)
    {
      int pos;

      if (obj->hash == NULL)
	hash_create (vm, obj);

      pos = hash_lookup (obj, (char *) sel->u.vstring->data,
			 sel->u.vstring->len);
      if (pos >= 0)
	{
	  /* Found it. */
	  obj->props[pos].name = JS_SYMBOL_NULL;
	  obj->props[pos].value.type = JS_UNDEFINED;

	  /* And, delete its name from the hash. */
	  hash_delete (vm, obj, (char *) sel->u.vstring->data,
		       sel->u.vstring->len);
	}
    }
  else
    {
      sprintf (vm->error, "delete_array: illegal array index");
      js_vm_error (vm);
    }
}


int
js_vm_object_nth (JSVirtualMachine *vm, JSObject *obj, int nth,
		  JSNode *value_return)
{
  int i;
  JSObjectPropHashBucket *b;

  value_return->type = JS_UNDEFINED;

  if (nth < 0)
    return 0;

  if (obj->hash == NULL)
    hash_create (vm, obj);

  for (i = 0; i < HASH_SIZE && nth >= obj->hash_lengths[i]; i++)
    nth -= obj->hash_lengths[i];

  if (i >= HASH_SIZE)
    return 0;

  /* The chain <i> is the correct one. */
  for (b = obj->hash[i]; b && nth > 0; b = b->next, nth--)
    ;
  if (b == NULL)
    {
      char buf[512];

      sprintf (buf,
	       "js_vm_object_nth(): chain didn't contain that many items%s",
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
      js_iostream_flush (vm->s_stderr);

      abort ();
    }

  js_vm_make_string (vm, value_return, b->data, b->len);

  return 1;
}


/*
 * Static functions.
 */

static void
hash_create (JSVirtualMachine *vm, JSObject *obj)
{
  int i;

  obj->hash = js_vm_alloc (vm, HASH_SIZE * sizeof (JSObjectPropHashBucket *));
  memset (obj->hash, 0, HASH_SIZE * sizeof (JSObjectPropHashBucket *));

  obj->hash_lengths = js_vm_alloc (vm, HASH_SIZE * sizeof (unsigned int));
  memset (obj->hash_lengths, 0, HASH_SIZE * sizeof (unsigned int));

  /* Insert all known properties to the hash. */
  for (i = 0; i < obj->num_props; i++)
    if (obj->props[i].name != JS_SYMBOL_NULL)
      {
	const char *name;

	name = js_vm_symname (vm, obj->props[i].name);
	hash_insert (vm, obj, name, strlen (name), i);
      }
}


static void
hash_insert (JSVirtualMachine *vm, JSObject *obj, const char *name,
	     unsigned int name_len, int pos)
{
  unsigned int hash;
  JSObjectPropHashBucket *b;

  hash = js_count_hash (name, name_len) % HASH_SIZE;
  for (b = obj->hash[hash]; b; b = b->next)
    if (b->len == name_len
	&& memcmp (b->data, name, name_len) == 0)
      {
	/* Ok, we already have a bucket */
	b->value = pos;
	return;
      }

  /* Create a new bucket. */
  b = js_vm_alloc (vm, sizeof (*b));
  b->len = name_len;
  b->data = js_vm_alloc (vm, b->len);
  memcpy (b->data, name, b->len);

  b->value = pos;

  b->next = obj->hash[hash];
  obj->hash[hash] = b;

  obj->hash_lengths[hash]++;
}


static void
hash_delete (JSVirtualMachine *vm, JSObject *obj, const char *name,
	     unsigned int name_len)
{
  unsigned int hash;
  JSObjectPropHashBucket *b, *prev;

  hash = js_count_hash (name, name_len) % HASH_SIZE;
  for (prev = NULL, b = obj->hash[hash]; b; prev = b, b = b->next)
    if (b->len == name_len
	&& memcmp (b->data, name, name_len) == 0)
      {
	/* Ok, found it. */
	if (prev)
	  prev->next = b->next;
	else
	  obj->hash[hash] = b->next;

	obj->hash_lengths[hash]--;

	break;
      }
}


static int
hash_lookup (JSObject *obj, char *name, unsigned int name_len)
{
  unsigned int hash;
  JSObjectPropHashBucket *b;

  hash = js_count_hash (name, name_len) % HASH_SIZE;
  for (b = obj->hash[hash]; b; b = b->next)
    if (b->len == name_len
	&& memcmp (b->data, name, name_len) == 0)
      return b->value;

  return -1;
}
