/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

/* INCLUDES ******************************************************************/

#include "diskpart.h"

/* FUNCTIONS ******************************************************************/

VOID
ShowHeader(VOID)
{
    WCHAR szComputerName[MAX_STRING_SIZE];
    DWORD comp_size = MAX_STRING_SIZE;

    /* Get the name of the computer for us and change the value of comp_name */
    GetComputerNameW(szComputerName, &comp_size);

    /* TODO: Remove this section of code when program becomes stable enough for production use. */
    ConPuts(StdOut, L"\n*WARNING*: This program is incomplete and may not work properly.\n");

    /* Print the header information */
    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_APP_HEADER);
    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_APP_LICENSE);
    ConResPrintf(StdOut, IDS_APP_CURR_COMPUTER, szComputerName);
}

/*
 * RunScript(const char *filename):
 * opens the file, reads the contents, convert the text into readable
 * code for the computer, and then execute commands in order.
 */
EXIT_CODE
RunScript(LPCWSTR filename)
{
    FILE *script;
    WCHAR tmp_string[MAX_STRING_SIZE];
    EXIT_CODE Result;

    /* Open the file for processing */
    script = _wfopen(filename, L"r");
    if (script == NULL)
    {
        /* if there was problems opening the file */
        ConResPrintf(StdErr, IDS_ERROR_MSG_NO_SCRIPT, filename);
        return FALSE; /* if there is no script, exit the program */
    }

    /* Read and process the script */
    while (fgetws(tmp_string, MAX_STRING_SIZE, script) != NULL)
    {
        Result = InterpretScript(tmp_string);
        if (Result != EXIT_SUCCESS)
        {
            fclose(script);
            return (Result == EXIT_EXIT) ? EXIT_SUCCESS : Result;
        }
    }

    /* Close the file */
    fclose(script);

    return EXIT_SUCCESS;
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

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Sets the title of the program so the user will have an easier time
    determining the current program, especially if diskpart is running a
    script */
    K32LoadStringW(GetModuleHandle(NULL), IDS_APP_HEADER, appTitle, ARRAYSIZE(appTitle));
    SetConsoleTitleW(appTitle);

    /* Sets the timeout value to 0 just in case the user doesn't
    specify a value */
    timeout = 0;

    CreatePartitionList();
    CreateVolumeList();

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
                ConResPrintf(StdErr, IDS_ERROR_MSG_BAD_ARG, argv[index]);
                result = EXIT_SYNTAX;
                goto done;
            }

            /* Checks for the /? flag first since the program
            exits as soon as the usage list is shown. */
            if (_wcsicmp(tmpBuffer, L"?") == 0)
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
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
                ConResPrintf(StdErr, IDS_ERROR_MSG_BAD_ARG, tmpBuffer);
                result = EXIT_SYNTAX;
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

            result = RunScript(script);
            if (result != EXIT_SUCCESS)
                goto done;
        }
        else
        {
            /* Exit failure since the user wanted to run a script */
            ConResPrintf(StdErr, IDS_ERROR_MSG_NO_SCRIPT, script);
            result = EXIT_SYNTAX;
            goto done;
        }
    }

    /* Let the user know the program is exiting */
    ConResPuts(StdOut, IDS_APP_LEAVING);

done:
    DestroyVolumeList();
    DestroyPartitionList();

    return result;
}
