#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midl.h"

typedef struct
{
   char* name;
   unsigned int value;
   unsigned int default_sign;
} type;

static type types[] = {
     {"boolean", BOOLEAN_TYPE, UNSIGNED_TYPE_OPTION},
     {"byte", BYTE_TYPE, 0},
     {"char", CHAR_TYPE, UNSIGNED_TYPE_OPTION},
     {"double", DOUBLE_TYPE, 0},
     {"error_status_t", ERROR_STATUS_TYPE, UNSIGNED_TYPE_OPTION},
     {"float", FLOAT_TYPE, 0},
     {"handle_t", HANDLE_TYPE, 0},
     {"hyper", HYPER_TYPE, SIGNED_TYPE_OPTION},
     {"int", INT_TYPE, SIGNED_TYPE_OPTION},
     {"__int32", INT32_TYPE, SIGNED_TYPE_OPTION},
     {"__int3264", INT32OR64_TYPE, SIGNED_TYPE_OPTION},
     {"__int64", INT64_TYPE, SIGNED_TYPE_OPTION},
     {"long", LONG_TYPE, SIGNED_TYPE_OPTION},
     {"short", SHORT_TYPE, SIGNED_TYPE_OPTION},
     {"small", SMALL_TYPE, SIGNED_TYPE_OPTION},
     {"void", VOID_TYPE, 0},
     {"wchar_t", WCHAR_TYPE, UNSIGNED_TYPE_OPTION},
     {NULL, 0, 0}
     };

void print_type(int tval)
{
   int i;
   
   for (i = 0; types[i].name != NULL; i++)
     {
	if (tval == types[i].value)
	  {
	     printf("%s", types[i].name);
	     return;
	  }
     }
   printf("unknown type");
}

int token_to_type(char* token)
{
   int i;
   
   for (i = 0; types[i].name != NULL; i++)
     {
	if (strcmp(types[i].name, token) == 0)
	  {
	     return(types[i].value);
	  }
     }
   return(0);
}
