#ifndef __MIDL_H
#define __MIDL_H

#include "types.h"

typedef struct _argument
{
   int type;
   char* name;
   struct _argument* next;
} argument;

typedef struct _function
{
   int return_type;
   char* name;
   argument* arguments;
   struct _function* next;
} function;

typedef struct
{
   char* name;
   function* function_list;
} interface;

void set_uuid(char* uuid);
void set_version(int major, int minor);
void set_pointer_default(int option);

void start_interface(void);
void end_interface(char* name);
void start_function(void);
void end_function(int rtype, char* name);
void add_argument(int type, char* name);

#endif __MIDL_H
