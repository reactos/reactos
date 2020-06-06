MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409
              )

MessageId=10000
SymbolicName=MSG_ARP_SYNTAX
Severity=Success
Facility=System
Language=English
Displays and modifies the IP-to-Physical address translation tables used by
address resolution protocol (ARP).

ARP -s inet_addr eth_addr [if_addr]
ARP -d inet_addr [if_addr]
ARP -a [inet_addr] [-N if_addr]

  -a            Displays current ARP entries by interrogating the current
                protocol data.  If inet_addr is specified, the IP and Physical
                addresses for only the specified computer are displayed.  If
                more than one network interface uses ARP, entries for each ARP
                table are displayed.
  -g            Same as -a.
  inet_addr     Specifies an internet address.
  -N if_addr    Displays the ARP entries for the network interface specified
                by if_addr.
  -d            Deletes the host specified by inet_addr. inet_addr may be
                wildcarded with * to delete all hosts.
  -s            Adds the host and associates the Internet address inet_addr
                with the Physical address eth_addr.  The Physical address is
                given as 6 hexadecimal bytes separated by hyphens. The entry
                is permanent.
  eth_addr      Specifies a physical address.
  if_addr       If present, this specifies the Internet address of the
                interface whose address translation table should be modified.
                If not present, the first applicable interface will be used.
Example:
  > arp -s 157.55.85.212   00-aa-00-62-c6-09  .... Adds a static entry.
  > arp -a                                    .... Displays the arp table.
.

MessageId=10001
SymbolicName=MSG_ARP_BAD_IP_ADDRESS
Severity=Success
Facility=System
Language=English
ARP: bad IP address: %1
.

MessageId=10002
SymbolicName=MSG_ARP_BAD_ARGUMENT
Severity=Success
Facility=System
Language=English
ARP: bad argument: %1
.

MessageId=10003
SymbolicName=MSG_ARP_INTERFACE
Severity=Success
Facility=System
Language=English

Interface: %1!s! --- 0x%2!lx!
  Internet Address      Physical Address      Type
.

MessageId=10004
SymbolicName=MSG_ARP_NO_MEMORY
Severity=Success
Facility=System
Language=English
ARP: not enough memory
.

MessageId=10005
SymbolicName=MSG_ARP_OTHER
Severity=Success
Facility=System
Language=English
other%0
.

MessageId=10006
SymbolicName=MSG_ARP_INVALID
Severity=Success
Facility=System
Language=English
invalid%0
.

MessageId=10007
SymbolicName=MSG_ARP_DYNAMIC
Severity=Success
Facility=System
Language=English
dynamic%0
.

MessageId=10008
SymbolicName=MSG_ARP_STATIC
Severity=Success
Facility=System
Language=English
static%0
.

MessageId=10013
SymbolicName=MSG_ARP_ENTRY_FORMAT
Severity=Success
Facility=System
Language=English
  %1!-20s!  %2!-20s!  %3!-10s!
.

MessageId=10018
SymbolicName=MSG_ARP_NO_ENTRIES
Severity=Success
Facility=System
Language=English
No ARP entires found
.
