/*
 * Byte-code file handling.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsdas/bc.js,v $
 * $Id: bc.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

function ByteCode$SymtabEntry (name, offset)
{
  this.name = name;
  this.offset = offset;
}


function ByteCode ()
{
  /* Private methods. */
  this.sectionName = ByteCode$sectionName;
  this.lookupSection = ByteCode$lookupSection;
  this.parseConstants = ByteCode$parseConstants;
  this.parseSymtab = ByteCode$parseSymtab;
  this.prettyPrintString = ByteCode$prettyPrintString;
  this.opIsSymbol = ByteCode$opIsSymbol;
  this.opIsJump = ByteCode$opIsJump;

  /* Public methods. */
  this.parse = ByteCode$parse;
  this.write = ByteCode$write;
  this.printInfo = ByteCode$printInfo;
  this.printCode = ByteCode$printCode;
  this.printConstants = ByteCode$printConstants;
  this.printSymtab = ByteCode$printSymtab;
  this.printDebug = ByteCode$printDebug;
  this.linkSection = ByteCode$linkSection;
  this.removeSection = ByteCode$removeSection;
}


function ByteCode$sectionName (type)
{
  var name = type.toString ();

  if (type == JSC$BC_SECT_CODE)
    name += " (code)";
  else if (type == JSC$BC_SECT_CONSTANTS)
    name += " (constants)";
  else if (type == JSC$BC_SECT_SYMTAB)
    name +=  " (symtab)";
  else if (type == JSC$BC_SECT_DEBUG)
    name += " (debug)";

  return name;
}


function ByteCode$lookupSection (sect)
{
  var i;
  for (i = 0; i < this.sect_types.length; i++)
    if (this.sect_types[i] == sect)
      return this.sect_data[i];

  System.print ("jsdas: no section ", this.sectionName (sect), "\n");
  return false;
}


