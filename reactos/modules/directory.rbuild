<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="rosapps">
		<xi:include href="rosapps/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="rostests">
		<xi:include href="rostests/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="wallpaper">
		<xi:include href="wallpaper/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="windows">
		<xi:include href="windows/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
</group>
