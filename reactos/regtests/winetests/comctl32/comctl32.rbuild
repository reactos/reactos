<module name="comctl32_winetest" type="win32cui" installbase="bin" installname="comctl32_winetest.exe" allowwarnings="true">
    <include base="comctl32_winetest">.</include>
    <define name="__USE_W32API" />
    <library>shlwapi</library>
    <library>ole32</library>
    <library>comctl32</library>
    <library>ntdll</library>
    <library>gdi32</library>
    <library>user32</library>
    <library>kernel32</library>
    <library>advapi32</library>
    <file>comboex.c</file>
    <file>dpa.c</file>
    <file>header.c</file>
    <file>imagelist.c</file>
    <file>listview.c</file>
    <file>monthcal.c</file>
    <file>mru.c</file>
    <file>progress.c</file>
    <file>propsheet.c</file>
    <file>subclass.c</file>
    <file>tab.c</file>
    <file>testlist.c</file>
    <file>toolbar.c</file>
    <file>tooltips.c</file>
    <file>treeview.c</file>
    <file>updown.c</file>
    <file>propsheet.rc</file>
</module>