function ByteCode$parseConstants ()
{
  if (this.constants)
    return this.constants;

  var data = this.lookupSection (JSC$BC_SECT_CONSTANTS);
  if (!data)
    return false;

  this.constants = new Array ();
  var pos, count = 0;
  for (pos = 0; pos < data.length; count++)
    {
      var type = data[pos++];
      var val;

      if (type == JSC$CONST_INT)
	{
	  val = data.substr (pos, 4).unpack ("N")[0];
	  pos += 4;
	}
      else if (type == JSC$CONST_STRING)
	{
	  var len = data.substr (pos, 4).unpack ("N")[0];
	  pos += 4;

	  val = "\"" + data.substr (pos, len);
	  pos += len;
	}
      else if (type == JSC$CONST_FLOAT)
	{
	  val = data.substr (pos, 8).unpack ("d")[0];
	  pos += 8;
	}
      else if (type == JSC$CONST_SYMBOL)
	{
	  val = new String ("S");
	  for (; data[pos] != #'\0'; pos++)
	    val.append (data.charAt (pos));
	  pos++;
	}
      else if (type == JSC$CONST_NAN)
	{
	  val = parseFloat ("****");
	}
      else if (type == JSC$CONST_REGEXP)
	{
	  var flags = data.substr (pos, 1).unpack ("C")[0];
	  pos++;

	  var len = data.substr (pos, 4).unpack ("N")[0];
	  pos += 4;

	  val = "/" + File.byteToString (flags) + data.substr (pos, len);
	  pos += len;
	}
      else
	{
	  /* Unknown constant type. */
	  this.constants = false;
	  return false;
	}

      this.constants[count] = val;
    }

  return this.constants;
}


function ByteCode$parseSymtab ()
{
  if (this.symtab)
    return this.symtab;

  var data = this.lookupSection (JSC$BC_SECT_SYMTAB);
  if (!data)
    return false;

  this.symtab = new Array ();

  /* Fetch the number of symtab entries. */
  var nentries = data.unpack ("N")[0];

  var pos, count = 0;
  for (pos = 4; pos < data.length; count++)
    {
      /* Read the symbol name. */
      var name = new String ("");
      for (; data[pos] != #'\0'; pos++)
	name.append (data.charAt (pos));
      pos++;

      /* Read the offset. */
      var offset = data.substr (pos, 4).unpack ("N")[0];
      pos += 4;

      this.symtab.push (new ByteCode$SymtabEntry (name, offset));
    }

  if (count != nentries)
    System.print ("jsdas: warning: expected ", nentries,
		  " symtab entries, but got ", count, "\n");

  return this.symtab;
}


function ByteCode$prettyPrintString (str)
{
  var i;
  var tail = "";
  var ender;

  /* Determine the type. */
  if (str[0] == #'"')
    {
      i = 1;
      ender = "\"";
    }
  else
    {
      /* Regexp. */
      var flags = str[1];
      ender = "/";

      if (flags & JSC$CONST_REGEXP_FLAG_G)
	tail += "g";
      if (flags & JSC$CONST_REGEXP_FLAG_I)
	tail += "i";

      i = 2;
    }

  System.print (ender);
  for (; i < str.length; i++)
    {
      if (str[i] == ender[0] || str[i] ==  #'\\')
	System.print ("\\" + str.charAt (i));
      else if (str[i] == #'\n')
	System.print ("\\n");
      else if (str[i] == #'\t')
	System.print ("\\t");
      else if (str[i] == #'\v')
	System.print ("\\v");
      else if (str[i] == #'\b')
	System.print ("\\b");
      else if (str[i] == #'\r')
	System.print ("\\r");
      else if (str[i] == #'\f')
	System.print ("\\f");
      else if (str[i] == #'\a')
	System.print ("\\a");
      else if (str[i] == #'\'')
	System.print ("\\\'");
      else if (str[i] == #'\"')
	System.print ("\\\"");
      else
	System.print (str.charAt (i));
    }
  System.print (ender, tail);
}

function ByteCode$opIsSymbol (op)
{
  return (DASM$op_flags[op] & 0x01) != 0;
}

function ByteCode$opIsJump (op)
{
  return (DASM$op_flags[op] & 0x02) != 0;
}


function ByteCode$parse (stream)
{
  var ch;
  var buf;

  ch = stream.readByte ();
  if (ch == #'#')
    {
      if (stream.readByte () != #'!')
	return false;

      /* Skip the first comment line. */
      this.first_line = "#!" + stream.readln ();
    }
  else
    {
      stream.ungetByte (ch);
      this.first_line = false;
    }

  buf = stream.read (4);
  if (buf.length != 4)
    return false;

  var a = buf.unpack ("N");
  if (a[0] != JSC$BC_MAGIC)
    {
      System.print ("jsdas: illegal magic: ", a[0].toString (16),
		    ", should be: ", JSC$BC_MAGIC.toString (16), "\n");
      return false;
    }

  buf = stream.read (4);
  if (buf.length != 4)
    return false;

  var num_sections = buf.unpack ("N")[0];
  this.sect_types = new Array ();
  this.sect_data = new Array ();

  /* Read sections. */
  for (i = 0; i < num_sections; i++)
    {
      /* Type and length. */
      buf = stream.read (8);
      if (buf.length != 8)
	return false;

      a = buf.unpack ("NN");

      this.sect_types[i] = a[0];

      var len = a[1];
      buf = stream.read (len);
      if (buf.length != len)
	{
	  System.stdout.writeln ("couldn't read section " + i.toString ()
				 + ", expected=" + len.toString ()
				 + ", got=" + buf.length.toString ());
	  return false;
	}

      this.sect_data[i] = buf;
    }

  return true;
}


function ByteCode$printInfo ()
{
  var i;

  for (i = 0; i < this.sect_types.length; i++)
    {
      var sectname = this.sectionName (this.sect_types[i]);
      System.print ("  section ", i, ": ");
      System.print ("type=", sectname);
      System.print (", length=", this.sect_data[i].length, "\n");
    }
}


function ByteCode$printConstants ()
{
  if (!this.parseConstants ())
    {
      System.print ("jsdas: couldn't find a parse the constants section\n");
      return;
    }

  var i;
  for (i = 0; i < this.constants.length; i++)
    {
      System.print ("  ", i, ":\t");

      var c = this.constants[i];
      if (typeof c == "number")
	System.print (c);
      else if (typeof c == "string")
	{
	  /* Must be a string, regexp or symbol. */
	  if (c[0] == #'S')
	    /* Symbol. */
	    System.print (c.substr (1));
	  else
	    this.prettyPrintString (c);
	}
      else
	{
	  System.print ("jsdas: illegal element in the constans array: type=",
			typeof c, "\n");
	  return;
	}

      System.print ("\n");
    }
}


function ByteCode$printSymtab ()
{
  var symtab = this.parseSymtab ();
  if (!symtab)
    {
      System.print ("jsdas: couldn't find or parse symtab section\n");
      return;
    }

  var i;
  for (i = 0; i < symtab.length; i++)
    {
      var name = symtab[i].name;
      var offset = symtab[i].offset;

      System.print ("  ", name);

      var pad = name.length;
      for (; pad < 40; pad++)
	System.print (" ");

      System.print (offset, "\n");
    }
}


function ByteCode$printDebug ()
{
  var data = this.lookupSection (JSC$BC_SECT_DEBUG);
  if (!data)
    return;

  var pos, count = 0;
  var filename = "<unknown>";
  for (pos = 0; pos < data.length; count++)
    {
      var type = data[pos++];

      if (type == JSC$DEBUG_FILENAME)
	{
	  var len = data.substr (pos, 4).unpack ("N")[0];
	  pos += 4;

	  filename = data.substr (pos, len);
	  pos += len;
	}
      else if (type == JSC$DEBUG_LINENUMBER)
	{
	  var a = data.substr (pos, 8).unpack ("NN");
	  pos += 8;

	  System.print ("  ", a[0], "\t", filename, ":", a[1], "\n");
	}
      else
	{
	  System.print ("jsdas: unknown debug entry: ",
			"skipping all remaining data\n");
	  return;
	}
    }
}


function ByteCode$printCode ()
{
  var data = this.lookupSection (JSC$BC_SECT_CODE);
  if (!data)
    return;

  var consts = this.parseConstants ();
  if (!consts)
    {
      System.print ("jsdas: couldn't find or parse constants\n");
      return;
    }

  var symtab = this.parseSymtab ();
  if (!symtab)
    {
      System.print ("jsdas: couldn't find or parse symtab\n");
      return;
    }

  var symtab_pos = -1;
  var next_symtab_offset = -1;

  var pos;
  for (pos = 0; pos < data.length; )
    {
      /* Handle symtab entries. */
      if (pos >= next_symtab_offset)
	{
	  while (pos > next_symtab_offset)
	    {
	      /* Lookup the next offset. */
	      symtab_pos++;
	      if (symtab_pos >= symtab.length)
		next_symtab_offset = data.length + 1;
	      else
		next_symtab_offset = symtab[symtab_pos].offset;
	    }

	  if (pos == next_symtab_offset)
	    System.print (pos == 0 ? "" : "\n",
			  symtab[symtab_pos].name, ":\n");
	}

      System.print ("  ", pos, "\t");

      var op = data[pos++];
      if (!DASM$op_names[op])
	{
	  System.print ("jsdas: unknown operand: ",
			"skipping all remaining data\n");
	  return;
	}

      System.print (DASM$op_names[op], "\t");
      if (DASM$op_names[op].length < 8)
	System.print ("\t");

      var ds = DASM$op_data[op];
      var val;

      if (ds == 1)
	val = data[pos];
      else if (ds == 2)
	val = data.substr (pos, 2).unpack ("n")[0];
      else if (ds == 4)
	val = data.substr (pos, 4).unpack ("N")[0];

      if (op == JSC$OP_CONST || this.opIsSymbol (op))
	{
	  /* Handle symbols and constants. */

	  var c = consts[val];
	  if (typeof c == "string")
	    {
	      if (c[0] == #'S')
		System.print (c.substr (1));
	      else
		this.prettyPrintString (c);
	    }
	  else
	    System.print (c);
	}
      else if (this.opIsJump (op))
	{
	  /* Local jumps. */
	  val += pos + ds;
	  System.print (val);
	}
      else
	{
	  /* Handle all the rest. */
	  if (ds != 0)
	    System.print (val);
	}

      pos += ds;

      System.print ("\n");
    }
}


function ByteCode$linkSection (type, data)
{
  var i;
  for (i = 0; i < this.sect_types.length; i++)
    if (this.sect_types[i] == type)
      {
	this.sect_data[i] = data;
	return;
      }

  /* Create a new section. */
  this.sect_types.push (type);
  this.sect_data.push (data);
}


function ByteCode$removeSection (type)
{
  var i;
  for (i = 0; i < this.sect_types.length; i++)
    if (this.sect_types[i] == type)
      {
	/* Found it. */
	this.sect_types.splice (i, 1);
	this.sect_data.splice (i, 1);
	return true;
      }

  return false;
}


function ByteCode$write (name)
{
  var fp = new File (name);
  if (fp.open ("w"))
    {
      /* Possible first line. */
      if (this.first_line)
	fp.writeln (this.first_line);

      /* Magic. */
      fp.write (String.pack ("N", JSC$BC_MAGIC));

      /* Number of sections. */
      fp.write (String.pack ("N", this.sect_types.length));

      var i;
      for (i = 0; i < this.sect_types.length; i++)
	{
	  /* Write type and the length of the section. */
	  fp.write (String.pack ("NN", this.sect_types[i],
				 this.sect_data[i].length));

	  /* Write the data. */
	  fp.write (this.sect_data[i]);
	}

      return fp.close ();
    }

  return false;
}


/*
Local variables:
mode: c
End:
*/
