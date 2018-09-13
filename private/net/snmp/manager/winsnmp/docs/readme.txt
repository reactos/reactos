NetPlus WinSNMP Developer Kit                    August 8, 1998

Current version of ACE*COMM's wsnmp32.dll:

	String value: v2.32.19980808
	Binary value: 2.0.0.25

Change log:

v2.32.19980808: No external changes...function error
		reporting mechanisms normalized across
		all internal modules.

v2.32.19980705: Fixed an internal error return test that
                would have caused the SnmpCreateVbl()
                function to encounter an access violatoin
                when passed a varbind with the (obsolete)
                SNMP_SYNTAX_BITS syntax value.

v2.32.19980622: Improved some internal error checking.
                Improved timeout interval handling for
                intervals larger than 429,496,729 centiseconds.

v2.32.19980608: Fixed some flawed logic concerning freeing
		entities which are acting as agents.
                Repaired a memory leak in the implementation
                of the RFC 2089 automatic SNMPv2 to SNMPv1
                trap mapping algorithm.

v2.32.19980522: Modified SnmpContextToStr() code to return the
                "raw" context value if the function is called
                in SNMPAPI_TRANSLATED mode and the subject
                hContext had been created in SNMPAPI_UNTRANSLATED
                (v1 or v2) mode.

v2.32.19980507: Fixed a problem which caused some trap sending
                apps to fail if they issued multiple consecutive
                traps to a given HSNMP_ENTITY value.  Added
                error return (SNMPAPI_TL_OTHER) to SnmpSendMsg()
                when WinSock sendto() fails for any reason.

v2.32.19980424: Internal integrity improvements in low-level
                parsing routines...accept case when IPX header
                indicates packet length greater than actual
                SNMP message length.

v2.32.19980416: Fixed a problem in the interface with the
                SNMPTRAP service that prevented non-admin users
                from accessing the service.  See the explanatory
                note in a separate section later in this file for
                more details.  Also, made some (minor) reliability
                and performance improvements to SNMPTRAP itself
                (its version is now 4.0.1381.4).

v2.32.19980402: Fixed a problem that caused a comm failure on Win95
                systems *only* when using WinSock v2 ws2_32.dll) with
                *both* TCP/IP *and* IPX protocols installed.

v2.32.19980330: Fixed a failure in SnmpCreateSession() to return
                SNMPAPI_HWND_INVALID when called in SnmpOpen() mode
                (i.e., fCallBack is NULL) and the hWnd parameter
                failed the Win32 IsWindow() check.

v2.32.19980323: Made improvements to session notification dispatching
                algorithm for improved performance and reliability
                specifically related to using callback notification
                mode in multi-threaded/multi-session applications.

v2.32.19980318: Fixed SnmpOidToStr() to return the correct byte count
                upon successful completion...previously it was not
                counting the terminating NULL byte of the output
                string as required by the WinSNMP spec...most apps
                tested only for the SNMPAPI_FAILURE case, so there
                were no field reports of this discrepancy.  Also,
                added the SNMPAPI_NOOP extended error code return
                to cover the case of a NULL scrOID input parameter.
                Again, there had been no field reports of this
                deficiency.

v2.32.19980309: Fixed a bug and made a modest performance improvement
                in SnmpCancelMsg().

v2.32.19980216: "Fix" added to enable SNMP request/response messages
                over IPX for Win95.  Required adding an otherwise
                ill-advised bind() call.

