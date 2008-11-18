%{
    open Tex
    open Render_info
    
    let sq_close_ri = HTMLABLEC(FONT_UFH,"]", "]")
%}
%token <Render_info.t> LITERAL DELIMITER
%token <string> FUN_AR2 FUN_INFIX FUN_AR1 DECL FUN_AR1opt BIG FUN_AR2nb
%token <string*string> BOX
%token <string*(string*string)> FUN_AR1hl
%token <string*Render_info.font_force> FUN_AR1hf DECLh
%token <string*(Tex.t->Tex.t->string*string*string)> FUN_AR2h
%token <string*(Tex.t list->Tex.t list->string*string*string)> FUN_INFIXh
%token EOF CURLY_OPEN CURLY_CLOSE SUB SUP SQ_CLOSE NEXT_CELL NEXT_ROW
%token BEGIN__MATRIX BEGIN_PMATRIX BEGIN_BMATRIX BEGIN_BBMATRIX BEGIN_VMATRIX BEGIN_VVMATRIX BEGIN_CASES BEGIN_ARRAY BEGIN_ALIGN BEGIN_ALIGNAT BEGIN_SMALLMATRIX
%token END__MATRIX END_PMATRIX END_BMATRIX END_BBMATRIX END_VMATRIX END_VVMATRIX END_CASES END_ARRAY END_ALIGN END_ALIGNAT END_SMALLMATRIX
%token LEFT RIGHT 
%type <Tex.t list> tex_expr
%start tex_expr

%%
tex_expr:
    expr EOF			{ $1 }
  | ne_expr FUN_INFIX ne_expr EOF
				{ [TEX_INFIX($2,$1,$3)] }
  | ne_expr FUN_INFIXh ne_expr EOF
				{ let t,h=$2 in [TEX_INFIXh(t,h,$1,$3)] }
expr:
    /* */			{ [] }
  | ne_expr			{ $1 }
ne_expr:
    lit_aq expr			{ $1 :: $2 }
  | litsq_aq expr		{ $1 :: $2 }
  | DECLh expr			{ let t,h = $1 in [TEX_DECLh(t,h,$2)] }
litsq_aq:
    litsq_zq			{ $1 }
  | litsq_dq			{ let base,downi = $1 in TEX_DQ(base,downi) }
  | litsq_uq			{ let base,upi = $1   in TEX_UQ(base,upi)}
  | litsq_fq			{ $1 }
litsq_fq:
    litsq_dq SUP lit		{ let base,downi = $1 in TEX_FQ(base,downi,$3) }
  | litsq_uq SUB lit		{ let base,upi = $1   in TEX_FQ(base,$3,upi) }
litsq_uq:
    litsq_zq SUP lit		{ $1,$3 }
litsq_dq:
    litsq_zq SUB lit		{ $1,$3 }
litsq_zq:
  | SQ_CLOSE 			{ TEX_LITERAL sq_close_ri }
expr_nosqc:
    /* */			{ [] }
  | lit_aq expr_nosqc		{ $1 :: $2 }
lit_aq:
    lit				{ $1 }
  | lit_dq			{ let base,downi = $1 in TEX_DQ(base,downi) }
  | lit_uq			{ let base,upi = $1   in TEX_UQ(base,upi)}
  | lit_dqn			{ TEX_DQN($1) }
  | lit_uqn			{ TEX_UQN($1) }
  | lit_fq			{ $1 }

lit_fq:
    lit_dq SUP lit		{ let base,downi = $1 in TEX_FQ(base,downi,$3) }
  | lit_uq SUB lit		{ let base,upi = $1   in TEX_FQ(base,$3,upi) }
  | lit_dqn SUP lit     { TEX_FQN($1, $3) }

lit_uq:
    lit SUP lit			{ $1,$3 }
lit_dq:
    lit SUB lit			{ $1,$3 }
lit_uqn:
    SUP lit             { $2 }
lit_dqn:
    SUB lit             { $2 }


left:
    LEFT DELIMITER		{ $2 }
  | LEFT SQ_CLOSE		{ sq_close_ri }
right:
    RIGHT DELIMITER		{ $2 }
  | RIGHT SQ_CLOSE		{ sq_close_ri }
lit:
    LITERAL			{ TEX_LITERAL $1 }
  | DELIMITER			{ TEX_LITERAL $1 }
  | BIG DELIMITER		{ TEX_BIG ($1,$2) }
  | BIG SQ_CLOSE		{ TEX_BIG ($1,sq_close_ri) }
  | left expr right		{ TEX_LR ($1,$3,$2) }
  | FUN_AR1 lit			{ TEX_FUN1($1,$2) }
  | FUN_AR1hl lit		{ let t,h=$1 in TEX_FUN1hl(t,h,$2) }
  | FUN_AR1hf lit		{ let t,h=$1 in TEX_FUN1hf(t,h,$2) }
  | FUN_AR1opt expr_nosqc SQ_CLOSE lit { TEX_FUN2sq($1,TEX_CURLY $2,$4) }
  | FUN_AR2 lit lit		{ TEX_FUN2($1,$2,$3) }
  | FUN_AR2nb lit lit		{ TEX_FUN2nb($1,$2,$3) }
  | FUN_AR2h lit lit		{ let t,h=$1 in TEX_FUN2h(t,h,$2,$3) }
  | BOX				{ let bt,s = $1 in TEX_BOX (bt,s) }
  | CURLY_OPEN expr CURLY_CLOSE
				{ TEX_CURLY $2 }
  | CURLY_OPEN ne_expr FUN_INFIX ne_expr CURLY_CLOSE
				{ TEX_INFIX($3,$2,$4) }
  | CURLY_OPEN ne_expr FUN_INFIXh ne_expr CURLY_CLOSE
				{ let t,h=$3 in TEX_INFIXh(t,h,$2,$4) }
  | BEGIN__MATRIX  matrix END__MATRIX	{ TEX_MATRIX ("matrix", $2) }
  | BEGIN_PMATRIX  matrix END_PMATRIX	{ TEX_MATRIX ("pmatrix", $2) }
  | BEGIN_BMATRIX  matrix END_BMATRIX	{ TEX_MATRIX ("bmatrix", $2) }
  | BEGIN_BBMATRIX matrix END_BBMATRIX	{ TEX_MATRIX ("Bmatrix", $2) }
  | BEGIN_VMATRIX  matrix END_VMATRIX	{ TEX_MATRIX ("vmatrix", $2) }
  | BEGIN_VVMATRIX matrix END_VVMATRIX	{ TEX_MATRIX ("Vmatrix", $2) }
  | BEGIN_ARRAY    matrix END_ARRAY	    { TEX_MATRIX ("array", $2) }
  | BEGIN_ALIGN    matrix END_ALIGN	    { TEX_MATRIX ("aligned", $2) }
  | BEGIN_ALIGNAT  matrix END_ALIGNAT	{ TEX_MATRIX ("alignedat", $2) }
  | BEGIN_SMALLMATRIX  matrix END_SMALLMATRIX { TEX_MATRIX ("smallmatrix", $2) }
  | BEGIN_CASES    matrix END_CASES	{ TEX_MATRIX ("cases", $2) }
matrix:
    line			{ [$1] }
  | line NEXT_ROW matrix	{ $1::$3 }
line:
    expr			{ [$1] }
  | expr NEXT_CELL line		{ $1::$3 }
;;
