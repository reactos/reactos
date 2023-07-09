
/******************************************************************************

                    D E B U G   O U T P U T   H E A D E R

    Name:       debugout.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for debugout.h

******************************************************************************/


extern  INT     DebugLevel;
extern  BOOL    DebugFile;



VOID PERROR (LPTSTR format, ...);


VOID PINFO (LPTSTR format, ...);
