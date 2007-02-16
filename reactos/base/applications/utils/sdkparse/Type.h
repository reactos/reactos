// Type.h

#ifndef TYPE_H
#define TYPE_H

typedef enum
{
	T_UNKNOWN = -1,
	T_IGNORED_STATEMENT,
	T_TIDENT,
	T_MACRO,
	T_DEFINE,
	T_VARIABLE,
	T_FUNCTION,
	T_FUNCTION_PTR,
	T_IF,
	T_WHILE,
	T_DO,
	T_STRUCT
} Type;

#endif//TYPE_H
