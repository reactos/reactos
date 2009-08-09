<module name="smdll" type="nativedll" baseaddress="${BASEADDRESS_SMDLL}" installbase="system32" installname="smdll.dll">
	<importlibrary definition="smdll.spec" />
	<include base="smdll">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<library>smlib</library>
	<library>ntdll</library>
	<file>dllmain.c</file>
	<file>query.c</file>
	<file>smdll.rc</file>
</module>
