<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}

?> <div class="navTitle">Search</div>   
 <div class="navBox"><form method="get" action="#" style="padding:0;margin:0">
  <div style="text-align:center;">

   <input name="q" value=""  size="12" maxlength="80" class="searchInput" type="text" /><input name="domains" value="http://www.reactos.org" type="hidden" /><input name="sitesearch" value="http://www.reactos.org" type="hidden" />
   <input name="btnG" value="Go" type="submit" class="button" />
	

  </div></form>
 </div>
 <ol>
  <div id="outputItemListShort" style="display: none"></div> 
 </ol>
<p></p>
<script type="text/javascript" language="javascript">
<!--

	// Global variable for the request-object
	var http_request = false;
	var tSearch = "";
	
	document.getElementById('bSearch').style.display = "none";
	
	
	
	function writeItemList_style_header() {
		var tempa = "";
		
		tempa = "<table width=\"700\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n";
		tempa += "<tr bgcolor=\"#5984C3\"> \n";
		tempa += "<td width=\"30%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Application</strong></font></div></td> \n";
		tempa += "<td width=\"30%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Vendor</strong></font></div></td> \n";
		tempa += "<td width=\"40%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Description</strong></font></div></td> \n";
	  	tempa += "</tr>";
//		alert('test:' + tempa);
//		document.getElementById("outputItemList").innerHTML = tempa;
//		document.getElementById('tempc').value = tempa;
		return tempa;
	}
	
	function writeItemList_style_entry(itemid, itemname, vendorid, vendorname, description, colorcur) {
		var tempb = "";
		
		tempb += "<tr> \n";
		tempb += "<td width=\"30%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><a href=\"<?php echo $RSDB_intern_link_group_comp; ?>" + itemid + "\">" + itemname + "</a></font></td> \n";
		if (vendorid != 0) {		
			tempb += "<td width=\"30%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><a href=\"<?php echo $RSDB_intern_link_vendor_sec_comp; ?>" + vendorid + "\">" + vendorname + "</a></font></td> \n";
		}
		else {
			tempb += "<td width=\"30%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">&nbsp;</font></td> \n";
		}
		if (description != ".") {		
			tempb += "<td width=\"40%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">" + description + "</font></td> \n";
		}
		else {
			tempb += "<td width=\"40%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">&nbsp;</font></td> \n";
		}
	  	tempb += "</tr>";
//		alert('test:' + tempb);
//		document.getElementById("outputItemList").innerHTML += tempb;
//		document.getElementById('tempc').value += tempb;
		return tempb;
	}
	
	function writeItemList_style_footer() {
//		document.getElementById("outputItemList").innerHTML += "</table>";
//		document.getElementById('tempc').value += "</table>";
		return "</table>";
	}	
		
	function deleteItemList() {
		document.getElementById("outputItemList").innerHTML = "";
	}
	

	function loadItemList(asearch) {
		tSearch=asearch;
		
		if (asearch.length > 1) {
			document.getElementById('outputItemList').style.display = "block";
			document.getElementById('ajaxload').style.display = "inline";
		   if (asearch != "") {
			 setCursor('wait');
			 if (http_request && (http_request.readyState == 2 || http_request.readyState == 3)) {
			   http_request.abort();     // falls ein Request läuft, diesen abbrechen
			 }
		
			 //loadXMLDoc('getentry.php');
			 makeRequest('<?php echo $RSDB_intern_link_export2; ?>grplst&search='+asearch);
		   }
	   }
	   else {
			document.getElementById('outputItemList').style.display = "none";
			document.getElementById('ajaxload').style.display = "none";
			deleteItemList();
	   }
	}
	
	
	function setCursor(mode) {
	  var pageBody = document.getElementsByTagName("body")[0];
	  pageBody.style.cursor = mode;
	}
	
	
	
	function makeRequest(url) {

		http_request = false;
	
		if (window.XMLHttpRequest) { // Mozilla, Safari,...
			http_request = new XMLHttpRequest();
			if (http_request.overrideMimeType) {
				http_request.overrideMimeType('text/xml');
			}
		} else if (window.ActiveXObject) { // IE
			try {
				http_request = new ActiveXObject("Msxml2.XMLHTTP");
			} catch (e) {
				try {
				http_request = new ActiveXObject("Microsoft.XMLHTTP");
				} catch (e) {}
			}
		}
	
		if (!http_request) {
			alert('Giving up :( Cannot create an XMLHTTP instance');
			return false;
		}
		http_request.onreadystatechange = showItemList;
		http_request.open('GET', url, true);
		http_request.send(null);
	
	}
	
	function showItemList() {
	
		if (http_request.readyState == 4) {
			if (http_request.status == 200) {
									
				var lstData = "";
				
				var xmldoc = http_request.responseXML;
				var root_node = xmldoc.getElementsByTagName('root').item(0);
				
				if ((root_node.firstChild.data.search(/#none#/)) == -1) {
					lstData = "";
					lstData = writeItemList_style_header();
					var items = http_request.responseXML.getElementsByTagName("item");
					var descs =  http_request.responseXML.getElementsByTagName("desc");
					var vendo =  http_request.responseXML.getElementsByTagName("vendor");
					
					// Colors
					var colorcur="";
					var color1="#E2E2E2";
					var color2="#EEEEEE";
					var colorcounter=0;
	
//					alert(items.length);
					for (var i = 0; i < items.length; i++) {
//						lstData += items[i].firstChild.data;
//						alert(items[i].firstChild.data);
						//alert(items[i].firstChild.nodeValue);
//						alert(items[i].getAttributeNode("id").value);
//						alert(descs[i].firstChild.data);
//						alert(vendo[i].firstChild.data);
//						alert(vendo[i].getAttributeNode("id").value);
						
						//writeItemList_style_entry(itemid, itemname, vendorid, vendorname, description, colorcur);
						
						colorcounter++;
						if (colorcounter == "1") {
							colorcur = color1;
						}
						else if (colorcounter == "2") {
							colorcounter="0";
							colorcur = color2;
						}
						
						lstData += writeItemList_style_entry(items[i].getAttributeNode("id").value, items[i].firstChild.data, vendo[i].getAttributeNode("id").value, vendo[i].firstChild.data, descs[i].firstChild.data, colorcur);
						
					}
					
					lstData += writeItemList_style_footer();
					
					document.getElementById("outputItemList").innerHTML = lstData;
				}
				else {
					document.getElementById("outputItemList").innerHTML = "<p>Your search - " + tSearch + " - did not match any database entries.</p>";
					//deleteItemList();
					//alert(root_node.firstChild.data);
				}	
				
				
				// reset mouse cursor
				setCursor('auto');
				
				// reset loading picture:
				document.getElementById('ajaxload').style.display = "none";
				 
			} else {
				alert('There was a problem with the request:\n' + http_request.statusText);
			}
		}
	
	}

-->
</script>