/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/usage.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

INT MainUsage(VOID)
{
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
//    _T("\t                   enumerates the status for types of services.\n"
    _T("\t  start          : Starts a service.\n")
    _T("\t  pause          : Sends a PAUSE control request to a service.\n")
    _T("\t  interrogate    : Sends a INTERROGATE control request to a service.\n")
    _T("\t  continue       : Sends a CONTINUE control request to a service.\n")
    _T("\t  stop           : Sends a STOP request to a service.\n")
//    "\t  config         : Changes the configuration of a service (persistant).\n"
//    "\t  description    : Changes the description of a service.\n"
//    "\t  failure        : Changes the actions taken by a service upon failure.\n"
//    "\t  qc             : Queries the configuration information for a service.\n"
//    "\t  qdescription   : Queries the description for a service.\n"
//    "\t  qfailure       : Queries the actions taken by a service upon failure.\n"
    _T("\t  delete         : Deletes a service (from the registry).\n")
    _T("\t  create         : Creates a service. (adds it to the registry).\n")
    _T("\t  control        : Sends a control to a service.\n"));
//    "\t  sdshow         : Displays a service's security descriptor.\n")
//    "\t  sdset          : Sets a service's security descriptor.\n")
//    "\t  GetDisplayName : Gets the DisplayName for a service.\n")
//    "\t  GetKeyName     : Gets the ServiceKeyName for a service.\n")
//    "\t  EnumDepend     : Enumerates Service Dependencies.\n")
//    "\n")
//    "\tService Name Independant Commands:\n")
//    "\t  boot           : (ok | bad) Indicates whether the last boot should\n")
//    "\t                   be saved as the last-known-good boot configuration\n")
//    "\t  Lock           : Locks the SCM Database\n")
//    "\t  QueryLock      : Queries the LockStatus for the SCM Database\n")

    return 0;
}


INT StartUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Starts a service running.\n")
                _T("USAGE:\n")
                _T("        sc <server> start [service name] <arg1> <arg2> ...\n"));

    return 0;
}


INT PauseUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends a PAUSE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> pause [service name]\n"));

    return 0;
}

INT InterrogateUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends an INTERROGATE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> interrogate [service name]\n"));

    return 0;
}


INT StopUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends an STOP control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> stop [service name]\n"));

    return 0;
}

INT ContinueUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sends an CONTINUE control request to a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> continue [service name]\n"));

    return 0;
}


INT ConfigUsage(VOID)
{
    _tprintf(_T("not yet implemented\n"));

    return 0;
}


INT DescriptionUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Sets the description string for a service.\n")
                _T("USAGE:\n")
                _T("        sc <server> description [service name]\n"));

    return 0;
}

INT DeleteUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("        Deletes a service entry from the registry.\n")
                _T("        If the service is running, or another process has an\n")
                _T("        open handle to the service, the service is simply marked\n")
                _T("        for deletion.\n")
                _T("USAGE:\n")
                _T("        sc <server> delete [service name]\n"));

    return 0;
}

INT CreateUsage(VOID)
{
    _tprintf(_T("DESCRIPTION:\n")
                _T("Creates a service entry in the registry and Service Database.\n")
                _T("        If the service is running, or another process has an\n")
                _T("        open handle to the service, the service is simply marked\n")
                _T("        for deletion.\n")
                _T("USAGE:\n")
                _T("        sc <server> delete [service name]\n"));

    return 0;
}