v2.32.19980113: Improvements to internal message buffer management
                scheme (to optimize for early release of resources
                used for out-going messages that will not receive
                a response (ie., traps and response PDUs).

                Simplification of the housekeeping timer thread.

                Rationalization of the method of invoking a
                session's callback function that prevents multiple
                instances of the callback being invoked concurrently.

v2.32.19971222: Improvements to internal message memory allocation
                scheme (to handle large numbers of outstanding messages
                more efficiently).

v2.32.19971216:	Improvements to worker thread termination logic.
------------------------------------------------------------------------
Additional WinSNMP-related files may be freely obtained from the file
download site, www.winsnmp.com.
------------------------------------------------------------------------
If you develop WinSNMP management applications or agents and would like
to participate in a co-marketing program with ACE*COMM, please contact
Ben Gray, bgray@acecomm.com.
------------------------------------------------------------------------
If you are new to WinSNMP development, please read the following
documents before proceeding, in the following order:

	- winsnmp2.txt
	- winsnmp.doc

The first is the WinSNMP v2.0 Addendum.  It clarifies certain aspects
of WinSNMP that were ambiguous in the previous version (v1.1a, defined
in winsnmp.doc) of the API and it describes the five new functions
added for v2.0.

This version of ACE*COMM's NetPlus WinSNMP implementation for Win32
fully implements WinSNMP v2.0 and incorporates many architectural and
performance improvements over earlier releases.  It is designed to work
identically over NT (3.51 through 5) and Win95, with a single difference
in runtime files for received trap dispatching for NT (SNMPTRAP.EXE) vs
Win95 (NP_WSX95.EXE).

Current users please note:
This wsnmp32.dll does not require the np_wsx32.exe "helper" app of
previous versions...instead you will need either snmptrap.exe (for NT)
or np_wsx95.exe (for Win95).  For clarity's sake, you should remove any
np_wsx32.exe from a machine running this version of wsnmp32.dll--or at
least move it to a directory off the execution path.  Nothing bad happens
if you don't, except some wasted disk space and an increased possibility
for confusion.
------------------------------------------------------------------------
SNMPTRAP Info (NT only):
This snmptrap.exe is a plug-replacement for the one MS ships (it will
eventually *be* the one that MS ships). It is functionally identical
(with some minor performance improvements), but does not have any
internal dependencies on either the wsnmp32.dll or the mgmtapi.dll,
although it services both.

Our wsnmp32.dll will create and install the service on the first call
to SnmpRegister() if it finds that the sevice has not been previously
installed.  The Win32 CreateService() API requires that the caller
have admin priviledges.

One we have created and installed the service, our wsnmp32.dll will
auto-start it if it's not already running when the first app successfully
calls SnmpRegister().

[If you are installing our runtime files by hand--rather than via the
Developer Kit automated install/setup package--then you must copy
snmptrap.exe to "%SystemRoot%\system32\".  If you did not previously
have the SNMPTRAP service installed, then we will auto-install it,
per above.  If you already had the original MS version installed, then
you should use an admin tool (such as sc.exe from the MS Resource Kit)
to remove/delete it.

If you are using the automated InstallShield procedure, then it will
perform as much of the above as it can for you; but depending upon
system security settings for any pre-existing SNMPTRAP installation,
you may still have to perform the service remove/delete operation.
------------------------------------------------------------------------
Win95 users please note:
The snmptrap.exe does not apply and you do not need to have it on your
system.  Instead, however, you must have np_wsx95.exe accessible for
auto-loading/termination by wsnmp32.dll (in response to SnmpRegister()
calls).
------------------------------------------------------------------------
You will note that the winsnmp.def does not contain any exports for
proprietary functions.  If you previously used any of our pre-standard
implementations of:

	- SnmpSetPort()
	- SnmpListen()
	- NPWSNMPSetV1TrapPduData()

you will need to at least re-link (for SnmpSetPort(), whose ordinal
changed) or re-build (SnmpListen() has slightly different calling
parameters in v2.0 than our pre-standard implementation and
NPWSNMPSetV1TrapPduData() no longer exists, since SNMPv1 trap PDU
generation is handled automatically by wsnmp32.dll).

Also, this version does not use any "shared" memory across processes.
Hence, there is no need to fiddle with the resource limits in the
[Startup] section of the np_wsnmp.ini file (in fact, that section can
be completely deleted from np_wsnmp.ini, if you want).

You will find information about our MIB compiler and the MIB access
routines in the underlying libraries in the NP_MIB++ directory.
You are free to use our MIB compiler technology in your own apps, but
please note that doing so will mean that you are using our proprietary
interfaces, rather than some industry-standard (which does not yet
exist anyway).

For information on HP's SNMP++ DLL and sample applications, please
refer to their web site at:

   http://rosegarden.external.hp.com/snmp++

Please report any problems/questions with this release directly to
Bob Natale, bnatale@acecomm.com.
