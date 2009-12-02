
#include "main/imports.h"
#include "shader/program.h"
#include "shader/prog_print.h"
#include "slang_compile.h"
#include "slang_compile_variable.h"
#include "slang_emit.h"
#include "slang_mem.h"
#include "slang_vartable.h"
#include "slang_ir.h"


static int dbg = 0;


typedef enum {
   FREE,
   VAR,
   TEMP
} TempState;


/**
 * Variable/register info for one variable scope.
 */
struct table
{
   int Level;
   int NumVars;
   slang_variable **Vars;  /* array [NumVars] */

   TempState Temps[MAX_PROGRAM_TEMPS * 4];  /* per-component state */
   int ValSize[MAX_PROGRAM_TEMPS * 4];     /**< For debug only */

   struct table *Parent;  /** Parent scope table */
};


/**
 * A variable table is a stack of tables, one per scope.
 */
struct slang_var_table_
{
   GLint CurLevel;
   GLuint MaxRegisters;
   struct table *Top;  /**< Table at top of stack */
};



slang_var_table *
_slang_new_var_table(GLuint maxRegisters)
{
   slang_var_table *vt
      = (slang_var_table *) _slang_alloc(sizeof(slang_var_table));
   if (vt) {
      vt->MaxRegisters = maxRegisters;
   }
   return vt;
}


void
_slang_delete_var_table(slang_var_table *vt)
{
   if (vt->Top) {
      _mesa_problem(NULL, "non-empty var table in _slang_delete_var_table()");
      return;
   }
   _slang_free(vt);
}



/**
 * Create new table on top of vartable stack.
 * Used when we enter a {} block.
 */
void
_slang_push_var_table(slang_var_table *vt)
{
   struct table *t = (struct table *) _slang_alloc(sizeof(struct table));
   if (t) {
      t->Level = vt->CurLevel++;
      t->Parent = vt->Top;
      if (t->Parent) {
         /* copy the info indicating which temp regs are in use */
         memcpy(t->Temps, t->Parent->Temps, sizeof(t->Temps));
         memcpy(t->ValSize, t->Parent->ValSize, sizeof(t->ValSize));
      }
      vt->Top = t;
      if (dbg) printf("Pushing level %d\n", t->Level);
   }
}


/**
 * Pop top entry from variable table.
 * Used when we leave a {} block.
 */
void
_slang_pop_var_table(slang_var_table *vt)
{
   struct table *t = vt->Top;
   int i;

   if (dbg) printf("Popping level %d\n", t->Level);

   /* free the storage allocated for each variable */
   for (i = 0; i < t->NumVars; i++) {
      slang_ir_storage *store = t->Vars[i]->store;
      GLint j;
      GLuint comp;
      if (dbg) printf("  Free var %s, size %d at %d.%s\n",
                      (char*) t->Vars[i]->a_name, store->Size,
                      store->Index,
                      _mesa_swizzle_string(store->Swizzle, 0, 0));

      if (store->File == PROGRAM_SAMPLER) {
         /* samplers have no storage */
         continue;
      }

      if (store->Size == 1)
         comp = GET_SWZ(store->Swizzle, 0);
      else
         comp = 0;

      /* store->Index may be -1 if we run out of registers */
      if (store->Index >= 0) {
         for (j = 0; j < store->Size; j++) {
            assert(t->Temps[store->Index * 4 + j + comp] == VAR);
            t->Temps[store->Index * 4 + j + comp] = FREE;
         }
      }
      store->Index = -1;
   }
   if (t->Parent) {
      /* just verify that any remaining allocations in this scope 
       * were for temps
       */
      for (i = 0; i < (int) vt->MaxRegisters * 4; i++) {
         if (t->Temps[i] != FREE && t->Parent->Temps[i] == FREE) {
            if (dbg) printf("  Free reg %d\n", i/4);
            assert(t->Temps[i] == TEMP);
         }
      }
   }

   if (t->Vars) {
      _slang_free(t->Vars);
      t->Vars = NULL;
   }

   vt->Top = t->Parent;
   _slang_free(t);
   vt->CurLevel--;
}


/**
 * Add a new variable to the given var/symbol table.
 */
void
_slang_add_variable(slang_var_table *vt, slang_variable *v)
{
   struct table *t;
   assert(vt);
   t = vt->Top;
   assert(t);
   if (dbg) printf("Adding var %s, store %p\n", (char *) v->a_name, (void *) v->store);
   t->Vars = (slang_variable **)
      _slang_realloc(t->Vars,
                     t->NumVars * sizeof(slang_variable *),
                     (t->NumVars + 1) * sizeof(slang_variable *));
   t->Vars[t->NumVars] = v;
   t->NumVars++;
}


/**
 * Look for variable by name in given table.
 * If not found, Parent table will be searched.
 */
slang_variable *
_slang_find_variable(const slang_var_table *vt, slang_atom name)
{
   struct table *t = vt->Top;
   while (1) {
      int i;
      for (i = 0; i < t->NumVars; i++) {
         if (t->Vars[i]->a_name == name)
            return t->Vars[i];
      }
      if (t->Parent)
         t = t->Parent;
      else
         return NULL;
   }
}


