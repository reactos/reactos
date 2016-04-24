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

#include <stdlib.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>

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

VOID
ShowHeader(VOID)
{
    WCHAR szComputerName[MAX_STRING_SIZE];
    DWORD comp_size = MAX_STRING_SIZE;

    /* Get the name of the computer for us and change the value of comp_name */
    GetComputerNameW(szComputerName, &comp_size);

    /* TODO: Remove this section of code when program becomes stable enough for production use. */
    wprintf(L"\n*WARNING*: This program is incomplete and may not work properly.\n");

    /* Print the header information */
    wprintf(L"\n");
    PrintResourceString(IDS_APP_HEADER);
    wprintf(L"\n");
    PrintResourceString(IDS_APP_LICENSE);
    PrintResourceString(IDS_APP_CURR_COMPUTER, szComputerName);
}

/*
 * RunScript(const char *filename):
 * opens the file, reads the contents, convert the text into readable
 * code for the computer, and then execute commands in order.
 */
BOOL
RunScript(LPCWSTR filename)
{
    FILE *script;
    WCHAR tmp_string[MAX_STRING_SIZE];

    /* Open the file for processing */
    script = _wfopen(filename, L"r");
    if (script == NULL)
    {
        /* if there was problems opening the file */
        PrintResourceString(IDS_ERROR_MSG_NO_SCRIPT, filename);
        return FALSE; /* if there is no script, exit the program */
    }

    /* Read and process the script */
    while (fgetws(tmp_string, MAX_STRING_SIZE, script) != NULL)
    {
        if (InterpretScript(tmp_string) == FALSE)
        {
            fclose(script);
            return FALSE;
        }
    }

    /* Close the file */
    fclose(script);

    return TRUE;
}

/*
 * wmain():
 * Main entry point of the application.
 */
int wmain(int argc, const LPWSTR argv[])
{
    LPCWSTR script = NULL;
    LPCWSTR tmpBuffer = NULL;
    WCHAR appTitle[50];
    int index, timeout;
    int result = EXIT_SUCCESS;

    /* Sets the title of the program so the user will have an easier time
    determining the current program, especially if diskpart is running a
    script */
    LoadStringW(GetModuleHandle(NULL), IDS_APP_HEADER, (LPWSTR)appTitle, 50);
    SetConsoleTitleW(appTitle);

    /* Sets the timeout value to 0 just in case the user doesn't
    specify a value */
    timeout = 0;

    CreatePartitionList();

    /* If there are no command arguments, then go straight to the interpreter */
    if (argc < 2)
    {
        ShowHeader();
        InterpretMain();
    }
    /* If there are command arguments, then process them */
    else
    {
        for (index = 1; index < argc; index++)
        {
            /* checks for flags */
            if ((argv[index][0] == '/')||
                (argv[index][0] == '-'))
            {
                tmpBuffer = argv[index] + 1;
            }
            else
            {
                /* If there is no flag, then return an error */
                PrintResourceString(IDS_ERROR_MSG_BAD_ARG, argv[index]);
                result = EXIT_FAILURE;
                goto done;
            }

            /* Checks for the /? flag first since the program
            exits as soon as the usage list is shown. */
            if (_wcsicmp(tmpBuffer, L"?") == 0)
            {
                PrintResourceString(IDS_APP_USAGE);
                result = EXIT_SUCCESS;
                goto done;
            }
            /* Checks for the script flag */
            else if (_wcsicmp(tmpBuffer, L"s") == 0)
            {
                if ((index + 1) < argc)
                {
                    index++;
                    script = argv[index];
                }
            }
            /* Checks for the timeout flag */
            else if (_wcsicmp(tmpBuffer, L"t") == 0)
            {
                if ((index + 1) < argc)
                {
                    index++;
                    timeout = _wtoi(argv[index]);

                    /* If the number is a negative number, then
                    change it so that the time is executed properly. */
                    if (timeout < 0)
                        timeout = 0;
                }
            }
            else
            {
                /* Assume that the flag doesn't exist. */
                PrintResourceString(IDS_ERROR_MSG_BAD_ARG, tmpBuffer);
                result = EXIT_FAILURE;
                goto done;
            }
        }

        /* Shows the program information */
        ShowHeader();

        /* Now we process the filename if it exists */
        if (script != NULL)
        {
            /* if the timeout is greater than 0, then assume
            that the user specified a specific time. */
            if (timeout > 0)
                Sleep(timeout * 1000);

            if (RunScript(script) == FALSE)
            {
                result = EXIT_FAILURE;
                goto done;
            }
        }
        else
        {
            /* Exit failure since the user wanted to run a script */
            PrintResourceString(IDS_ERROR_MSG_NO_SCRIPT, script);
            result = EXIT_FAILURE;
            goto done;
        }
    }

    /* Let the user know the program is exiting */
    PrintResourceString(IDS_APP_LEAVING);

done:
    DestroyPartitionList();

    return result;
}
