<module name="msi" type="win32dll" baseaddress="${BASEADDRESS_MSI}" installbase="system32" installname="msi.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="msi.spec.def" />
	<include base="msi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>urlmon</library>
	<library>wininet</library>
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
	<file>action.c</file>
	<file>alter.c</file>
	<file>appsearch.c</file>
	<file>classes.c</file>
	<file>cond.tab.c</file>
	<file>create.c</file>
	<file>custom.c</file>
	<file>database.c</file>
	<file>delete.c</file>
	<file>dialog.c</file>
	<file>distinct.c</file>
	<file>events.c</file>
	<file>files.c</file>
	<file>font.c</file>
	<file>format.c</file>
	<file>handle.c</file>
	<file>helpers.c</file>
	<file>insert.c</file>
	<file>install.c</file>
	<file>join.c</file>
	<file>msi.c</file>
	<file>msi_main.c</file>
	<file>msiquery.c</file>
	<file>order.c</file>
	<file>package.c</file>
	<file>preview.c</file>
	<file>record.c</file>
	<file>registry.c</file>
	<file>regsvr.c</file>
	<file>select.c</file>
	<file>source.c</file>
	<file>sql.tab.c</file>
	<file>string.c</file>
	<file>suminfo.c</file>
	<file>table.c</file>
	<file>tokenize.c</file>
	<file>update.c</file>
	<file>upgrade.c</file>
	<file>where.c</file>
	<file>msi.rc</file>
	<file>msi.spec</file>
</module>
