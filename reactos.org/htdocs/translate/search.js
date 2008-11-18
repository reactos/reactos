<!--

	// Ajax driven search box
	// (c) by Klemens Friedl 2006 - http://www.reactos.org/support/


	// Global variable for the request-object
	var http_request = false;
	var tSearch = "";
	var tView = "";
	var tWhere = "";
	var tPicAnimation = "";
	var tResults = "";
	var twebsite = "http://localhost/reactos.org/support/index.php";
	var twebsite2 = "http://localhost/reactos.org/";
	//var twebsite = "http://www.reactos.org/support/index.php";
	//var twebsite2 = "http://www.reactos.org/";
	

	function writeItemList_style_header() {
		var tempa = "";
		
		tempa = "<table width=\"700\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n";
		tempa += "<tr bgcolor=\"#5984C3\"> \n";
		tempa += "<td width=\"30%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Application</strong></font></div></td> \n";
		tempa += "<td width=\"30%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Vendor</strong></font></div></td> \n";
		tempa += "<td width=\"40%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Description</strong></font></div></td> \n";
	  	tempa += "</tr>";
		return tempa;
	}
	
	function writeItemList_style_entry(itemid, itemname, vendorid, vendorname, description, colorcur) {
		var tempb = "";
		
		tempb += "<tr> \n";
		tempb += "<td width=\"30%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><b><a href=\""+twebsite+"?page=db&amp;view=comp&amp;sec=category&amp;group=" + itemid + "\">" + itemname + "</a></b></font></td> \n";
		if (vendorid != 0) {		
			tempb += "<td width=\"30%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><a href=\""+twebsite+"?page=db&amp;view=comp&amp;sec=vendor&amp;vendor=" + vendorid + "\">" + vendorname + "</a></font></td> \n";
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
		return tempb;
	}
	
	function writeItemList_style_footer() {
		return "</table>";
	}	
		
	function write_bar_entry(itemid, itemname) {
		var tempb = "";	
		tempb += "<a href=\""+twebsite+"?page=db&amp;view=comp&amp;sec=category&amp;group=" + itemid + "\">&bull; " + itemname + "</a>\n";
		return tempb;
	}

	function write_rosweb_entry(itemname, itemid) {
		var tempb = "";	
		tempb += "<a href=\""+twebsite2+"?page=" + itemid + "\">&bull; " + itemname + "</a>\n";
		return tempb;
	}

	function write_compsubmit_header() {
		var tempa = "";
		
		tempa = "<table width=\"500\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n";
		tempa += "<tr bgcolor=\"#5984C3\"> \n";
		tempa += "<td width=\"60%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Application</strong></font></div></td> \n";
		tempa += "<td width=\"40%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Vendor</strong></font></div></td> \n";
	  	tempa += "</tr>";
		return tempa;
	}

	function write_compsubmit_entry(itemid, itemname, vendorname, colorcur) {
		var tempb = "";
		
		tempb += "<tr> \n";
		tempb += "<td width=\"60%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><b><a href=\""+twebsite+"?page=db&amp;view=comp&amp;sec=category&amp;group=" + itemid + "\">" + itemname + "</a></b></font></td> \n";
		if (vendorname != ". ") {		
			tempb += "<td width=\"40%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">" + vendorname + "</font></td> \n";
		}
		else {
			tempb += "<td width=\"40%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><i>no vendor defined</i></font></td> \n";
		}
	  	tempb += "</tr>";
		return tempb;
	}

	function write_vendorsubmit_header() {
		var tempa = "";
		
		tempa = "<table width=\"500\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\">\n";
		tempa += "<tr bgcolor=\"#5984C3\"> \n";
		tempa += "<td width=\"50%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Vendor</strong></font></div></td> \n";
		tempa += "<td width=\"50%\"> <div align=\"center\"><font color=\"#FFFFFF\" face=\"Arial, Helvetica, sans-serif\"><strong>Website</strong></font></div></td> \n";
	  	tempa += "</tr>";
		return tempa;
	}

	function write_vendorsubmit_entry(vendorid, vendorname, vendorurl, colorcur) {
		var tempb = "";
		
		tempb += "<tr> \n";
		tempb += "<td width=\"50%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\"><b><a href=\"javascript:\/\/\" onclick=\"UseThisVendor("+vendorid+",\'"+vendorname+"\')\">" + vendorname + "</a></b></font></td> \n";
		if (vendorurl != ".") {	
			tempb += "<td width=\"50%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">" + vendorurl + "</font></td> \n";
		}
		else {
			tempb += "<td width=\"50%\" valign=\"top\" bgcolor=\"" + colorcur + "\"><font size=\"2\">&nbsp;</font></td> \n";
		}
	  	tempb += "</tr>";
		return tempb;
	}

	function deleteItemList() {
		document.getElementById(tResults).innerHTML = "";
	}

	function loadItemList(asearch,aview,awhere,apicani,aoutput) {
		
		// Prevent extra load, when using onblur and nothing has changed
		if (tSearch == asearch) {
			return 0;
		}
		
		tSearch=asearch;
		tView=aview;
		tWhere=awhere;
		tPicAnimation=apicani;
		tResults=aoutput;
		
		if (tWhere == "vendor") {
			checkFields();
		}
		if (tResults == "submitresult" && tSearch.length < 2) {
			enableButtonWizPageNext2(1);
		}
		
		if (asearch.length > 1) {
			document.getElementById(tResults).style.display = "block";
			document.getElementById(tPicAnimation).style.display = "inline";
			if (asearch != "") {
				setCursor('wait');
				if (http_request && (http_request.readyState == 2 || http_request.readyState == 3)) {
					http_request.abort();   // stop running request
				}
				
			
				if (tWhere == "comp") {
					makeRequest(twebsite+'?page=dat&export=grplst&search='+asearch);
				}
				else if (tWhere == "vendor") {
					//alert("vendor");
					makeRequest(twebsite+'?page=dat&export=vdrlst&search='+asearch);
				}
				else if (tWhere.substr(0, 6) == "roscms") {
					//alert("vendor");
					makeRequest(twebsite2+'roscms/search.php?search='+asearch+'&searchlang='+tWhere.substr(7, 2));
				}
			}
		}
		else {
			document.getElementById(tResults).style.display = "none";
			document.getElementById(tPicAnimation).style.display = "none";
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
					
					// Table header:
					if (tView == "table") {
						lstData = writeItemList_style_header();
					}
					else if (tView == "submit") {
						lstData = write_compsubmit_header();
					}
					else if (tView == "submit_vendor") {
						lstData = write_vendorsubmit_header();
					}
					
					// XML schema:
					if (tWhere == "comp") {
						var items = http_request.responseXML.getElementsByTagName("item");
						var descs =  http_request.responseXML.getElementsByTagName("desc");
						var vendo =  http_request.responseXML.getElementsByTagName("vendor");
					}
					else if (tWhere == "vendor") {
						var vendo =  http_request.responseXML.getElementsByTagName("vendor");
						var vendurl =  http_request.responseXML.getElementsByTagName("url");
					}
					else if (tWhere.substr(0, 6) == "roscms") {
						var webcontent =  http_request.responseXML.getElementsByTagName("content");
					}
	
					// Colors:
					var colorcur="";
					var color1="#E2E2E2";
					var color2="#EEEEEE";
					var colorcounter=0;
					
					// XML length:
					var xmllength=0;
					if (tWhere == "comp") {
						xmllength = items.length;
					}
					else if (tWhere == "vendor") {
						xmllength = vendo.length;
					}
					else if (tWhere.substr(0, 6) == "roscms") {
						xmllength = webcontent.length;
					}
					
					// XML-Loop:
					for (var i = 0; i < xmllength; i++) {
						// Table colors:
						colorcounter++;
						if (colorcounter == "1") {
							colorcur = color1;
						}
						else if (colorcounter == "2") {
							colorcounter="0";
							colorcur = color2;
						}
						
						// Table data:
						if (tView == "table") {
							lstData += writeItemList_style_entry(items[i].getAttributeNode("id").value, items[i].firstChild.data, vendo[i].getAttributeNode("id").value, vendo[i].firstChild.data, descs[i].firstChild.data, colorcur);
						}
						else if (tView == "submit") {
							lstData += write_compsubmit_entry(items[i].getAttributeNode("id").value, items[i].firstChild.data, vendo[i].firstChild.data, colorcur);
						}
						else if (tView == "bar") {
							lstData += write_bar_entry(items[i].getAttributeNode("id").value, items[i].firstChild.data);
						}
						else if (tView == "submit_vendor") {
							lstData += write_vendorsubmit_entry(vendo[i].getAttributeNode("id").value, vendo[i].firstChild.data, vendurl[i].firstChild.data, colorcur);
						}
						else if (tView == "rosweb") {
							lstData += write_rosweb_entry(webcontent[i].getAttributeNode("id").value, webcontent[i].firstChild.data);
						}
						
					}
					
					// Table footer:
					if (tView == "table") {
						lstData += writeItemList_style_footer();
					}
					else if (tView == "submit") {
						lstData += writeItemList_style_footer();
						lstData += "<p>Click on an application in the list above or <a href=\"javascript://\" onclick=\"WizPag2()\">submit new application</a> to the database.</p>";
					}
					else if (tView == "submit_vendor") {
						lstData += writeItemList_style_footer();
						lstData += "<p>Click on a vendor name in the search result list above, choose a vendor from the <b><a href=\"javascript://\" onclick=\"SelectVendor()\">vendor list</a></b> or <b><a href=\"javascript://\" onclick=\"AddVendor()\">submit a new vendor</a></b> to the database.</p>";
					}
					
					if (tResults == "submitresult") {
						enableButtonWizPageNext2(1);
					}
					
					// HTML output:
					document.getElementById(tResults).innerHTML = lstData;
				}
				else {
					// No related database entries found:
					if (tView == "table") {
						document.getElementById(tResults).innerHTML = "<p>Your search - " + tSearch + " - did not match any database entries.</p>";
					}	
					else if (tView == "bar") {
						document.getElementById(tResults).innerHTML = "<center>no entries found</center>";
					}	
					else if (tView == "submit") {
						document.getElementById(tResults).innerHTML = "<p>Your search - " + tSearch + " - did not match any database entries.</p><p><b><a href=\"javascript://\" onclick=\"WizPag2()\">Submit new application</a></b> to the database.</p>";
						enableButtonWizPageNext2(2);
					}	
					else if (tView == "submit_vendor") {
						document.getElementById(tResults).innerHTML = "<p>Your search - " + tSearch + " - did not match any database entries.</p><p>Choose a vendor from the <b><a href=\"javascript://\" onclick=\"SelectVendor()\">vendor list</a></b> or <b><a href=\"javascript://\" onclick=\"AddVendor()\">submit a new vendor</a></b> to the database.</p>";
					}	
					else if (tView == "rosweb") {
						document.getElementById(tResults).innerHTML = "<center>no entries found</center>";
					}	
				}	
				
				// reset mouse cursor
				setCursor('auto');
				
				// reset loading picture:
				document.getElementById(tPicAnimation).style.display = "none";
				 
			} 
			else {
				alert('There was a problem with the request:\n' + http_request.statusText);
			}
		}
	
	}

-->