Texutil.set_encoding (try Sys.argv.(2) with _ -> "UTF-8");
try print_string (Util.mapjoin Texutil.render_tex (Parser.tex_expr Lexer.token (Lexing.from_string Sys.argv.(1))))
with _ -> ()
