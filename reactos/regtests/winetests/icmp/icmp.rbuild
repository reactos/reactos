<module name="icmp_winetest" type="win32cui" installbase="bin" installname="icmp_winetest.exe" allowwarnings="true">
    <include base="icmp_winetest">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>ntdll</library>
    <library>icmp</library>
    <file>icmp.c</file>
    <file>testlist.c</file>
</module>
