/*
 * Namespace handling.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/namespace.js,v $
 * $Id: namespace.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Global functions.
 */

JSC$SCOPE_ARG = 1;
JSC$SCOPE_LOCAL = 2;

function JSC$NameSpace ()
{
  this.frame = new JSC$NameSpaceFrame ();
  this.push_frame = JSC$NameSpace_push_frame;
  this.pop_frame = JSC$NameSpace_pop_frame;
  this.alloc_local = JSC$NameSpace_alloc_local;
  this.define_symbol = JSC$NameSpace_define_symbol;
  this.lookup_symbol = JSC$NameSpace_lookup_symbol;
}


function JSC$NameSpace_push_frame ()
{
  var f = new JSC$NameSpaceFrame ();

  f.num_locals = this.frame.num_locals;

  f.next = this.frame;
  this.frame = f;
}


function JSC$NameSpace_pop_frame ()
{
  var i;

  for (i = this.frame.defs; i != null; i = i.next)
    if (i.usecount == 0)
      {
	if (i.scope == JSC$SCOPE_ARG)
	  {
	    if (JSC$warn_unused_argument)
	      JSC$warning (JSC$filename + ":" + i.linenum.toString ()
			   + ": warning: unused argument `" + i.symbol + "'");
	  }
	else
	  {
	    if (JSC$warn_unused_variable)
	      JSC$warning (JSC$filename + ":" + i.linenum.toString ()
			   + ": warning: unused variable `" + i.symbol + "'");
	  }
      }

  this.frame = this.frame.next;
}


function JSC$NameSpace_alloc_local ()
{
  return this.frame.num_locals++;
}


function JSC$NameSpace_define_symbol (symbol, scope, value, linenum)
{
  var i;

  for (i = this.frame.defs; i != null; i = i.next)
    if (i.symbol == symbol)
      {
	if (i.scope == scope)
	  error (JSC$filename + ":" + i.linenum.toString()
		 + ": redeclaration of `" + i.symbol + "'");
	if (i.scope == JSC$SCOPE_ARG && JSC$warn_shadow)
	  JSC$warning (JSC$filename + ":" + linenum.toString ()
		       + ": warning: declaration of `" + symbol
		       + "' shadows a parameter");

	i.scope = scope;
	i.value = value;
	i.linenum = linenum;

	return;
      }

  /* Create a new definition. */
  i = new JSC$SymbolDefinition (symbol, scope, value, linenum);
  i.next = this.frame.defs;
  this.frame.defs = i;
}


function JSC$NameSpace_lookup_symbol (symbol)
{
  var f, i;

  for (f = this.frame; f != null; f = f.next)
    for (i = f.defs; i != null; i = i.next)
      if (i.symbol == symbol)
	{
	  i.usecount++;
	  return i;
	}

  return null;
}

/*
 * Static helpers.
 */

function JSC$NameSpaceFrame ()
{
  this.next = null;
  this.defs = null;
  this.num_locals = 0;
}


function JSC$SymbolDefinition (symbol, scope, value, linenum)
{
  this.next = null;
  this.symbol = symbol;
  this.scope = scope;
  this.value = value;
  this.linenum = linenum;
  this.usecount = 0;
}


/*
Local variables:
mode: c
End:
*/
