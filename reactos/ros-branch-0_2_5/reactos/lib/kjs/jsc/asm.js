/*
 * JavaScript Assembler.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/asm.js,v $
 * $Id: asm.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/* Byte-code file definitions. */

JSC$BC_MAGIC 		= 0xc0014a53;

JSC$BC_SECT_CODE 	= 0;
JSC$BC_SECT_CONSTANTS	= 1;
JSC$BC_SECT_SYMTAB 	= 2;
JSC$BC_SECT_DEBUG	= 3;

JSC$CONST_INT		= 3;
JSC$CONST_STRING	= 4;
JSC$CONST_FLOAT		= 5;
JSC$CONST_SYMBOL	= 10;
JSC$CONST_REGEXP	= 11;
JSC$CONST_NAN		= 13;

JSC$CONST_REGEXP_FLAG_G	= 0x01;
JSC$CONST_REGEXP_FLAG_I	= 0x02;

JSC$DEBUG_FILENAME	= 1;
JSC$DEBUG_LINENUMBER	= 2;

/* Opcode definitions. */

JSC$OP_HALT		= 0;
JSC$OP_DONE		= 1;
JSC$OP_NOP		= 2;
JSC$OP_DUP		= 3;
JSC$OP_POP		= 4;
JSC$OP_POP_N		= 5;
JSC$OP_APOP		= 6;
JSC$OP_SWAP		= 7;
JSC$OP_ROLL		= 8;
JSC$OP_CONST		= 9;
JSC$OP_CONST_NULL	= 10;
JSC$OP_CONST_TRUE	= 11;
JSC$OP_CONST_FALSE	= 12;
JSC$OP_CONST_UNDEFINED	= 13;
JSC$OP_CONST_I0		= 14;
JSC$OP_CONST_I1		= 15;
JSC$OP_CONST_I2		= 16;
JSC$OP_CONST_I3		= 17;
JSC$OP_CONST_I		= 18;
JSC$OP_LOAD_GLOBAL	= 19;
JSC$OP_STORE_GLOBAL	= 20;
JSC$OP_LOAD_ARG		= 21;
JSC$OP_STORE_ARG	= 22;
JSC$OP_LOAD_LOCAL	= 23;
JSC$OP_STORE_LOCAL	= 24;
JSC$OP_LOAD_PROPERTY	= 25;
JSC$OP_STORE_PROPERTY	= 26;
JSC$OP_LOAD_ARRAY	= 27;
JSC$OP_STORE_ARRAY	= 28;
JSC$OP_NTH		= 29;
JSC$OP_CMP_EQ		= 30;
JSC$OP_CMP_NE		= 31;
JSC$OP_CMP_LT		= 32;
JSC$OP_CMP_GT		= 33;
JSC$OP_CMP_LE		= 34;
JSC$OP_CMP_GE		= 35;
JSC$OP_CMP_SEQ		= 36;
JSC$OP_CMP_SNE		= 37;
JSC$OP_SUB		= 38;
JSC$OP_ADD		= 39;
JSC$OP_MUL		= 40;
JSC$OP_DIV		= 41;
JSC$OP_MOD		= 42;
JSC$OP_NEG		= 43;
JSC$OP_AND		= 44;
JSC$OP_NOT		= 45;
JSC$OP_OR		= 46;
JSC$OP_XOR		= 47;
JSC$OP_SHIFT_LEFT	= 48;
JSC$OP_SHIFT_RIGHT	= 49;
JSC$OP_SHIFT_RRIGHT	= 50;
JSC$OP_IFFALSE		= 51;
JSC$OP_IFTRUE		= 52;
JSC$OP_CALL_METHOD	= 53;
JSC$OP_JMP		= 54;
JSC$OP_JSR		= 55;
JSC$OP_RETURN		= 56;
JSC$OP_TYPEOF		= 57;
JSC$OP_NEW		= 58;
JSC$OP_DELETE_PROPERTY	= 59;
JSC$OP_DELETE_ARRAY	= 60;
JSC$OP_LOCALS		= 61;
JSC$OP_MIN_ARGS		= 62;
JSC$OP_LOAD_NTH_ARG	= 63;
JSC$OP_WITH_PUSH	= 64;
JSC$OP_WITH_POP		= 65;
JSC$OP_TRY_PUSH		= 66;
JSC$OP_TRY_POP		= 67;
JSC$OP_THROW		= 68;

/* Type aware operands. */
JSC$OP_IFFALSE_B	= 69;
JSC$OP_IFTRUE_B		= 70;
JSC$OP_ADD_1_I		= 71;
JSC$OP_ADD_2_I		= 72;
JSC$OP_LOAD_GLOBAL_W	= 73;
JSC$OP_JSR_W		= 74;

/* Internal values. */
JSC$ASM_SYMBOL		= 1000;
JSC$ASM_LABEL		= 1001;

/*
 * General helpers.
 */

/* Generate byte-code for operands with Int8 value. */
function JSC$ASM_bytecode_int8 ()
{
  return String.pack ("C", this.value);
}

/* Generate byte-code for operands with Int16 value. */
function JSC$ASM_bytecode_int16 ()
{
  return String.pack ("n", this.value);
}

/* Generate byte-code for operands with Int32 value. */
function JSC$ASM_bytecode_int32 ()
{
  return String.pack ("N", this.value);
}

/* Generate byte-code for operands with Symbol value. */
function JSC$ASM_bytecode_symbol ()
{
  var cid = JSC$asm_genconstant (String.pack ("C", JSC$CONST_SYMBOL)
				 + this.value + String.pack ("C", 0));
  return String.pack ("N", cid);
}