/**
 * Allocation helper.
 * \param size  var size in floats
 * \return  position for var, measured in floats
 */
static GLint
alloc_reg(slang_var_table *vt, GLint size, GLboolean isTemp)
{
   struct table *t = vt->Top;
   /* if size == 1, allocate anywhere, else, pos must be multiple of 4 */
   const GLuint step = (size == 1) ? 1 : 4;
   GLuint i, j;
   assert(size > 0); /* number of floats */

   for (i = 0; i <= vt->MaxRegisters * 4 - size; i += step) {
      GLuint found = 0;
      for (j = 0; j < (GLuint) size; j++) {
         assert(i + j < 4 * MAX_PROGRAM_TEMPS);
         if (i + j < vt->MaxRegisters * 4 && t->Temps[i + j] == FREE) {
            found++;
         }
         else {
            break;
         }
      }
      if (found == size) {
         /* found block of size free regs */
         if (size > 1)
            assert(i % 4 == 0);
         for (j = 0; j < (GLuint) size; j++) {
            assert(i + j < 4 * MAX_PROGRAM_TEMPS);
            t->Temps[i + j] = isTemp ? TEMP : VAR;
         }
         assert(i < MAX_PROGRAM_TEMPS * 4);
         t->ValSize[i] = size;
         return i;
      }
   }

   /* if we get here, we ran out of registers */
   return -1;
}


/**
 * Allocate temp register(s) for storing a variable.
 * \param size  size needed, in floats
 * \param swizzle  returns swizzle mask for accessing var in register
 * \return  register allocated, or -1
 */
GLboolean
_slang_alloc_var(slang_var_table *vt, slang_ir_storage *store)
{
   struct table *t = vt->Top;
   int i;

   if (store->File == PROGRAM_SAMPLER) {
      /* don't really allocate storage */
      store->Index = 0;
      return GL_TRUE;
   }

   i = alloc_reg(vt, store->Size, GL_FALSE);
   if (i < 0)
      return GL_FALSE;

   store->Index = i / 4;
   store->Swizzle = _slang_var_swizzle(store->Size, i % 4);

   if (dbg)
      printf("Alloc var storage sz %d at %d.%s (level %d) store %p\n",
             store->Size, store->Index,
             _mesa_swizzle_string(store->Swizzle, 0, 0),
             t->Level,
             (void*) store);

   return GL_TRUE;
}



/**
 * Allocate temp register(s) for storing an unnamed intermediate value.
 */
GLboolean
_slang_alloc_temp(slang_var_table *vt, slang_ir_storage *store)
{
   struct table *t = vt->Top;
   const int i = alloc_reg(vt, store->Size, GL_TRUE);
   if (i < 0)
      return GL_FALSE;

   assert(store->Index < 0);

   store->Index = i / 4;
   store->Swizzle = _slang_var_swizzle(store->Size, i % 4);

   if (dbg) printf("Alloc temp sz %d at %d.%s (level %d) store %p\n",
                   store->Size, store->Index,
                   _mesa_swizzle_string(store->Swizzle, 0, 0), t->Level,
                   (void *) store);

   return GL_TRUE;
}


void
_slang_free_temp(slang_var_table *vt, slang_ir_storage *store)
{
   struct table *t = vt->Top;
   GLuint i;
   GLuint r = store->Index;
   assert(store->Size > 0);
   assert(r >= 0);
   assert(r + store->Size <= vt->MaxRegisters * 4);
   if (dbg) printf("Free temp sz %d at %d.%s (level %d) store %p\n",
                   store->Size, r,
                   _mesa_swizzle_string(store->Swizzle, 0, 0),
                   t->Level, (void *) store);
   if (store->Size == 1) {
      const GLuint comp = GET_SWZ(store->Swizzle, 0);
      /* we can actually fail some of these assertions because of the
       * troublesome IR_SWIZZLE handling.
       */
#if 0
      assert(store->Swizzle == MAKE_SWIZZLE4(comp, comp, comp, comp));
      assert(comp < 4);
      assert(t->ValSize[r * 4 + comp] == 1);
#endif
      assert(t->Temps[r * 4 + comp] == TEMP);
      t->Temps[r * 4 + comp] = FREE;
   }
   else {
      /*assert(store->Swizzle == SWIZZLE_NOOP);*/
      assert(t->ValSize[r*4] == store->Size);
      for (i = 0; i < (GLuint) store->Size; i++) {
         assert(t->Temps[r * 4 + i] == TEMP);
         t->Temps[r * 4 + i] = FREE;
      }
   }
}


GLboolean
_slang_is_temp(const slang_var_table *vt, const slang_ir_storage *store)
{
   struct table *t = vt->Top;
   GLuint comp;
   assert(store->Index >= 0);
   assert(store->Index < (int) vt->MaxRegisters);
   if (store->Swizzle == SWIZZLE_NOOP)
      comp = 0;
   else
      comp = GET_SWZ(store->Swizzle, 0);

   if (t->Temps[store->Index * 4 + comp] == TEMP)
      return GL_TRUE;
   else
      return GL_FALSE;
}
