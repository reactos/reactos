/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

/* INCLUDES ******************************************************************/
#include "diskpart.h"

/* FUNCTIONS ******************************************************************/

VOID
PrintResourceString(INT resID, ...)
{
    WCHAR szMsg[3072];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadStringW(GetModuleHandle(NULL), resID, szMsg, 3072);
    vwprintf(szMsg, arg_ptr);
    va_end(arg_ptr);
}


/*
 * run_script(const char *filename):
 * opens the file, reads the contents, convert the text into readable
 * code for the computer, and then execute commands in order.
 */
BOOL run_script(LPCWSTR filename)
{
    FILE *script_file;
    WCHAR tmp_string[MAX_STRING_SIZE];

    /* Open the file for processing */
    script_file = _wfopen(filename, L"r");
    if (script_file == NULL)
    {
        /* if there was problems opening the file */
        PrintResourceString(IDS_ERROR_MSG_NO_SCRIPT, filename);
        return FALSE; /* if there is no script, exit the program */
    }

    /* Read and process the script */
    while (fgetws(tmp_string, MAX_STRING_SIZE, script_file) != NULL)
    {
        if (interpret_script(tmp_string) == FALSE)
            return FALSE;
    }

    /* Close the file */
    fclose(script_file);

    return TRUE;
}

/*
 * main():
 * Main entry point of the application.
 */
int wmain(int argc, const WCHAR *argv[])
{
    /* Gets the current name of the computer */
    WCHAR comp_name[MAX_STRING_SIZE]; //used to store the name of the computer */
    DWORD comp_size = MAX_STRING_SIZE; // used for the char size of comp_name */
    BOOL interpreter_running = TRUE; //used for the main program loop */

    /* Get the name of the computer for us and change the value of comp_name */
    GetComputerName(comp_name, &comp_size);

    /* TODO: Remove this section of code when program becomes stable enough for production use. */
    wprintf(L"\n*WARNING*: This program is incomplete and may not work properly.\n");

    /* Print the header information */
    PrintResourceString(IDS_APP_HEADER, DISKPART_VERSION);
    PrintResourceString(IDS_APP_LICENSE);
    PrintResourceString(IDS_APP_CURR_COMPUTER, comp_name);

    /* Find out if the user is loading a script */
    if (argc >= 2)
    {
        /* if there are arguments when starting the program
           determine if the script flag is enabled */
        if ((wcsicmp(argv[1], L"/s") == 0) ||
            (wcsicmp(argv[1], L"-s") == 0))
        {
            /* see if the user has put anything after the script flag; if not,
               then it doesn't run the run_script() command. */
            /* Alternative comment: Fail if the script name is missing */
            if (argc == 3)
                return EXIT_FAILURE;

            interpreter_running = run_script(argv[2]);
        }
    }

    /* the main program loop */
    while (interpreter_running)
    {
        interpreter_running = interpret_main();
    }

    /* Let the user know the program is exiting */
    PrintResourceString(IDS_APP_LEAVING);

    return EXIT_SUCCESS;
}
