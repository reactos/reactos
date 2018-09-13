<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>
	<HTML>
		<BODY>
			<TABLE>
				<TBODY>
					<xsl:repeat for="BREAKFAST-MENU/FOOD">
						<TR>
							<xsl:use-templates for="*"/>				
						</TR>
					</xsl:repeat>
				</TBODY>
			</TABLE>
		</BODY>
	</HTML>

    <xsl:templates>

        <xsl:template for = "NAME">
            <TD><DIV><xsl:get-value /></DIV></TD>
        </xsl:template>

        <xsl:template for = "PRICE">
            <TD><DIV><xsl:get-value /></DIV></TD>
        </xsl:template>

        <xsl:template for = "DESCRIPTION">
            <TD><DIV><xsl:get-value /></DIV></TD>
        </xsl:template>

        <xsl:template for = "CALORIES">
            <TD><DIV><xsl:get-value /></DIV></TD>				
        </xsl:template>

    </xsl:templates>
</xsl:document>

