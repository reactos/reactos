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
					    <COL/>
					    <TBODY STYLE="type:head background-color:tan">
						    <TR>
							    <TD><P STYLE="font-weight:bold">Name</P></TD>
							    <TD><P STYLE="font-weight:bold">Price</P></TD>
							    <TD><P STYLE="font-weight:bold">Description</P></TD>
							    <TD><P STYLE="font-weight:bold">Calories*</P></TD>
						    </TR>
					    </TBODY>
                        <TBODY STYLE="color:blue">
                            <xsl:repeat for="BREAKFAST-MENU/FOOD" >
                                <TR>
                                    <TD><P STYLE="font-weight:bold"><xsl:get-value for = "NAME" /></P></TD>
                                    <TD><P STYLE="font-size:12pt"><xsl:get-value for = "PRICE" /></P></TD>
                                    <TD><P STYLE="font-size:12pt"><xsl:get-value for = "DESCRIPTION" /></P></TD>
                                    <TD><P STYLE="font-size:12pt"><xsl:get-value for = "CALORIES" /></P></TD>
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

</xsl:document>