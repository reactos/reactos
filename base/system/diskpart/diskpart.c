/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.c
 * PURPOSE:         Manages all the partitions of the OS in
 *                  an interactive way
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
BOOL
run_script(LPCWSTR filename)
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
 * wmain():
 * Main entry point of the application.
 */
int wmain(int argc, const WCHAR *argv[])
{
    WCHAR szComputerName[MAX_STRING_SIZE];
    DWORD comp_size = MAX_STRING_SIZE;
    LPCWSTR file_name = NULL;
    int i;
    int timeout = 0;

    /* Get the name of the computer for us and change the value of comp_name */
    GetComputerName(szComputerName, &comp_size);

    /* TODO: Remove this section of code when program becomes stable enough for production use. */
    wprintf(L"\n*WARNING*: This program is incomplete and may not work properly.\n");

    /* Print the header information */
    PrintResourceString(IDS_APP_HEADER, DISKPART_VERSION);
    PrintResourceString(IDS_APP_LICENSE);
    PrintResourceString(IDS_APP_CURR_COMPUTER, szComputerName);

    /* Process arguments */
    for (i = 1; i < argc; i++)
    {
        if ((argv[i][0] == L'-') || (argv[i][0] == L'/'))
        {
            if (wcsicmp(&argv[i][1], L"s") == 0)
            {
                /*
                 * Get the file name only if there is at least one more
                 * argument and it is not another option
                 */
                if ((i + 1 < argc) &&
                    (argv[i + 1][0] != L'-') &&
                    (argv[i + 1][0] != L'/'))
                {
                    /* Next argument */
                    i++;

                    /* Get the file name */
                    file_name = argv[i];
                }
            }
            else if (wcsicmp(&argv[i][1], L"t") == 0)
            {
                /*
                 * Get the timeout value only if there is at least one more
                 * argument and it is not another option
                 */
                if ((i + 1 < argc) &&
                    (argv[i + 1][0] != L'-') &&
                    (argv[i + 1][0] != L'/'))
                {
                    /* Next argument */
                    i++;

                    /* Get the timeout value */
                    timeout = _wtoi(argv[i]);
                }
            }
            else if (wcscmp(&argv[i][1], L"?") == 0)
            {
                PrintResourceString(IDS_APP_USAGE);
                return EXIT_SUCCESS;
            }
        }
    }

    /* Run the script if we got a script name or call the interpreter otherwise */
    if (file_name != NULL)
    {
        if (run_script(file_name) == FALSE)
            return EXIT_FAILURE;
    }
    else
    {
        interpret_main();
    }

    /* Let the user know the program is exiting */
    PrintResourceString(IDS_APP_LEAVING);

    return EXIT_SUCCESS;
}
