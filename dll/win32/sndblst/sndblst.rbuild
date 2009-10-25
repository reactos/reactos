<!-- Temporarily compiling as a CUI app for testing purposes */
<!--module name="mmdrv" type="win32dll" baseaddress="${BASEADDRESS_MMDRV}" installbase="system32" installname="mmdrv.dll" unicode="yes"-->
<module name="sndblst" type="win32dll" installbase="system32" installname="sndblst.dll" unicode="yes">
	<importlibrary definition="sndblst.spec" />
    <include base="ReactOS">include/reactos/libs/sound</include>
	<include base="sndblst">.</include>
	<define name="DEBUG_NT4" />
    <library>mment4</library>
    <library>mmebuddy</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
    <library>advapi32</library>
    <file>sndblst.c</file>
</module>
