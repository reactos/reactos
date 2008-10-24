<module name="aclui" type="win32dll" baseaddress="${BASEADDRESS_ACLUI}" installbase="system32" installname="aclui.dll" unicode="yes">
	<importlibrary definition="aclui.spec" />
	<include base="aclui">.</include>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<define name="SUPPORT_UXTHEME" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>uxtheme</library>
	<library>advapi32</library>
	<file>aclui.c</file>
	<file>checklist.c</file>
	<file>guid.c</file>
	<file>misc.c</file>
	<file>sidcache.c</file>
	<file>aclui.rc</file>
	<file>aclui.spec</file>
	<pch>precomp.h</pch>
</module>
