////////////////////////////////////////////////
//
// package.hpp
//				   Header for the script stuff
////////////////////////////////////////////////

#include <vector>
#include <string>

using namespace std;


/* Structs */

typedef struct
{
  string name;
  int start, end;

} SUB;

typedef struct
{
  vector<string> code;
  vector<SUB> subs;

} SCRIPT;


/* Prototypes */

int RPS_Load (SCRIPT** script, const char* path);
int RPS_Execute (SCRIPT* script, const char* function);
int RPS_getVar (const char* name);
void RPS_Clear (SCRIPT* script);


/* Callbacks */

typedef int (*FUNC_PROC)(int, char**); // function callback


/* Function table */

typedef struct
{
  char* name;
  FUNC_PROC function;
} FUNC_TABLE;

// very function is listed in there 
extern const FUNC_TABLE FuncTable[];

// count of functions
#define FUNC_COUNT 3


/* For the helper-funtions */

#define STR_NO    0x1;
#define STR_ONLY  0x0;
#define STR_YES   0x2;

// ^^ I would write down here that they 
// mean but I don't know anymore myself :O
