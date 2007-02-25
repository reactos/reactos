<module name="urlmon" type="win32dll" baseaddress="${BASEADDRESS_URLMON}" installbase="system32" installname="urlmon.dll" allowwarnings="true">
	<importlibrary definition="urlmon.spec.def" />
	<include base="urlmon">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_STDDEF_H" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>cabinet</library>
	<library>wininet</library>
	<file>binding.c</file>
	<file>file.c</file>
	<file>format.c</file>
	<file>ftp.c</file>
	<file>http.c</file>
	<file>internet.c</file>
	<file>regsvr.c</file>
	<file>sec_mgr.c</file>
	<file>session.c</file>
	<file>umon.c</file>
	<file>umstream.c</file>
	<file>urlmon_main.c</file>
	<file>rsrc.rc</file>
	<file>urlmon.spec</file>
</module>