/* Generate byte-code for local jump operands. */
function JSC$ASM_bytecode_local_jump ()
{
  var delta = this.value.offset - (this.offset + this.size);
  return String.pack ("N", delta);
}


/*
 * Assembler operands.
 */

/* Symbol. */

function JSC$ASM_symbol (ln, value)
{
  this.type = JSC$ASM_SYMBOL;
  this.linenum = ln;
  this.value = value;
  this.size = 0;
  this.print = JSC$ASM_symbol_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_symbol_print (stream)
{
  stream.write ("\n" + this.value + ":\n");
}

/* Label */

function JSC$ASM_label ()
{
  this.type = JSC$ASM_LABEL;
  this.linenum = 0;
  this.size = 0;
  this.value = JSC$asm_label_count++;
  this.referenced = false;
  this.next = null;
  this.print = JSC$ASM_label_print;
  this.link = JSC$asm_link;
  this.format = JSC$ASM_label_format;
}

function JSC$ASM_label_print (stream)
{
  stream.write (this.format () + ":\n");
}

function JSC$ASM_label_format ()
{
  return ".L" + this.value.toString ();
}

/* halt */

function JSC$ASM_halt (ln)
{
  this.type = JSC$OP_HALT;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_halt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_halt_print (stream)
{
  stream.write ("\thalt\n");
}

/* done */

function JSC$ASM_done (ln)
{
  this.type = JSC$OP_DONE;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_done_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_done_print (stream)
{
  stream.write ("\tdone\n");
}

/* nop */

function JSC$ASM_nop (ln)
{
  this.type = JSC$OP_NOP;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_nop_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_nop_print (stream)
{
  stream.write ("\tnop\n");
}

/* dup */

function JSC$ASM_dup (ln)
{
  this.type = JSC$OP_DUP;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_dup_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_dup_print (stream)
{
  stream.write ("\tdup\n");
}

/* pop */

function JSC$ASM_pop (ln)
{
  this.type = JSC$OP_POP;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_pop_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_pop_print (stream)
{
  stream.write ("\tpop\n");
}

/* pop_n */

function JSC$ASM_pop_n (ln, value)
{
  this.type = JSC$OP_POP_N;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -value;
  this.size = 2;
  this.print = JSC$ASM_pop_n_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_pop_n_print (stream)
{
  stream.write ("\tpop_n\t\t" + this.value.toString () + "\n");
}

/* apop */

function JSC$ASM_apop (ln, value)
{
  this.type = JSC$OP_APOP;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -value;
  this.size = 2;
  this.print = JSC$ASM_apop_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_apop_print (stream)
{
  stream.write ("\tapop\t\t" + this.value.toString () + "\n");
}

/* swap */

function JSC$ASM_swap (ln)
{
  this.type = JSC$OP_SWAP;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_swap_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_swap_print (stream)
{
  stream.write ("\tswap\n");
}

/* roll */
function JSC$ASM_roll (ln, value)
{
  this.type = JSC$OP_ROLL;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_roll_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_roll_print (stream)
{
  stream.write ("\troll\t\t" + this.value.toString () + "\n");
}

/* const */

function JSC$ASM_const (ln, value)
{
  this.type = JSC$OP_CONST;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_const_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_const_bytecode;
}

function JSC$ASM_const_print (stream)
{
  if (typeof this.value == "number")
    stream.write ("\tconst\t\t" + this.value.toString () + "\n");
  else if (typeof this.value == "string"
	   || typeof this.value == "#builtin")
    {
      var i, c;
      var ender, src;
      var stringp = (typeof this.value == "string");

      if (stringp)
	{
	  ender = "\"";
	  src = this.value;
	}
      else
	{
	  ender = "/";
	  src = this.value.source;
	}

      stream.write ("\tconst\t\t" + ender);
      for (i = 0; i < src.length; i++)
	{
	  c = src.charCodeAt (i);
	  if (c == ender[0] || c == #'\\')
	    stream.write ("\\" + src.charAt (i));
	  else if (c == #'\n')
	    stream.write ("\\n");
	  else if (c == #'\r')
	    stream.write ("\\r");
	  else if (c == #'\t')
	    stream.write ("\\t");
	  else if (c == #'\f')
	    stream.write ("\\f");
	  else
	    stream.write (src.charAt (i));
	}
      stream.write (ender);

      if (!stringp)
	{
	  if (this.value.global)
	    stream.write ("g");
	  if (this.value.ignoreCase)
	    stream.write ("i");
	}

      stream.write ("\n");
    }
}

function JSC$ASM_const_bytecode ()
{
  var cid;

  if (typeof this.value == "number")
    {
      if (isInt (this.value))
	cid = JSC$asm_genconstant (String.pack ("CN", JSC$CONST_INT,
						this.value));
      else if (isFloat (this.value))
	cid = JSC$asm_genconstant (String.pack ("Cd", JSC$CONST_FLOAT,
						this.value));
      else
	cid = JSC$asm_genconstant (String.pack ("C", JSC$CONST_NAN));
    }
  else if (typeof this.value == "string")
    cid = JSC$asm_genconstant (String.pack ("CN", JSC$CONST_STRING,
					    this.value.length)
			       + this.value);
  else if (typeof this.value == "#builtin")
    {
      /* Regular expression. */
      var flags = 0;

      if (this.value.global)
	flags |= JSC$CONST_REGEXP_FLAG_G;
      if (this.value.ignoreCase)
	flags |= JSC$CONST_REGEXP_FLAG_I;

      cid = JSC$asm_genconstant (String.pack ("CCN", JSC$CONST_REGEXP, flags,
					      this.value.source.length)
				 + this.value.source);
    }
  else
    error ("ASM_const_bytecode(): unknown type: " + typeof this.value);

  return String.pack ("N", cid);
}

/* const_null */

function JSC$ASM_const_null (ln)
{
  this.type = JSC$OP_CONST_NULL;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_null_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_null_print (stream)
{
  stream.write ("\tconst_null\n");
}

/* const_true */

function JSC$ASM_const_true (ln)
{
  this.type = JSC$OP_CONST_TRUE;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_true_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_true_print (stream)
{
  stream.write ("\tconst_true\n");
}

/* const_false */

function JSC$ASM_const_false (ln)
{
  this.type = JSC$OP_CONST_FALSE;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_false_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_false_print (stream)
{
  stream.write ("\tconst_false\n");
}

/* const_undefined */

function JSC$ASM_const_undefined (ln)
{
  this.type = JSC$OP_CONST_UNDEFINED;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_undefined_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_undefined_print (stream)
{
  stream.write ("\tconst_undefined\n");
}

/* const_i0 */

function JSC$ASM_const_i0 (ln)
{
  this.type = JSC$OP_CONST_I0;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i0_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i0_print (stream)
{
  stream.write ("\tconst_i0\n");
}

/* const_i1 */

function JSC$ASM_const_i1 (ln)
{
  this.type = JSC$OP_CONST_I1;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i1_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i1_print (stream)
{
  stream.write ("\tconst_i1\n");
}

/* const_i2 */

function JSC$ASM_const_i2 (ln)
{
  this.type = JSC$OP_CONST_I2;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i2_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i2_print (stream)
{
  stream.write ("\tconst_i2\n");
}

/* const_i3 */

function JSC$ASM_const_i3 (ln)
{
  this.type = JSC$OP_CONST_I3;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i3_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i3_print (stream)
{
  stream.write ("\tconst_i3\n");
}

/* const_i */

function JSC$ASM_const_i (ln, value)
{
  this.type = JSC$OP_CONST_I;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_const_i_print;
  this.bytecode = JSC$ASM_bytecode_int32;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i_print (stream)
{
  stream.write ("\tconst_i\t\t" + this.value.toString () + "\n");
}

/* load_global */

function JSC$ASM_load_global (ln, value)
{
  this.type = JSC$OP_LOAD_GLOBAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_load_global_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_global_print (stream)
{
  stream.write ("\tload_global\t" + this.value + "\n");
}

/* store_global */

function JSC$ASM_store_global (ln, value)
{
  this.type = JSC$OP_STORE_GLOBAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_store_global_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_global_print (stream)
{
  stream.write ("\tstore_global\t" + this.value + "\n");
}

/* load_arg */

function JSC$ASM_load_arg (ln, value)
{
  this.type = JSC$OP_LOAD_ARG;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 2;
  this.print = JSC$ASM_load_arg_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_arg_print (stream)
{
  stream.write ("\tload_arg\t" + this.value.toString () + "\n");
}

/* store_arg */

function JSC$ASM_store_arg (ln, value)
{
  this.type = JSC$OP_STORE_ARG;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 2;
  this.print = JSC$ASM_store_arg_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_arg_print (stream)
{
  stream.write ("\tstore_arg\t" + this.value.toString () + "\n");
}

/* load_local */

function JSC$ASM_load_local (ln, value)
{
  this.type = JSC$OP_LOAD_LOCAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 3;
  this.print = JSC$ASM_load_local_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_local_print (stream)
{
  stream.write ("\tload_local\t" + this.value.toString () + "\n");
}

/* store_local */

function JSC$ASM_store_local (ln, value)
{
  this.type = JSC$OP_STORE_LOCAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 3;
  this.print = JSC$ASM_store_local_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_local_print (stream)
{
  stream.write ("\tstore_local\t" + this.value.toString () + "\n");
}

/* load_property */

function JSC$ASM_load_property (ln, value)
{
  this.type = JSC$OP_LOAD_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_load_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_property_print (stream)
{
  stream.write ("\tload_property\t" + this.value + "\n");
}

/* store_property */

function JSC$ASM_store_property (ln, value)
{
  this.type = JSC$OP_STORE_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -2;
  this.size = 5;
  this.print = JSC$ASM_store_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_property_print (stream)
{
  stream.write ("\tstore_property\t" + this.value + "\n");
}

/* load_array */

function JSC$ASM_load_array (ln)
{
  this.type = JSC$OP_LOAD_ARRAY;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_load_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_array_print (stream)
{
  stream.write ("\tload_array\n");
}

/* store_array */

function JSC$ASM_store_array (ln)
{
  this.type = JSC$OP_STORE_ARRAY;
  this.linenum = ln;
  this.stack_delta = -3;
  this.size = 1;
  this.print = JSC$ASM_store_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_array_print (stream)
{
  stream.write ("\tstore_array\n");
}

/* nth */

function JSC$ASM_nth (ln)
{
  this.type = JSC$OP_NTH;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_nth_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_nth_print (stream)
{
  stream.write ("\tnth\n");
}

/* cmp_eq */

function JSC$ASM_cmp_eq (ln)
{
  this.type = JSC$OP_CMP_EQ;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_eq_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_eq_print (stream)
{
  stream.write ("\tcmp_eq\n");
}

/* cmp_ne */

function JSC$ASM_cmp_ne (ln)
{
  this.type = JSC$OP_CMP_NE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_ne_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_ne_print (stream)
{
  stream.write ("\tcmp_ne\n");
}

/* cmp_lt */

function JSC$ASM_cmp_lt (ln)
{
  this.type = JSC$OP_CMP_LT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_lt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_lt_print (stream)
{
  stream.write ("\tcmp_lt\n");
}

/* cmp_gt */

function JSC$ASM_cmp_gt (ln)
{
  this.type = JSC$OP_CMP_GT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_gt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_gt_print (stream)
{
  stream.write ("\tcmp_gt\n");
}

/* cmp_le */

function JSC$ASM_cmp_le (ln)
{
  this.type = JSC$OP_CMP_LE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_le_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_le_print (stream)
{
  stream.write ("\tcmp_le\n");
}

/* cmp_ge */

function JSC$ASM_cmp_ge (ln)
{
  this.type = JSC$OP_CMP_GE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_ge_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_ge_print (stream)
{
  stream.write ("\tcmp_ge\n");
}

/* cmp_seq */

function JSC$ASM_cmp_seq (ln)
{
  this.type = JSC$OP_CMP_SEQ;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_seq_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_seq_print (stream)
{
  stream.write ("\tcmp_seq\n");
}

/* cmp_sne */

function JSC$ASM_cmp_sne (ln)
{
  this.type = JSC$OP_CMP_SNE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_sne_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_sne_print (stream)
{
  stream.write ("\tcmp_sne\n");
}

/* sub */

function JSC$ASM_sub (ln)
{
  this.type = JSC$OP_SUB;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_sub_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_sub_print (stream)
{
  stream.write ("\tsub\n");
}

/* add */

function JSC$ASM_add (ln)
{
  this.type = JSC$OP_ADD;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_add_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_print (stream)
{
  stream.write ("\tadd\n");
}

/* mul */

function JSC$ASM_mul (ln)
{
  this.type = JSC$OP_MUL;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_mul_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_mul_print (stream)
{
  stream.write ("\tmul\n");
}

/* div */

function JSC$ASM_div (ln)
{
  this.type = JSC$OP_DIV;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_div_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_div_print (stream)
{
  stream.write ("\tdiv\n");
}

/* mod */

function JSC$ASM_mod (ln)
{
  this.type = JSC$OP_MOD;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_mod_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_mod_print (stream)
{
  stream.write ("\tmod\n");
}

/* neg */

function JSC$ASM_neg (ln)
{
  this.type = JSC$OP_NEG;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_neg_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_neg_print (stream)
{
  stream.write ("\tneg\n");
}

/* and */

function JSC$ASM_and (ln)
{
  this.type = JSC$OP_AND;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_and_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_and_print (stream)
{
  stream.write ("\tand\n");
}

/* not */

function JSC$ASM_not (ln)
{
  this.type = JSC$OP_NOT;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_not_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_not_print (stream)
{
  stream.write ("\tnot\n");
}

/* or */

function JSC$ASM_or (ln)
{
  this.type = JSC$OP_OR;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_or_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_or_print (stream)
{
  stream.write ("\tor\n");
}

/* xor */

function JSC$ASM_xor (ln)
{
  this.type = JSC$OP_XOR;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_xor_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_xor_print (stream)
{
  stream.write ("\txor\n");
}

/* shift_left */

function JSC$ASM_shift_left (ln)
{
  this.type = JSC$OP_SHIFT_LEFT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_left_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_left_print (stream)
{
  stream.write ("\tshift_left\n");
}

/* shift_right */

function JSC$ASM_shift_right (ln)
{
  this.type = JSC$OP_SHIFT_RIGHT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_right_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_right_print (stream)
{
  stream.write ("\tshift_right\n");
}

/* shift_rright */

function JSC$ASM_shift_rright (ln)
{
  this.type = JSC$OP_SHIFT_RRIGHT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_rright_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_rright_print (stream)
{
  stream.write ("\tshift_rright\n");
}

/* iffalse */

function JSC$ASM_iffalse (ln, value)
{
  this.type = JSC$OP_IFFALSE;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iffalse_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iffalse_print (stream)
{
  stream.write ("\tiffalse\t\t" + this.value.format () + "\n");
}

/* iftrue */

function JSC$ASM_iftrue (ln, value)
{
  this.type = JSC$OP_IFTRUE;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iftrue_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iftrue_print (stream)
{
  stream.write ("\tiftrue\t\t" + this.value.format () + "\n");
}

/* call_method */

function JSC$ASM_call_method (ln, value)
{
  this.type = JSC$OP_CALL_METHOD;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_call_method_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_call_method_print (stream)
{
  stream.write ("\tcall_method\t" + this.value + "\n");
}

/* jmp */

function JSC$ASM_jmp (ln, value)
{
  this.type = JSC$OP_JMP;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_jmp_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_jmp_print (stream)
{
  stream.write ("\tjmp\t\t" + this.value.format () + "\n");
}

/* jsr */

function JSC$ASM_jsr (ln)
{
  this.type = JSC$OP_JSR;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_jsr_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_jsr_print (stream)
{
  stream.write ("\tjsr\n");
}

/* return */

function JSC$ASM_return (ln)
{
  this.type = JSC$OP_RETURN;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_return_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_return_print (stream)
{
  stream.write ("\treturn\n");
}

/* typeof */

function JSC$ASM_typeof (ln)
{
  this.type = JSC$OP_TYPEOF;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_typeof_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_typeof_print (stream)
{
  stream.write ("\ttypeof\n");
}

/* new */

function JSC$ASM_new (ln)
{
  this.type = JSC$OP_NEW;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_new_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_new_print (stream)
{
  stream.write ("\tnew\n");
}

/* delete_property */

function JSC$ASM_delete_property (ln, value)
{
  this.type = JSC$OP_DELETE_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_delete_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_delete_property_print (stream)
{
  stream.write ("\tdelete_property\t" + this.value + "\n");
}

/* delete_array */

function JSC$ASM_delete_array (ln)
{
  this.type = JSC$OP_DELETE_ARRAY;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_delete_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_delete_array_print (stream)
{
  stream.write ("\tdelete_array\n");
}

/* locals */

function JSC$ASM_locals (ln, value)
{
  this.type = JSC$OP_LOCALS;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = value;
  this.size = 3;
  this.print = JSC$ASM_locals_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_locals_print (stream)
{
  stream.write ("\tlocals\t\t" + this.value.toString () + "\n");
}

/* min_args */

function JSC$ASM_min_args (ln, value)
{
  this.type = JSC$OP_MIN_ARGS;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 2;
  this.print = JSC$ASM_min_args_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_min_args_print (stream)
{
  stream.write ("\tmin_args\t" + this.value.toString () + "\n");
}

/* load_nth_arg */

function JSC$ASM_load_nth_arg (ln)
{
  this.type = JSC$OP_LOAD_NTH_ARG;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_load_nth_arg_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_nth_arg_print (stream)
{
  stream.write ("\tload_nth_arg\n");
}

/* with_push */

function JSC$ASM_with_push (ln)
{
  this.type = JSC$OP_WITH_PUSH;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_with_push_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_with_push_print (stream)
{
  stream.write ("\twith_push\n");
}

/* with_pop */

function JSC$ASM_with_pop (ln, value)
{
  this.type = JSC$OP_WITH_POP;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_with_pop_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_with_pop_print (stream)
{
  stream.write ("\twith_pop\t" + this.value.toString () + "\n");
}

/* try_push */

function JSC$ASM_try_push (ln, value)
{
  this.type = JSC$OP_TRY_PUSH;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_try_push_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_try_push_print (stream)
{
  stream.write ("\ttry_push\t" + this.value.format () + "\n");
}

/* try_pop */

function JSC$ASM_try_pop (ln, value)
{
  this.type = JSC$OP_TRY_POP;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_try_pop_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_try_pop_print (stream)
{
  stream.write ("\ttry_pop\t\t" + this.value.toString () + "\n");
}

/* throw */

function JSC$ASM_throw (ln)
{
  this.type = JSC$OP_THROW;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_throw_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_throw_print (stream)
{
  stream.write ("\tthrow\n");
}

/* iffalse_b */

function JSC$ASM_iffalse_b (ln, value)
{
  this.type = JSC$OP_IFFALSE_B;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iffalse_b_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iffalse_b_print (stream)
{
  stream.write ("\tiffalse_b\t" + this.value.format () + "\n");
}

/* iftrue */

function JSC$ASM_iftrue_b (ln, value)
{
  this.type = JSC$OP_IFTRUE_B;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iftrue_b_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iftrue_b_print (stream)
{
  stream.write ("\tiftrue_b\t" + this.value.format () + "\n");
}

/* add_1_i */

function JSC$ASM_add_1_i (ln)
{
  this.type = JSC$OP_ADD_1_I;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_add_1_i_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_1_i_print (stream)
{
  stream.write ("\tadd_1_i\n");
}

/* add_2_i */

function JSC$ASM_add_2_i (ln)
{
  this.type = JSC$OP_ADD_2_I;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_add_2_i_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_2_i_print (stream)
{
  stream.write ("\tadd_2_i\n");
}

/* load_global_w */

function JSC$ASM_load_global_w (ln, value)
{
  this.type = JSC$OP_LOAD_GLOBAL_W;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_load_global_w_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_global_w_print (stream)
{
  stream.write ("\tload_global_w\t" + this.value + "\n");
}

/* jsr_w */

function JSC$ASM_jsr_w (ln, value)
{
  this.type = JSC$OP_JSR_W;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_jsr_w_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_jsr_w_print (stream)
{
  stream.write ("\tjsr_w\t\t" + this.value + "\n");
}

/*
 * General helpers.
 */

function JSC$asm_link ()
{
  this.next = null;

  if (JSC$asm_tail != null)
    {
      JSC$asm_tail_prev = JSC$asm_tail;
      JSC$asm_tail.next = this;
    }
  else
    JSC$asm_head = this;

  JSC$asm_tail = this;
}


/*
 * The phases of the assembler.
 */

/* This is called from the compiler initialization code. */
function JSC$asm_reset ()
{
  JSC$asm_label_count = 1;
  JSC$asm_head = JSC$asm_tail = JSC$asm_tail_prev = null;
  JSC$asm_constcount = 0;
  JSC$asm_constants = null;
  JSC$asm_known_constants = null;
}


function JSC$asm_generate ()
{
  var i;

  if (JSC$verbose)
    JSC$message ("jsc: generating assembler");

  JSC$ns = new JSC$NameSpace ();

  /* Functions. */
  for (i = 0; i < JSC$functions.length; i++)
    JSC$functions[i].asm ();

  /* Global statements. */
  if (JSC$global_stmts.length > 0)
    {
      /* Define the `.global' symbol. */
      new JSC$ASM_symbol (JSC$global_stmts[0].linenum, ".global").link ();

      /* Handle local variables. */
      var num_locals = JSC$count_locals_from_stmt_list (JSC$global_stmts);
      if (num_locals > 0)
	new JSC$ASM_locals (JSC$global_stmts[0].linenum, num_locals).link ();

      /* Generate assembler. */
      for (i = 0; i < JSC$global_stmts.length; i++)
	JSC$global_stmts[i].asm ();

      /*
       * Fix things so that also the global statement returns something
       * (this is required when we use eval() in JavaScript).
       */
      if (JSC$asm_tail_prev == null)
	{
	  /* This is probably illegal, but we don't panic. */
	  new JSC$ASM_const_undefined (0).link ();
	}
      else
	{
	  /*
	   * If the latest op is `pop', remove it.  Otherwise, append
	   * a `const_undefined'.
	   */
	  if (JSC$asm_tail.type == JSC$OP_POP)
	    {
	      JSC$asm_tail = JSC$asm_tail_prev;
	      JSC$asm_tail.next = null;
	      JSC$asm_tail_prev = null;
	    }
	  else
	    new JSC$ASM_const_undefined (JSC$asm_tail.linenum).link ();
	}
    }

  JSC$ns = null;
}


function JSC$asm_print (src_stream, stream)
{
  var i;
  var last_ln;
  var annotate = src_stream ? true : false;

  if (annotate)
    {
      stream.write ("; -*- asm -*-\n");

      /* Set the prev properties. */
      var prev = null;
      for (i = JSC$asm_head; i != null; prev = i, i = i.next)
	i.prev = prev;

      /*
       * Fix the label line numbers to be the same that the next
       * assembler operand has.
       */
      last_ln = 0;
      for (i = JSC$asm_tail; i != null; i = i.prev)
	{
	  if (i.type == JSC$ASM_LABEL)
	    i.linenum = last_ln;
	  else if (typeof i.linenum != "undefined")
	    last_ln = i.linenum;
	}
    }

  last_ln = 0;
  for (i = JSC$asm_head; i != null; i = i.next)
    {
      if (typeof i.linenum == "undefined")
	{
	  if (annotate)
	    stream.write ("; undefined linenum\n");
	}
      else
	while (annotate && i.linenum > last_ln)
	  {
	    var line = src_stream.readln ();
	    stream.write ("; " + line + "\n");
	    last_ln++;
	  }

      i.print (stream);
    }
}


function JSC$asm_is_load_op (op)
{
  return (op.type == JSC$OP_LOAD_GLOBAL
	  || op.type == JSC$OP_LOAD_ARG
	  || op.type == JSC$OP_LOAD_LOCAL);
}


function JSC$asm_is_store_op (op)
{
  return (op.type == JSC$OP_STORE_GLOBAL
	  || op.type == JSC$OP_STORE_ARG
	  || op.type == JSC$OP_STORE_LOCAL);
}


function JSC$asm_is_local_jump (op)
{
  return (op.type == JSC$OP_JMP
	  || op.type == JSC$OP_IFFALSE
	  || op.type == JSC$OP_IFTRUE
	  || op.type == JSC$OP_IFFALSE_B
	  || op.type == JSC$OP_IFTRUE_B
	  || op.type == JSC$OP_TRY_PUSH);
}


function JSC$asm_is_const_op (op)
{
  return (JSC$OP_CONST <= op.type && op.type <= JSC$OP_CONST_I3);
}


function JSC$asm_lookup_next_op (item)
{
  while (item != null &&
	 (item.type == JSC$ASM_LABEL || item.type == JSC$ASM_SYMBOL))
    item = item.next;

  return item;
}


function JSC$asm_optimize (flags)
{
  var item;

  /* Simple peephole optimization. */
  if ((flags & JSC$FLAG_OPTIMIZE_PEEPHOLE) != 0)
    {
      if (JSC$verbose)
	JSC$message ("jsc: optimize: peephole");

      for (item = JSC$asm_head; item != null; item = item.next)
	{
	  /*
	   * Optimization for dup ... pop cases where pop removes the
	   * item duplicated by dup.
	   */
	  if (item.next != null && item.next.type == JSC$OP_DUP)
	    {
	      var balance = 2;
	      var found = false;
	      var i1;

	      for (i1 = item.next.next;
		   i1 != null && i1.next != null;
		   i1 = i1.next)
		{
		  var i2 = i1.next;

		  /*
		   * The lookup ends on branches, and on dup, throw,
		   * and try_pop operands.  We optimize on a basic
		   * block and we match the closest dup-pop pairs.
		   */
		  if (JSC$asm_is_local_jump (i1)
		      || i1.type == JSC$OP_JSR
		      || i1.type == JSC$OP_NEW
		      || i1.type == JSC$OP_CALL_METHOD
		      || i1.type == JSC$OP_RETURN
		      || i1.type == JSC$ASM_SYMBOL
		      || i1.type == JSC$ASM_LABEL
		      || i1.type == JSC$OP_DUP
		      || i1.type == JSC$OP_TRY_POP
		      || i1.type == JSC$OP_THROW)
		    break;

		  if (i1.stack_delta)
		    {
		      balance += i1.stack_delta;
		      if (balance <= 0)
			/* Going to negative.  Stop here. */
			break;
		    }

		  if (i2.type == JSC$OP_POP && balance == 1)
		    {
		      /* Found a matching pop. */
		      found = true;
		      i1.next = i2.next;
		      break;
		    }
		}

	      if (found)
		{
		  /* The dup can be removed. */
		  item.next = item.next.next;
		}
	    }

	  /* Two instruction optimization (starting from item.next). */
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_APOP
		  && i2.type == JSC$OP_POP)
		{
		  /*
		   * i1:	apop n
		   * i2:	pop		->	pop_n n + 1
		   */
		  var i = new JSC$ASM_pop_n (i1.linenum, i1.value + 1);
		  item.next = i;
		  i.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_CONST_TRUE
		  && (i2.type == JSC$OP_IFFALSE
		      || i2.type == JSC$OP_IFFALSE_B))
		{
		  /*
		   * i1:	const_true
		   * i2:	iffalse{,_b}	.LX	=> ---
		   */
		  item.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_CONST_FALSE
		  && (i2.type == JSC$OP_IFTRUE
		      || i2.type == JSC$OP_IFTRUE_B))
		{
		  /*
		   * i1:	const_false
		   * i2:	iftrue{,_b}	.LX	=> ---
		   */
		  item.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if ((i1.type == JSC$OP_CONST_FALSE
		   && (i2.type == JSC$OP_IFFALSE
		       || i2.type == JSC$OP_IFFALSE_B))
		  || (i1.type == JSC$OP_CONST_TRUE
		      && (i2.type == JSC$OP_IFTRUE
			  || i2.type == JSC$OP_IFTRUE_B)))
		{
		  /*
		   * i1:	const_false
		   * i2:	iffalse{,_b}	.LX	=> jmp .LX
		   *
		   * i1:	const_true
		   * i2:	iftrue{,_b}	.LX	=> jmp .LX
		   */
		  var i = new JSC$ASM_jmp (i1.linenum, i2.value);
		  item.next = i;
		  i.next = i2.next;
		}
	    }
	}
    }

  /* Jumps to jumps. */
  if ((flags & JSC$FLAG_OPTIMIZE_JUMPS) != 0)
    {
      if (JSC$verbose)
	JSC$message ("jsc: optimize: jumps to jumps");
      for (item = JSC$asm_head; item != null; item = item.next)
	if (JSC$asm_is_local_jump (item))
	  {
	    var i2;

	    /* Operand's value is a label */
	    i2 = JSC$asm_lookup_next_op (item.value);

	    if (i2 != null && i2.type == JSC$OP_JMP)
	      /* Ok, we can jump there directly. */
	      item.value = i2.value;
	  }
    }

  if ((flags & JSC$FLAG_OPTIMIZE_HEAVY) != 0)
    JSC$optimize_heavy ();

  /*
   * Optimizations for the size of the generated byte-code.  It isn't
   * probably worth of doing these optimization for interactive
   * scripts since these won't affect the speed of the execution.
   * However, these optimizations make the byte-code files smaller so
   * these are nice for batch-compiled files.
   */
  if ((flags & JSC$FLAG_OPTIMIZE_BC_SIZE) != 0)
    {
      var delta = true;

      while (delta)
	{
	  delta = false;

	  /* Remove un-referenced labels. */

	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: removing un-referenced labels");

	  /* First, make all labels unreferenced. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (item.type == JSC$ASM_LABEL)
	      item.referenced = false;

	  /* Second, mark all referenced labels. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (JSC$asm_is_local_jump (item))
	      item.value.referenced = true;

	  /* Third, remove all un-referenced labels. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    while (item.next != null && item.next.type == JSC$ASM_LABEL
		   && !item.next.referenced
		   && item.next.next != null)
	      {
		delta = true;
		item.next = item.next.next;
	      }

	  /* Dead code elimination. */
	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: dead code elimination");
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (item.type == JSC$OP_RETURN || item.type == JSC$OP_JMP)
	      while (item.next != null && item.next.type != JSC$ASM_SYMBOL
		     && item.next.type != JSC$ASM_LABEL)
		{
		  delta = true;
		  item.next = item.next.next;
		}


	  /* Simple peephole optimization. */
	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: peephole");
	  for (item = JSC$asm_head; item != null; item = item.next)
	    {
	      /* Two instruction optimization (starting from item.next). */
	      if (item.next != null && item.next.next != null)
		{
		  var i1 = item.next;
		  var i2 = i1.next;

		  if (i1.type == JSC$OP_JMP
		      && i2.type == JSC$ASM_LABEL
		      && i1.value == i2)
		    {
		      /*
		       * i1:	jmp	.LX
		       * i2:	.LX		=> .LX
		       */
		      item.next = i2;
		      delta = true;
		    }
		}
	    }
	}
    }
}


function JSC$optimize_heavy ()
{
  if (JSC$verbose)
    JSC$message ("jsc: optimize: liveness analyzing");

  /* First, set the prev pointers and zero usage flags. */
  var item, prev = null;

  for (item = JSC$asm_head; item != null; prev = item, item = item.next)
    {
      item.prev = prev;
      item.live_args = 0;
      item.live_locals = 0;
      item.live_used = false;
    }

  /* For each function. */
  var ftail, fhead;
  for (ftail = JSC$asm_tail; ftail != null; ftail = fhead.prev)
    {
      var change = true;

      /* While there is change in the liveness. */
      while (change)
	{
	  change = false;

	  for (fhead = ftail;
	       fhead.type != JSC$ASM_SYMBOL;
	       fhead = fhead.prev)
	    {
	      var floc, farg;

	      if (fhead.next != null)
		{
		  floc = fhead.next.live_locals;
		  farg = fhead.next.live_args;
		}
	      else
		floc = farg = 0;

	      if (fhead.type == JSC$OP_LOAD_LOCAL && fhead.value < 32)
		floc |= (1 << fhead.value);

	      if (fhead.type == JSC$OP_STORE_LOCAL && fhead.value < 32)
		floc &= ~(1 << fhead.value);

	      if (fhead.type == JSC$OP_LOAD_ARG && fhead.value < 32)
		farg |= (1 << fhead.value);

	      if (fhead.type == JSC$OP_STORE_ARG && fhead.value < 32)
		farg &= ~(1 << fhead.value);

	      if (JSC$asm_is_local_jump (fhead))
		{
		  floc |= fhead.value.live_locals;
		  fhead.value.live_used = true;
		}

	      if (fhead.live_used && (fhead.live_locals != floc
				      || fhead.live_args != farg))
		change = true;

	      fhead.live_used = false;
	      fhead.live_locals = floc;
	      fhead.live_args = farg;
	    }
	}
    }

  /*
   * When we have the liveness analyzing performed, we can do some
   * fancy optimizations.
   */

  if (JSC$verbose)
    JSC$message ("jsc: optimize: peephole");

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      /* Three instruction optimization. */
      if (item.next != null && item.next.next != null
	  && item.next.next.next != null)
	{
	  var i1 = item.next;
	  var i2 = i1.next;
	  var i3 = i2.next;

	  if (i1.type == JSC$OP_STORE_LOCAL
	      && i2.type == JSC$OP_LOAD_LOCAL
	      && i1.value == i2.value
	      && (i3.live_locals & (1 << i1.value)) == 0)
	    {
	      /*
	       * i1:	store_local 	n
	       * i2:	load_local	n
	       * i3:	nnn (n not live)	=> nnn
	       */

	      item.next = i3;
	    }
	}
    }
}


function JSC$asm_finalize ()
{
  var item;
  var offset = 0;

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      item.offset = offset;
      offset += item.size;
    }
}


function JSC$ConstantReg ()
{
}

function JSC$asm_genconstant (val)
{
  if (JSC$asm_known_constants == null)
    JSC$asm_known_constants = new JSC$ConstantReg ();

  /* Lookup <val> from a list of known constants. */
  var id = JSC$asm_known_constants[val];
  if (typeof id == "number")
    return id;

  /* This is a new constant. */
  JSC$asm_constants.append (val);
  JSC$asm_known_constants[val] = JSC$asm_constcount;

  return JSC$asm_constcount++;
}

function JSC$asm_bytecode ()
{
  var item;
  var symtab = new String ("");
  var nsymtab_entries = 0;
  var code = new String ("");
  var debug = new String ("");
  var debug_last_linenum = 0;

  if (JSC$verbose)
    JSC$message ("jsc: generating byte-code");

  if (JSC$generate_debug_info)
    /* Source file name. */
    debug.append (String.pack ("CN", JSC$DEBUG_FILENAME, JSC$filename.length)
		  + JSC$filename);

  JSC$asm_constants = new String ("");

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      if (item.type == JSC$ASM_SYMBOL)
	{
	  symtab.append (item.value + String.pack ("CN", 0, item.offset));
	  nsymtab_entries++;
	}
      else if (item.type == JSC$ASM_LABEL)
	;
      else
	{
	  /* Real assembler operands. */

	  if (JSC$generate_debug_info)
	    if (item.linenum != debug_last_linenum)
	      {
		debug.append (String.pack ("CNN", JSC$DEBUG_LINENUMBER,
					   item.offset + item.size,
					   item.linenum));
		debug_last_linenum = item.linenum;
	      }

	  if (item.size == 1)
	    /* We handle these. */
	    code.append (String.pack ("C", item.type));
	  else
	    {
	      /*
	       * All operands which take an argument, have a method to create
	       * the byte code entry for their argument.
	       */
	      code.append (String.pack ("C", item.type) + item.bytecode ());
	    }
	}
    }

  symtab = String.pack ("N", nsymtab_entries) + symtab;

  if (JSC$verbose)
    {
      var msg = ("jsc: code=" + code.length.toString ()
		 + ", constants=" + JSC$asm_constants.length.toString ()
		 + ", symtab=" + symtab.length.toString ());

      if (JSC$generate_debug_info)
	msg += ", debug=" + debug.length.toString ();

      msg += (", headers="
	      + (32 + (JSC$generate_debug_info ? 8 : 0)).toString ()
	      + ", total="
	      + (code.length + JSC$asm_constants.length + symtab.length
		 + debug.length + 32
		 + (JSC$generate_debug_info ? 8 : 0)).toString ());
      JSC$message (msg);
    }

  return (String.pack ("NN", JSC$BC_MAGIC,
		       3 + (JSC$generate_debug_info ? 1 : 0))
	  + String.pack ("NN", JSC$BC_SECT_CODE, code.length) + code

	  + String.pack ("NN", JSC$BC_SECT_CONSTANTS,
			 JSC$asm_constants.length)
	  + JSC$asm_constants

	  + String.pack ("NN", JSC$BC_SECT_SYMTAB, symtab.length) + symtab

	  + (JSC$generate_debug_info
	     ? String.pack ("NN", JSC$BC_SECT_DEBUG, debug.length) + debug
	     : ""));
}


/*
Local variables:
mode: c
End:
*/
