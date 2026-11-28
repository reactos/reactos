FNET TCP/IP Stack.
==================
Please read the FNET Quick Start chapter. You can find it in the /fnet_doc/fnet_user_manual.chm file.

The stack provides following protocols and services:
- Supported Platforms:
	- ARM Cortex-M4
		- Kinetis:
			- MK64FN1M (FRDM-K64F board)
			- MK66FN2M (FRDM-K66F board)
			- MK6DN512 (TWR-K60D100M board)
		- i.MX-RT: 
			- MIMXRT1052 (IMXRT1050-EVKB board).
			- MIMXRT1062 (MIMXRT1060-EVK board).
		- LPC: LPC54628 (OM13098: LPCXpresso54628 board)
	- Qualcomm/Atheros Wi-Fi 202.11: 
		- QCA4002 (GT202-GC3013-FRDM4-KIT board).
		- QCA4004 (SX-ULPAN-2401-SHIELD board). 
- Supported Compilers:
	- IAR: Embedded Workbench for ARM, version 8.22
	- GCC: Kinetis Design Studio, version 3.2 
- Bare-metal TCP/IP stack. No underlying RTOS is required, although it can be used with it. FreeRTOS example is provided.
- Certified logos for:
	- Golden IPv6 Ready.
	- Microsoft Azure IoT.
	- Apple Bonjour. 
- Non-blocking Socket API (DGRAM, STREAM, RAW).
- Core Protocols:
	- TCP (Transmission Control Protocol).
	- UDP (User Datagram Protocol).
	- IPv4.
		- ICMPv4 (Internet Control Message Protocol).
		- IGMP (Internet Group Management Protocol).
		- ARP (Address Resolution Protocol).
	- IPv6. 
		- "IPv6 Ready Logo Certified". Passed IPv6 Core Protocols Conformance and Interoperability tests.
		- ICMPv6 (Internet Control Message Protocol).
		- MLDv1 (Multicast Listener Discovery).
		- Neighbor Discovery for IPv6.
		- IPv6 Stateless Address Autoconfiguration.
		- Path MTU Discovery for IPv6
- Network Interfaces:
	- Ethernet (FEC/ENET module).
	- Wi-Fi (QCA4002/4 module).
	- Loopback.
- Services:
	- HTTP server:
		- HTTP/1.0 or HTTP/0.9 protocols.
		- GET and POST requests.
		- CGI and SSI.
		- Basic access authentication.
		- HTTP over TLS (HTTPS).
	- TELNET server. 
	- DHCPv4 client and server.
	- Auto-IP service. Passed "Link-Local Address Allocation", Bonjour Conformance Test.
	- Azure IoT Hub client adapter. Microsoft Azure certified.
	- DNS client/resolver.
	- Link-Detection service.
	- Multicast DNS (mDNS) "Bonjour" Server/Responder. Passed the Bonjour Conformance Test.
	- LLMNR Server/Responder.
	- PING service.
	- SNTP client.	
	- TFTP server and client. Both Read and Write requests are supported.
	- TLS using mbedTLS library.
	- ROM File System.
	- On-chip Flash Memory driver.
	- Command line shell.
 	- Serial driver.

FNET project directory structure:
	fnet                        - FNET root directory.
	|-- fnet_stack                - FNET TCP/IP stack.
	|    |-- stack                  - FNET TCP/IP stack platform-independent source code.
	|    |-- service                - FNET Services.
	|    |    |-- autoip              - Auto-IP service.
	|    |    |-- azure               - Azure IoT Hub adapter.
	|    |    |-- bench               - Benchmark client and server.
	|    |    |-- dhcp                - DHCPv4 client and server.
	|    |    |-- dns                 - DNS client/resolver.
	|    |    |-- flash               - Flash Memory driver.
	|    |    |-- fs                  - File System driver.
	|    |    |-- http                - HTTP Server service.
	|    |    |-- link                - Link-Detection service.
	|    |    |-- llmnr               - LLMNR server/responder.
	|    |    |-- mdns                - mDNS server/responder.
	|    |    |-- ping                - PING service.
	|    |    |-- poll                - Polling Mechanism library.
	|    |    |-- serial              - Serial Input/Output driver.
	|    |    |-- shell               - Command Shell service.
	|    |    |-- sntp                - SNTP client.
	|    |    |-- telnet              - TELNET server.
	|    |    |-- tftp                - TFTP server and client.
	|    |    |-- tls                 - TLS library.
	|    |-- port                   - FNET port-specific source code.
	|         |-- compiler            - Compiler-specific source code.
	|         |-- cpu                 - FNET platform-specific source code.
	|         |    |-- lpc              - LPC-specific source code.
	|         |    |-- mcf              - ColdFire-specific source code.
	|         |    |-- mk               - Kinetis-specific source code.
	|         |    |-- mpc              - Power Architecture-specific source code.
	|         |-- netif            - Platform-specific network interface drivers.
	|              |-- enet          - ENET Ethernet module driver, used by LPC platform.
	|              |-- fec           - FEC/ENET Ethernet module driver, used by MCF,MPC and MK platforms.
	|              |-- qca           - QCA4002/4 Wi-Fi module driver, adapter for "third_party/qca-x.x.x".
	|-- fnet_doc                  - FNET documentation.
	|-- fnet_demos                - FNET demo projects.
	|    |-- common                 - Common source code used by demos.
	|    |    |-- fnet_application    - Demo application source code.
	|    |    |-- fnet_webpage        - Demo web page source code.
	|    |    |-- startup             - Platform-specific start-up source code and drivers.
	|    |-- build                 - Demo projects.
	|         |-- <board_name>        - Demo projects for a <board_name> board.
	|              |-- shell          - "Shell" demo is a fully featured shell. It allows
	|              |                    view/change various parameters (e.g. IP address,
	|              |                    subnet mask), explore mounted file systems, run 
	|              |                    DHCP client, HTTP and Telnet servers.
	|              |-- shell_azure    - FNET Azure IoT Hub client example.
	|              |-- shell_boot     - FNET TFTP Bootloader.
	|              |-- shell_boot_app - Example application used for the FNET TFTP Bootloader
	|              |                    demonstration. This demo has the same features as
	|              |                    the "Shell" demo, but it`s modified to be able to
	|              |                    work with the FNET Bootloader.
	|              |-- shell_bench    - TCP/UDP Throughput Benchmark application. 
	|              |-- shell_tls      - Example application used for the HTTPS server demonstration.
	|              |                    This demo has the same features as the "Shell" demo,
	|              |                    but in addition it includes the mbedTLS library.
	|              |-- shell_wifi_<board> - Example application used for the Wi-Fi interface demonstration.
	|                                       It has the same features as the "Shell FreeRTOS" demo,
	|                                       but in addition it supports the Wi-Fi network interface using the Qualcomm QCA4002/4 module.
	|-- fnet_tools                - FNET PC tools.
	|-- third_party               - Optional, third-party libraries.
		 |-- azure_iot-x.x.x        - Azure IoT Hub Device C SDK.
		 |-- cmsis-x.x              - ARM CMSIS-Core(M) library, used for ARM Cortex M platforms.
		 |-- mbedtls-x.x.x          - mbedTLS library, used by FNET TLS.
		 |-- freertos-x.x.x         - FreeRTOS, used by freertos examples.
		 |-- qca-x.x.x              - Qualcomm QCA4002/4 driver, used by shell_wifi_<board> example.
		 
		 