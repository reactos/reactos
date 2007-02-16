<module name="msi_winetest" type="win32cui" installbase="bin" installname="msi_winetest.exe" allowwarnings="true">
    <include base="msi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>cabinet</library>
    <library>msi</library>
    <library>ole32</library>
    <library>advapi32</library>
    <library>kernel32</library>
    <library>ntdll</library>
    <file>db.c</file>
    <file>format.c</file>
    <file>install.c</file>
    <file>msi.c</file>
    <file>package.c</file>
    <file>record.c</file>
    <file>suminfo.c</file>
    <file>testlist.c</file>
</module>
