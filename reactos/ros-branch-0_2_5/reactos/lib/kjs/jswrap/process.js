/*
 * Process the jswrap input files and generate output.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jswrap/process.js,v $
 * $Id: process.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Constants and definitions.
 */

/* Tokens. */

tEOF 		= 1000;

tFUNCTION 	= 1001;
tIDENTIFIER 	= 1002;

tCSTRING 	= 2001;
tDOUBLE		= 2002;
tINT		= 2003;
tSTRING		= 2004;
tVOID		= 2005;

tIN		= 2010;
tOUT		= 2011;
tINOUT		= 2012;

tSTATIC		= 2020;

function is_type_token (token)
{
  return (token == tCSTRING || token == tDOUBLE || token == tINT
	  || token == tSTRING || token == tVOID);
}

function is_copy_type_token (token)
{
  return (token == tIN || token == tOUT || token == tINOUT);
}

function typename (token)
{
  if (token == tCSTRING)
    return "cstring";
  else if (token == tDOUBLE)
    return "double";
  else if (token == tINT)
    return "int";
  else if (token == tSTRING)
    return "string";
  else if (token == tVOID)
    return "void";
  else if (token == tIN)
    return "in";
  else if (token == tOUT)
    return "out";
  else if (token == tINOUT)
    return "inout";
  else
    return "";
}

function c_typename (token)
{
  if (token == tCSTRING)
    return "char *";
  else if (token == tDOUBLE)
    return "double ";
  else if (token == tINT)
    return "long ";
  else if (token == tVOID)
    return "void ";
  else if (token == tOUT || token == tINOUT)
    return "*";
  else
    return "";
}

function jsint_typename (token)
{
  if (token == tCSTRING)
    return "JS_STRING";
  else if (token == tDOUBLE)
    return "JS_FLOAT";
  else if (token == tINT)
    return "JS_INTEGER";
  else if (token == tSTRING)
    return "JS_STRING";
  else
    return "";
}

valid_c_identifier_re = new RegExp ("^[A-Za-z_][A-Za-z_0-9]*$");

end_brace_re = new RegExp ("^}[ \t]*$");

/*
 * Variables.
 */

/* The input file line number. */
linenum = 1;

token_linenum = 1;

token_value = null;

/*
 * Functions.
 */

function process ()
{
  if (false)
    System.print (program, ": processing file `", input_file,
		  "', header=`", header_file,
		  "', output=`", output_file,
		  "', reentrant=", opt_reentrant, "\n");

  linenum = 1;

  /* Open input file. */
  ifp = new File (input_file);
  if (!ifp.open ("r"))
    {
      System.error (program, ": couldn't open input file `", input_file,
		    "': ", System.strerror (System.errno), "\n");
      System.exit (1);
    }

  /* Create output files. */

  ofp_c = new File (output_file);
  if (!ofp_c.open ("w"))
    {
      System.error (program, ": couldn't create output file `", output_file,
		    "': ", System.strerror (System.errno), "\n");
      System.exit (1);
    }

  ofp_h = new File (header_file);
  if (!ofp_h.open ("w"))
    {
      ofp_c.close ();
      File.remove (output_file);

      System.error (program, ": couldn't create header file `", header_file,
		    "': ", System.strerror (System.errno), "\n");
      System.exit (1);
    }

  if (false)
    {
      ofp_js = new File (js_file);
      if (!ofp_js.open ("w"))
	{
	  ofp_c.close ();
	  ofp_h.close ();
	  File.remove (output_file);
	  File.remove (header_file);

	  System.error (program, ": couldn't create JS file `", js_file,
			"': ", System.strerror (System.errno), "\n");
	  System.exit (1);
	}
    }

  /* Parse the input. */
  parse ();

  /* Cleanup. */

  ofp_c.close ();
  ofp_h.close ();
}


/*
 * The Func class to hold function definitions.
 */

function Func ()
{
}

new Func ();

function Func$generate_js ()
{
  this.code = new String ("function " + this.name + " (");
  var i;
  for (i = 0; i < this.args.length; i++)
    {
      this.code.append (this.args[i].name);
      if (i + 1 < this.args.length)
	this.code.append (", ");
    }
  this.code.append (")\n{");
  this.code.append (this.body);
  this.code.append ("}\n");
}
Func.prototype.generate_js = Func$generate_js;

