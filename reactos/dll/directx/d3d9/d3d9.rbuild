<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="d3d9" type="win32dll" entrypoint="0" installbase="system32" installname="d3d9.dll" baseaddress="0x4fdd0000">
	<importlibrary definition="d3d9.spec.def" />

	<library>advapi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>uuid</library>
	<library>dxguid</library>
	<library>version</library>
	<library>d3d8thk</library>

	<file>d3d9.c</file>
	<file>d3d9_baseobject.c</file>
	<file>d3d9_caps.c</file>
	<file>d3d9_create.c</file>
	<file>d3d9_cursor.c</file>
	<file>d3d9_device.c</file>
	<file>d3d9_haldevice.c</file>
	<file>d3d9_helpers.c</file>
	<file>d3d9_impl.c</file>
	<file>d3d9_mipmap.c</file>
	<file>d3d9_puredevice.c</file>
	<file>d3d9_resource.c</file>
	<file>d3d9_swapchain.c</file>
	<file>d3d9_texture.c</file>
	<file>adapter.c</file>
	<file>device.c</file>
	<file>format.c</file>
	<file>d3d9.rc</file>
	<file>d3d9.spec</file>
</module>
