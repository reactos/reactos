<module name="shdocvw" type="win32dll" baseaddress="${BASEADDRESS_SHDOCVW}" installbase="system32" installname="shdocvw.dll" usewrc="false" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="shdocvw.spec.def" />
	<include base="shdocvw">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_SHDOCVW_" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>urlmon</library>
	<file>classinfo.c</file>
	<file>client.c</file>
	<file>dochost.c</file>
	<file>events.c</file>
	<file>factory.c</file>
	<file>frame.c</file>
	<file>misc.c</file>
	<file>oleobject.c</file>
	<file>persist.c</file>
	<file>regsvr.c</file>
	<file>shdocvw_main.c</file>
        <file>shlinstobj.c</file>
	<file>view.c</file>
	<file>webbrowser.c</file>
	<file>shdocvw.rc</file>
	<file>shdocvw.spec</file>
</module>
