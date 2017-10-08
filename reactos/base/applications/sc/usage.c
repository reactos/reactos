/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/usage.c
 * PURPOSE:     display usage info
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

#include <conio.h>

VOID MainUsage(VOID)
{
    INT c;

    _tprintf(_T("DESCRIPTION:\n")
    _T("\tSC is a command line program used for communicating with\n")
    _T("\tthe Service Control Manager and its services.\n")
    _T("USAGE:\n")
    _T("\tsc <server> [command] [service name] <option1> <option2>...\n")

    _T("\tThe optional parameter <server> has the form \"\\ServerName\"\n")
    _T("\tFurther help on commands can be obtained by typing: \"sc [command]\"\n")
    _T("\tService Commands:\n")
    _T("\t  query          : Queries the status for a service, or\n")
    _T("\t                   enumerates the status for types of services.\n")
    _T("\t  queryex        : Queries the extended status for a service, or\n")
    _T("\t                   enumerates the status for types of services.\n")
    _T("\t  start          : Starts a service.\n")
    _T("\t  pause          : Sends a PAUSE control request to a service.\n")
    _T("\t  interrogate    : Sends a INTERROGATE control request to a service.\n")
    _T("\t  continue       : Sends a CONTINUE control request to a service.\n")
    _T("\t  stop           : Sends a STOP request to a service.\n")
    _T("\t  config         : Changes the configuration of a service (persistent).\n")
    _T("\t  description    : Changes the description of a service.\n")
    _T("\t  failure        : Changes the actions taken by a service upon failure.\n")
    _T("\t  qc             : Queries the configuration information for a service.\n")
    _T("\t  qdescription   : Queries the description for a service.\n")
    _T("\t  qfailure       : Queries the actions taken by a service upon failure.\n")
    _T("\t  delete         : Deletes a service (from the registry).\n")
    _T("\t  create         : Creates a service (adds it to the registry).\n")
    _T("\t  control        : Sends a control to a service.\n")
    _T("\t  sdshow         : Displays a service's security descriptor.\n")
    _T("\t  sdset          : Sets a service's security descriptor.\n")
    _T("\t  GetDisplayName : Gets the DisplayName for a service.\n")
    _T("\t  GetKeyName     : Gets the ServiceKeyName for a service.\n")
    _T("\t  EnumDepend     : Enumerates Service Dependencies.\n"));
//    "\n")
//    "\tService Name Independent Commands:\n")
//    "\t  boot           : (ok | bad) Indicates whether the last boot should\n")
//    "\t                   be saved as the last-known-good boot configuration\n")
//    "\t  Lock           : Locks the SCM Database\n")
//    "\t  QueryLock      : Queries the LockStatus for the SCM Database\n")

    _tprintf(_T("\nWould you like to see help for the QUERY and QUERYEX commands? [ y | n ]: "));
    c = _getch(); // _gettch isn't defined in our tchar.h
    _tprintf(_T("%c\n"), c);
    if (tolower(c) == 'y')
    {
        _tprintf(_T("QUERY and QUERYEX OPTIONS :\n")
        _T("        If the query command is followed by a service name, the status\n")
        _T("        for that service is returned.  Further options do not apply in\n")
        _T("        this case.  If the query command is followed by nothing or one of\n")
        _T("        the options listed below, the services are enumerated.\n")
        _T("    type=    Type of services to enumerate (driver, service, interact, all)\n")
        _T("             (default = service)\n")
        _T("    state=   State of services to enumerate (inactive, all)\n")
        _T("             (default = active)\n")
/*
        _T("    bufsize= The size (in bytes) of the enumeration buffer\n")
        _T("             (default = 4096)\n")
        _T("    ri=      The resume index number at which to begin the enumeration\n")
        _T("             (default = 0)\n")
        _T("    group=   Service group to enumerate\n")
        _T("             (default = all groups)\n")
*/
        _T("SYNTAX EXAMPLES\n")
        _T("sc query                - Enumerates status for active services & drivers\n")
        _T("sc query messenger      - Displays status for the messenger service\n")
        _T("sc queryex messenger    - Displays extended status for the messenger service\n")
        _T("sc query type= driver   - Enumerates only active drivers\n")
        _T("sc query type= service  - Enumerates only Win32 services\n")
        _T("sc query state= all     - Enumerates all services & drivers\n")
//        _T("sc query bufsize= 50    - Enumerates with a 50 byte buffer.\n")
//        _T("sc query ri= 14         - Enumerates with resume index = 14\n")
//        _T("sc queryex group= ""    - Enumerates active services not in a group\n")
        _T("sc query type= service type= interact - Enumerates all interactive services\n")
        _T("sc query type= driver group= NDIS     - Enumerates all NDIS drivers\n"));
    }
}


VOID StartUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Starts a service running.\n")
                _T("USAGE:\n")
                _T("        sc <server> start [service name] <arg1> <arg2> ...\n"));
}


VOID PauseUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends a PAUSE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> pause [service name]\n"));
}

VOID InterrogateUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends an INTERROGATE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> interrogate [service name]\n"));
}


VOID StopUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends a STOP control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> stop [service name]\n"));
}

