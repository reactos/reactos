<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

    <HTML>
        <BODY>
            <DIV STYLE="text-align:center font-family:sans-serif">
                <P STYLE="font-size:24pt">Welcome to the Breakfast Source!</P>
                <P>
	                Your information warehouse
	                for all sorts of edible items.
                </P>
                <P>
	                <TABLE STYLE="background-color:beige" BORDER="yes">
		                <COL/>
		                <COL/>
		                <COL/>
		                <TBODY STYLE="type:head background-color:tan color:blue">
			                <TR>
				                <TD><P STYLE="font-weight:bold">Name</P></TD>
				                <TD><P STYLE="font-weight:bold">Price</P></TD>
				                <TD><P STYLE="font-weight:bold">Description</P></TD>
			                </TR>
		                </TBODY>
		                <TBODY>
                            <xsl:repeat for="BREAKFAST-MENU/FOOD" >
                                <TR>
                                    <xsl:use-templates for = "*" />
                                </TR>
                            </xsl:repeat>
                        </TBODY>
                    </TABLE>
                </P>
            </DIV>
            <P STYLE="font-size:12pt">* Daily caloric percentage shown for 3000 calories/day.</P>
            <P STYLE="font-style:italic">Come back soon!</P>
        </BODY>
    </HTML>

    <xsl:templates>

        <xsl:template for="NAME">
	        <TD><P STYLE="font-weight:bold"><xsl:get-value /></P></TD>
        </xsl:template>

        <xsl:template for="PRICE">
	        <TD><P STYLE="font-size:12pt"><xsl:get-value /></P></TD>
        </xsl:template>

        <xsl:template for="DESCRIPTION">
	        <TD><P STYLE="font-size:12pt"><xsl:get-value /></P></TD>
	    </xsl:template>

    </xsl:templates>
    
</xsl:document>