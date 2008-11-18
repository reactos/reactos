open Render_info
open Tex
open Util

exception Too_difficult_for_html
type context = CTX_NORMAL | CTX_IT | CTX_RM 
type conservativeness_t = CONSERVATIVE | MODERATE | LIBERAL

let conservativeness = ref CONSERVATIVE
let html_liberal () = conservativeness := LIBERAL
let html_moderate () = if !conservativeness = CONSERVATIVE then conservativeness := MODERATE else ()


let new_ctx = function
    FONTFORCE_IT -> CTX_IT
  | FONTFORCE_RM -> CTX_RM
let font_render lit = function
    (_,     FONT_UFH) -> lit
  | (_,     FONT_UF)  -> lit
  | (CTX_IT,FONT_RTI) -> raise Too_difficult_for_html
  | (_,     FONT_RTI) -> lit
  | (CTX_IT,FONT_RM)  -> "<i>"^lit^"</i>"
  | (_,     FONT_RM)  -> lit
  | (CTX_RM,FONT_IT)  -> lit
  | (_,     FONT_IT)  -> "<i>"^lit^"</i>"

let rec html_render_flat ctx = function
    TEX_LITERAL (HTMLABLE (ft,_,sh))::r -> (html_liberal (); (font_render sh (ctx,ft))^html_render_flat ctx r)
  | TEX_LITERAL (HTMLABLEC(ft,_,sh))::r -> (font_render sh (ctx,ft))^html_render_flat ctx r
  | TEX_LITERAL (MHTMLABLEC(ft,_,sh,_,_))::r -> (font_render sh (ctx,ft))^html_render_flat ctx r
  | TEX_LITERAL (HTMLABLEM(ft,_,sh))::r -> (html_moderate(); (font_render sh (ctx,ft))^html_render_flat ctx r)
  | TEX_LITERAL (HTMLABLE_BIG (_,sh))::r -> (html_liberal (); sh^html_render_flat ctx r)
  | TEX_FUN1hl (_,(f1,f2),a)::r -> f1^(html_render_flat ctx [a])^f2^html_render_flat ctx r
  | TEX_FUN1hf (_,ff,a)::r -> (html_render_flat (new_ctx ff) [a])^html_render_flat ctx r
  | TEX_DECLh (_,ff,a)::r -> (html_render_flat (new_ctx ff) a)^html_render_flat ctx r
  | TEX_CURLY ls::r -> html_render_flat ctx (ls @ r)
  | TEX_DQ (a,b)::r  -> (html_liberal ();
			 let bs = html_render_flat ctx [b] in match html_render_size ctx a with
		         true, s -> raise Too_difficult_for_html
		       | false, s -> s^"<sub>"^bs^"</sub>")^html_render_flat ctx r
  | TEX_UQ (a,b)::r  -> (html_liberal ();
		         let bs = html_render_flat ctx [b] in match html_render_size ctx a with
		         true, s ->  raise Too_difficult_for_html
		       | false, s -> s^"<sup>"^bs^"</sup>")^html_render_flat ctx r
  | TEX_FQ (a,b,c)::r -> (html_liberal ();
			 (let bs = html_render_flat ctx [b] in let cs = html_render_flat ctx [c] in
		          match html_render_size ctx a with
		          true, s -> raise Too_difficult_for_html
		        | false, s -> s^"<sub>"^bs^"</sub><sup>"^cs^"</sup>")^html_render_flat ctx r)
  | TEX_DQN (a)::r  -> (html_liberal ();
                 let bs = html_render_flat ctx [a] in "<sub>"^bs^"</sub>")^html_render_flat ctx r
  | TEX_UQN (a)::r  -> (html_liberal ();
                 let bs = html_render_flat ctx [a] in "<sup>"^bs^"</sup>")^html_render_flat ctx r
  | TEX_FQN (a,b)::r -> (html_liberal ();
			 (let bs = html_render_flat ctx [a] in let cs = html_render_flat ctx [b] in  "<sub>"^bs^"</sub><sup>"^cs^"</sup>")^html_render_flat ctx r)  
  | TEX_BOX (_,s)::r -> s^html_render_flat ctx r
  | TEX_LITERAL (TEX_ONLY _)::_ -> raise Too_difficult_for_html
  | TEX_FUN1 _::_ -> raise Too_difficult_for_html
  | TEX_FUN2  _::_ -> raise Too_difficult_for_html
  | TEX_FUN2nb  _::_ -> raise Too_difficult_for_html
  | TEX_FUN2h  _::_ -> raise Too_difficult_for_html
  | TEX_FUN2sq  _::_ -> raise Too_difficult_for_html
  | TEX_INFIX _::_ -> raise Too_difficult_for_html
  | TEX_INFIXh _::_ -> raise Too_difficult_for_html
  | TEX_MATRIX _::_ -> raise Too_difficult_for_html
  | TEX_LR _::_ -> raise Too_difficult_for_html
  | TEX_BIG _::_ -> raise Too_difficult_for_html
  | [] -> ""
