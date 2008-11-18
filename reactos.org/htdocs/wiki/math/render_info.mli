type font_force =
    FONTFORCE_IT
  | FONTFORCE_RM
type font_class =
    FONT_IT  (* IT default, may be forced to be RM *)
  | FONT_RM  (* RM default, may be forced to be IT *)
  | FONT_UF  (* not affected by IT/RM setting *)
  | FONT_RTI (* RM - any, IT - not available in HTML *)
  | FONT_UFH (* in TeX UF, in HTML RM *)
type math_class =
    MN
  | MI
  | MO
type t =
      HTMLABLEC of font_class * string * string
    | HTMLABLEM of font_class * string * string
    | HTMLABLE of font_class * string * string
    | MHTMLABLEC of font_class * string * string * math_class * string
    | HTMLABLE_BIG of string * string
    | TEX_ONLY of string
