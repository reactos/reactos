

#include "main/glheader.h"
#include "main/mtypes.h"


/**
 * Write shader and associated info to a file.
 */
void
_mesa_write_shader_to_file(const struct gl_shader *shader)
{
   const char *type;
   char filename[100];
   FILE *f;

   if (shader->Type == GL_FRAGMENT_SHADER)
      type = "frag";
   else
      type = "vert";

   snprintf(filename, strlen(filename), "shader_%u.%s", shader->Name, type);
   f = fopen(filename, "w");
   if (!f) {
      fprintf(stderr, "Unable to open %s for writing\n", filename);
      return;
   }

   fprintf(f, "/* Shader %u source */\n", shader->Name);
   fputs(shader->Source, f);
   fprintf(f, "\n");

   fprintf(f, "/* Compile status: %d */\n", shader->CompileStatus);
   fprintf(f, "\n");

   if (shader->CompileStatus) {
      FILE *stdout_save;

      stdout_save = stdout;
      stdout = f;

      fprintf(f, "/*GPU code */\n");
      _mesa_print_program(shader->Program);

      stdout = stdout_save;
   }

   fclose(f);
}