and html_render_size ctx = function
    TEX_LITERAL (HTMLABLE_BIG (_,sh)) -> true,sh
  | x -> false,html_render_flat ctx [x]

let rec html_render_deep ctx = function
    TEX_LITERAL (HTMLABLE (ft,_,sh))::r -> (html_liberal (); ("",(font_render sh (ctx,ft)),"")::html_render_deep ctx r)
  | TEX_LITERAL (HTMLABLEM(ft,_,sh))::r -> (html_moderate(); ("",(font_render sh (ctx,ft)),"")::html_render_deep ctx r)
  | TEX_LITERAL (HTMLABLEC(ft,_,sh))::r -> ("",(font_render sh (ctx,ft)),"")::html_render_deep ctx r
  | TEX_LITERAL (MHTMLABLEC(ft,_,sh,_,_))::r -> ("",(font_render sh (ctx,ft)),"")::html_render_deep ctx r
  | TEX_LITERAL (HTMLABLE_BIG (_,sh))::r -> (html_liberal (); ("",sh,"")::html_render_deep ctx r)
  | TEX_FUN2h (_,f,a,b)::r -> (html_liberal (); (f a b)::html_render_deep ctx r)
  | TEX_INFIXh (_,f,a,b)::r -> (html_liberal (); (f a b)::html_render_deep ctx r)
  | TEX_CURLY ls::r -> html_render_deep ctx (ls @ r)
  | TEX_DQ (a,b)::r  -> (let bs = html_render_flat ctx [b] in match html_render_size ctx a with
  true, s ->  "","<span style='font-size: x-large; font-family: serif;'>"^s^"</span>",bs
		       | false, s -> "",(s^"<sub>"^bs^"</sub>"),"")::html_render_deep ctx r
  | TEX_UQ (a,b)::r  -> (let bs = html_render_flat ctx [b] in match html_render_size ctx a with
  true, s ->  bs,"<span style='font-size: x-large; font-family: serif;'>"^s^"</span>",""
		       | false, s -> "",(s^"<sup>"^bs^"</sup>"),"")::html_render_deep ctx r
  | TEX_FQ (a,b,c)::r -> (html_liberal ();
			 (let bs = html_render_flat ctx [b] in let cs = html_render_flat ctx [c] in
		          match html_render_size ctx a with
                  true, s ->  (cs,"<span style='font-size: x-large; font-family: serif;'>"^s^"</span>",bs)
		        | false, s -> ("",(s^"<sub>"^bs^"</sub><sup>"^cs^"</sup>"),""))::html_render_deep ctx r)
  | TEX_DQN (a)::r  -> (let bs = html_render_flat ctx [a] in "",("<sub>"^bs^"</sub>"),"")::html_render_deep ctx r
  | TEX_UQN (a)::r  -> (let bs = html_render_flat ctx [a] in "",("<sup>"^bs^"</sup>"),"")::html_render_deep ctx r
  | TEX_FQN (a,b)::r -> (html_liberal ();
			 (let bs = html_render_flat ctx [a] in let cs = html_render_flat ctx [b] in
		         ("",("<sub>"^bs^"</sub><sup>"^cs^"</sup>"),""))::html_render_deep ctx r)  
  | TEX_FUN1hl (_,(f1,f2),a)::r -> ("",f1,"")::(html_render_deep ctx [a]) @ ("",f2,"")::html_render_deep ctx r
  | TEX_FUN1hf (_,ff,a)::r -> (html_render_deep (new_ctx ff) [a]) @ html_render_deep ctx r
  | TEX_DECLh  (_,ff,a)::r -> (html_render_deep (new_ctx ff) a) @ html_render_deep ctx r
  | TEX_BOX (_,s)::r -> ("",s,"")::html_render_deep ctx r
  | TEX_LITERAL (TEX_ONLY _)::_ -> raise Too_difficult_for_html
  | TEX_FUN1 _::_ -> raise Too_difficult_for_html
  | TEX_FUN2 _::_ -> raise Too_difficult_for_html
  | TEX_FUN2nb _::_ -> raise Too_difficult_for_html
  | TEX_FUN2sq  _::_ -> raise Too_difficult_for_html
  | TEX_INFIX _::_ -> raise Too_difficult_for_html
  | TEX_MATRIX _::_ -> raise Too_difficult_for_html
  | TEX_LR _::_ -> raise Too_difficult_for_html
  | TEX_BIG _::_ -> raise Too_difficult_for_html
  | [] -> []

