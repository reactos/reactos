/*
 * Input stream definitions.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/streams.js,v $
 * $Id: streams.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * File stream.
 */

function JSC$StreamFile (name)
{
  this.name = name;
  this.stream = new File (name);
  this.error = "";

  this.open		= JSC$StreamFile_open;
  this.close		= JSC$StreamFile_close;
  this.rewind		= JSC$StreamFile_rewind;
  this.readByte		= JSC$StreamFile_read_byte;
  this.ungetByte	= JSC$StreamFile_unget_byte;
  this.readln		= JSC$StreamFile_readln;
}


function JSC$StreamFile_open ()
{
  if (!this.stream.open ("r"))
    {
      this.error = System.strerror (System.errno);
      return false;
    }

  return true;
}


function JSC$StreamFile_close ()
{
  return this.stream.close ();
}


function JSC$StreamFile_rewind ()
{
  return this.stream.setPosition (0);
}


function JSC$StreamFile_read_byte ()
{
  return this.stream.readByte ();
}


function JSC$StreamFile_unget_byte (byte)
{
  this.stream.ungetByte (byte);
}


function JSC$StreamFile_readln ()
{
  return this.stream.readln ();
}


/*
 * String stream.
 */

function JSC$StreamString (str)
{
  this.name = "StringStream";
  this.string = str;
  this.pos = 0;
  this.unget_byte = -1;
  this.error = "";

  this.open		= JSC$StreamString_open;
  this.close		= JSC$StreamString_close;
  this.rewind 		= JSC$StreamString_rewind;
  this.readByte		= JSC$StreamString_read_byte;
  this.ungetByte	= JSC$StreamString_unget_byte;
  this.readln 		= JSC$StreamString_readln;
}


function JSC$StreamString_open ()
{
  return true;
}


function JSC$StreamString_close ()
{
  return true;
}


function JSC$StreamString_rewind ()
{
  this.pos = 0;
  this.unget_byte = -1;
  this.error = "";
  return true;
}


function JSC$StreamString_read_byte ()
{
  var ch;

  if (this.unget_byte >= 0)
    {
      ch = this.unget_byte;
      this.unget_byte = -1;
      return ch;
    }

  if (this.pos >= this.string.length)
    return -1;

  return this.string.charCodeAt (this.pos++);
}


function JSC$StreamString_unget_byte (byte)
{
  this.unget_byte = byte;
}


function JSC$StreamString_readln ()
{
  var line = new String ("");
  var ch;

  while ((ch = this.readByte ()) != -1 && ch != #'\n')
    line.append (String.pack ("C", ch));

  return line;
}


/*
Local variables:
mode: c
End:
*/
