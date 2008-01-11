

#ifndef SLANG_PRINT
#define SLANG_PRINT

extern void
slang_print_function(const slang_function *f, GLboolean body);

extern void
slang_print_tree(const slang_operation *op, int indent);

extern const char *
slang_type_qual_string(slang_type_qualifier q);

extern void
slang_print_type(const slang_fully_specified_type *t);

extern void
slang_print_variable(const slang_variable *v);

extern void
_slang_print_var_scope(const slang_variable_scope *s, int indent);


extern int
slang_checksum_tree(const slang_operation *op);

#endif /* SLANG_PRINT */

