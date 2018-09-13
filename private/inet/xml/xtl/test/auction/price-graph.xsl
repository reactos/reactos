<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

    <DIV STYLE="font-family:Arial; font-size:12pt">
        <xsl:repeat for = "AUCTIONBLOCK/ITEM" >
		    <DIV STYLE="font-weight:bold; font-size:14pt">
                <xsl:get-value for = "TITLE" />
			    <SPAN STYLE="font-weight:normal"> by </SPAN>
			    <xsl:get-value for = "ARTIST" />
		    </DIV>
		    <xsl:repeat for ="BIDS" >
	    	    <DIV STYLE = "margin-top:4px">
    			    <xsl:repeat for = "BID" >
            		    <DIV>
                            <xsl:attribute name = "STYLE">position:relative; height:1em; background-color:#FF8800; text-align:right; margin-top:2px;
                                width:<xsl:repeat for = "PRICE[1]"><xsl:eval>calcBidWidth(this)</xsl:eval>%</xsl:repeat>
                            </xsl:attribute>
                   		    <SPAN STYLE="font-weight:bold">$<xsl:get-value for = "PRICE" /></SPAN>
		                    <DIV>
                                <xsl:attribute name = "STYLE">
                                    margin-left:4px; position:absolute; top:0px;
                                    left:<xsl:eval>(50 > width ? 105 : 0)</xsl:eval>%;
                                    width:<xsl:eval>(50 > width ? 10000/width - 110 : 100)</xsl:eval>%;
	                                text-align:left
                                </xsl:attribute>
                                <SPAN STYLE="font-weight:bold"><xsl:get-value for = "BIDDER"/></SPAN>
                                <SPAN STYLE="font-style:italic" font-size="10pt">
	                                (<xsl:eval>trimSeconds(selectNodes("TIME").nextNode().text)</xsl:eval>)
                                </SPAN>
		                    </DIV>
		                </DIV>
                    </xsl:repeat>
		        </DIV>
            </xsl:repeat>
        </xsl:repeat>
    </DIV>
	
    <xsl:script><![CDATA[
    var width;

	function calcWidth(thisBid)
	{
		// Calculate the width of one bar of the graph
		var thisPrice, minPrice, maxPrice, price;
		var bids, aBid, aPrice;
        var bidlist;

		thisPrice = parseInt(thisBid.text, 10);

		bids = thisBid.parentNode;
        bidlist = bids.selectNodes("BID");

		// Find the highest and lowest bids for this item
		maxPrice = thisPrice;
		minPrice = thisPrice;
		for (aBid = bidlist.nextNode(); aBid != null; aBid = bidlist.nextNode())
		{
            aPrice = aBid.selectNodes("PRICE").nextNode();
            if (aPrice != null)
            {
			    price = parseInt(aPrice.text, 10);
			    if (price < minPrice) minPrice = price;
			    if (price > maxPrice) maxPrice = price;
            }
		}

		// Scale the return value with minPrice at 25% and maxPrice at 100%
		if (maxPrice == minPrice)
			width = 100;
		else
			width = (thisPrice - minPrice) * 75 / (maxPrice - minPrice) + 25;

        return width;
	}

	function calcBidWidth(thisBid)
	{
        return calcWidth(thisBid.parentNode);
	}


	function trimSeconds(time)
	{
		// This function trims the "seconds" off of a time string
		var i = 0;
		var start = 0;
		var end = 0;
		while (time.charAt(i) != ":")
			i++;
		i++;
		while (time.charAt(i) != ":")
			i++;
		start = i;
		while (time.charAt(i) != " ")
			i++;
		end = i;
		return time.substring(0,start) + time.substring(end, time.length);
	}

	]]></xsl:script>


</xsl:document>
