val render_tex : Tex.t -> string

val set_encoding : string -> unit
val tex_use_nonascii: unit -> unit
val tex_use_ams: unit -> unit

val get_preface : unit -> string
val get_footer : unit -> string

exception Illegal_tex_function of string
val find: string -> Parser.token
