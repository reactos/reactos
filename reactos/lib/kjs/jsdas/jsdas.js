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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsdas/jsdas.js,v $
 * $Id: jsdas.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
/*                                                              -*- c -*-
 * Operand definitions for the JavaScript byte-code.
 *
 * This file is automatically create from the operands.def file.
 * Editing is strongly discouraged.  You should edit the file
 * `extract-op-names.js' instead.
 */

DASM$op_names = new Array ();
DASM$op_data = new Array ();
DASM$op_flags = new Array ();
DASM$op_names[0]	= "halt";
DASM$op_data[0] 	= 0;
DASM$op_flags[0] 	= 0x0;
DASM$op_names[1]	= "done";
DASM$op_data[1] 	= 0;
DASM$op_flags[1] 	= 0x0;
DASM$op_names[2]	= "nop";
DASM$op_data[2] 	= 0;
DASM$op_flags[2] 	= 0x0;
DASM$op_names[3]	= "dup";
DASM$op_data[3] 	= 0;
DASM$op_flags[3] 	= 0x0;
DASM$op_names[4]	= "pop";
DASM$op_data[4] 	= 0;
DASM$op_flags[4] 	= 0x0;
DASM$op_names[5]	= "pop_n";
DASM$op_data[5] 	= 1;
DASM$op_flags[5] 	= 0x0;
DASM$op_names[6]	= "apop";
DASM$op_data[6] 	= 1;
DASM$op_flags[6] 	= 0x0;
DASM$op_names[7]	= "swap";
DASM$op_data[7] 	= 0;
DASM$op_flags[7] 	= 0x0;
DASM$op_names[8]	= "roll";
DASM$op_data[8] 	= 1;
DASM$op_flags[8] 	= 0x0;
DASM$op_names[9]	= "const";
DASM$op_data[9] 	= 4;
DASM$op_flags[9] 	= 0x0;
DASM$op_names[10]	= "const_null";
DASM$op_data[10] 	= 0;
DASM$op_flags[10] 	= 0x0;
DASM$op_names[11]	= "const_true";
DASM$op_data[11] 	= 0;
DASM$op_flags[11] 	= 0x0;
DASM$op_names[12]	= "const_false";
DASM$op_data[12] 	= 0;
DASM$op_flags[12] 	= 0x0;
DASM$op_names[13]	= "const_undefined";
DASM$op_data[13] 	= 0;
DASM$op_flags[13] 	= 0x0;
DASM$op_names[14]	= "const_i0";
DASM$op_data[14] 	= 0;
DASM$op_flags[14] 	= 0x0;
DASM$op_names[15]	= "const_i1";
DASM$op_data[15] 	= 0;
DASM$op_flags[15] 	= 0x0;
DASM$op_names[16]	= "const_i2";
DASM$op_data[16] 	= 0;
DASM$op_flags[16] 	= 0x0;
DASM$op_names[17]	= "const_i3";
DASM$op_data[17] 	= 0;
DASM$op_flags[17] 	= 0x0;
DASM$op_names[18]	= "const_i";
DASM$op_data[18] 	= 4;
DASM$op_flags[18] 	= 0x0;
DASM$op_names[19]	= "load_global";
DASM$op_data[19] 	= 4;
DASM$op_flags[19] 	= 0x1;
DASM$op_names[20]	= "store_global";
DASM$op_data[20] 	= 4;
DASM$op_flags[20] 	= 0x1;
DASM$op_names[21]	= "load_arg";
DASM$op_data[21] 	= 1;
DASM$op_flags[21] 	= 0x0;
DASM$op_names[22]	= "store_arg";
DASM$op_data[22] 	= 1;
DASM$op_flags[22] 	= 0x0;
DASM$op_names[23]	= "load_local";
DASM$op_data[23] 	= 2;
DASM$op_flags[23] 	= 0x0;
DASM$op_names[24]	= "store_local";
DASM$op_data[24] 	= 2;
DASM$op_flags[24] 	= 0x0;
DASM$op_names[25]	= "load_property";
DASM$op_data[25] 	= 4;
DASM$op_flags[25] 	= 0x1;
DASM$op_names[26]	= "store_property";
DASM$op_data[26] 	= 4;
DASM$op_flags[26] 	= 0x1;
DASM$op_names[27]	= "load_array";
DASM$op_data[27] 	= 0;
DASM$op_flags[27] 	= 0x0;
DASM$op_names[28]	= "store_array";
DASM$op_data[28] 	= 0;
DASM$op_flags[28] 	= 0x0;
DASM$op_names[29]	= "nth";
DASM$op_data[29] 	= 0;
DASM$op_flags[29] 	= 0x0;
DASM$op_names[30]	= "cmp_eq";
DASM$op_data[30] 	= 0;
DASM$op_flags[30] 	= 0x0;
DASM$op_names[31]	= "cmp_ne";
DASM$op_data[31] 	= 0;
DASM$op_flags[31] 	= 0x0;
DASM$op_names[32]	= "cmp_lt";
DASM$op_data[32] 	= 0;
DASM$op_flags[32] 	= 0x0;
DASM$op_names[33]	= "cmp_gt";
DASM$op_data[33] 	= 0;
DASM$op_flags[33] 	= 0x0;
DASM$op_names[34]	= "cmp_le";
DASM$op_data[34] 	= 0;
DASM$op_flags[34] 	= 0x0;
DASM$op_names[35]	= "cmp_ge";
DASM$op_data[35] 	= 0;
DASM$op_flags[35] 	= 0x0;
DASM$op_names[36]	= "cmp_seq";
DASM$op_data[36] 	= 0;
DASM$op_flags[36] 	= 0x0;
DASM$op_names[37]	= "cmp_sne";
DASM$op_data[37] 	= 0;
DASM$op_flags[37] 	= 0x0;
DASM$op_names[38]	= "sub";
DASM$op_data[38] 	= 0;
DASM$op_flags[38] 	= 0x0;
DASM$op_names[39]	= "add";
DASM$op_data[39] 	= 0;
DASM$op_flags[39] 	= 0x0;
DASM$op_names[40]	= "mul";
DASM$op_data[40] 	= 0;
DASM$op_flags[40] 	= 0x0;
DASM$op_names[41]	= "div";
DASM$op_data[41] 	= 0;
DASM$op_flags[41] 	= 0x0;
DASM$op_names[42]	= "mod";
DASM$op_data[42] 	= 0;
DASM$op_flags[42] 	= 0x0;
DASM$op_names[43]	= "neg";
DASM$op_data[43] 	= 0;
DASM$op_flags[43] 	= 0x0;
DASM$op_names[44]	= "and";
DASM$op_data[44] 	= 0;
DASM$op_flags[44] 	= 0x0;
DASM$op_names[45]	= "not";
DASM$op_data[45] 	= 0;
DASM$op_flags[45] 	= 0x0;
DASM$op_names[46]	= "or";
DASM$op_data[46] 	= 0;
DASM$op_flags[46] 	= 0x0;
DASM$op_names[47]	= "xor";
DASM$op_data[47] 	= 0;
DASM$op_flags[47] 	= 0x0;
DASM$op_names[48]	= "shift_left";
DASM$op_data[48] 	= 0;
DASM$op_flags[48] 	= 0x0;
DASM$op_names[49]	= "shift_right";
DASM$op_data[49] 	= 0;
DASM$op_flags[49] 	= 0x0;
DASM$op_names[50]	= "shift_rright";
DASM$op_data[50] 	= 0;
DASM$op_flags[50] 	= 0x0;
DASM$op_names[51]	= "iffalse";
DASM$op_data[51] 	= 4;
DASM$op_flags[51] 	= 0x2;
DASM$op_names[52]	= "iftrue";
DASM$op_data[52] 	= 4;
DASM$op_flags[52] 	= 0x2;
DASM$op_names[53]	= "call_method";
DASM$op_data[53] 	= 4;
DASM$op_flags[53] 	= 0x1;
DASM$op_names[54]	= "jmp";
DASM$op_data[54] 	= 4;
DASM$op_flags[54] 	= 0x2;
DASM$op_names[55]	= "jsr";
DASM$op_data[55] 	= 0;
DASM$op_flags[55] 	= 0x0;
DASM$op_names[56]	= "return";
DASM$op_data[56] 	= 0;
DASM$op_flags[56] 	= 0x0;
DASM$op_names[57]	= "typeof";
DASM$op_data[57] 	= 0;
DASM$op_flags[57] 	= 0x0;
DASM$op_names[58]	= "new";
DASM$op_data[58] 	= 0;
DASM$op_flags[58] 	= 0x0;
DASM$op_names[59]	= "delete_property";
DASM$op_data[59] 	= 4;
DASM$op_flags[59] 	= 0x1;
DASM$op_names[60]	= "delete_array";
DASM$op_data[60] 	= 0;
DASM$op_flags[60] 	= 0x0;
DASM$op_names[61]	= "locals";
DASM$op_data[61] 	= 2;
DASM$op_flags[61] 	= 0x0;
DASM$op_names[62]	= "min_args";
DASM$op_data[62] 	= 1;
DASM$op_flags[62] 	= 0x0;
DASM$op_names[63]	= "load_nth_arg";
DASM$op_data[63] 	= 0;
DASM$op_flags[63] 	= 0x0;
DASM$op_names[64]	= "with_push";
DASM$op_data[64] 	= 0;
DASM$op_flags[64] 	= 0x0;
DASM$op_names[65]	= "with_pop";
DASM$op_data[65] 	= 1;
DASM$op_flags[65] 	= 0x0;
DASM$op_names[66]	= "try_push";
DASM$op_data[66] 	= 4;
DASM$op_flags[66] 	= 0x2;
DASM$op_names[67]	= "try_pop";
DASM$op_data[67] 	= 1;
DASM$op_flags[67] 	= 0x0;
DASM$op_names[68]	= "throw";
DASM$op_data[68] 	= 0;
DASM$op_flags[68] 	= 0x0;
DASM$op_names[69]	= "iffalse_b";
DASM$op_data[69] 	= 4;
DASM$op_flags[69] 	= 0x2;
DASM$op_names[70]	= "iftrue_b";
DASM$op_data[70] 	= 4;
DASM$op_flags[70] 	= 0x2;
DASM$op_names[71]	= "add_1_i";
DASM$op_data[71] 	= 0;
DASM$op_flags[71] 	= 0x0;
DASM$op_names[72]	= "add_2_i";
DASM$op_data[72] 	= 0;
DASM$op_flags[72] 	= 0x0;
DASM$op_names[73]	= "load_global_w";
DASM$op_data[73] 	= 4;
DASM$op_flags[73] 	= 0x1;
DASM$op_names[74]	= "jsr_w";
DASM$op_data[74] 	= 4;
DASM$op_flags[74] 	= 0x1;
/*
 * Disassembler for JavaScript byte-code files.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsdas/jsdas.js,v $
 * $Id: jsdas.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Variables and definitions.
 */

