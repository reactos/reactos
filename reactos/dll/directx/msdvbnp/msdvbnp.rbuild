<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msdvbnp" type="win32dll" baseaddress="${BASEADDRESS_MSDVBNP}" installbase="system32" installname="msdvbnp.ax">
	<importlibrary definition="msdvbnp.spec" />
	<include base="msdvbnp">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>msvcrt</library>
	<library>strmiids</library>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<file>classfactory.cpp</file>
	<file>enum_mediatypes.cpp</file>
	<file>enumpins.cpp</file>
	<file>ethernetfilter.cpp</file>
	<file>msdvbnp.cpp</file>
	<file>msdvbnp.rc</file>
	<file>ipv4.cpp</file>
	<file>ipv6.cpp</file>
	<file>networkprovider.cpp</file>
	<file>pin.cpp</file>
	<file>scanningtuner.cpp</file>
</module>
</group>
