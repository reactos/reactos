<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

	<DIV STYLE="padding:.3in .1in .3in .3in; font-family:Arial Black; background-image:URL(swallows.jpg)">
		<xsl:repeat for = "AUCTIONBLOCK/ITEM" >
	        <TABLE>
		        <TR>
			        <TD COLSPAN="2">		
                        <IMG>
                            <xsl:attribute name="STYLE">
                                border:1px solid black;
                                width:<xsl:get-value for = "PREVIEW-SMALL/@width" />;
                                height:<xsl:get-value for = "PREVIEW-SMALL/@height" />;
                                alt:<xsl:get-value for = "PREVIEW-SMALL/@alt" />;
                            </xsl:attribute>
                            <xsl:attribute name = "src"><xsl:get-value for = "PREVIEW-SMALL/@src" /></xsl:attribute>
                        </IMG>
                     </TD>
			        <TD STYLE="padding-left:1em">
                   		<DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%; font-size:18pt; color:yellow">
	    	                <xsl:get-value for = "TITLE"/>
    	                </DIV>
	                	<DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%; margin-top:1em; font-style:italic; font-size:18pt; color:yellow">
			                <xsl:get-value for = "ARTIST"/>
		                </DIV>
			        </TD>
		        </TR>
		        <TR>
			        <TD>
                        <DIV STYLE="color:white; font:10pt. Verdana; font-style:italic; font-weight:normal">Size: <xsl:get-value for = "DIMENSIONS" /></DIV>
                    </TD>
		            <TD>
			            <DIV STYLE="text-align:right; color:white; font:10pt. Verdana; font-style:italic font-weight:normal">
    			            <xsl:get-value for="MATERIALS"/>
				            <xsl:get-value for="YEAR"/>
			            </DIV>
		            </TD>
		            <TD>
		            </TD>
	            </TR>
	            <TR>
		            <TD COLSPAN="2">        
                        <xsl:repeat for = "BIDS/BID[1]"><DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">
			                High bid: 	<SPAN color="yellow"> (<xsl:get-value for = "BIDDER" />)</SPAN></DIV>
                        </xsl:repeat>
                        <xsl:repeat for = "BIDS/BID[end()]">
                            <DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">Opening bid: $<xsl:get-value for ="PRICE"/></DIV>
                        </xsl:repeat>
                    </TD>
		            <TD	STYLE="text-align:right; font:10pt Verdana; font-style:italic; color:yellow">
			            <DIV STYLE="margin-top:.5em">Copyright &amp;copy;1997 Linda Mann, all rights reserved.</DIV>
			            <DIV STYLE="font-weight:bold">
				            <a href="http://home.navisoft.com/lindamann/" target="_top">Linda Mann Art Gallery</a>
			            </DIV>
		            </TD>
	            </TR>
            </TABLE>
        </xsl:repeat>
	</DIV>

</xsl:document>