version_number = "0.0.1";

/*
 * Options.
 */

/*
 * -c, --code
 *
 * Print code section.
 */

opt_code = false;

/*
 * -C, --constants
 *
 * Print constants section.
 */

opt_constants = false;

/*
 * -d, --debug
 *
 * Print the debug section.
 */

opt_debug = false;

/*
 * -h, --help
 *
 * Print short help and exit successfully.
 */

/*
 * -i, --info
 *
 * Show general information about the file.
 */

opt_info = false;

/*
 * -l TYPE DATA, --link TYPE DATA
 *
 * Link a new section to the byte-code file.  The new section's type is
 * TYPE and its data is in file DATA.
 */

opt_link = false;
opt_link_type = 0;
opt_link_data = "";

/*
 * -r TYPE, --remove TYPE
 *
 * Remove section TYPE.
 */

opt_remove = false;
opt_remove_type = 0;

/*
 * -s, --symtab
 *
 * Print symtab section.
 */

opt_symtab = false;

/*
 * -S, --strip
 *
 * Remove the debug section from byte-code files.
 */

/*
 * -V, --version
 *
 * Print version information and exit successfully.
 */


/*
 * Functions.
 */

function main ()
{
  var opt_default = true;

  var idx = ARGS[0].lastIndexOf ("/");
  if (idx >= 0)
    program = ARGS[0].substr (idx + 1);
  else
    program = ARGS[0];

  /* Handle arguments. */
  var i;
  for (i = 1; i < ARGS.length; i++)
    {
      if (ARGS[i][0] == #'-')
	{
	  if (ARGS[i] == "-c" || ARGS[i] == "--code")
	    {
	      opt_code = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-C" || ARGS[i] == "--constants")
	    {
	      opt_constants = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-d" || ARGS[i] == "--debug")
	    {
	      opt_debug = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-i" || ARGS[i] == "--info")
	    {
	      opt_info = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-l" || ARGS[i] == "--link")
	    {
	      opt_link = true;
	      opt_default = false;

	      if (i + 2 >= ARGS.length)
		{
		  System.error (program, ": no arguments for option --link\n");
		  System.exit (1);
		}

	      opt_link_type = parseInt (ARGS[++i]);
	      if (isNaN (opt_link_type))
		{
		  System.error (program, ": illegal section type `",
				ARGS[i], "'\n");
		  System.exit (1);
		}
	      opt_link_data = (ARGS[++i]);
	    }
	  else if (ARGS[i] == "-r" || ARGS[i] == "--remove")
	    {
	      opt_remove = true;
	      opt_default = false;

	      if (i + 1 >= ARGS.length)
		{
		  System.error (program,
				": no arguments for option --remove\n");
		  System.exit (1);
		}

	      opt_remove_type = parseInt (ARGS[++i]);
	      if (isNaN (opt_remove_type))
		{
		  System.error (program, ": illegal section type `",
				ARGS[i], "'\n");
		  System.exit (1);
		}
	    }
	  else if (ARGS[i] == "-s" || ARGS[i] == "--symtab")
	    {
	      opt_symtab = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-S" || ARGS[i] == "--strip")
	    {
	      opt_remove = true;
	      opt_default = false;
	      opt_remove_type = JSC$BC_SECT_DEBUG;
	    }
	  else if (ARGS[i] == "-h" || ARGS[i] == "--help")
	    {
	      usage ();
	      System.exit (0);
	    }
	  else if (ARGS[i] == "-V" || ARGS[i] == "--version")
	    {
	      version ();
	      System.exit (0);
	    }
	  else
	    {
	      /* Unrecognized option. */
	      System.error (program, ": unrecognized option `",
			    ARGS[i], "'\n");
	      System.error ("Try `", program,
			    " --help' for more information.\n");
	      System.exit (1);
	    }
	}
      else
	{
	  /* End of arguments. */
	  break;
	}
    }

  if (i >= ARGS.length)
    {
      System.error (program, ": no files specified\n");
      System.exit (1);
    }

  /* Process files. */
  var first = true;
  for (; i < ARGS.length; i++)
    {
      if (first)
	first = false;
      else
	System.print ("\n");

      System.print (ARGS[i], ":\n");
      var is = new File (ARGS[i]);
      if (is.open ("r"))
	{
	  var bc = new ByteCode ();
	  var result = bc.parse (is);
	  is.close ();

	  if (!result)
	    System.print (program, ": couldn't parse byte-code file `",
			  ARGS[i], "'\n");
	  else
	    {
	      var write_back = false;

	      if (opt_info)
		{
		  System.print ("\n* byte-code file information\n\n");
		  bc.printInfo ();
		}

	      if (opt_constants)
		{
		  System.print ("\n* section `Constants'\n\n");
		  bc.printConstants ();
		}

	      if (opt_default || opt_code)
		{
		  System.print ("\n* section `Code'\n\n");
		  bc.printCode ();
		}

	      if (opt_symtab)
		{
		  System.print ("\n* section `Symtab'\n\n");
		  bc.printSymtab ();
		}

	      if (opt_debug)
		{
		  System.print ("\n* section `Debug'\n\n");
		  bc.printDebug ();
		}

	      if (opt_link)
		{
		  var df = new File (opt_link_data);
		  if (df.open ("r"))
		    {
		      var len = df.getLength ();
		      var data = df.read (len);
		      df.close ();

		      if (data.length < len)
			System.error (program,
				      ": couldn't read data from file `",
				      opt_link_data, "': ",
				      System.strerror (System.errno), "\n");
		      else
			{
			  System.print (program, ": linking ", data.length,
					" bytes of data to section ",
					opt_link_type.toString (), "\n");
			  bc.linkSection (opt_link_type, data);
			  write_back = true;
			}
		    }
		  else
		    System.error (program, ": couldn't open data file `",
				  opt_link_data, "': ",
				  System.strerror (System.errno), "\n");
		}

	      if (opt_remove)
		{
		  System.print (program, ": removing section ",
				opt_remove_type, "\n");
		  if (bc.removeSection (opt_remove_type))
		    write_back = true;
		}

	      if (write_back)
		{
		  /* Write the byte-code file back to its original file. */
		  if (!bc.write (ARGS[i]))
		    {
		      System.error (program, ": write failed: ",
				    System.strerror (System.errno),
				    "\n");
		      System.exit (1);
		    }
		}
	    }
	}
      else
	System.error (program, ": couldn't open bc file `", ARGS[i], "': ",
		      System.strerror (System.errno), "\n");
    }
}


function usage ()
{
  System.print ("\
Usage: ", program, " [OPTION]... FILE...\n\
Mandatory arguments to long options are mandatory for short options too.\n");

  System.print ("\
  -c, --code                 print code section (default)\n\
  -C, --constants            print constants section\n\
  -d, --debug                print debug section\n\
  -h, --help                 print this help and exit\n\
  -i, --info                 print the byte-code file header and general\n\
                             information about the sections\n\
  -l, --link TYPE DATA       link data from file DATA to section TYPE\n\
  -r, --remove TYPE          remove section TYPE\n\
  -s, --symtab               print symtab section\n\
  -S, --strip                remove debug section\n\
  -V, --version              print version number\n\
");

  System.print ("\nReport bugs to mtr@ngs.fi.\n");
}


function version ()
{
  System.print ("NGS JavaScript disassembler ", version_number, "\n");
  System.print ("\
Copyright (C) 1998 New Generation Software (NGS) Oy.\n\
NGS JavaScript Interpreter comes with NO WARRANTY, to the extent\n\
permitted by law.  You may redistribute copies of NGS JavaScript\n\
Interpreter under the terms of the GNU Library General Public License.\n\
For more information about these matters, see the files named COPYING.\n\
");
}


main ();


/*
Local variables:
mode: c
End:
*/
