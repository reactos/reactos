<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<directory name="midimap">
	<xi:include href="midimap/midimap.rbuild" />
</directory>
<directory name="wavemap">
	<xi:include href="wavemap/wavemap.rbuild" />
</directory>
<module name="winmm" type="win32dll" baseaddress="${BASEADDRESS_WINMM}" installbase="system32" installname="winmm.dll" allowwarnings="true">
	<importlibrary definition="winmm.spec.def" />
	<include base="winmm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>driver.c</file>
	<file>joystick.c</file>
	<file>lolvldrv.c</file>
	<file>mci.c</file>
	<file>mmio.c</file>
	<file>playsound.c</file>
	<file>time.c</file>
	<file>winmm.c</file>
	<file>registry.c</file>
	<file>winmm_res.rc</file>
	<file>winmm.spec</file>
</module>
</group>
