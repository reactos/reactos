#include <stdio.h>
#include <stdlib.h>

#include "ddrawtest.h"

INT NumTests(void);

int main(int argc, char *argv[])
{
	INT Num = NumTests();
	INT i, j;
	INT passed, failed, opassed, ofailed;

	opassed = 0;
	ofailed = 0;
	printf("DirectDraw tests\n");
	if (argc > 1)
	{
		for (i = 1; i < argc; i++)
		{
			for (j = 0; j < NumTests(); j++)
			{
				if (_stricmp(argv[i], TestList[j].Test) == 0)
				{
					passed = 0;
					failed = 0;
					TestList[j].Proc(&passed, &failed);
					opassed += passed;
					ofailed += failed;
					printf(" tests: %d, passed: %d, failed: %d\n\n", passed+failed, passed, failed);
				}
			}
		}
	}
	else
	{
		for (i = 0; i < Num; i++)
		{
			passed = 0;
			failed = 0;
			printf("Test: %s\n", TestList[i].Test);
			TestList[i].Proc(&passed, &failed);
			opassed += passed;
			ofailed += failed;
			printf(" tests: %d, passed: %d, failed: %d\n\n", passed+failed, passed, failed);
		}
	}
	printf("\nOverall tests: %d, passed: %d, failed: %d\n", opassed+ofailed, opassed, ofailed);

	return ofailed;
}
