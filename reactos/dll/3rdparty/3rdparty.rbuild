<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="freetype">
		<xi:include href="freetype/freetype.rbuild" />
	</directory>
	<directory name="mesa32">
		<xi:include href="mesa32/mesa32.rbuild" />
	</directory>

	<ifnot property="NSWPAT" value="0">
		<directory name="dxtn">
			<xi:include href="dxtn/dxtn.rbuild" />
		</directory>
	</ifnot>
</group>
	