<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="quartz" type="win32dll" baseaddress="${BASEADDRESS_QUARTZ}" installbase="system32" installname="quartz.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="quartz.spec" />
	<include base="quartz">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>uuid</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>dsound</library>
	<library>strmiids</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>shlwapi</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>msvfw32</library>
	<library>msacm32</library>
	<library>ntdll</library>
	<file>avidec.c</file>
	<file>acmwrapper.c</file>
	<file>waveparser.c</file>
	<file>videorenderer.c</file>
	<file>transform.c</file>
	<file>systemclock.c</file>
	<file>regsvr.c</file>
	<file>pin.c</file>
	<file>parser.c</file>
	<file>nullrenderer.c</file>
	<file>mpegsplit.c</file>
	<file>memallocator.c</file>
	<file>main.c</file>
	<file>filtermapper.c</file>
	<file>filtergraph.c</file>
	<file>filesource.c</file>
	<file>enumregfilters.c</file>
	<file>enumpins.c</file>
	<file>enummoniker.c</file>
	<file>enumfilters.c</file>
	<file>dsoundrender.c</file>
	<file>enummedia.c</file>
	<file>control.c</file>
	<file>avisplit.c</file>
	<file>version.rc</file>
</module>
</group>