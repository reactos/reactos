#include "stdio.h"
#include "Windows.h"

int main()
{
    TCHAR Buffer = 0;
    DWORD Count = 0;

    //
    // We clear the mode, most importantly turn off ENABLE_ECHO_INPUT and ENABLE_LINE_INPUT
    // This is the same mode as that is set up by getch() when trying to get a char
    //
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),0);

    //
    // We read one char from the input and then return
    //
    ReadConsole(GetStdHandle(STD_INPUT_HANDLE),&Buffer,1,&Count,NULL);

    //
    // We print out this char as an int to show that infact a backspace does count as input
    //
    printf("You printed %c :: ", Buffer);
    printf("With a value %d :: ", Buffer);
    printf("Number of chars recieved %lu :: ", Count);
    printf("Char equal to backspace %d \n", (Buffer == '\b'));

    //
    // :)
    //
    return 0;
}
