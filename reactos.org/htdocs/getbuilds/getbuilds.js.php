<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/getbuilds/getbuilds.js.php
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/
?>

var currentpage;
var endrev;
var filenum;
var fullrange;
var inputbox_startrev;
var inputbox_endrev;
var isoparameters;
var pagecount;
var startrev;

function loadingsplash(zeroone)
{
	if (zeroone == 1)
		document.getElementById("ajaxloadinginfo").style.visibility = "visible";
	else
		document.getElementById("ajaxloadinginfo").style.visibility = "hidden";
}

function ajaxGet(action, parameters, data)
{
	loadingsplash(1);

	var http_request = false;

	switch(action)
	{
		case "getfiles":
			var url = "ajax-getfiles.php?" + parameters;
			var callback = "getfilesCallback(http_request, data);";
			break;
	}
	
	if (window.XMLHttpRequest)
	{
		// Mozilla, Safari, ...
		http_request = new XMLHttpRequest();
	}
	else if (window.ActiveXObject)
	{
		// IE
		try
		{
			http_request = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			try
			{
				http_request = new ActiveXObject("Microsoft.XMLHTTP");
			}
			catch (e) {}
		}
	}

	if (!http_request)
	{
		alert("Giving up :( Cannot create an XMLHTTP instance");
		return false;
	}
	
	http_request.open("GET", url, true);
	http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");			// Bypass the IE Cache
	http_request.send(null);
	
	http_request.onreadystatechange = function()
	{
		if(http_request.readyState == 4)
		{
			if(http_request.status == 200)
				eval(callback);
			else
				alert("AJAX Request problem!" + "\n\nError Code: " + http_request.status + "\nError Text: " + http_request.statusText + "\nURL: " + url);
		}
	}
}

