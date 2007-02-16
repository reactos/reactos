#include "../../grammar/grammar_crt.h"
#include "../../grammar/grammar_crt.c"
#include <stdio.h>

static const char *slang_shader_syn =
#include "slang_shader_syn.h"
;

static void gc_to_bin (grammar id, const char *in, const char *out)
{
	FILE *f;
	byte *source, *prod;
	unsigned int size, i, line = 0;

	f = fopen (in, "r");
	if (f == NULL)
		return;
	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	source = (byte *) grammar_alloc_malloc (size + 1);
	source[fread (source, 1, size, f)] = '\0';
	fclose (f);

	if (!grammar_fast_check (id, source, &prod, &size, 65536))
	{
		grammar_alloc_free (source);
		return;
	}

	f = fopen (out, "w");
	for (i = 0; i < size; i++)
	{
		unsigned int a;
		if (prod[i] < 10)
			a = 1;
		else if (prod[i] < 100)
			a = 2;
		else
			a = 3;
		if (i < size - 1)
			a++;
		if (line + a > 100)
		{
			fprintf (f, "\n");
			line = 0;
		}
		line += a;
		fprintf (f, "%d", prod[i]);
		if (i < size - 1)
			fprintf (f, ",");
	}
	fclose (f);
	grammar_alloc_free (prod);
}

int main ()
{
	grammar id;

	id = grammar_load_from_text ((const byte *) slang_shader_syn);
	if (id == 0)
		return 1;

	grammar_set_reg8 (id, (const byte *) "parsing_builtin", 1);

	grammar_set_reg8 (id, (const byte *) "shader_type", 1);
	gc_to_bin (id, "slang_core.gc", "slang_core_gc_bin.h");
	gc_to_bin (id, "slang_common_builtin.gc", "slang_common_builtin_gc_bin.h");
	gc_to_bin (id, "slang_fragment_builtin.gc", "slang_fragment_builtin_gc_bin.h");

	grammar_set_reg8 (id, (const byte *) "shader_type", 2);
	gc_to_bin (id, "slang_vertex_builtin.gc", "slang_vertex_builtin_gc_bin.h");

	grammar_destroy (id);

	return 0;
}