let rec html_render_table = function
    sf,u,d,("",a,"")::("",b,"")::r -> html_render_table (sf,u,d,(("",a^b,"")::r))
  | sf,u,d,(("",a,"") as c)::r     -> html_render_table (c::sf,u,d,r)
  | sf,u,d,((_,a,"") as c)::r      -> html_render_table (c::sf,true,d,r)
  | sf,u,d,(("",a,_) as c)::r      -> html_render_table (c::sf,u,true,r)
  | sf,u,d,((_,a,_) as c)::r       -> html_render_table (c::sf,true,true,r)
  | sf,false,false,[]              -> mapjoin (function (u,m,d) -> m) (List.rev sf)
  | sf,true,false,[]               -> let ustr,mstr = List.fold_left (fun (us,ms) (u,m,d) -> (us^"<td>"^u^"</td>",ms^"<td>"^u^"</td>"))
					("","") (List.rev sf) in
                    "\n<table>\n" ^
                    "\t\t<tr style='text-align: center; vertical-align: bottom;'>" ^ ustr ^ "</tr>\n" ^
                    "\t\t<tr style='text-align: center;'>" ^ mstr ^ "</tr>\n" ^ 
                    "</table>\n"
  | sf,false,true,[]               -> let mstr,dstr = List.fold_left (fun (ms,ds) (u,m,d) -> (ms^"<td>"^m^"</td>",ds^"<td>"^d^"</td>"))
					("","") (List.rev sf) in
                    "\n<table>\n" ^ 
                    "\t\t<tr style='text-align: center;'>" ^ mstr ^ "</tr>\n" ^ 
                    "\t\t<tr style='text-align: center; vertical-align: top;'>" ^ dstr ^ "</tr>\n" ^
                    "</table>\n"
  | sf,true,true,[]               -> let ustr,mstr,dstr = List.fold_left (fun (us,ms,ds) (u,m,d) ->
					(us^"<td>"^u^"</td>",ms^"<td>"^m^"</td>",ds^"<td>"^d^"</td>")) ("","","") (List.rev sf) in
					"\n<table>\n" ^ 
                    "\t\t<tr style='text-align: center; vertical-align: bottom;'>" ^ ustr ^ "</tr>\n" ^ 
                    "\t\t<tr style='text-align: center;'>" ^ mstr ^ "</tr>\n" ^ 
                    "\t\t<tr style='text-align: center; vertical-align: top;'>" ^ dstr ^ "</tr>\n" ^ 
                    "</table>\n"

let html_render tree = html_render_table ([],false,false,html_render_deep CTX_NORMAL tree)

let render tree = try Some (html_render tree) with _ -> None
