<!--module name="mmdrv" type="win32dll" baseaddress="${BASEADDRESS_MMDRV}" installbase="system32" installname="mmdrv.dll" unicode="yes"-->
<module name="mmdrv" type="win32cui" installbase="system32" installname="mmdrv.exe" unicode="yes">
	<!--importlibrary definition="mmdrv.def" /-->
    <include base="ReactOS">include/reactos/libs/sound</include>
	<include base="mmdrv">.</include>
	<!--define name="NDEBUG" /-->
    <library>mmebuddy</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
    <library>advapi32</library>
    <file>entry.c</file>
</module>
