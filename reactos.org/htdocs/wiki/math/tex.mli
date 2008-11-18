type t =
      TEX_LITERAL of Render_info.t
    | TEX_CURLY of t list
    | TEX_FQ of t * t * t
    | TEX_DQ of t * t
    | TEX_UQ of t * t
    | TEX_FQN of t * t
    | TEX_DQN of t
    | TEX_UQN of t
    | TEX_LR of Render_info.t * Render_info.t * t list
    | TEX_BOX of string * string
    | TEX_BIG of string * Render_info.t
    | TEX_FUN1 of string * t
    | TEX_FUN2 of string * t * t
    | TEX_FUN2nb of string * t * t
    | TEX_INFIX of string * t list * t list
    | TEX_FUN2sq of string * t * t
    | TEX_FUN1hl  of string * (string * string) * t
    | TEX_FUN1hf  of string * Render_info.font_force * t
    | TEX_FUN2h  of string * (t -> t -> string * string * string) * t * t
    | TEX_INFIXh of string * (t list -> t list -> string * string * string) * t list * t list
    | TEX_MATRIX of string * t list list list
    | TEX_DECLh  of string * Render_info.font_force * t list
