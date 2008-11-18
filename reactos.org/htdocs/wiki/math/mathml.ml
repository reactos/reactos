open Tex
open Render_info

type t = TREE_MN of string | TREE_MO of string | TREE_MI of string

let rec make_mathml_tree = function
    TREE_MN a::otr,TEX_LITERAL(MHTMLABLEC(_,_,_,MN,b))::itr -> make_mathml_tree(TREE_MN (a^b)::otr,itr)
  | otr,TEX_LITERAL(MHTMLABLEC(_,_,_,MN,a))::itr -> make_mathml_tree(TREE_MN a::otr,itr)
  | otr,TEX_LITERAL(MHTMLABLEC(_,_,_,MO,a))::itr -> make_mathml_tree(TREE_MO a::otr,itr)
  | otr,TEX_LITERAL(MHTMLABLEC(_,_,_,MI,a))::itr -> make_mathml_tree(TREE_MI a::otr,itr)
  | otr,TEX_CURLY(crl)::itr -> make_mathml_tree(otr,crl@itr)
  | otr,[] -> List.rev otr
  | _ -> failwith "failed to render mathml"

let render_mathml_tree = function
    TREE_MN s -> "<mn>"^s^"</mn>"
  | TREE_MI s -> "<mi>"^s^"</mi>"
  | TREE_MO s -> "<mo>"^s^"</mo>"

let render tree = try Some (Util.mapjoin render_mathml_tree (make_mathml_tree ([],tree))) with _ -> None
