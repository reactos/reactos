INTRODUCTION

lwIP is a small independent implementation of the TCP/IP protocol suite.

The focus of the lwIP TCP/IP implementation is to reduce the RAM usage
while still having a full scale TCP. This making lwIP suitable for use
in embedded systems with tens of kilobytes of free RAM and room for
around 40 kilobytes of code ROM.

lwIP was originally developed by Adam Dunkels at the Computer and Networks
Architectures (CNA) lab at the Swedish Institute of Computer Science (SICS)
and is now developed and maintained by a worldwide network of developers.

FEATURES

  * IP (Internet Protocol, IPv4 and IPv6) including packet forwarding over
    multiple network interfaces
  * ICMP (Internet Control Message Protocol) for network maintenance and debugging
  * IGMP (Internet Group Management Protocol) for multicast traffic management
  * MLD (Multicast listener discovery for IPv6). Aims to be compliant with
    RFC 2710. No support for MLDv2
  * ND (Neighbor discovery and stateless address autoconfiguration for IPv6).
    Aims to be compliant with RFC 4861 (Neighbor discovery) and RFC 4862
    (Address autoconfiguration)
  * DHCP, AutoIP/APIPA (Zeroconf), ACD (Address Conflict Detection)
    and (stateless) DHCPv6
  * UDP (User Datagram Protocol) including experimental UDP-lite extensions
  * TCP (Transmission Control Protocol) with congestion control, RTT estimation
    fast recovery/fast retransmit and sending SACKs
  * raw/native API for enhanced performance
  * Optional Berkeley-like socket API
  * TLS: optional layered TCP ("altcp") for nearly transparent TLS for any
    TCP-based protocol (ported to mbedTLS) (see changelog for more info)
  * PPPoS and PPPoE (Point-to-point protocol over Serial/Ethernet)
  * DNS (Domain name resolver incl. mDNS)
  * 6LoWPAN (via IEEE 802.15.4, BLE or ZEP)


APPLICATIONS

  * HTTP server with SSI and CGI (HTTPS via altcp)
  * SNMPv2c agent with MIB compiler (Simple Network Management Protocol), v3 via altcp
  * SNTP (Simple network time protocol)
  * NetBIOS name service responder
  * MDNS (Multicast DNS) responder
  * iPerf server implementation
  * MQTT client (TLS support via altcp)


LICENSE

lwIP is freely available under a BSD license.


DEVELOPMENT

lwIP has grown into an excellent TCP/IP stack for embedded devices,
and developers using the stack often submit bug fixes, improvements,
and additions to the stack to further increase its usefulness.

Development of lwIP is hosted on Savannah, a central point for
software development, maintenance and distribution. Everyone can
help improve lwIP by use of Savannah's interface, Git and the
mailing list. A core team of developers will commit changes to the
Git source tree.

The lwIP TCP/IP stack is maintained in the 'src' directory and
contributions (such as platform ports and applications) are in
the 'contrib' directory.

See doc/savannah.txt for details on Git server access for users and
developers.

The current Git tree is web-browsable:
  https://git.savannah.gnu.org/cgit/lwip.git

Submit patches and bugs via the lwIP project page:
  https://savannah.nongnu.org/projects/lwip/

Continuous integration builds (GCC, clang):
  https://github.com/lwip-tcpip/lwip/actions


DOCUMENTATION

Self documentation of the source code is regularly extracted from the current
Git sources and is available from this web page:
  https://www.nongnu.org/lwip/

Also, there are mailing lists you can subscribe at
  https://savannah.nongnu.org/mail/?group=lwip
plus searchable archives:
  https://lists.nongnu.org/archive/html/lwip-users/
  https://lists.nongnu.org/archive/html/lwip-devel/

There is a wiki about lwIP at
  https://lwip.wikia.com/wiki/LwIP_Wiki
You might get questions answered there, but unfortunately, it is not as
well maintained as it should be.

lwIP was originally written by Adam Dunkels:
  http://dunkels.com/adam/

Reading Adam's papers, the files in docs/, browsing the source code
documentation and browsing the mailing list archives is a good way to
become familiar with the design of lwIP.

Adam Dunkels <adam@sics.se>
Leon Woestenberg <leon.woestenberg@gmx.net>
