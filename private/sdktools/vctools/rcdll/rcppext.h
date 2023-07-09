extern  WCHAR   Union_str[];
extern  WCHAR   Struct_str[];
extern  WCHAR   Cdecl_str[];
extern  WCHAR   Cdecl1_str[];
extern  WCHAR   Fortran_str[];
extern  WCHAR   Fortran1_str[];
extern  WCHAR   Pascal_str[];
extern  WCHAR   Pascal1_str[];
extern  WCHAR   PPelse_str[];
extern  WCHAR   PPendif_str[];
extern  WCHAR   PPifel_str[];
extern  WCHAR   Syntax_str[];

extern  FILE    * OUTPUTFILE;

extern  WCHAR   * A_string;
extern  WCHAR   * Debug;
extern  WCHAR   * Input_file;
extern  WCHAR   * Output_file;
extern  WCHAR   * Q_string;
extern  WCHAR   * Version;
extern  int     In_alloc_text;
extern  int     Bad_pragma;
extern  int     Cross_compile;
extern  int     Ehxtension;
extern  int     HugeModel;
extern  LIST    Defs;
extern  LIST    UnDefs;
extern  LIST    Includes;
extern  lextype_t yylval;
extern  token_t Basic_token;
extern  WCHAR   * Basename;
extern  WCHAR   * Path_chars;
extern  int     Char_align;
extern  int     Dump_tables;
extern  int     StunOpSeen;
extern  int     Inteltypes;
extern  int     List_type;
extern  int     Need_enddata;
extern  int     Nerrors;
extern  int     NoPasFor;

extern  int     Cmd_intrinsic;
extern  int     Cmd_pointer_check;
extern  int     Pointer_check;
extern  int     Cmd_stack_check;
extern  int     Stack_check;
extern  int     Cmd_loop_opt;
extern  int     Loop_opt;
extern  int     Cmd_pack_size;
extern  int     Pack_size;

extern  int     N_types;
extern  int     Got_type;
extern  int     Out_funcdef;
extern  int     Plm;
extern  int     Prep;
extern  int     Prep_ifstack;
extern  int     Ret_seen;
extern  int     Srclist;
extern  int     Stack_depth;
extern  int     Symbolic_debug;
extern  int     Table_index;
extern  int     Switch_check;
extern  int     Load_ds_with;
extern  int     Plmn;
extern  int     Plmf;
extern  int     Cflag;
extern  int     Eflag;
extern  int     Jflag;
extern  int     Pflag;
extern  int     Rflag;
extern  int     ZcFlag;
extern  int     StunDepth;

extern  long    Enum_val;
extern  long    Max_ival[];
extern  table_t *Table_stack[];

extern  int     Extension;

extern  WCHAR   *Filename;
extern  int     Linenumber;
extern  WCHAR   Filebuff[MED_BUFFER + 1];
extern  CHAR    chBuf[MED_BUFFER+1];
extern  WCHAR   Reuse_W[BIG_BUFFER];
extern  hash_t  Reuse_W_hash;
extern  UINT    Reuse_W_length;
extern  WCHAR   Reuse_Include[MED_BUFFER+1];
extern  WCHAR   Macro_buffer[BIG_BUFFER * 4];
extern  int     In_define;
extern  int     InIf;
extern  int     InInclude;
extern  int     Macro_depth;
extern  int     On_pound_line;
extern  int     Listing_value;
extern  token_t Currtok;

extern  long    Currval;
extern  int     Comment_type;
extern  WCHAR   *Comment_string;
extern  int     Tiny_lexer_nesting;
extern  WCHAR   *Exp_ptr;
extern  int     ifstack[IFSTACK_SIZE];

extern WCHAR    Contmap[], Charmap[];

extern keytab_t Tokstrings[];

#define EXTENSION    (Extension || Ehxtension)

/*** I/O Variable for PreProcessor ***/
extern  ptext_t Current_char;

/*** w-BrianM - Re-write of fatal(), error() ***/
extern CHAR     Msg_Text[MSG_BUFF_SIZE];
extern PCHAR    Msg_Temp;