function getfilesCallback(http_request, data)
{
	// Check for an error
	if( http_request.responseXML.getElementsByTagName("error").length > 0 )
	{
		// For some errors, we show a localized error message
		if( http_request.responseXML.getElementsByTagName("message")[0].firstChild.data == "LIMIT" )
			alert( '<?php printf( addslashes($getbuilds_langres["rangelimitexceeded"]), "' + http_request.responseXML.getElementsByTagName(\"limit\")[0].firstChild.data + '" ); ?>' );
		else
			alert( http_request.responseXML.getElementsByTagName("message")[0].firstChild.data );
		
		loadingsplash(0);
		return;
	}
	
	var html = "";
	
	if( data["requesttype"] == "FirstPageFullLoad" || data["requesttype"] == "PageSwitch" )
	{
		html += '<table id="infotable" cellspacing="0" cellpadding="0"><tr><td id="infobox">';
		
		if( data["requesttype"] == "FirstPageFullLoad" )
		{
			filenum = parseInt( http_request.responseXML.getElementsByTagName("filenum")[0].firstChild.data );
			html += '<?php printf( addslashes($getbuilds_langres["foundfiles"]), "<span id=\"filenum\">' + filenum + '<\/span>" ); ?>';
		}
		else
			html += document.getElementById("infobox").innerHTML;
		
		html += '<\/td>';
		
		// Page number boxes
		html += '<td id="pagesbox">';
		
		if( currentpage == 1 )
		{
			html += '<strong>&laquo;<\/strong> ';
			html += '<strong>&lsaquo; <?php echo addslashes($getbuilds_langres["prevpage"]); ?><\/strong> ';
		}
		else
		{
			html += '<a href="javascript:firstPage()" title="<?php echo addslashes($getbuilds_langres["firstpage_title"]); ?>">&laquo;<\/a> ';
			html += '<a href="javascript:prevPage()" title="<?php echo addslashes($getbuilds_langres["prevpage_title"]); ?>">&lsaquo; <?php echo addslashes($getbuilds_langres["prevpage"]); ?><\/a> ';
		}
		
		html += '<select id="pagesel" size="1" onchange="pageboxChange(this)">';
		
		if( data["requesttype"] == "FirstPageFullLoad" )
		{
			pagecount = 1;
			
			html += '<option selected="selected" value="' + currentpage + '-' + startrev + '"><?php echo addslashes($getbuilds_langres["page"]); ?> ' + currentpage;
			
			if( http_request.responseXML.getElementsByTagName("filenum")[0].firstChild.data > 0 )
				html += ' - ' + http_request.responseXML.getElementsByTagName("firstrev")[0].firstChild.data + ' ... ' + http_request.responseXML.getElementsByTagName("lastrev")[0].firstChild.data + '<\/option>';
		}
		else
			html += document.getElementById("pagesel").innerHTML;
		
		html += '<\/select> ';
		
		if( http_request.responseXML.getElementsByTagName("morefiles")[0].firstChild.data == 0 )
		{
			html += '<strong><?php echo addslashes($getbuilds_langres["nextpage"]); ?> &rsaquo;<\/strong> ';
			html += '<strong>&raquo;<\/strong>';
		}
		else
		{
			html += '<a href="javascript:nextPage()" title="<?php echo addslashes($getbuilds_langres["nextpage_title"]); ?>"><?php echo addslashes($getbuilds_langres["nextpage"]); ?> &rsaquo;<\/a> ';
			html += '<a href="javascript:lastPage()" title="<?php echo addslashes($getbuilds_langres["lastpage_title"]); ?>">&raquo;<\/a>';
		}
		
		html += '<\/td><\/tr><\/table>';

		// File table
		html += '<table class="datatable" cellspacing="0" cellpadding="1">';
		html += '<thead><tr class="head"><th class="fname"><?php echo addslashes($getbuilds_langres["filename"]); ?><\/th><th class="fsize"><?php echo addslashes($getbuilds_langres["filesize"]); ?><\/th><th class="fdate"><?php echo addslashes($getbuilds_langres["filedate"]); ?><\/th><\/tr><\/thead>';
		html += '<tbody>';
		
		var files = http_request.responseXML.getElementsByTagName("file");
	
		if( files.length == 0 )
			html += '<tr class="odd"><td><?php printf( addslashes($getbuilds_langres["nofiles"]), "' + fullrange + '" ); ?><\/td><td>&nbsp;<\/td><td>&nbsp;<\/td><\/tr>';
		else
		{
			var oddeven = false;
			
			for( var i = 0; i < files.length; i++ )
			{
				var fname = files[i].getElementsByTagName("name")[0].firstChild.data;
				var fsize = files[i].getElementsByTagName("size")[0].firstChild.data;
				var fdate = files[i].getElementsByTagName("date")[0].firstChild.data;
				var flink = '<a href="<?php echo $ISO_DOWNLOAD_URL; ?>' + fname.substr(0, 6) + "/" + fname + '">';
				oddeven = !oddeven;
				
				html += '<tr class="' + (oddeven ? "odd" : "even") + '" onmouseover="tr_mouseover(this);" onmouseout="tr_mouseout(this);">';
				html += '<td>' + flink + '<img src="images/cd.gif" alt=""> ' + fname + '<\/a><\/td>';
				html += '<td>' + flink + fsize + '<\/a><\/td>';
				html += '<td>' + flink + fdate + '<\/a><\/td>';
				html += '<\/tr>';
			}
		}
		
		html += '<\/tbody><\/table>';
		
		document.getElementById("filetable").innerHTML = html;
		
		if( data["requesttype"] == "PageSwitch" )
		{
			// Switch the selected page in the Page ComboBox
			var options = document.getElementById("pagesel").getElementsByTagName("option");
			
			for( var i = 0; i < options.length; i++ )
			{
				if( options[i].value.substr( 0, options[i].value.indexOf("-") ) == currentpage )
					options[i].selected = true;
				else if( options[i].selected )
					options[i].selected = false;
			}
		}
	}
	else if( data["requesttype"] == "FirstPageAddPage" )
	{
		pagecount++;
		filenum += parseInt( http_request.responseXML.getElementsByTagName("filenum")[0].firstChild.data );
		
		document.getElementById("filenum").firstChild.data = filenum;
		
		// As always, we have to work around an IE bug
		// If I use "innerHTML" here, the first <OPTION> start tag gets dropped in the IE...
		// Therefore I have to use the DOM functions in this case.
		var option_elem = document.createElement("option");
		var option_text = document.createTextNode( '<?php echo addslashes($getbuilds_langres["page"]); ?> ' + pagecount + ' - ' + http_request.responseXML.getElementsByTagName("firstrev")[0].firstChild.data + ' ... ' + http_request.responseXML.getElementsByTagName("lastrev")[0].firstChild.data );
		
		option_elem.value = pagecount + "-" + data["new_startrev"];
		option_elem.appendChild( option_text );
		
		document.getElementById("pagesel").appendChild( option_elem );
	}
	
	if( http_request.responseXML.getElementsByTagName("morefiles")[0].firstChild.data == 1 && ( data["requesttype"] == "FirstPageFullLoad" || data["requesttype"] == "FirstPageAddPage" ) )
	{
		// There are more files available in the full range. Therefore we have to start another request and add a new page
		data["new_startrev"] = http_request.responseXML.getElementsByTagName("lastrev")[0].firstChild.data;
		data["requesttype"] = "FirstPageAddPage";
		ajaxGet( 'getfiles', 'get=infos&startrev=' + data["new_startrev"] + '&endrev=' + endrev + isoparameters, data );
	}
	else
		loadingsplash(0);
}

function setRowColor(elem, color)
{
	tdl = elem.getElementsByTagName("td");
	
	for( var i = 0; i < tdl.length; i++ )
		tdl[i].style.background = color;
}

function tr_mouseover(elem)
{
	setRowColor( elem, "#FFFFCC" );
}

function tr_mouseout(elem)
{
	if( elem.className == "odd" )
		setRowColor( elem, "#DDDDDD" );
	else
		setRowColor( elem, "#EEEEEE" );
}

function getRevNums()
{
	var rev = document.getElementById("revnum").value;
	
	if( isNaN(rev) || rev < 1 )
	{
		// Maybe the user entered a revision range
		var hyphen = rev.indexOf("-");
		
		if( hyphen > 0 )
		{
			inputbox_startrev = rev.substr( 0, hyphen );
			inputbox_endrev = rev.substr( hyphen + 1 );
		}
		
		if( hyphen <= 0 || isNaN(inputbox_startrev) || isNaN(inputbox_endrev) )
		{
			alert("Invalid revision number!");
			return false;
		}
	}
	else
	{
		inputbox_startrev = rev;
		inputbox_endrev = rev;
	}
	
	return true;
}

function prevRev()
{
	if( getRevNums() )
	{
		inputbox_startrev--;
		
		// 25700 is the lowest rev on the server at the time, when this script has been written
		// There is no harm if this rev does not exist anymore on the FTP server, it's just a min value
		if(inputbox_startrev < 25700)
			return;
		
		document.getElementById("revnum").value = inputbox_startrev;
	}
}

function nextRev()
{
	if( getRevNums() )
	{
		inputbox_startrev++;
		document.getElementById("revnum").value = inputbox_startrev;
	}
}

function showRev()
{
	if( getRevNums() )
	{
		var data = new Array();
		
		currentpage = 1;
		endrev = inputbox_endrev;
		fullrange = document.getElementById("revnum").value;
		isoparameters = "";
		startrev = inputbox_startrev;
		
		if( document.getElementById("bootcd-dbg").checked )
			isoparameters += '&bootcd-dbg=1';
		if( document.getElementById("livecd-dbg").checked )
			isoparameters += '&livecd-dbg=1';
		if( document.getElementById("bootcd-rel").checked )
			isoparameters += '&bootcd-rel=1';
		if( document.getElementById("livecd-rel").checked )
			isoparameters += '&livecd-rel=1';

		data["requesttype"] = "FirstPageFullLoad";
		
		ajaxGet( 'getfiles', 'get=all&startrev=' + startrev + '&endrev=' + endrev + isoparameters, data );
	}
}

function checkForReturn( keyevent )
{
	// keyevent.which - supported under NS 4.0, Opera 5.12, Firefox, Konqueror 3.3, Safari
	if( keyevent )
	{
		if( keyevent.which == 13 )
			showRev();
	}
		
	// window.event - for IE Browsers
	else if( window.event )
	{
		if( window.event.keyCode == 13 )
			showRev();
	}
}

function checkRevNum(elem)
{
	var val = elem.value.replace( /[^[0-9-]/g, "");
	
	// First check if something was changed by the replace function.
	// If not, don't set elem.value = val. Otherwise the cursor would always jump to the last character in IE, when you press any key.
	if( elem.value != val )
		elem.value = val;
}

function load()
{
	document.getElementById("revnum").onkeypress = checkForReturn;
	
	// Show latest files
	var data = new Array();
	
	currentpage = 1;
	endrev = <?php echo $rev; ?>;
	fullrange = <?php echo $rev; ?>;
	isoparameters = "&bootcd-dbg=1&livecd-dbg=1&bootcd-rel=1&livecd-rel=1";
	startrev = <?php echo $rev; ?>
	
	data["requesttype"] = "FirstPageFullLoad";
	
	ajaxGet( 'getfiles', 'get=all&startrev=<?php echo $rev; ?>&endrev=<?php echo $rev; ?>' + isoparameters, data );
}

function pageSwitch(new_page, new_startrev)
{
	var data = new Array();
	
	currentpage = new_page;
	startrev = new_startrev;
	data["requesttype"] = "PageSwitch";
	
	ajaxGet( 'getfiles', 'get=all&startrev=' + startrev + '&endrev=' + endrev + isoparameters, data );
}

function firstPage()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[0].value.split("-");
	pageSwitch( info[0], info[1] );
}

function prevPage()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[ currentpage - 2 ].value.split("-");
	pageSwitch( info[0], info[1] );
}

function pageboxChange(obj)
{
	var info = obj.value.split("-");
	pageSwitch( info[0], info[1] );
}

function nextPage()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[currentpage].value.split("-");
	pageSwitch( info[0], info[1] );
}

function lastPage()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[ document.getElementById("pagesel").getElementsByTagName("option").length - 1 ].value.split("-");
	pageSwitch( info[0], info[1] );
}
