val render : Tex.t list -> string option
val html_render : Tex.t list -> string

type conservativeness_t = CONSERVATIVE | MODERATE | LIBERAL
val conservativeness : conservativeness_t ref
