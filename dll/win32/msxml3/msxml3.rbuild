<module name="msxml3" type="win32dll" baseaddress="${BASEADDRESS_MSXML3}" installbase="system32" installname="msxml3.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="msxml3.spec.def" />
	<include base="msxml3">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x601</define>
	<define name="WINVER">0x501</define>
	<define name="LIBXML_STATIC" />
	<library>libxml2</library>
	<library>wine</library>
	<library>urlmon</library>
	<library>wininet</library>
	<library>ws2_32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>cabinet</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>version</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>attribute.c</file>
	<file>comment.c</file>
	<file>domdoc.c</file>
	<file>element.c</file>
	<file>factory.c</file>
	<file>main.c</file>
	<file>node.c</file>
	<file>nodelist.c</file>
	<file>nodemap.c</file>
	<file>parseerror.c</file>
	<file>pi.c</file>
	<file>queryresult.c</file>
	<file>regsvr.c</file>
	<file>schema.c</file>
	<file>text.c</file>
	<file>uuid.c</file>
	<file>xmldoc.c</file>
	<file>xmlelem.c</file>
	<file>msxml3.spec</file>
</module>
