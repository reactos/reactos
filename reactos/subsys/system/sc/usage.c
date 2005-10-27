#include "sc.h"

INT MainUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("\tSC is a command line program used for communicating with\n");
    dprintf("\tthe Service Control Manager and its services.\n");
    dprintf("USAGE:\n");
    dprintf("\tsc <server> [command] [service name] <option1> <option2>...\n");

    dprintf("\tThe optional parameter <server> has the form \"\\ServerName\"\n");
    dprintf("\tFurther help on commands can be obtained by typing: \"sc [command]\"\n");
    dprintf("\tService Commands:\n");
    dprintf("\t  query          : Queries the status for a service, or\n");
    dprintf("\t                   enumerates the status for types of services.\n");
    dprintf("\t  queryex        : Queries the extended status for a service, or\n");
//    dprintf("\t                   enumerates the status for types of services.\n");
    dprintf("\t  start          : Starts a service.\n");
    dprintf("\t  pause          : Sends a PAUSE control request to a service.\n");
    dprintf("\t  interrogate    : Sends a INTERROGATE control request to a service.\n");
//    dprintf("\t  continue       : Sends a CONTINUE control request to a service.\n");
    dprintf("\t  stop           : Sends a STOP request to a service.\n");
//    dprintf("\t  config         : Changes the configuration of a service (persistant).\n");
//    dprintf("\t  description    : Changes the description of a service.\n");
//    dprintf("\t  failure        : Changes the actions taken by a service upon failure.\n");
//    dprintf("\t  qc             : Queries the configuration information for a service.\n");
//    dprintf("\t  qdescription   : Queries the description for a service.\n");
//    dprintf("\t  qfailure       : Queries the actions taken by a service upon failure.\n");
    dprintf("\t  delete         : Deletes a service (from the registry).\n");
    dprintf("\t  create         : Creates a service. (adds it to the registry).\n");
    dprintf("\t  control        : Sends a control to a service.\n");
//    dprintf("\t  sdshow         : Displays a service's security descriptor.\n");
//    dprintf("\t  sdset          : Sets a service's security descriptor.\n");
//    dprintf("\t  GetDisplayName : Gets the DisplayName for a service.\n");
//    dprintf("\t  GetKeyName     : Gets the ServiceKeyName for a service.\n");
//    dprintf("\t  EnumDepend     : Enumerates Service Dependencies.\n");
//    dprintf("\n");
//    dprintf("\tService Name Independant Commands:\n");
//    dprintf("\t  boot           : (ok | bad) Indicates whether the last boot should\n");
//    dprintf("\t                   be saved as the last-known-good boot configuration\n");
//    dprintf("\t  Lock           : Locks the SCM Database\n");
//    dprintf("\t  QueryLock      : Queries the LockStatus for the SCM Database\n");

    return 0;
}


INT StartUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Starts a service running.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> start [service name] <arg1> <arg2> ...\n");

    return 0;
}


INT PauseUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Sends a PAUSE control request to a service.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> pause [service name]\n");

    return 0;
}

INT InterrogateUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Sends an INTERROGATE control request to a service.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> interrogate [service name]\n");

    return 0;
}


INT ContinueUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Sends an CONTINUE control request to a service.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> continue [service name]\n");

    return 0;
}

INT StopUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Sends an STOP control request to a service.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> stop [service name]\n");

    return 0;
}

INT ConfigUsage(VOID)
{
    dprintf("not yet implemented\n");

    return 0;
}


INT DescriptionUsage(VOID)
{
    dprintf("DESCRIPTION:\n");
    dprintf("        Sets the description string for a service.\n");
    dprintf("USAGE:\n");
    dprintf("        sc <server> description [service name]\n");

    return 0;
}
