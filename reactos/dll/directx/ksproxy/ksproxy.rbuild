<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ksproxy" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_KSPROXY}" installbase="system32" installname="ksproxy.ax">
	<importlibrary definition="ksproxy.spec" />
	<include base="ksproxy">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>msvcrt</library>
	<library>strmiids</library>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>

	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<file>basicaudio.cpp</file>
	<file>classfactory.cpp</file>
	<file>clockforward.cpp</file>
	<file>cvpconfig.cpp</file>
	<file>cvpvbiconfig.cpp</file>
	<file>datatype.cpp</file>
	<file>interface.cpp</file>
	<file>ksproxy.cpp</file>
	<file>ksproxy.rc</file>
	<file>proxy.cpp</file>
	<file>qualityforward.cpp</file>
</module>
</group>
