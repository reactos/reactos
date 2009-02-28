#ifndef SLANG_LABEL_H
#define SLANG_LABEL_H 1

#include "main/imports.h"
#include "main/mtypes.h"
#include "shader/prog_instruction.h"


struct slang_label_
{
   char *Name;
   GLint Location;
   /**
    * List of instruction references (numbered starting at zero) which need
    * their BranchTarget field filled in with the location eventually
    * assigned to the label.
    */
   GLuint NumReferences;
   GLuint *References;   /** Array [NumReferences] */
};

typedef struct slang_label_ slang_label;


extern slang_label *
_slang_label_new(const char *name);

extern slang_label *
_slang_label_new_unique(const char *name);

extern void
_slang_label_delete(slang_label *l);

extern void
_slang_label_add_reference(slang_label *l, GLuint inst);

extern GLint
_slang_label_get_location(const slang_label *l);

extern void
_slang_label_set_location(slang_label *l, GLint location,
                          struct gl_program *prog);


#endif /* SLANG_LABEL_H */
