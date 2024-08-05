#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#

# CONTRIBAPPFILES: Contrib Applications.
CONTRIBAPPFILES=$(CONTRIBDIR)/apps/httpserver/httpserver-netconn.c \
	$(CONTRIBDIR)/apps/chargen/chargen.c \
	$(CONTRIBDIR)/apps/udpecho/udpecho.c \
	$(CONTRIBDIR)/apps/tcpecho/tcpecho.c \
	$(CONTRIBDIR)/apps/shell/shell.c \
	$(CONTRIBDIR)/apps/udpecho_raw/udpecho_raw.c \
	$(CONTRIBDIR)/apps/tcpecho_raw/tcpecho_raw.c \
	$(CONTRIBDIR)/apps/netio/netio.c \
	$(CONTRIBDIR)/apps/ping/ping.c \
	$(CONTRIBDIR)/apps/socket_examples/socket_examples.c \
	$(CONTRIBDIR)/apps/rtp/rtp.c \
	$(CONTRIBDIR)/examples/httpd/fs_example/fs_example.c \
	$(CONTRIBDIR)/examples/httpd/https_example/https_example.c \
	$(CONTRIBDIR)/examples/httpd/ssi_example/ssi_example.c \
	$(CONTRIBDIR)/examples/lwiperf/lwiperf_example.c \
	$(CONTRIBDIR)/examples/mdns/mdns_example.c \
	$(CONTRIBDIR)/examples/mqtt/mqtt_example.c \
	$(CONTRIBDIR)/examples/ppp/pppos_example.c \
	$(CONTRIBDIR)/examples/snmp/snmp_private_mib/lwip_prvmib.c \
	$(CONTRIBDIR)/examples/snmp/snmp_v3/snmpv3_dummy.c \
	$(CONTRIBDIR)/examples/snmp/snmp_example.c \
	$(CONTRIBDIR)/examples/sntp/sntp_example.c \
	$(CONTRIBDIR)/examples/tftp/tftp_example.c \
	$(CONTRIBDIR)/addons/tcp_isn/tcp_isn.c \
	$(CONTRIBDIR)/addons/ipv6_static_routing/ip6_route_table.c