function Func$print_js (stream)
{
  stream.write (this.code);
}
Func.prototype.print_js = Func$print_js;

function Func$print_h (stream, header)
{
  stream.write ("\n");
  stream.write (c_typename (this.return_type) + this.name + " (");

  if (opt_reentrant)
    {
      /* The first argument is the interpreter handle. */
      stream.write ("\n\tJSInterpPtr interp");
      if (this.args.length > 0)
	stream.write (",");
    }

  var i;
  for (i = 0; i < this.args.length; i++)
    {
      stream.write ("\n\t");
      if (this.args[i].type == tSTRING)
	{
	  stream.write ("unsigned char *"
			+ c_typename (this.args[i].copy_type)
			+ this.args[i].name + ",\n\t"
			+ "unsigned int "
			+ c_typename (this.args[i].copy_type)
			+ this.args[i].name + "_len");
	}
      else
	{
	  stream.write (c_typename (this.args[i].type)
			+ c_typename (this.args[i].copy_type));
	  stream.write (this.args[i].name);
	}

      if (i + 1 < this.args.length)
	stream.write (", ");
    }
  stream.write ("\n\t)");

  if (header)
    stream.write (";");

  stream.write ("\n");
}
Func.prototype.print_h = Func$print_h;

function Func$print_c (stream)
{
  this.compile ();
  this.dump_bc (stream);

  this.print_h (stream, false);

  stream.write ("{\n");

  if (!opt_reentrant)
    stream.write ("  JSInterpPtr interp = jswrap_interp;\n");

  stream.write ("  JSVirtualMachine *vm = interp->vm;\n");

  stream.write ("  JSNode argv["
		+ (this.args.length + 1).toString ()
		+ "];\n");
  stream.write ("  int result;\n");

  if (this.return_type == tCSTRING)
    stream.write ("  char *cp;\n");

  stream.write ("\n");

  /* Argument count. */
  stream.write ("  argv[0].type = JS_INTEGER;\n");
  stream.write ("  argv[0].u.vinteger = "
		+ this.args.length.toString ()
		+ ";\n");

  /* Arguments. */

  /* Construct the argv array. */
  for (i = 0; i < this.args.length; i++)
    {
      var arg = this.args[i];
      var cspec = "";

      stream.write ("\n");

      var argnum = (i + 1).toString ();

      if (arg.copy_type == tOUT)
	{
	  stream.write ("  argv[" + argnum + "].type = JS_UNDEFINED;\n");
	}
      else
	{
	  if (arg.copy_type == tINOUT)
	    cspec = "*";

	  if (arg.type == tCSTRING)
	    {
	      stream.write ("  js_vm_make"
			    + (arg.staticp ? "_static" : "")
			    + "_string (vm, &argv["
			    + argnum + "], "
			    + cspec + arg.name + ", strlen ("
			    + cspec + arg.name + "));\n");
	    }
	  else if (arg.type == tDOUBLE)
	    {
	      stream.write ("  argv[" + argnum + "].type = JS_FLOAT;\n");
	      stream.write ("  argv[" + argnum + "].u.vfloat = "
			    + cspec + arg.name + ";\n");
	    }
	  else if (arg.type == tINT)
	    {
	      stream.write ("  argv[" + argnum + "].type = JS_INTEGER;\n");
	      stream.write ("  argv[" + argnum + "].u.vinteger = "
			    + cspec + arg.name + ";\n");
	    }
	  else if (arg.type == tSTRING)
	    {
	      stream.write ("  js_vm_make_static_string (vm, &argv["
			    + argnum + "], "
			    + cspec + arg.name + ", "
			    + cspec + arg.name + "_len);\n");
	    }
	  else
	    VM.stackTrace ();
	}
    }

  /* Call the function. */
  stream.write ("\

 retry:

  result = js_vm_apply (vm, \"" + this.name + "\", NULL, "
		+ (i + 1).toString () + ", argv);
  if (result == 0)
    {
      JSNode *n = &vm->globals[js_vm_intern (vm, \"" + this.name + "\")];

      if (n->type != JS_FUNC)
        {
          JSByteCode *bc = js_bc_read_data (" + this.name + "_bc,
                                            sizeof (" + this.name + "_bc));
          result = js_vm_execute (vm, bc);
          js_bc_free (bc);

          if (result == 0)
            jswrap_error (interp, \"initialization of function`"
		+ this.name + "' failed\");
          goto retry;
        }
      jswrap_error (interp, \"execution of function `"
		+ this.name + "' failed\");
    }
");

  /* Handle out and inout parameters. */
  for (i = 0; i < this.args.length; i++)
    {
      var arg = this.args[i];
      if (arg.copy_type == tIN)
	continue;

      var spos = (this.args.length - i).toString ();

      /* Check that we have there a correct type. */
      stream.write ("
  if ((vm->sp - " + spos + ")->type != " + jsint_typename (arg.type) + ")
    jswrap_error (interp,
                  \"wrong return type for argument `"
		    + arg.name + "' of function `" + this.name + "'\");
");

      /* Handle the different types. */
      if (arg.type == tCSTRING)
	{
	  stream.write ("\
  *" + arg.name + " = (char *) js_vm_alloc (vm, (vm->sp - "
			+ spos + ")->u.vstring->len + 1);
  memcpy (*" + arg.name + ", (vm->sp - " + spos + ")->u.vstring->data,
          (vm->sp - " + spos + ")->u.vstring->len);
  " + arg.name + "[(vm->sp - " + spos + ")->u.vstring->len] = '\\0';
");
	}
      else if (arg.type == tDOUBLE)
	{
	  stream.write ("\
  *" + arg.name + " = (vm->sp - " + spos + ")->u.vfloat;\n");
	}
      else if (arg.type == tINT)
	{
	  stream.write ("\
  *" + arg.name + " = (vm->sp - " + spos + ")->u.vinteger;\n");
	}
      else if (arg.type == tSTRING)
	{
	  stream.write ("\
  *" + arg.name + " = (vm->sp - " + spos + ")->u.vstring->data;
  *" + arg.name + "_len = (vm->sp - " + spos + ")->u.vstring->len;
");
	}
    }

  /* Handle the function return value. */
  if (this.return_type != tVOID)
    {
      /* Check that the code returned correct type. */
      stream.write ("
  if (vm->exec_result.type != " + jsint_typename (this.return_type) + ")
    jswrap_error (interp, \"return type mismatch in function `"
		    + this.name + "'\");

");

      /* Handle the different types. */
      if (this.return_type == tCSTRING)
	{
	  stream.write ("\
  cp = (char *) js_vm_alloc (vm, vm->exec_result.u.vstring->len + 1);
  memcpy (cp, vm->exec_result.u.vstring->data, vm->exec_result.u.vstring->len);
  cp[vm->exec_result.u.vstring->len] = '\\0';

  return cp;
");
	}
      else if (this.return_type == tDOUBLE)
	stream.write ("  return vm->exec_result.u.vfloat;\n");
      else if (this.return_type == tINT)
	stream.write ("  return vm->exec_result.u.vinteger;\n");
    }

  stream.write ("}\n");
}
Func.prototype.print_c = Func$print_c;

function Func$compile ()
{
  /* Create the byte-code for this function. */

  var flags = 0;

  if (false)
    flags |= JSC$FLAG_VERBOSE;

  if (opt_debug)
    flags |= JSC$FLAG_GENERATE_DEBUG_INFO;

  flags |= (JSC$FLAG_OPTIMIZE_PEEPHOLE
	    | JSC$FLAG_OPTIMIZE_JUMPS
	    | JSC$FLAG_OPTIMIZE_BC_SIZE);

  flags |= JSC$FLAG_WARN_MASK;

  try
    {
      this.bc = JSC$compile_string (this.code, flags, null, null);
    }
  catch (error)
    {
      System.error (error, "\n");
      System.exit (1);
    }
}
Func.prototype.compile = Func$compile;

function Func$dump_bc (stream)
{
  stream.write ("\nstatic unsigned char " + this.name + "_bc[] = {");

  var i;
  for (i = 0; i < this.bc.length; i++)
    {
      if ((i % 12) == 0)
	stream.write ("\n ");

      var item = this.bc[i].toString (16);
      if (item.length == 1)
	item = "0" + item;

      stream.write (" 0x" + item + ",");
    }

  stream.write ("\n};\n");
}
Func.prototype.dump_bc = Func$dump_bc;

/*
 * The Argument class to hold function argument definitions.
 */

function Argument ()
{
  this.type = false;
  this.copy_type = tIN;
  this.staticp = false;
}

new Argument ();

function Argument$print ()
{
  System.print (typename (this.copy_type), " ", typename (this.type),
		" ", this.name);
}
Argument.prototype.print = Argument$print;



/*
 * The .jsw file parsing.
 */

function parse ()
{
  var token;
  var func;

  headers ();

  while ((token = get_token ()) != tEOF)
    {
      if (token != tFUNCTION)
	syntax_error ();

      func = new Func ();

      /* Possible return type. */
      token = get_token ();
      if (is_type_token (token))
	{
	  if (token == tSTRING)
	    {
	      System.error (input_file, ":", linenum,
			    ": the function return value can't be `string'\n");
	      System.exit (1);
	    }

	  func.return_type = token;

	  token = get_token ();
	}
      else
	func.return_type = tVOID;

      /* The name of the function. */
      if (token != tIDENTIFIER)
	syntax_error ();

      if (!valid_c_identifier_re.test (token_value))
	{
	  System.error (input_file, ":", linenum,
			": function name is not a valid C identifier `",
			token_value, "'\n");
	  System.exit (1);
	}

      func.name = token_value;

      /* Arguments. */
      token = get_token ();
      if (token != #'(')
	syntax_error ();

      var args = new Array ();

      token = get_token ();
      while (token != #')')
	{
	  var arg = new Argument ();

	  while (token != tIDENTIFIER)
	    {
	      if (token == tEOF)
		syntax_error ();

	      if (is_type_token (token))
		arg.type = token;
	      else if (is_copy_type_token (token))
		arg.copy_type = token;
	      else if (token == tSTATIC)
		arg.staticp = true;
	      else
		syntax_error ();

	      token = get_token ();
	    }

	  if (!valid_c_identifier_re.test (token_value))
	    {
	      System.error (input_file, ":", linenum,
			    ": argument name is not a valid C identifier `",
			    token_value, "'\n");
	      System.exit (1);
	    }
	  arg.name = token_value;

	  /* Check some validity conditions. */
	  if (!arg.type)
	    {
	      System.error (input_file, ":", linenum,
			    ": no type specified for argument `",
			    arg.name, "'\n");
	      System.exit (1);
	    }
	  if (arg.staticp)
	    {
	      if (arg.type != tCSTRING && arg.type != tSTRING)
		{
		  System.error (input_file, ":", linenum,
				": type `static' can only be used with `cstring' and `string' arguments\n");
		  System.exit (1);
		}
	      if (arg.copy_type == tOUT)
		System.error (input_file, ":", linenum,
			      ": warning: type `static' is meaningful only with `in' and `inout' arguments\n");
	    }

	  args.push (arg);

	  token = get_token ();
	  if (token == #',')
	    token = get_token ();
	}

      func.args = args;

      /* The opening '{' of the function body. */
      token = get_token ();
      if (token != #'{')
	syntax_error ();

      /* Get the function body. */
      var code = new String ("");
      while (true)
	{
	  var line = ifp.readln ();
	  if (!line)
	    syntax_error ();
	  linenum++;

	  if (end_brace_re.test (line))
	    break;

	  code.append (line + "\n");
	}

      func.body = code;
      func.generate_js ();

      // func.print_js (ofp_js);
      func.print_h (ofp_h, true);
      func.print_c (ofp_c);
    }

  trailers ();
}

function get_token ()
{
  var ch;

  while ((ch = ifp.readByte ()) != -1)
    {
      if (ch == #' ' || ch == #'\t' || ch == #'\v' || ch == #'\r'
	  || ch == #'\f')
	continue;

      if (ch == #'\n')
	{
	  linenum++;
	  continue;
	}

      token_linenum = linenum;

      if (ch == #'/' && peek_char () == #'*')
	{
	  /* Multi line comment. */
	  ifp.readByte ();
	  while ((ch = ifp.readByte ()) != -1
		 && (ch != #'*' || peek_char () != #'/'))
	    if (ch == #'\n')
	      linenum++;

	  /* Consume the peeked #'/' character. */
	  ifp.readByte ();
	}

      /* Identifiers and keywords. */
      else if (JSC$lexer_is_identifier_letter (ch))
	{
	  /* An identifier. */
	  var id = String.fromCharCode (ch);

	  while ((ch = ifp.readByte ()) != -1
		 && (JSC$lexer_is_identifier_letter (ch)
		     || JSC$lexer_is_decimal_digit (ch)))
	    id.append (File.byteToString (ch));
	  ifp.ungetByte (ch);

	  /* Keywords. */
	  if (id == "function")
	    return tFUNCTION;

	  /* Types. */
	  else if (id == "cstring")
	    return tCSTRING;
	  else if (id == "double")
	    return tDOUBLE;
	  else if (id == "int")
	    return tINT;
	  else if (id == "string")
	    return tSTRING;
	  else if (id == "void")
	    return tVOID;
	  else if (id == "in")
	    return tIN;
	  else if (id == "out")
	    return tOUT;
	  else if (id == "inout")
	    return tINOUT;
	  else if (id == "static")
	    return tSTATIC;

	  else
	    {
	      /* It really is an identifier. */
	      token_value = id;
	      return tIDENTIFIER;
	    }
	}

      /* Just return the character as-is. */
      else
	return ch;
    }

  /* EOF reached. */
  return tEOF;
}

function peek_char ()
{
  var ch = ifp.readByte ();
  ifp.ungetByte (ch);

  return ch;
}

function syntax_error ()
{
  System.error (input_file, ":", linenum, ": syntax error\n");
  System.exit (1);
}

function headers ()
{
  var stream;

  /* The header file. */

  stream = ofp_h;
  header_banner (stream);

  var ppname = path_to_ppname (header_file);
  stream.write ("\n#ifndef " + ppname
		+ "\n#define " + ppname + "\n");

  /* The C file. */
  stream = ofp_c;
  header_banner (stream);
  stream.write ("\n#include <jsint.h>\n");

  if (!opt_reentrant)
    {
      stream.write ("
/*
 * This global interpreter handle can be removed with " + program + "'s
 * option -r, --reentrant.
 */
");
      stream.write ("extern JSInterpPtr jswrap_interp;\n");
    }

  if (opt_no_error_handler)
    {
      stream.write ("
/* Prototype for the error handler of the JS runtime errors. */
");
      stream.write ("void jswrap_error (JSInterpPtr interp, char *error);\n");
    }
  else
    {
      /* Define the default jswrap_error() function. */
      stream.write ("
/*
 * This is the default error handler for the JS runtime errors.  The default
 * handler can be removed with " + program + "'s option -n, --no-error-handler.
 * In this case, your code must define the handler function.
 */
");
      stream.write ("\
static void
jswrap_error (JSInterpPtr interp, char *error)
{
  const char *cp;

  fprintf (stderr, \"JS runtime error: %s\", error);

  cp = js_error_message (interp);
  if (cp[0])
    fprintf (stderr, \": %s\", cp);
  fprintf (stderr, \"\\n\");

  exit (1);
}
");
    }

  stream.write ("

/*
 * The global function definitions.
 */
");
}

function header_banner (stream)
{
  stream.write ("/* This file is automatically generated from `"
		    + input_file + "' by " + program + ". */\n");
}

path_to_ppname_re = new RegExp ("[^A-Za-z_0-9]", "g");

function path_to_ppname (path)
{
  var str = path.toUpperCase ().replace (path_to_ppname_re, "_");
  if (#'0' <= str[0] && str[0] <= #'9')
    str = "_" + str;

  return str;
}

function trailers ()
{
  /* The header file. */
  var stream = ofp_h;
  stream.write ("\n#endif /* not " + path_to_ppname (header_file) + " */\n");
}


/*
Local variables:
mode: c
End:
*/
