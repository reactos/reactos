#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midl.h"

struct _type;

typedef struct _struct_member
{
   char* name;
   unsigned int value;
   struct _struct_member* next;
} struct_member;

typedef struct
{
   struct_member* member_list;
   char* tag;
   struct _type* type_value;
} struct_type;

typedef struct _type
{
   char* name;
   unsigned int value;
   unsigned int default_sign;
   struct_type* struct_desc;         
} type;

static struct_type struct_types[255];

static int next_struct_slot_free = 0;
static struct_type* current_struct;

static type types[255] = {
     {NULL, 0, 0},
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

static int next_free_slot = 18;

unsigned int struct_to_type(char* tag)
{
   int i;
   
   for (i = 0; i < next_struct_slot_free; i++)
     {
	if (strcmp(tag, struct_types[i].tag) == 0)
	  {
	     return(struct_types[i].type_value->value);
	  }
     }
   return(0);
}

void start_struct(char* tag)
{
   char* name;
   
   if (tag == NULL)
     {
	tag = malloc(255);
	sprintf(tag, "__unnamed_struct%d", next_struct_slot_free);
     }
   
   name = malloc(strlen("struct ") + strlen(tag) + 1);
   strcpy(name, "struct ");
   strcat(name, tag);
      
   struct_types[next_struct_slot_free].tag = strdup(tag);
   struct_types[next_struct_slot_free].member_list = NULL;
   current_struct = &struct_types[next_struct_slot_free];
   
   types[next_free_slot].name = name;
   types[next_free_slot].value = next_free_slot << 8;
   types[next_free_slot].default_sign = 0;
   types[next_free_slot].struct_desc = 
     &struct_types[next_struct_slot_free];
   
   struct_types[next_struct_slot_free].type_value = &types[next_free_slot];
   
   next_struct_slot_free++;
   next_free_slot++;
}

void add_struct_member(char* name, unsigned int type)
{
   struct_member* member;
   
   member = malloc(sizeof(struct_member));
   
   member->name = strdup(name);
   member->value = type;
   member->next = current_struct->member_list;
   current_struct->member_list = member;
}

unsigned int end_struct(void)
{
   int n;
   struct_member* cur;
   
   printf("Defining struct %s {\n", current_struct->tag);
   
   cur = current_struct->member_list;
   while (cur != NULL)
     {
	print_type(cur->value);
	printf(" %s\n", cur->name);
	
	cur = cur->next;
     }
   printf("}\n");
   
   n = current_struct->type_value->value;
   current_struct = NULL;
   return(n);
}

void add_typedef(char* name, int type)
{
   printf("Adding typedef %s to ", name);
   print_type(type);
   printf("\n");
   
   types[next_free_slot].name = strdup(name);
   types[next_free_slot].value = type;
   types[next_free_slot].default_sign = 0;
   next_free_slot++;
}

void print_type(int tval)
{
   int i;
   
   for (i = 1; i < next_free_slot; i++)
     {
	if ((tval & BASE_TYPE_MASK) == types[i].value)
	  {
	     if (tval & UNSIGNED_TYPE_OPTION)
	       {
		  printf("unsigned ");
	       }
	     if (tval & SIGNED_TYPE_OPTION)
	       {
		  printf("signed ");
	       }
	     printf("%s", types[i].name);
	     if (tval & POINTER_TYPE_OPTION)
	       {
		  printf("*");
	       }
	     return;
	  }
     }
   printf("unknown type");
}

int token_to_type(char* token)
{
   int i;
   
   for (i = 1; i < next_free_slot; i++)
     {
	if (strcmp(types[i].name, token) == 0)
	  {
	     return(types[i].value);
	  }
     }
   return(0);
}
