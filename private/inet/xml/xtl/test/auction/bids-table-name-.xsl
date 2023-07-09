<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>
	<!-- Root rule - start processing here -->

	<DIV>
		<xsl:repeat for = ".//BIDS" >
		    <TABLE border="1px solid black">
			    <TR	STYLE = "font-size:12pt; font-family:Verdana; font-weight:bold; text-decoration:underline">
				    <TD>Price</TD>
				    <TD>Time</TD>
				    <TD>Bidder</TD>
			    </TR>
			    <xsl:repeat for = "BID" order-by = "-BIDDER">
           		    <TR STYLE="font-family:Verdana; font-size:12pt; padding:0px 6px">
	        		    <TD>$<xsl:get-value for = "PRICE"/></TD>
	        		    <TD><xsl:get-value for = "TIME"/></TD>
	        		    <TD><xsl:get-value for = "BIDDER"/></TD>
		            </TR>
                </xsl:repeat>
		    </TABLE>
        </xsl:repeat>
	</DIV>

</xsl:document>
