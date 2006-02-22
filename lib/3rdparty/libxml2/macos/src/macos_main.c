#include <GUSICommandLine.h>
#include <stdlib.h>

#undef main

DECLARE_MAIN(test)

REGISTER_MAIN_START
REGISTER_MAIN(test)
REGISTER_MAIN_END

int main()
{
	(void) exec_commands();
	
	return 0;
}