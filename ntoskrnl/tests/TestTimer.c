#include <stdio.h>
#include <conio.h>
#include <windows.h>

void main ( int argc, char** argv, char** environ )
{
 LARGE_INTEGER liFrequency;
 LARGE_INTEGER liStartTime;
 LARGE_INTEGER liCurrentTime;

 QueryPerformanceFrequency ( &liFrequency );
 printf ( "HIGH RESOLUTION PERFOMANCE COUNTER Frequency = %I64d CLOCKS IN SECOND\n",
	     liFrequency.QuadPart );


 if (liFrequency.QuadPart == 0)
 {
	printf("Your computer does not support High Resolution Performance counter\n");
	return;
 }

 printf ( "Press <ENTER> to start test...\n" );
 getchar();

 printf ( "\nPress any key to quit test\n\n" );
 QueryPerformanceCounter ( &liStartTime );
 for (;;)
 {
	QueryPerformanceCounter ( &liCurrentTime );
	printf("Elapsed Time : %8.6f mSec\r",
          ((double)( (liCurrentTime.QuadPart - liStartTime.QuadPart)* (double)1000.0/(double)liFrequency.QuadPart )) );
	if (_kbhit())
		break;
 }


}
