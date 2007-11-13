
#ifndef SLANG_VARTABLE_H
#define SLANG_VARTABLE_H

struct _slang_ir_storage;

typedef struct slang_var_table_ slang_var_table;

struct slang_variable_;

extern slang_var_table *
_slang_new_var_table(GLuint maxRegisters);

extern void
_slang_delete_var_table(slang_var_table *vt);

extern void
_slang_push_var_table(slang_var_table *parent);

extern void
_slang_pop_var_table(slang_var_table *t);

extern void
_slang_add_variable(slang_var_table *t, struct slang_variable_ *v);

extern struct slang_variable_ *
_slang_find_variable(const slang_var_table *t, slang_atom name);

extern GLboolean
_slang_alloc_var(slang_var_table *t, struct _slang_ir_storage *store);

extern GLboolean
_slang_alloc_temp(slang_var_table *t, struct _slang_ir_storage *store);

extern void
_slang_free_temp(slang_var_table *t, struct _slang_ir_storage *store);

extern GLboolean
_slang_is_temp(const slang_var_table *t, const struct _slang_ir_storage *store);


#endif /* SLANG_VARTABLE_H */
