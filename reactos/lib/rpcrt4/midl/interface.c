#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "midl.h"
#include "idl.tab.h"

static interface* current_interface;
static function* current_function;

void add_argument(int type, char* name)
{
   argument* arg;
   
   arg = malloc(sizeof(argument));
   
   arg->type = type;
   arg->name = strdup(name);
   
   arg->next = current_function->arguments;
   current_function->arguments = arg;
}

void start_interface(void)
{
   current_interface = (interface *)malloc(sizeof(interface));
}

void start_function(void)
{
   function* f;
   
   f = (function *)malloc(sizeof(function));
   
   f->arguments = NULL;
   
   f->next = current_interface->function_list;
   current_interface->function_list = f;
   current_function = f;
}

void end_function(int rtype, char* name)
{
   current_function->return_type = rtype;
   current_function->name = strdup(name);
}

void end_interface(char* name)
{
   function* cur;
   argument* a;
   
   printf("interface_name: %s\n", name);
   
   current_interface->name = strdup(name);
   cur = current_interface->function_list;
   while (cur != NULL)
     {
	print_type(cur->return_type);
	printf(" function_name: %s (\n", cur->name, cur);
	
	a = cur->arguments;
	while (a != NULL)
	  {
	     printf("\t");
	     print_type(a->type);
	     printf(" %s\n", a->name);
	     a = a->next;
	  }
	printf(")\n");
	
	cur = cur->next;
     }
}
