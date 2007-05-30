Routines to handle .inf files.

This library is used to share .inf handling code between build tools and
ReactOS code. Two versions are built, "inflib_host" (for use by build tools)
and "inflib" (for use by ReactOS code). Both depend on the same core source,
with a wrapper for the appropriate interface.
Most of the differences between the host and the ReactOS environment are
abstracted away in builddep.h. Of particular note is that the host version
uses Ansi characters while the ReactOS version uses Unicode. So, the core
source uses TCHARs. builddep.h depends on a preprocessor variable INFLIB_HOST
which is defined when building the host version (inflib.mak) but not defined
when building the ReactOS version (inflib.xml).
The wrappers have "host" or "ros" in their filename. The library interface is
"infhost.h" for the host version, "infros.h" for the ReactOS version.
