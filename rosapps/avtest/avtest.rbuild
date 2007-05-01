<module name="avtest" type="kernelmodedriver" installbase="system32/drivers" installname="avtest.sys" warnings="true">
	<include base="avtest">.</include>
	<include base="avtest">..</include>
	<define name="__USE_W32API" />
	<library>ks</library>
	<library>ntoskrnl</library>
</module>