VOID ContinueUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends a CONTINUE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> continue [service name]\n"));
}

VOID DeleteUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Deletes a service entry from the registry.\n")
                _T("        If the service is running, or another process has an\n")
                _T("        open handle to the service, the service is simply marked\n")
                _T("        for deletion.\n")
                _T("USAGE:\n")
                _T("        sc <server> delete [service name]\n"));
}

VOID CreateUsage(VOID)
{
    _tprintf(_T("Creates a service entry in the registry and Service Database.\n")
                _T("SYNTAX:\n")
                _T("sc create [service name] [binPath= ] <option1> <option2>...\n")
                _T("CREATE OPTIONS:\n")
                _T("NOTE: The option name includes the equal sign.\n")
                _T(" type= <own|share|interact|kernel|filesys|rec>\n")
                _T("       (default = own)\n")
                _T(" start= <boot|system|auto|demand|disabled>\n")
                _T("       (default = demand)\n")
                _T(" error= <normal|severe|critical|ignore>\n")
                _T("       (default = normal)\n")
                _T(" binPath= <BinaryPathName>\n")
                _T(" group= <LoadOrderGroup>\n")
                _T(" tag= <yes|no>\n")
                _T(" depend= <Dependencies(separated by / (forward slash))>\n")
                _T(" obj= <AccountName|ObjectName>\n")
                _T("       (default = LocalSystem)\n")
                _T(" DisplayName= <display name>\n")
                _T(" password= <password>\n"));
}

VOID ControlUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Sends a CONTROL control request to a service.\n")
             _T("USAGE:\n")
             _T("        sc <server> control [service name] <value>\n"));
}

VOID SdShowUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Displays a service's security descriptor in SDDL format.\n")
             _T("USAGE:\n")
             _T("        sc <server> sdshow <service name>\n"));
}

VOID SdSetUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Sets a service's security descriptor.\n")
             _T("USAGE:\n")
             _T("        sc <server> sdset <service name> <SD in SDDL format>\n"));
}

VOID QueryConfigUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Queries the configuration information for a service.\n")
             _T("USAGE:\n")
             _T("        sc <server> qc [service name] <bufferSize>\n"));
}

VOID QueryDescriptionUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Retrieves the description string of a service.\n")
             _T("USAGE:\n")
             _T("        sc <server> qdescription [service name] <bufferSize>\n"));
}

VOID QueryFailureUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Retrieves the actions performed on service failure.\n")
             _T("USAGE:\n")
             _T("        sc <server> qfailure [service name] <bufferSize>\n"));
}

VOID SetDescriptionUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Sets the description string of a service.\n")
             _T("USAGE:\n")
             _T("        sc <server> description [service name] [description]\n"));
}

VOID SetConfigUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Modifies a service entry in the registry and Service Database.\n")
             _T("USAGE:\n")
             _T("        sc <server> config [service name] <option1> <option2>...\n")
             _T("CONFIG OPTIONS:\n")
             _T("        NOTE: The option name includes the equal sign.\n")
             _T("        type= <own|share|interact|kernel|filesys|rec>\n")
             _T("        start= <boot|system|auto|demand|disabled>\n")
             _T("        error= <normal|severe|critical|ignore>\n")
             _T("        binPath= <BinaryPathName>\n")
             _T("        group= <LoadOrderGroup>\n")
             _T("        tag= <yes|no>\n")
             _T("        depend= <Dependencies(separated by / (forward slash))>\n")
             _T("        obj= <AccountName|ObjectName>\n")
             _T("        DisplayName= <display name>\n")
             _T("        password= <password>\n"));
}

VOID SetFailureUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Changes the actions upon failure.\n")
             _T("USAGE:\n")
             _T("        sc <server> failure [service name] <option1> <option2>...\n")
             _T("\n")
             _T("OPTIONS:\n")
             _T("        reset= <Length of period of no failures (in seconds)\n")
             _T("                after which to reset the failure count to 0 (may be INFINITE)>\n")
             _T("               (Must be used in conjunction with actions= )\n")
             _T("        reboot= <Message broadcast before rebooting on failure>\n")
             _T("        command= <Command line to be run on failure>\n")
             _T("        actions= <Failure actions and their delay time (in milliseconds),\n")
             _T("                  separated by / (forward slash) -- e.g., run/5000/reboot/800\n")
             _T("                  Valid actions are <run|restart|reboot> >\n")
             _T("                 (Must be used in conjunction with the reset= option)\n"));
}

VOID GetDisplayNameUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Gets the display name associated with a particular service.\n")
             _T("USAGE:\n")
             _T("        sc <server> GetDisplayName <service key name> <bufsize>\n"));
}

VOID GetKeyNameUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Gets the key name associated with a particular service, using the\n")
             _T("        display name as input.\n")
             _T("USAGE:\n")
             _T("        sc <server> GetKeyName <service display name> <bufsize>\n"));
}

VOID EnumDependUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
             _T("        Enumerates te Services that are dependent on this one.\n")
             _T("USAGE:\n")
             _T("        sc <server> EnumDepend <service display name> <bufsize>\n"));
}
