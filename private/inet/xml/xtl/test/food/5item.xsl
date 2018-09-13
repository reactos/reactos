<?xml:namespace ns="uri:xsl" prefix="xsl"?>

<xsl:document>
	<HTML>
		<BODY STYLE="color:green">
			<h4>Stylesheet FOOD1.XSL</h4>
			<TABLE border="true">
				<TBODY>
					<xsl:repeat for="BREAKFAST-MENU/FOOD">
						<TR>
                           				<TD><DIV><xsl:get-value for="NAME"/></DIV></TD>
							<TD><DIV><xsl:get-value for="PRICE"/></DIV></TD>
							<TD><DIV><xsl:get-value for="DESCRIPTION"/></DIV></TD>
							<TD><DIV><xsl:get-value for="CALORIES"/></DIV></TD>										
							<TD><DIV><xsl:get-value for="PUKE"/></DIV></TD>
						</TR>
					</xsl:repeat>
				</TBODY>
			</TABLE>
		</BODY>
	</HTML>
</xsl:document>

