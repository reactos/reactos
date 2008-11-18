open Netcgi;;
open Netcgi_types;;
open Netcgi_env;;
open Netchannels;;

let cgi = new Netcgi.std_activation ()
let out = cgi # output # output_string
let math = cgi # argument_value ~default:"" "math"
let tmppath = "/home/taw/public_html/wiki/tmp/"
let finalpath = "/home/taw/public_html/wiki/math/"
let finalurl = "http://wroclaw.taw.pl.eu.org/~taw/wiki/math/"
;;

let h_header = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""^
	     " \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"^
	     "<html><head><title>texvc</title></head><body>"^
	     "<form method=post action=\"http://wroclaw.taw.pl.eu.org/~taw/cgi-bin/newcodebase/math/texvc_cgi\">"^
	     "<textarea name='math' rows=10 cols=80>"
let h_middle = "</textarea><br /><input type=submit value=\"Preview\" name='preview'></form>"
let h_footer = "</body></html>\n"

let render tmppath finalpath tree =
    let outtex = Texutil.mapjoin Texutil.print tree in
    let md5 = Digest.to_hex (Digest.string outtex) in
    begin
	out "<h3>TeX</h3>";
        out outtex; (* <, &  and > should be protected *)
	(try out ("<h3>HTML</h3>" ^ (Texutil.html_render tree))
         with _ -> out "<h3>HTML could not be rendered</h3>");
	try  Render.render tmppath finalpath outtex md5;
	    out ("<h3>Image:</h3><img src=\""^finalurl^md5^".png\">")
	with Util.FileAlreadyExists -> out ("<h3>Image:</h3><img src=\""^finalurl^md5^".png\">")
           | Failure s -> out ("<h3>Other failure: " ^ s ^ "</h3>")
	   | Render.ExternalCommandFailure "latex" -> out "<h3>latex failed</h3>"
	   | Render.ExternalCommandFailure "dvips" -> out "<h3>dvips failed</h3>"
           | _ ->  out "<h3>Other failure</h3>"
    end
;;

cgi#set_header ();;

out h_header;;
out math;;
out h_middle;;

exception LexerException of string
let lexer_token_safe lexbuf =
    try Lexer.token lexbuf
    with Failure s -> raise (LexerException s)
;;
if math = ""
then ()
else try
	render tmppath finalpath (Parser.tex_expr lexer_token_safe (Lexing.from_string math))
    with Parsing.Parse_error -> out "<h3>Parse error</h3>"
       | LexerException s -> out "<h3>Lexing failure</h3>"
       | Texutil.Illegal_tex_function s -> out ("<h3>Illegal TeX function: " ^ s ^ "</h3>")
       | Failure s -> out ("<h3>Other failure: " ^ s ^ "</h3>")
       | _ -> out "<h3>Other failure</h3>"
;;

out h_footer
