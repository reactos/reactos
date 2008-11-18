{
    open Parser
    open Render_info
    open Tex
}
let space = [' ' '\t' '\n' '\r']
let alpha = ['a'-'z' 'A'-'Z']
let literal_id = ['a'-'z' 'A'-'Z']
let literal_mn = ['0'-'9']
let literal_uf_lt = [',' ':' ';' '?' '!' '\'']
let delimiter_uf_lt = ['(' ')' '.']
let literal_uf_op = ['+' '-' '*' '=']
let delimiter_uf_op = ['/' '|']
let boxchars  = ['0'-'9' 'a'-'z' 'A'-'Z' '+' '-' '*' ',' '=' '(' ')' ':' '/' ';' '?' '.' '!' ' ' '\128'-'\255']
let aboxchars = ['0'-'9' 'a'-'z' 'A'-'Z' '+' '-' '*' ',' '=' '(' ')' ':' '/' ';' '?' '.' '!' ' ']

rule token = parse
    space +			{ token lexbuf }
  | "\\text" space * '{' boxchars + '}'
				{ Texutil.tex_use_ams (); let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  BOX ("\\text", String.sub str n (String.length str - n - 1)) }
  | "\\mbox" space * '{' aboxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  BOX ("\\mbox", String.sub str n (String.length str - n - 1)) }
  | "\\hbox" space * '{' aboxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  BOX ("\\hbox", String.sub str n (String.length str - n - 1)) }
  | "\\vbox" space * '{' aboxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  BOX ("\\vbox", String.sub str n (String.length str - n - 1)) }
  | "\\mbox" space * '{' boxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  Texutil.tex_use_nonascii();
				  BOX ("\\mbox", String.sub str n (String.length str - n - 1)) }
  | "\\hbox" space * '{' boxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  Texutil.tex_use_nonascii();
				  BOX ("\\hbox", String.sub str n (String.length str - n - 1)) }
  | "\\vbox" space * '{' boxchars + '}'
				{ let str = Lexing.lexeme lexbuf in
				  let n = String.index str '{' + 1 in
				  Texutil.tex_use_nonascii();
				  BOX ("\\vbox", String.sub str n (String.length str - n - 1)) }
  | literal_id			{ let str = Lexing.lexeme lexbuf in LITERAL (MHTMLABLEC (FONT_IT, str,str,MI,str)) }
  | literal_mn			{ let str = Lexing.lexeme lexbuf in LITERAL (MHTMLABLEC (FONT_RM, str,str,MN,str)) }
  | literal_uf_lt		{ let str = Lexing.lexeme lexbuf in LITERAL (HTMLABLEC (FONT_UFH, str,str)) }
  | delimiter_uf_lt		{ let str = Lexing.lexeme lexbuf in DELIMITER (HTMLABLEC (FONT_UFH, str,str)) }
  | "-"				{ let str = Lexing.lexeme lexbuf in LITERAL (MHTMLABLEC (FONT_UFH,"-"," &minus; ",MO,str))}
  | literal_uf_op		{ let str = Lexing.lexeme lexbuf in LITERAL (MHTMLABLEC (FONT_UFH, str," "^str^" ",MO,str)) }
  | delimiter_uf_op		{ let str = Lexing.lexeme lexbuf in DELIMITER (MHTMLABLEC (FONT_UFH, str," "^str^" ",MO,str)) }
  | "\\" alpha + 		{ Texutil.find (Lexing.lexeme lexbuf) }
  | "\\sqrt" space * "["	{ FUN_AR1opt "\\sqrt" }
  | "\\xleftarrow" space * "["	{ Texutil.tex_use_ams(); FUN_AR1opt "\\xleftarrow" }
  | "\\xrightarrow" space * "["	{ Texutil.tex_use_ams(); FUN_AR1opt "\\xrightarrow" }
  | "\\," 			{ LITERAL (HTMLABLE (FONT_UF, "\\,","&nbsp;")) }
  | "\\ " 			{ LITERAL (HTMLABLE (FONT_UF, "\\ ","&nbsp;")) }
  | "\\;" 			{ LITERAL (HTMLABLE (FONT_UF, "\\;","&nbsp;")) }
  | "\\!" 			{ LITERAL (TEX_ONLY "\\!") }
  | "\\{" 			{ DELIMITER (HTMLABLEC(FONT_UFH,"\\{","{")) }
  | "\\}" 			{ DELIMITER (HTMLABLEC(FONT_UFH,"\\}","}")) }
  | "\\|" 			{ DELIMITER (HTMLABLE (FONT_UFH,"\\|","||")) }
  | "\\_" 			{ LITERAL (HTMLABLEC(FONT_UFH,"\\_","_")) }
  | "\\#" 			{ LITERAL (HTMLABLE (FONT_UFH,"\\#","#")) }
  | "\\%"			{ LITERAL (HTMLABLE (FONT_UFH,"\\%","%")) }
  | "\\$"			{ LITERAL (HTMLABLE (FONT_UFH,"\\$","$")) }
  | "&"				{ NEXT_CELL }
  | "\\\\"			{ NEXT_ROW }
  | "\\begin{matrix}"		{ Texutil.tex_use_ams(); BEGIN__MATRIX }
  | "\\end{matrix}"		{ END__MATRIX }
  | "\\begin{pmatrix}"		{ Texutil.tex_use_ams(); BEGIN_PMATRIX }
  | "\\end{pmatrix}"		{ END_PMATRIX }
  | "\\begin{bmatrix}"		{ Texutil.tex_use_ams(); BEGIN_BMATRIX }
  | "\\end{bmatrix}"		{ END_BMATRIX }
  | "\\begin{Bmatrix}"		{ Texutil.tex_use_ams(); BEGIN_BBMATRIX }
  | "\\end{Bmatrix}"		{ END_BBMATRIX }
  | "\\begin{vmatrix}"		{ Texutil.tex_use_ams(); BEGIN_VMATRIX }
  | "\\end{vmatrix}"		{ END_VMATRIX }
  | "\\begin{Vmatrix}"		{ Texutil.tex_use_ams(); BEGIN_VVMATRIX }
  | "\\end{Vmatrix}"		{ END_VVMATRIX }
  | "\\begin{array}"		{ Texutil.tex_use_ams(); BEGIN_ARRAY }
  | "\\end{array}"  		{ END_ARRAY }
  | "\\begin{align}"		{ Texutil.tex_use_ams(); BEGIN_ALIGN }
  | "\\end{align}"  		{ END_ALIGN }
  | "\\begin{alignat}"		{ Texutil.tex_use_ams(); BEGIN_ALIGNAT }
  | "\\end{alignat}"  		{ END_ALIGNAT }
  | "\\begin{smallmatrix}"	{ Texutil.tex_use_ams(); BEGIN_SMALLMATRIX }
  | "\\end{smallmatrix}"  	{ END_SMALLMATRIX }
  | "\\begin{cases}"		{ Texutil.tex_use_ams(); BEGIN_CASES }
  | "\\end{cases}"		{ END_CASES }
  | '>'				{ LITERAL (HTMLABLEC(FONT_UFH,">"," &gt; ")) }
  | '<'				{ LITERAL (HTMLABLEC(FONT_UFH,"<"," &lt; ")) }
  | '%'				{ LITERAL (HTMLABLEC(FONT_UFH,"\\%","%")) }
  | '$'				{ LITERAL (HTMLABLEC(FONT_UFH,"\\$","$")) }
  | '~'				{ LITERAL (HTMLABLE (FONT_UF, "~","&nbsp;")) }
  | '['				{ DELIMITER (HTMLABLEC(FONT_UFH,"[","[")) }
  | ']'				{ SQ_CLOSE }
  | '{'				{ CURLY_OPEN }
  | '}'				{ CURLY_CLOSE }
  | '^'				{ SUP }
  | '_'				{ SUB }
  | eof				{ EOF }
