<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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

	// To prevent hacking activity:
	if ( !defined('ROSCMS_SYSTEM') )
	{
		die("Hacking attempt");
	}
	
	global $roscms_intern_account_id;
	global $roscms_intern_login_check_username;
	global $roscms_security_level;
	global $roscms_security_memberships;
	global $roscms_standard_language;
	
	global $RosCMS_GET_debug;
	
	$RosCMS_GET_cms_edit = "";
	if (array_key_exists("edit", $_GET)) $RosCMS_GET_cms_edit=htmlspecialchars($_GET["edit"]);
	
	$roscms_intern_entry_per_page = 25;
	
	if ($RosCMS_GET_debug) {
		echo "<h1>DEBUG-Mode</h1>";
	}
	
?><noscript>
	<h3>RosCMS v3 requires Javascript, please activate/allow it.</h3>
	<p>It does work fine in Internet Explorer 5.5+, Mozilla Firefox 1.5+, Opera 9.1+, Safari 3.2+ and probably every client with basic Javascript (+AJAX) support.</p>
</noscript>
<script type="text/javascript" language="javascript">
<!--
	/* Global Vars */
	var roscms_branch = 'website';
	
	var roscms_current_page = 'new';
	var roscms_prev_page = 'new';
	var roscms_current_tbl_position = 0;
	
	var roscms_archive = 0;
	
	var autosave_coundown = 100000; // 10000; 100000
	
	var submenu_button = '';
	
	var autosave_val = '';
	var autosave_cache = '';
	var autosave_timer;
	
	var filtstring1 = '';
	var filtstring2 = '';
	
	var roscms_page_load_finished = false;

	

	
	
	/* Users favorite language */
	
	<?php	
		$query_account_lang = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
		$result_account_lang = @mysql_fetch_array($query_account_lang);
		
		if ($result_account_lang['user_language'] != "") {
			echo "var userlang = '".$result_account_lang['user_language']."';";
		}
		else {
			echo "var userlang = '".$roscms_standard_language."';";
		}
	?>
	
										
	var filtercounter = 0;
	var filterid = 0;
	var alertactiv = '';
	
	var timerquickinfo;
	
	exitmsg = "Click Cancel to continue with RosCMS, click OK to leave RosCMS.\n\nThanks for using RosCMS!";
	exitmsg2 = exitmsg;
									
	
-->
</script>
<script type="text/javascript" language="javascript">
<!--
	function load_script(src) {
		var document_scripts = document.getElementsByTagName("script");
		
		for (document_scripts_index = 0; document_scripts_index < document_scripts.length; ++document_scripts_index) {
			var document_script = document_scripts[document_scripts_index];
			if (document_script.src == src) return false;
		}
		
		var script = document.createElement('script');
		script.type = 'text/javascript';
		script.src = src;
		document.getElementsByTagName('head')[0].appendChild(script); 
	}
-->
</script>
<script language="javascript" type="text/javascript" src="<?php echo $roscms_intern_webserver_roscms; ?>editor/jscripts/tiny_mce/tiny_mce_src.js"></script>
<script language="javascript" type="text/javascript" src="<?php echo $roscms_intern_webserver_roscms; ?>editor/jscripts/mef.js"></script>
<script language="javascript" type="text/javascript" src="<?php echo $roscms_intern_webserver_roscms; ?>js/diffv3.js"></script>
<script type="text/javascript" language="javascript">
<!--

	var nres=1;


	function selcb(cbid) {
		if (cbid.substr(0,2) == 'cb') { /* check for checkbox id */
			cbid = cbid.substr(2);
		}

		if (document.getElementById("cb"+cbid).checked == true) {
			document.getElementById("cb"+cbid).checked = false;
		}
		else {
			document.getElementById("cb"+cbid).checked = true;
		} 
	}

	function setrowcolor(rownum,rowcolor) {
		if (!document.getElementById || !document.getElementsByTagName || !document.getElementById("tr"+rownum)) {
			return;
		}
		
		cell_arr = document.getElementById("tr"+rownum).getElementsByTagName('td');
		
		for (i=0; i<cell_arr.length; i++) {
			cell_arr[i].style.backgroundColor = rowcolor;
		}
	}
	
	markedrows = new Array(); //global
	function hlrow(rownum,hlmode) {
		rowstatus = document.getElementById("tr"+rownum).className;
		
		switch(hlmode) {
			case 1: //on mouseover
				setrowcolor(rownum,"#ffffcc");
				break;
			case 2: //on mouseout
				if (markedrows["tr"+rownum]!=1) {
						if (rowstatus == 'odd' || rowstatus == 'even') {
							if (rownum%2) setrowcolor(rownum,"#dddddd");
							else setrowcolor(rownum,"#eeeeee");
						}
						else if(rowstatus == 'new') {
							setrowcolor(rownum,"#B5EDA3");
						}
						else if(rowstatus == 'draft') {
							setrowcolor(rownum,"#FFE4C1");
						}
						else if(rowstatus == 'transg') {
							setrowcolor(rownum,"#A3EDB4");
						}
						else if(rowstatus == 'transb') {
							setrowcolor(rownum,"#D6CAE4");
						}
						else if(rowstatus == 'transr') {
							setrowcolor(rownum,"#FAA5A5");
						}
						else {
							setrowcolor(rownum,"#FFCCFF");
						}
				}
				else {
					setrowcolor(rownum,"#ffcc99");	
				}
				break;
			case 3: //on click
				if (markedrows["tr"+rownum]!=1) {
					setrowcolor(rownum,"#ffcc99");
					markedrows["tr"+rownum]=1;
					//rcol_menu();
				}
				else {
					if (rowstatus == 'odd' || rowstatus == 'even') {
						if (rownum%2) setrowcolor(rownum,"#dddddd");
						else setrowcolor(rownum,"#eeeeee");
					}
					else if(rowstatus == 'new') {
						setrowcolor(rownum,"#B5EDA3");
					}
					else if(rowstatus == 'draft') {
						setrowcolor(rownum,"#FFE4C1");
					}
					else if(rowstatus == 'transg') {
						setrowcolor(rownum,"#A3EDB4");
					}
					else if(rowstatus == 'transb') {
						setrowcolor(rownum,"#D6CAE4");
					}
					else if(rowstatus == 'transr') {
						setrowcolor(rownum,"#FAA5A5");
					}
					else {
						setrowcolor(rownum,"#FFCCFF");
					}
					markedrows["tr"+rownum]=0;
				}
				break;
		}
	}
	
	function select_all_checkboxes(zeroone) {
		var scb = true;
		
		if (zeroone == 1) {
			scb = true;
		}
		else {
			scb = false;
		}
	
		for (var i=1; i<=nres; i++) {
			document.getElementById("cb"+i).checked = scb;
		}
	}

	function select_all(zeroone) {
		for (var i=1; i<=nres; i++) {
			markedrows["tr"+i] = zeroone;
			if (zeroone==1) {
				setrowcolor(i,"#ffcc99");
			}
			else {
				rowstatus = document.getElementById("tr"+i).className;
				if (rowstatus == 'odd' || rowstatus == 'even') {
					if (i%2) setrowcolor(i,"#dddddd");
					else setrowcolor(i,"#eeeeee");
				}
				else if(rowstatus == 'new') {
					setrowcolor(i,"#B5EDA3");
				}
				else if(rowstatus == 'draft') {
					setrowcolor(i,"#FFE4C1");
				}
				else if(rowstatus == 'transg') {
					setrowcolor(i,"#A3EDB4");
				}
				else if(rowstatus == 'transb') {
					setrowcolor(i,"#D6CAE4");
				}
				else if(rowstatus == 'transr') {
					setrowcolor(i,"#FAA5A5");
				}
				else {
					setrowcolor(i,"#FFCCFF");
				}
			}
		}
		select_all_checkboxes(zeroone);
	}
		
	function add_js_extras() { // table mouse events
		if (!document.getElementById) return;
		
		for (i=1; i<=nres; i++) {
			//row highlighting
			if (hlRows) {
				document.getElementById("tr"+i).onmouseover = function() {hlrow(this.id.substr(2,4),1); show_quickinfo(this.getElementsByTagName('td')[3].className);}
				document.getElementById("tr"+i).onmouseout = function() {hlrow(this.id.substr(2,4),2); stop_quickinfo();}
				document.getElementById("tr"+i).getElementsByTagName('td')[0].onclick = function() {hlrow(this.parentNode.id.substr(2,4),3); selcb(this.parentNode.id.substr(2,4));}
				//document.getElementById("tr"+i).getElementsByTagName('td')[c1].onclick = function() {hlrow(this.parentNode.id.substr(2,4),3); alert(this.parentNode.id.substr(2,4));}
				//document.getElementById("tr"+i).getElementsByTagName('td')[c2].onclick = function() {hlrow(this.parentNode.id.substr(2,4),3);}
				
				//document.getElementById("tr"+i).getElementsByTagName('td')[1].onclick = function() {load_frameedit(roscms_current_page, this.parentNode.id.substr(2));}
				document.getElementById("tr"+i).getElementsByTagName('td')[1].onclick = function() {selstar(this.className, document.getElementById('bstar_'+this.parentNode.id.substr(2,4)).className, <?php echo $roscms_intern_account_id; ?>, 'bstar_'+this.parentNode.id.substr(2,4));}
				document.getElementById("tr"+i).getElementsByTagName('td')[2].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				document.getElementById("tr"+i).getElementsByTagName('td')[3].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				document.getElementById("tr"+i).getElementsByTagName('td')[4].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				
				// check for optional columns
				if (document.getElementById("tr"+i).getElementsByTagName('td')[5]) document.getElementById("tr"+i).getElementsByTagName('td')[5].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[6]) document.getElementById("tr"+i).getElementsByTagName('td')[6].onclick = function() {load_frameedit(roscms_current_page, this.className);}				
				if (document.getElementById("tr"+i).getElementsByTagName('td')[7]) document.getElementById("tr"+i).getElementsByTagName('td')[7].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[8]) document.getElementById("tr"+i).getElementsByTagName('td')[8].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[9]) document.getElementById("tr"+i).getElementsByTagName('td')[9].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[10]) document.getElementById("tr"+i).getElementsByTagName('td')[10].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[11]) document.getElementById("tr"+i).getElementsByTagName('td')[11].onclick = function() {load_frameedit(roscms_current_page, this.className);}
				if (document.getElementById("tr"+i).getElementsByTagName('td')[12]) document.getElementById("tr"+i).getElementsByTagName('td')[12].onclick = function() {load_frameedit(roscms_current_page, this.className);}
			}
		}
	}

	function do_something() {
		alert(selectedEntries());
	}

	function selectedEntries() {
		var i=0;
		var n_ids=0; //number of ids in cookie str
		var mvstr = " ";
		
		var currentcolor = "";
		
		for (var i=1; i<=nres; i++) {
			currentcolor = document.getElementById("tr"+i).getElementsByTagName('td')[1].style.backgroundColor;
			if(currentcolor == "rgb(255, 204, 153)" || currentcolor == "#ffcc99") {
				n_ids++;
				if (n_ids>500) {
					alert("Maximum: 500 entries");
					break;
				}
				//alert(document.getElementById('bstar_'+i).parentNode.className);
				var tsplit1 = document.getElementById('bstar_'+i).parentNode.className.split("|");
				var tsplit2 = tsplit1[1].split("-");
				
				mvstr = mvstr +"-"+ i +"_"+ tsplit2[0];
			}
		}
		return n_ids + "|"+ mvstr.substr(2, mvstr.length);
	}
	
	function select_inverse() {
		var currentcolor, rownum;
		
		for (var i=1; i<=nres; i++) {
			rownum = i;
			currentcolor = document.getElementById("tr"+i).getElementsByTagName('td')[1].style.backgroundColor;
			if(currentcolor == "rgb(255, 204, 153)" || currentcolor == "#ffcc99") {
				
				rowstatus = document.getElementById("tr"+rownum).className;
				if (rowstatus == 'odd' || rowstatus == 'even') {
					if (rownum%2) setrowcolor(rownum,"#dddddd");
					else setrowcolor(rownum,"#eeeeee");
				}
				else if(rowstatus == 'new') {
					setrowcolor(rownum,"#B5EDA3");
				}
				else if(rowstatus == 'draft') {
					setrowcolor(rownum,"#FFE4C1");
				}
				else if(rowstatus == 'transg') {
					setrowcolor(rownum,"#A3EDB4");
				}
				else if(rowstatus == 'transb') {
					setrowcolor(rownum,"#D6CAE4");
				}
				else if(rowstatus == 'transr') {
					setrowcolor(rownum,"#FAA5A5");
				}
				else {
					setrowcolor(rownum,"#FFCCFF");
				}
				document.getElementById("cb"+i).checked = false;
				markedrows["tr"+i]=0;

			}
			else {
				setrowcolor(rownum,"#ffcc99");	
				document.getElementById("cb"+i).checked = true;
				markedrows["tr"+i]=1;

			}
		}
	}
	
	function select_stars(zeroone) {
		select_all(0); /* deselect all */
		
		var sstar = 'cStarOff';
		
		if (zeroone == 1) {
			sstar = 'cStarOn';
		}
		else {
			sstar = 'cStarOff';
		}

		for (var i=1; i<=nres; i++) {
			if (document.getElementById("tr"+i).getElementsByTagName('td')[1].getElementsByTagName('div')[0].className == sstar) {
				setrowcolor(i,"#ffcc99");
				document.getElementById("cb"+i).checked = true;
				markedrows["tr"+i]=1;
			}
			else {
				document.getElementById("cb"+i).checked = false;
				markedrows["tr"+i]=0;
			}
		}
	}
	
	function select_nds(ndsb) {
		select_all(0); /* deselect all */
		
		var sstar1 = 'odd';
		var sstar2 = 'even';
		
		switch (ndsb) {
			default:
			case 'stable':
				sstar1 = 'odd';
				sstar2 = 'even';
				break;
			case 'new':
				sstar1 = 'new';
				sstar2 = 'new';
				break;
			case 'draft':
				sstar1 = 'draft';
				sstar2 = 'draft';
				break;
			case 'transg':
				sstar1 = 'transg';
				sstar2 = 'transg';
				break;
			case 'transb':
				sstar1 = 'transb';
				sstar2 = 'transb';
				break;
			case 'transr':
				sstar1 = 'transr';
				sstar2 = 'transr';
				break;
			case 'unknown':
				sstar1 = 'unknown';
				sstar2 = 'unknown';
				break;
			/* @todo: 'archive' */
		}
		
		for (var i=1; i<=nres; i++) {
			if (document.getElementById("tr"+i).className == sstar1 || document.getElementById("tr"+i).className == sstar2) {
				setrowcolor(i,"#ffcc99");
				document.getElementById("cb"+i).checked = true;
				markedrows["tr"+i]=1;
			}
			else {
				document.getElementById("cb"+i).checked = false;
				markedrows["tr"+i]=0;
			}
		}
	}

	
-->
</script>

	
<style type="text/css">
	<!--
	
		.roscms_container {
			position:				relative;
			top:					0px; 
			left:					0px;
			min-width: 350px;
		}
	
		.datatable {
			/*border:1px dashed red;*/
			overflow: 				hidden;
			min-width: 350px;
			width: 100%;
			cursor: pointer;
			table-layout: fixed; /* without this, IE's trident engine went nuts and forget about "overflow" and "white-space" !!! */
		}
		
		.datatable tr.head th,
		.datatable tr.head {
			background-image:   none;
			background:         #5984C3;
			color:				#FFFFFF;
			font-family: 		Verdana, Arial, Helvetica, sans-serif;
			/*font-size:			80%;*/
			font-style:			normal;
			text-align:         left;
			cursor: default;
		}
		
		.datatable tr.new td,
		.datatable tr.draft td,
		.datatable tr.transg td,
		.datatable tr.transb td,
		.datatable tr.transr td,
		.datatable tr.unknown td,
		.datatable tr.odd td,
		.datatable tr.even td {
			color:				#000000;
			font-family: 		Verdana, Arial, Helvetica, sans-serif;
			font-style:			normal;	
			border-bottom:		1px solid #bbb;
			border-bottom-width: 	1px;
			background-image:   	none;
			overflow: 				hidden;
			border-bottom-style: 	solid;
			border-bottom-color: 	#bbbbbb;
			/*font-size: 			80%;*/
			empty-cells: 		show;
			
			
			empty-cells: 		show;
			/*white-space: 		nowrap;*/
			overflow-x: 		hidden;
			overflow-y: 		hidden;
			overflow:			hidden;
			
		}		
	
		.datatable tr.odd td,
		.datatable tr.odd {
			background-color: #E2E2E2;
		}
		.datatable tr.even td,
		.datatable tr.even {
			background-color: #EEEEEE;
		}
		
		.datatable tr.new td,
		.datatable tr.new {
			background-color: #B5EDA3;
		}
		
		.datatable tr.draft td,
		.datatable tr.daft {
			background-color: #FFE4C1;
		}

		.datatable tr.unknown td,
		.datatable tr.unknown {
			background-color: #FFCCFF;
		}
		
		.datatable tr.transg td,
		.datatable tr.transg {
			background-color: #A3EDB4;
		}

		.datatable tr.transb td,
		.datatable tr.transb {
			background-color: #D6CAE4;
		}
		
		.datatable tr.transr td,
		.datatable tr.transr {
			background-color: #FAA5A5;
		}
		
	
		.cMark {
			width: 30px;
			text-align:         right;
			/*max-width:			50px;*/
			/* border: 1px dashed red; */
		}
		
		.cStar {
			/*overflow:			hidden;*/
			width: 				18px;
			/* border: 1px dashed red; */
		}
	
		.cCid {
			width: 				180px;
			overflow: 			hidden;
			/* border: 1px dashed red; */
		}
	
		.cExtraCol {
			width: 				75px;
			overflow: 			hidden;
			/* border: 1px dashed red; */
		}
		
		.cRev {
			/*width: 40%; */
			overflow: hidden;
			/* border: 1px dashed red; */
		}
		
		.cDate {
			width: 				90px;
			overflow: 			hidden;
			/* border: 1px dashed red; */
		}
	
		.cSpace {
			width: 10px;
			overflow: hidden;
			/* border: 1px dashed red; */
		}
		
		.tcp { /*table content preview*/
			color: #777777;
		}
		
		.cell-height {
			overflow: hidden;
			white-space: nowrap;
			/*height: 14px;*/
			display: block;
			vertical-align: middle;
			text-align: left;
			width: 100%;
			float: left;
		}
		
		.cStarOn {
			/*background:			url('images/star_on.gif') no-repeat left;
			display: 			block;*/
			border: 0px;
			background-x-position: 	1px;
			background-y-position: 	3px;
			background-repeat: 		no-repeat;
			background-image: 		url('images/star_on.gif');
			cursor: pointer;
		}
		
		.cStarOff {
			border: 0px;
			background-x-position: 	1px;
			background-y-position: 	3px;
			background-repeat: 		no-repeat;
			background-image: 		url('images/star_off.gif');
			cursor: pointer;
		}
		
		.tabselect {
			/*font-size: 80%;	*/
			padding-top: 4px;
			padding-right: 0px;
			padding-bottom: 2px;
			padding-left: 0px;
			background-color: #C9DAF8;
		}
		
		.listxofy {
			float:right;
			text-align:right;
			padding-top:2px;
		}
		
		.tableswhitespace {		
			font-size: 			80%;
			text-align: 		center;
			padding-top: 		0px;
			padding-right: 		20px;
			padding-bottom:		0px;
			padding-left: 		20px;
			/*height:				50px;*/
			background-color:	#FFFFFF;
		}
				
		.bubble_bg {
			background:#C9DAF8 none repeat scroll 0%;
			/*margin-bottom:0.6em;*/
		}
		.rounded_ul {
			background:transparent url(images/ul.gif) no-repeat scroll left top;
		}
		.rounded_ur {
			background:transparent url(images/ur.gif) no-repeat scroll right top;
		}
		.rounded_ll {
			background:transparent url(images/ll.gif) no-repeat scroll left bottom;
		}
		.rounded_lr {
			background:transparent url(images/lr.gif) no-repeat scroll right bottom;
		}
		.bubble {
			padding:8px;
		}
		
		
		.subma {
			background:#C9DAF8 none repeat scroll 0%;
		}
		.submb {
			background: #FFFFFF none repeat scroll 0%;
		}
	
		.tabmenu {
			margin-top: 10px;
		}
		
		.subm1 {
			background:transparent url(images/ul.gif) no-repeat scroll left top;
		}
		.subm2 {
			background:transparent url(images/ll.gif) no-repeat scroll left bottom;
			padding-top: 3px;
			padding-bottom: 3px;
			padding-left: 10px;
			color: #006090;
			text-decoration: underline;
			cursor: pointer;
		}
		.subm3 {
			color: #006090;
			cursor: pointer;
		}
		
		
		.laba {
			background: #C9DAF8 none repeat scroll 0%;
			width: 140px;
		}
		
		.lab1 {
			background:transparent url(images/ul.gif) no-repeat scroll left top;
		}
		.lab2 {
			background:transparent url(images/ll.gif) no-repeat scroll left bottom;
		}
		.lab3 {
			background:transparent url(images/ur.gif) no-repeat scroll right top;
		}
		.lab4 {
			background:transparent url(images/lr.gif) no-repeat scroll right bottom;
		}

		.labtitel {
			cursor:	pointer;
			padding-top: 2px;
			padding-left: 4px;
			padding-right: 4px;
		}
		.labcontent {
			padding-bottom: 4px;
			padding-left: 4px;
			padding-right: 4px;
		}
		
		.lablinks {
			background: #FFFFFF none repeat scroll 0%;
			padding-top: 3px;
			padding-bottom: 3px;
			padding-left: 10px;
			padding-right: 3px;
			text-decoration: underline;
		}

		.lablinks2 {
			background: #FFFFFF none repeat scroll 0%;
			padding-top: 3px;
			padding-bottom: 3px;
			padding-left: 10px;
			padding-right: 3px;
		}

		.filterbar {
			padding-bottom: 5px;
			border-bottom: 1px;
			border-bottom-style: solid;
			border-bottom-color: #bbbbbb;
		}
		
		.filterbar2 {
			padding-top: 4px;
		}
		
		.filterbutton {
			/*border: 1px solid #bbbbbb;*/
			padding-top: 2px;
			padding-bottom: 2px;
			padding-right: 3px;
			padding-left: 3px;
			cursor:pointer;
			color:#006090;
		}
		
		.tfind {
			color:#999999;
		}
		
		.infobox {
			background: #FAD163 none repeat scroll 0%;
			width: 400px;
		}
		
		.infoboxc {
			padding-top: 4px;
			padding-bottom: 4px;
			padding-left: 4px;
			padding-right: 4px;
			text-align: center;;
		}
		
		.l {
			text-decoration:underline;
			color: #006090;
			font-family: Verdana, Arial, Helvetica, sans-serif;
			font-style: normal;
			cursor: pointer;
		}
		
		
		/* data_edit */

		.edittagbody {
			border-bottom: 1px solid #bbb; 
			border-bottom-width: 1px; 
			border-right: 1px solid #bbb; 
			border-right-width: 1px; 
			background: #F2F2F2 none repeat scroll 0%;
		}
		.edittagbody2 {
			margin:10px;
		}

		.detailbody {
			background: #FAE7B2 none repeat scroll 0%; 
			width: 100%; 
			height: 30px; 
			vertical-align: middle;
		}
		
		.detailmenubody {
			padding: 5px; 
			font-size: 105%; 
			text-decoration:none;
		}
		
		.detailmenu {
			color: #006090;
			text-decoration: underline;
			cursor: pointer;
		}
		
		.frmeditbutton {
			cursor:pointer;
			color:#006090;
		}
		
	-->
	</style>
	<!--[if IE]>
	<style type="text/css">
		.bubble_bg {
			/* fix IE glitch */
			margin-bottom: -35px;
		}
		.filterbutton {
			padding-top: 4px;
			padding-bottom: 0px;
		}
		.spacer {
			/* fix IE glitch */
			margin-bottom: -18px;
		}
	</style>
	<![endif]-->

	<script type="text/javascript" language="javascript">
	<!--
	
		function textbox_hint(objid, objval, objhint, zeroone) {
			//alert('id: '+objid+', value: '+objval+'#; '+document.getElementById(objid).value);
			
			if (zeroone == 1 && objval == objhint) {
				document.getElementById(objid).value = '';
				document.getElementById(objid).style.color ='#000000';
			}
			if (zeroone == 0 && objval == '') {
				document.getElementById(objid).value = objhint;
				document.getElementById(objid).style.color = '#999999';
			}
		
		}
	-->
	</script>
<?php require("inc/data_menu.php"); ?>
	<div class="spacer">&nbsp;</div>
	<div align="center" style="padding-top: 8px; padding-bottom: 5px;">
		<div id="alertb" class="infobox" style="visibility:hidden;">
			<div class="lab1">
				<div class="lab2">
					<div class="lab3">
						<div class="lab4">
							<div id="alertbc" class="infoboxc">
								&nbsp;
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>
	</div>
	
	<div class="roscms_container" style="border: 1px dashed white; z-index: 2;">
		<div class="tabmenu" style="position: absolute; top: 0px; width: 150px; left: 0px; border: 0px; z-index:1;">
			<div id="smenutab1" class="submb" onclick="smenutab_open(this.id)"<?php if ($roscms_security_level == 1 || roscms_security_grp_member("transmaint")) { echo " style=\"display:none;\""; } ?>>
				<div class="subm1">
					<div id="smenutabc1" class="subm2"><b>New Entry</b></div>
				</div>
			</div>
			<?php if ($roscms_security_level > 1) { ?>
				<div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
			<?php } ?>
			<div id="smenutab2" class="subma" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc2" class="subm2"><b>New</b></div>
				</div>
			</div>
			<div id="smenutab3" class="submb" onclick="smenutab_open(this.id)"<?php if ($roscms_security_level == 1 || roscms_security_grp_member("transmaint")) { echo " style=\"display:none;\""; } ?>>
				<div class="subm1">
					<div id="smenutabc3" class="subm2">Page</div>
				</div>
			</div>
			<div id="smenutab4" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc4" class="subm2">Content</div>
				</div>
			</div>
			<div id="smenutab5" class="submb" onclick="smenutab_open(this.id)"<?php if ($roscms_security_level == 1 || roscms_security_grp_member("transmaint")) { echo " style=\"display:none;\""; } ?>>
				<div class="subm1">
					<div id="smenutabc5" class="subm2">Template</div>
				</div>
			</div>
			<div id="smenutab6" class="submb" onclick="smenutab_open(this.id)"<?php if ($roscms_security_level == 1 || roscms_security_grp_member("transmaint")) { echo " style=\"display:none;\""; } ?>>
				<div class="subm1">
					<div id="smenutabc6" class="subm2">Script</div>
				</div>
			</div>
			<div id="smenutab7" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc7" class="subm2">Translate</div>
				</div>
			</div>
			<div id="smenutab8" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc8" class="subm2">All Entries</div>
				</div>
			</div>
			<div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
			<div id="smenutab9" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc9" class="subm2">Bookmark&nbsp;<img src="images/star_on_small.gif" alt="" style="width:13px; height:13px; border:0px;" /></div>
				</div>
			</div>
			<div id="smenutab10" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc10" class="subm2">Drafts</div>
				</div>
			</div>
			<div id="smenutab11" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc11" class="subm2">My Entries</div>
				</div>
			</div>
			<div id="smenutab12" class="submb" onclick="smenutab_open(this.id)">
				<div class="subm1">
					<div id="smenutabc12" class="subm2">Archive</div>
				</div>
			</div>
			<div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
			<div class="laba" style="background: #FFD4BC none repeat scroll 0%;">
				<div class="lab1">
					<div class="lab2">
						<div class="lab3">
							<div class="lab4">
								<div class="labtitel" id="labtitel1" onclick="TabOpenCloseEx(this.id)">
									<img id="labtitel1i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Quick Info
								</div>
								<div class="labcontent" id="labtitel1c" style="display:block;">
									<div id="qiload" style="display:none; text-align:right;" class="lablinks"><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" /></div>
									<div id="lablinks1" class="lablinks2"><span style="color:#FF6600;">Move the mouse over an item to get some details</span></div>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
			<div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
			<div class="laba" style="background: #B5EDA3 none repeat scroll 0%;">
				<div class="lab1">
					<div class="lab2">
						<div class="lab3">
							<div class="lab4">
								<div class="labtitel" id="labtitel2" onclick="TabOpenCloseEx(this.id)">
									<img id="labtitel2i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Smart Filters
								</div>
								<div class="labcontent" id="labtitel2c">
									<div id="lablinks2" class="lablinks2" style="color:#009900;">&nbsp;</div>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
			<div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
			<div class="laba" style="background: #DFD3EC none repeat scroll 0%;">
				<div class="lab1">
					<div class="lab2">
						<div class="lab3">
							<div class="lab4">
								<div class="labtitel" id="labtitel3" onclick="TabOpenCloseEx(this.id)">
									<img id="labtitel3i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Labels
								</div>
								<div class="labcontent" id="labtitel3c">
									<div id="lablinks3" class="lablinks2" style="color:#8868AC;">&nbsp;</div>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>
		<script type="text/javascript" language="javascript">
		<!--

			function beautifystr (tmp_str) {
				//var tmp2_str = tmp_str;
				
				// remove invalid characters
				tmp_str = tmp_str.replace(/\|/g, '');
				tmp_str = tmp_str.replace(/=/g, '');
				tmp_str = tmp_str.replace(/&/g, '');
				tmp_str = tmp_str.replace(/_/g, '');
				
				//alert(tmp2_str +'\n'+tmp_str);				
				
				return tmp_str;
			}

			function beautifystr2 (tmp_str) {
				// remove invalid characters
				tmp_str = tmp_str.replace(/\|/g, '');
				tmp_str = tmp_str.replace(/=/g, '');
				tmp_str = tmp_str.replace(/&/g, '');
							
				return tmp_str;
			}
						

			function show_quickinfo(dataid_revid) {
				if (document.getElementById('labtitel1c').style.display == 'block') { // only if the quick info box is 'visible'
					window.clearTimeout(timerquickinfo); /* deactivate quickinfo-timer */
					document.getElementById('qiload').style.display = 'none';
					timerquickinfo = window.setTimeout("load_quickinfo('"+dataid_revid+"')", 700);
				}
			}
			
			function load_quickinfo(dataid_revid) {
				var usf_req = '';
				var qistr = '';
				
				qistr = dataid_revid.split('|');
				

				window.clearTimeout(timerquickinfo); /* deactivate quickinfo-timer */
				
				//document.getElementById('lablinks1').innerHTML = dataid_revid+'<br />loading ...';

				usf_req = '?page=data_out&d_f=text&d_u=uqi&d_val=ptm&d_id='+encodeURIComponent(qistr[0].substr(2))+'&d_r_id='+encodeURIComponent(qistr[1]);
				<?php if ($RosCMS_GET_debug) { ?>
					debugLog(usf_req);
				<?php } ?>
				document.getElementById('qiload').style.display = 'block';
				makeRequest(usf_req, 'uqi', 'lablinks1', 'html', 'GET', '');
			}

			function stop_quickinfo() {
				if (document.getElementById('labtitel1c').style.display == 'block') { // only if the quick info box is 'visible'
					window.clearTimeout(timerquickinfo); /* deactivate quickinfo-timer */
					document.getElementById('qiload').style.display = 'none';
					timerquickinfo = window.setTimeout("clear_quickinfo()", 5000);
				}
			}

			function clear_quickinfo() {
				window.clearTimeout(timerquickinfo); /* deactivate quickinfo-timer */
				document.getElementById('qiload').style.display = 'none';
				document.getElementById('lablinks1').innerHTML = '<span style="color:#FF6600;">Move the mouse over an item to get some details</span>';
			}


			function add_user_filter(uf_type, uf_str) {
				var uf_name = '';
				var usf_req = '';
				var uf_objid = '';
							
				if (uf_type == 'label') {
					try {
						uf_name = window.prompt("Input a new Label name:", "");
						uf_str = 'a_is_'+ beautifystr(uf_name);
						uf_objid = 'lablinks3';
					} catch (e) {}
				}
				else {
					try {
						uf_name = window.prompt("Input a new Smart Filter name:", "");
						uf_objid = 'lablinks2';
					} catch (e) {}
				}
				
				if (!uf_name) { // cancel button
					return;
				}
				
				if (uf_name != '' && uf_name.length < 50) {
					//alert('type: '+uf_type+', name: '+uf_name+', string: '+uf_str);
					
					usf_req = '?page=data_out&d_f=text&d_u=ufs&d_val=add&d_val2='+encodeURIComponent(uf_type)+'&d_val3='+encodeURIComponent(uf_name)+'&d_val4='+encodeURIComponent(uf_str);
					<?php if ($RosCMS_GET_debug) { ?>
						debugLog(usf_req);
					<?php } ?>
					makeRequest(usf_req, 'ufs', uf_objid, 'html', 'GET', '');
				}
			}
			
			function delete_user_filter(uf_id, uf_type, uf_name) {
				var uf_check = '';
				var usf_req = '';
						
				if (uf_type == 'label') {
					uf_check = confirm("Do you want to delete Label '"+uf_name+"' ?");
					uf_objid = 'lablinks3';
				}
				else {
					uf_check = confirm("Do you want to delete Smart Filter '"+uf_name+"' ?");
					uf_objid = 'lablinks2';
				}

				if (uf_check == true) {
					usf_req = '?page=data_out&d_f=text&d_u=ufs&d_val=del&d_val2='+encodeURIComponent(uf_type)+'&d_val3='+encodeURIComponent(uf_id);
					<?php if ($RosCMS_GET_debug) { ?>
						debugLog(usf_req);
					<?php } ?>
					makeRequest(usf_req, 'ufs', uf_objid, 'html', 'GET', '');
				}
			}
			
			function load_user_filter(uf_type) {
				var usf_req = '';
				
				if (uf_type == 'label') {
					uf_objid = 'lablinks3';
				}
				else {
					uf_objid = 'lablinks2';
				}
						
				document.getElementById(uf_objid).innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" /></div>';
				usf_req = '?page=data_out&d_f=text&d_u=ufs&d_val=load&d_val2='+encodeURIComponent(uf_type);
				<?php if ($RosCMS_GET_debug) { ?>
					debugLog(usf_req);
				<?php } ?>
				makeRequest(usf_req, 'ufs', uf_objid, 'html', 'GET', '');
			}
						
		-->
		</script>	
		<script type="text/javascript" language="javascript">
		<!--
			
			function TabOpenClose(objid) {
				if (document.getElementById(objid +'c').style.display == 'none') {
					document.getElementById(objid +'c').style.display = 'block';
					document.getElementById(objid +'i').src = 'images/tab_open.gif';
				}
				else {
					document.getElementById(objid +'c').style.display = 'none';
					document.getElementById(objid +'i').src = 'images/tab_closed.gif';
				}
			}

			function TabOpenCloseEx(objid) {
				if (document.getElementById(objid +'c').style.display == 'none') {
					document.getElementById(objid +'c').style.display = 'block';
					document.getElementById(objid +'i').src = 'images/tab_open.gif';
					createCookie(objid,'1',365); // 365 days
				}
				else {
					document.getElementById(objid +'c').style.display = 'none';
					document.getElementById(objid +'i').src = 'images/tab_closed.gif';
					createCookie(objid,'0',365); // 365 days
				}
			}
			
		-->
		</script>
		<script type="text/javascript" language="javascript">
		<!--
		
			function createCookie(name,value,days) {
				if (days) {
					var date = new Date();
					date.setTime(date.getTime()+(days*24*60*60*1000));
					var expires = "; expires="+date.toGMTString();
				}
				else var expires = "";
				document.cookie = name+"="+value+expires+"; path=/";
			}
			
			function readCookie(name) {
				var nameEQ = name + "=";
				var ca = document.cookie.split(';');
				for(var i=0;i < ca.length;i++) {
					var c = ca[i];
					while (c.charAt(0)==' ') c = c.substring(1,c.length);
					if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
				}
				return null;
			}
			
			function eraseCookie(name) {
				createCookie(name,"",-1);
			}
		
		-->
		</script>
		<script type="text/javascript" language="javascript">
		<!--
			
			var smenutabs = 12; // sync this value with the tab-menu entry-count !!!
			
			function tbl_user_filter(ufiltstr, ufilttype, ufilttitel) {
				var tentrs = selectedEntries().split("|");

				if (ufilttype == 2 && tentrs[0] > 0 && tentrs[0] != '') {
					<?php
						if ($RosCMS_GET_debug) {
							echo "alert('tag it ;)');";
						}
					?>
					
					makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetags&d_val='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3=tg&d_val4='+ufilttitel, 'mef', 'changetags', 'html', 'GET', '');
				}
				else {
					smenutab_highlight('smenutab8');
	
					filtstring2 = ufiltstr;
	
					// reset search box:
					filtstring1 = '';
					document.getElementById('txtfind').value = '';
					textbox_hint('txtfind', document.getElementById('txtfind').value, 'Search & Filters', 0);
	
	
					chtabtext = null;
					select_all(0); /* deselect all */
	
					load_frametable('all');
					filtpopulate(ufiltstr);
				}
			}
			
			function smenutab_highlight(objid) {
				for (var i=1; i<=smenutabs; i++) {
					//alert(i+' == '+objid.substring(8));
					
					/* remove all 'bold' html-tags */
					if (document.getElementById('smenutab'+i).className == 'subma' && i != 1) {
						chtabtext = '';
						chtabtext = document.getElementById('smenutabc'+i).innerHTML;
						chtabtext = chtabtext.replace(/<B>/, '');
						chtabtext = chtabtext.replace(/<\/B>/, '');
						chtabtext = chtabtext.replace(/<b>/, '');
						chtabtext = chtabtext.replace(/<\/b>/, '');
						document.getElementById('smenutabc'+i).innerHTML = chtabtext;
						chtabtext = '';
						//alert(document.getElementById('smenutabc'+i).innerHTML);
					}
					
					/* highlight menu_tab_button */
					if (i == objid.substring(8)) {
						document.getElementById('smenutab'+i).className = 'subma';
						document.getElementById('smenutabc'+i).innerHTML = '<b>'+document.getElementById('smenutabc'+i).innerHTML+'</b>';
					}
					else {
						document.getElementById('smenutab'+i).className = 'submb';
					}				
				}
			}
			
			function smenutab_open(objid) {
				var chtabtext = '';
				if (document.getElementById('smenutab'+objid.substring(8)).className != 'subma') {
					//alert(objid.substring(8));
					//alert(document.getElementById('smenutab'+objid.substring(8)).className);
					
					smenutab_highlight(objid);
				}
				
				submenu_button = 'true';

				// reset search box:
				filtstring1 = '';
				document.getElementById('txtfind').value = '';
				textbox_hint('txtfind', document.getElementById('txtfind').value, 'Search & Filters', 0);
				
				window.clearTimeout(autosave_timer);
				autosave_cache = '';
				
				
				chtabtext = null;
				select_all(0); /* deselect all */	
							
				var translang = '';
				
				if (getlang() == '<?php echo $roscms_standard_language; ?>') {
					translang = '<?php echo $roscms_standard_language_trans; ?>';
				}
				else {
					translang = getlang();
				}
				
				<?php
					/*
						$query_filtstring2_account = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
						$result_filtstring2_account = @mysql_fetch_array($query_filtstring2_account);
						
						$temp_langid = "";
						if ($result_filtstring2_account['user_language'] != "") {
							$temp_langid = $result_filtstring2_account['user_language'];
							if ($temp_langid == $roscms_standard_language) {
								$temp_langid = $roscms_standard_language_trans; // temp solution
							}
						}
					*/
				?>

				roscms_archive = 0;
				
				switch(objid.substring(8)) {
					case '1':
						filtstring2 = '';
						load_frameedit('newentry', 'new');
						break;
					default:
					case '2':
						filtstring2 = 'k_is_new_0|c_is_type_0|l_is_'+getlang()+'_0|o_desc_datetime';
						load_frametable('new');
						break;
					case '3':
						filtstring2 = 'y_is_page_0|k_is_stable_0|l_is_'+getlang()+'_0|o_asc_name';
						load_frametable('page');
						break;
					case '4':
						filtstring2 = 'y_is_content_0|k_is_stable_0|l_is_'+getlang()+'_0|o_asc_name';
						load_frametable('content');
						break;
					case '5':
						filtstring2 = 'y_is_template_0|k_is_stable_0|l_is_'+getlang()+'_0|o_asc_name';
						load_frametable('template');
						break;
					case '6':
						filtstring2 = 'y_is_script_0|k_is_stable_0|l_is_'+getlang()+'_0|o_asc_name';
						load_frametable('script');
						break;
					case '7':
						filtstring2 = 'y_is_content_0|k_is_stable_0|i_is_default_0|c_is_user_0|l_is_<?php echo $roscms_standard_language; ?>_0|r_is_'+translang+'|o_asc_name';
						load_frametable('translate');
						break;
					case '8':
						filtstring2 = 'c_is_type_0|l_is_'+getlang()+'|o_desc_datetime';
						load_frametable('all');
						break;
					case '9':
						filtstring2 = 's_is_true_0|c_is_type_0|l_is_'+getlang()+'_0|o_desc_datetime';
						load_frametable('starred');
						break;
					case '10':
						filtstring2 = 'k_is_draft_0|u_is_<?php echo $roscms_intern_login_check_username; ?>_0|c_is_type_0|o_desc_datetime';
						load_frametable('draft');
						break;
					case '11':
						filtstring2 = 'u_is_<?php echo $roscms_intern_login_check_username; ?>_0|c_is_type_0|o_desc_datetime';
						load_frametable('my');
						break;
					case '12':
						filtstring2 = 'k_is_archive_0|c_is_version_0|c_is_type_0|l_is_'+getlang()+'_0|o_asc_name|o_desc_ver';
						roscms_archive = 1; /* activate archive mode*/
						load_frametable('archive');
						break;
				}
				
			}
			
			function load_frametable(objevent) {
				if (document.getElementById('frametable').style.display != 'block') {
					document.getElementById('frametable').style.display = 'block';
					document.getElementById('frameedit').style.display = 'none';
					
					window.clearTimeout(alertactiv); /* deactivate alert-timer */
					document.getElementById('alertb').style.visibility = 'hidden';
					document.getElementById('alertbc').innerHTML = '&nbsp;';
					select_all(0); /* deselect all table entries */
	
					//document.getElementById('tablist').innerHTML = '';
				}
				
				if (submenu_button == 'true') {
					roscms_prev_page = roscms_current_page;
					roscms_current_page = objevent;
					//alert('1: '+filtstring2);
					filtpopulate(filtstring2);
					//alert('2: '+filtstring2);
					submenu_button = '';
				}
				
				// update table-cmdbar
				switch (objevent) {
					case 'new':
						tblcmdbar('new');
						break;
					case 'page':
					case 'content':
						tblcmdbar('page');
						break;
					case 'script':
					case 'template':
						tblcmdbar('script');
						break;
					case 'draft':
						tblcmdbar('draft');
						break;
					case 'translate':
						tblcmdbar('trans');
						break;
					case 'archive':
						tblcmdbar('archive');
						break;
					default:
						tblcmdbar('all');
						break;
				}

				// update table-selectbar
				switch (objevent) {
					case 'new':
					case 'page':
					case 'content':
					case 'script':
					case 'template':
					case 'draft':
						tblselectbar('basic');
						break;
					case 'translate':
						tblselectbar('trans');
						break;
					case 'starred':
						tblselectbar('bookmark');
						break;
					case 'archive':
						tblselectbar('archive');
						break;
					default:
						tblselectbar('common');
						break;
				}
				
				<?php if ($RosCMS_GET_debug) { ?>
					debugLog('?page=data_out&d_f=xml&d_u=ptm&d_fl='+objevent+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0');
				<?php } ?>
				//alert('3: '+filtstring2 + ' | '+objevent);
				
				makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+objevent+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0', 'ptm', 'tablist', 'xml', 'GET', '');
			}
			
			function load_frametable_cp(tcp) {
				window.clearTimeout(alertactiv); /* deactivate alert-timer */
				document.getElementById('alertb').style.visibility = 'hidden';
				document.getElementById('alertbc').innerHTML = '&nbsp;';
				select_all(0); /* deselect all table entries */
					
				
				<?php if ($RosCMS_GET_debug) { ?>
					debugLog('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp='+tcp);
				<?php } ?>
				makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp='+tcp, 'ptm', 'tablist', 'xml', 'GET', '');
			}

			function load_frametable_cp2(tcp) {
				if (document.getElementById('frametable').style.display != 'block') {
					document.getElementById('frametable').style.display = 'block';
					document.getElementById('frameedit').style.display = 'none';
				}
				load_frametable_cp(tcp);
			}


			function load_frametable_cp3(tcp) {
				if (document.getElementById('frametable').style.display != 'block') {
					document.getElementById('frametable').style.display = 'block';
					document.getElementById('frameedit').style.display = 'none';
				}

				window.clearTimeout(alertactiv); /* deactivate alert-timer */
				document.getElementById('alertb').style.visibility = 'hidden';
				document.getElementById('alertbc').innerHTML = '&nbsp;';
				select_all(0); /* deselect all table entries */
					
				
				<?php if ($RosCMS_GET_debug) { ?>
					debugLog('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2);
				<?php } ?>
				makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2, 'ptm', 'tablist', 'xml', 'GET', '');
			}
			
			function load_frameedit_helper() {
				if (document.getElementById('frameedit').style.display != 'block') {
					document.getElementById('frametable').style.display = 'none';
					document.getElementById('frameedit').style.display = 'block';
				}
				
				window.clearTimeout(alertactiv); /* deactivate alert-timer */
				document.getElementById('alertb').style.visibility = 'hidden';
				document.getElementById('alertbc').innerHTML = '&nbsp;';
				select_all(0); /* deselect all table entries */
			}
			
			function load_frameedit(objevent, entryid) {
				<?php if ($RosCMS_GET_debug) echo "alert(objevent+', '+entryid);"; ?>
				//alert(roscms_prev_page+" - "+objevent);

				switch (objevent) {
					case 'newentry':
						load_frameedit_helper();
						roscms_prev_page = roscms_current_page;
						roscms_current_page = objevent;

						document.getElementById('frmedithead').innerHTML = '<b>New Entry</b>';
						
						document.getElementById('editzone').innerHTML = '<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p>';
						makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new&d_val=0', 'mef', 'addnew', 'html', 'GET', '');
						break;
					case 'diffentry':
						load_frameedit_helper();
						//alert(objevent);
						document.getElementById('frmedithead').innerHTML = '<span class="l" onclick="load_frametable_cp2(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <b>Compare two Entries</b>';
						break;
					default:
						if (entryid.indexOf("|") > -1) { 
							var devideids1 = ''
							var devideids2 = ''
							var devideids3 = ''
							devideids1 = entryid.indexOf("|");
							devideids2 = entryid.substr(2, devideids1-2);
							devideids3 = entryid.substr(devideids1+1);

							if (devideids2.substr(0,2) == 'tr') {
								uf_check = confirm("Do you want to translate this entry?");
								if (uf_check != true) {
									//load_frametable_cp2(0);
									break;
								}
								//alertbox('Translation functions is still partly unimplemented.');
								alertbox('Translation copy created.');
							}
							if (devideids2 == 'notrans') {
								alertbox('You don\'t have enough rights to translate this entry.');
								break;
							}


							load_frameedit_helper();

							roscms_prev_page = roscms_current_page;
							roscms_current_page = objevent;
	
							document.getElementById('frmedithead').innerHTML = '<span class="l" onclick="load_frametable_cp2(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <b>Edit Entry</b>';
							autosave_timer = window.setTimeout("autosave_try()", autosave_coundown);

												
							//alert(devideids2+', '+devideids3);
							
							<?php if ($RosCMS_GET_debug) { ?>
								debugLog('?page=data_out&d_f=text&d_u=mef&d_fl='+objevent+'&d_id='+devideids2+'&d_r_id='+devideids3);
							<?php } ?>
							
							// loading screen:
							document.getElementById('editzone').innerHTML = '<div style="background:#FFFFFF; border-bottom: 1px solid #bbb; border-bottom-width: 1px; border-right: 1px solid #bbb; border-right-width: 1px;"><div style="margin:10px;"><div style="width:95%;"><br /><br /><center><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" />&nbsp;&nbsp;loading ...</center><br /><br /></div></div></div>';
							
							//alert('userlang: '+userlang);
							
							makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl='+objevent+'&d_id='+devideids2+'&d_r_id='+devideids3+'&d_r_lang='+userlang, 'mef', 'editzone', 'html', 'GET', '');
						}
						else {
							alert('bad request: load_frameedit('+objevent+', '+entryid+')');
						}
						break;
				}
				
				
			}
		
		-->
		</script>

	  <div class="style1" id="impcont" style="margin-left: 150px; z-index:100;">
			<div class="bubble_bg">
				  <div class="rounded_ll">
					<div class="rounded_lr">	
				<div class="rounded_ul">
					<div class="rounded_ur">
						<div class="bubble" id="bub">
							<div id="frametable" name="frametable" style="border: 0px dashed white;">
							  <div class="filterbar">
									<input id="txtfind" type="text" accesskey="f" tabindex="1" title="Search &amp; Filters" onfocus="textbox_hint(this.id, this.value, 'Search &amp; Filters', 1)" onblur="textbox_hint(this.id, this.value, 'Search &amp; Filters', 0)" onkeyup="filtsearchbox()" value="Search &amp; Filters" size="39" maxlength="250" class="tfind" />&nbsp;
									<span id="filters" class="filterbutton" onclick="TabOpenClose(this.id)"><img id="filtersi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Filters</span>&nbsp;
									<div id="filtersc" style="display:none;">
										<div id="filtersct">&nbsp;</div>
										<div id="filt2" class="filterbar2">
											<span class="filterbutton" onclick="filtadd()"><img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add</span>
											&nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="filtentryclear2()"><img src="images/clear.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Clear</span>
											&nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="add_user_filter('filter', filtstring2)"><img src="images/save.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Save</span>
											&nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="filtsearch()"><img src="images/search.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Search</span>										</div>
									</div>
									<?php
										// disable combobox entries for novice user
										if ($roscms_security_level > 1) {
											$cbm_item_hide = "";
										}
										else {
											$cbm_item_hide = " disabled=\"disabled\" style=\"color:#CCCCCC;\"";
										}
									?>
									<script type="text/javascript" language="javascript">
									<!--

										function alertbox_close(zeroonetwo) {
											if (zeroonetwo == 0) { /* close and delete */
												document.getElementById('alertb').style.visibility = 'hidden';
												document.getElementById('alertbc').innerHTML = '&nbsp;';
											}
											else if (zeroonetwo == 1) { /* close */
												document.getElementById('alertb').style.visibility = 'hidden';
												alertactiv = window.setTimeout("alertbox_close(2)", 500);
											}
											else { /* open */
												document.getElementById('alertb').style.visibility = 'visible';
												alertactiv = window.setTimeout("alertbox_close(0)", 6000);												
											}
										}
										
										function alertbox(atxt) {
											window.clearTimeout(alertactiv);
											document.getElementById('alertb').style.visibility = 'visible';
											document.getElementById('alertbc').innerHTML = '<b>'+atxt+'</b>';
											alertactiv = window.setTimeout("alertbox_close(1)", 500);												
										}
										
										
										function filtentrydel(objid) {
											document.getElementById('filt'+objid.substr(4)).style.display = 'none';
											document.getElementById('filt'+objid.substr(4)).innerHTML = '';
											//filtercounter--;
										}
										
										function filtentryselect(objid) {
											var filtselstr = '';
											var filtselstr2 = '';
										
											//alert(objid.substring(3));
											//alert(document.getElementById(objid).value);
											
											filtscanfilters();
											
											filtselstr = filtstring2.split('|');
											filtselstr[(objid.substring(3)-1)] = document.getElementById(objid).value+'__';
											
											filtselstr2 = '';
											for (var i=0; i < filtselstr.length; i++) {
												filtselstr2 += filtselstr[i]+'|';
											}
											filtselstr2 = filtselstr2.substr(0,filtselstr2.length-1); /* remove last '|' */
											
											filtpopulate(filtselstr2);
										}
										
										function filtpopulatehelper(objidval, objidval2, filterid) {
											var filtentryselstr = '';
											var filtentryselstrs1 = '';
											var filtentryselstrs2 = '';
											
											//alert('objidval: '+objidval);
											
											if (objidval2 == 0 <?php if ($roscms_security_level > 1) { echo "&& 1 == 2"; } ?>) { // hidden filter entries don't need a combobox (only for SecLev = 1 user) 
												filtentryselstrs1 = '<input type="hidden" name="sfb'+filterid+'" id="sfb'+filterid+'" value="" />';
												filtentryselstrs2 = '<input type="hidden" name="sfc'+filterid+'" id="sfc'+filterid+'" value="" />';
											}
											else {
												switch(objidval) {
													//default:
													case 'k': /* kind */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><?php if ($roscms_security_level > 1) { ?><option value="no"<?php echo $cbm_item_hide; ?>>is not</option><?php } ?></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="stable">Stable</option><option value="new">New</option><option value="draft">Draft</option><option value="unknown">Unknown or no status</option><?php if ($roscms_security_level > 1) { ?><option value="archive"<?php echo $cbm_item_hide; ?>>Archive</option><?php } ?></select>';
														break;
													case 'y': /* type */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><?php if ($roscms_security_level > 1) { ?><option value="no"<?php echo $cbm_item_hide; ?>>is not</option><?php } ?></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="page">Page</option><option value="content">Content</option><option value="template">Template</option><option value="script">Script</option><option value="system">System</option></select>';
														break;
													case 's': /* starred */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="true">on</option><option value="false">off</option></select>';
														break;
													case 'd': /* date */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 2007-02-22)';
														break;
													case 't': /* time */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 15:30)';
														break;
													case 'l': /* language */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php
																														$query_language = mysql_query("SELECT * 
																																						FROM languages
																																						WHERE lang_level > '0'
																																						ORDER BY lang_name ASC ;");
																														while($result_language=mysql_fetch_array($query_language)) {
																															echo '<option value="';
																															/*if ($result_language['lang_level'] == '10') {
																																echo "all";
																															}
																															else {*/
																																echo $result_language['lang_id'];
																															//}
																															echo '"';
																															$query_account = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
																															$result_account = @mysql_fetch_array($query_account);
																															
																															if ($result_language['lang_id'] == $result_account['user_language']) {
																																echo ' selected="selected"';
																															}
																															echo '>'.$result_language['lang_name'].'</option>';
																															
																														}
																													  ?></select>';
														break;
													case 'r': /* translate */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">to</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php
																														$query_language = mysql_query("SELECT * 
																																						FROM languages
																																						WHERE lang_level > '0'
																																						ORDER BY lang_name ASC ;");
																														while($result_language=mysql_fetch_array($query_language)) {
																															if ($result_language['lang_level'] != '10') {
																																echo '<option value="';
																																echo $result_language['lang_id'];
																																echo '"';
																																$query_account = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
																																$result_account = @mysql_fetch_array($query_account);
																																
																																if ($result_language['lang_id'] == $result_account['user_language']) {
																																	echo ' selected="selected"';
																																}
																																echo '>'.$result_language['lang_name'].'</option>';
																															}																													
																														}
																													  ?></select>';
														break;
													case 'u': /* user */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><?php if ($roscms_security_level > 1) { ?><option value="no"<?php echo $cbm_item_hide; ?>>is not</option><?php } ?></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. John Doe)';
														break;
													case 'v': /* version */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="5" maxlength="10" />&nbsp;&nbsp;(e.g. 12)';
														break;
													case 'c': /* column */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="language">Language</option><option value="user">User</option><option value="type">Type</option><option value="version">Version</option><?php if ($roscms_security_level > 1) { ?><option value="security"<?php echo $cbm_item_hide; ?>>Security</option><option value="rights"<?php echo $cbm_item_hide; ?>>Rights</option><?php } ?></select>';
														break;
													case 'o': /* order by */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="asc">Ascending</option><option value="desc">Descending</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="datetime">Date &amp; Time</option><option value="name">Name</option><option value="lang">Language</option><option value="usr">User</option><option value="type">Type</option><option value="ver">Version</option><option value="nbr">Number ("dynamic" entry)</option><?php if ($roscms_security_level > 1) { ?><option value="security"<?php echo $cbm_item_hide; ?>>Security</option><option value="revid"<?php echo $cbm_item_hide; ?>>RevID</option><option value="ext"<?php echo $cbm_item_hide; ?>>Extension</option><option value="status"<?php echo $cbm_item_hide; ?>>Status</option><option value="kind"<?php echo $cbm_item_hide; ?>>Kind</option><?php } ?></select>';
														break;
													case 'i': /* security (ACL) */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php
																														$query_sec_acl = mysql_query("SELECT sec_name, sec_fullname   
																																						FROM data_security
																																						ORDER BY sec_fullname ASC ;");
																														while($result_sec_acl=mysql_fetch_array($query_sec_acl)) {
																															echo '<option value="'. $result_sec_acl['sec_name'] .'">'. $result_sec_acl['sec_fullname'] .'</option>';
																														}
																													  ?></select>';
														break;
													case 'm': /* metadata */
														filtentryselstrs1 = '<input id="sfb'+filterid+'" type="text" value="" size="10" maxlength="50" />';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(entry: value)';
														break;
													case 'n': /* name */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><?php if ($roscms_security_level > 1) { ?><option value="no"<?php echo $cbm_item_hide; ?>>is not</option><option value="likea"<?php echo $cbm_item_hide; ?>>is like *...*</option><?php } ?><option value="likeb">is like ...*</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. about)';
														break;
													case 'a': /* tag */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><?php if ($roscms_security_level > 1) { ?><option value="no"<?php echo $cbm_item_hide; ?>>is not</option><?php } ?></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />&nbsp;&nbsp;(e.g. todo)';
														break;
													<?php if ($roscms_security_level == 3) { ?>
														case 'e': /* system */
															filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="dataid">Data-ID</option><option value="revid">Rev-ID</option><option value="usrid">User-ID</option><option value="langid">Lang-ID</option></select>';
															filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />';
															break;
													<?php } ?>
												}
											}
												
											filtentryselstr += '<span id="sfz'+filterid+'">';
											filtentryselstr += filtentryselstrs1;
											filtentryselstr += '</span>&nbsp;';
											filtentryselstr += '<span id="sfy'+filterid+'">';
											filtentryselstr += filtentryselstrs2;
											filtentryselstr += '</span>';
											filtentryselstr += '<span id="sfx'+filterid+'">';
											filtentryselstr += '<input name="sfd'+filterid+'" type="hidden" value="'+objidval2+'" />';
											filtentryselstr += '</span>';
																					
											return filtentryselstr;
										}
										
										function filtentryclear() {
											document.getElementById('filtersct').innerHTML = '';
											document.getElementById('txtfind').value = '';
											textbox_hint('txtfind', document.getElementById('txtfind').value, 'Search & Filters', 0);
											filtercounter = 0;
											filterid = 0;
										}
										
										function filtentryclear2() {
											filtentryclear();
											
											filtstring1 = '';
											filtstring2 = '';
											load_frametable(roscms_current_page);												
										}
										
										function filtpopulate(filtpopstr) {
											var lstfilterstr = '';
											var lstfilterstr2 = '';

											document.getElementById('filtersct').innerHTML = '';
											filtercounter = 0;
											filterid = 0;
											
											if (filtpopstr != '') {
												//alert(filtpopstr);
												var filtpopstr2 = '';
												var indexid = '';
												var filtvisibility = '';
												filtpopstr2 = filtpopstr.split('|');
												//alert(filtpopstr2[0]);
												
												for (var i=0; i < filtpopstr2.length; i++) {
													lstfilterstr2 = '';
													lstfilterstr2 = filtpopstr2[i].split('_');	
													
													if (lstfilterstr2[3] == 0) {
														filtvisibility = 0;
														<?php if ($roscms_security_level > 1) { ?>
														lstfilterstr +=  '<span style="font-style: italic;">';
														<?php } else { ?>
														lstfilterstr +=  '<span style="display: none">';
														<?php } ?>
													}
													else {
														filtvisibility = 1;
														lstfilterstr +=  '<span style="font-style: normal;">';
														//lstfilterstr +=  '<span style="display: block">';
													}
													
													//alert(lstfilterstr2[0]);
													
													indexid = i + 1;


													lstfilterstr +=  '<div id="filt'+indexid+'" class="filterbar2">and&nbsp;';

													if (lstfilterstr2[3] == 0 <?php if ($roscms_security_level > 1) {echo "&& 1 == 2"; } ?>) { // hidden filter entries don't need a combobox (only for SecLev = 1 user) 
														lstfilterstr +=  '<input type="hidden" name="sfa'+indexid+'" id="sfa'+indexid+'" value="" />';
													}
													else {
														lstfilterstr +=  '<select id="sfa'+indexid+'" onchange="filtentryselect(this.id)">';
																<?php if ($roscms_security_level > 1) { ?>
																	lstfilterstr += '<option value="k"<?php echo $cbm_item_hide; ?>>Status</option>';
																	lstfilterstr += '<option value="y"<?php echo $cbm_item_hide; ?>>Type</option>';
																<?php } ?>
																lstfilterstr += '<option value="n">Name</option>';
																lstfilterstr += '<option value="v">Version</option>';
																lstfilterstr += '<option value="s">Starred</option>';
																lstfilterstr += '<option value="a">Tag</option>';
																lstfilterstr += '<option value="l">Language</option>';
																<?php if ($roscms_security_level > 1) { ?>
																	lstfilterstr += '<option value="r"<?php echo $cbm_item_hide; ?>>Translate</option>';
																	lstfilterstr += '<option value="i"<?php echo $cbm_item_hide; ?>>Security</option>';
																	lstfilterstr += '<option value="m"<?php echo $cbm_item_hide; ?>>Metadata</option>';
																	lstfilterstr += '<option value="u"<?php echo $cbm_item_hide; ?>>User</option>';
																<?php } ?>
																<?php if ($roscms_security_level == 3) { ?>
																	lstfilterstr += '<option value="e"<?php echo $cbm_item_hide; ?>>System</option>';
																<?php } ?>
																lstfilterstr += '<option value="d">Date</option>';
																lstfilterstr += '<option value="t">Time</option>';
																lstfilterstr += '<option value="c">Column</option>';
																lstfilterstr += '<option value="o">Order</option>';
														lstfilterstr += '</select>&nbsp;';
													}

													lstfilterstr +=  filtpopulatehelper(lstfilterstr2[0], lstfilterstr2[3], indexid);
													lstfilterstr +=  '&nbsp;&nbsp;&nbsp;<span id="fdel'+indexid+'" class="filterbutton" onclick="filtentrydel(this.id)"><img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Delete</span>';
													lstfilterstr +=  '</div>';

													
													if (lstfilterstr2[3] == 0) {
														lstfilterstr +=  '<span id="sfv'+indexid+'" class="filthidden"></span>'; // store visibility-status
													}
													else {
														lstfilterstr +=  '<span id="sfv'+indexid+'" class="filtvisible"></span>'; // store visibility-status
													}
													lstfilterstr +=  '</span>';
												}
											
												document.getElementById('filtersct').innerHTML = lstfilterstr;	
												
												
												for (var i=0; i < filtpopstr2.length; i++) {
													lstfilterstr2 = '';
													lstfilterstr2 = filtpopstr2[i].split('_');	
													
													//alert(lstfilterstr2[0]);
													
													indexid = i + 1;
													
													document.getElementById('sfa'+indexid).value = lstfilterstr2[0];
													if (lstfilterstr2[1] != '') {
														document.getElementById('sfb'+indexid).value = lstfilterstr2[1];
													}
													if (lstfilterstr2[2] != '') {
														document.getElementById('sfc'+indexid).value = lstfilterstr2[2];
													}
												}
												filtercounter = filtpopstr2.length;
												
												//alert('populate_counter: '+filtercounter);
											}
										}
										
										function filtscanfilters() {									
												filtstring2 = '';
												
												//alert('start_filtscanfilters: '+filtercounter);

												for (var i=1; i <= filtercounter; i++) {
													
													//alert('loop_filtscanfilters: '+i+' of '+filtercounter);
													if (document.getElementById('sfa'+i)) {
														//alert('loop22_filtscanfilters: '+i+' of '+filtercounter);
														//alert(filtercounter+': '+document.getElementById('sfa'+i).value);
														
														filtstring2 += beautifystr(document.getElementById('sfa'+i).value);
														filtstring2 += '_';
														filtstring2 += beautifystr(document.getElementById('sfb'+i).value);
														filtstring2 += '_';
														filtstring2 += beautifystr(document.getElementById('sfc'+i).value);
														if (document.getElementById('sfv'+i).id) { // care about visibility-status
															if (document.getElementById('sfv'+i).className == "filthidden") {
																filtstring2 += '_0';
															}
														}
														filtstring2 += '|';
													}
													/*else {
														if (filtercounter < 10) {
															filtercounter++;
															alert('++filtscanfilters: '+i+' of '+filtercounter);
														}
													}*/
												}
												
												filtstring2 = filtstring2.substr(0,filtstring2.length-1); /* remove last '|' */
												//alert(filtstring2);
										}
	
										function filtsearchbox() { // standard search box
											if (document.getElementById('filtersc').style.display == 'none') { // without filters visible
												//alert('search!');

												if (document.getElementById('txtfind').value != 'Search & Filters') {
													if (document.getElementById('txtfind').value.length > 1) {
														filtstring1 = document.getElementById('txtfind').value;
													}
													else {
														filtstring1 = '';
													}
												}
												else {
													filtstring1 = '';
												}
												
												filtscanfilters();
												load_frametable(roscms_current_page);	
											}
										}
																					
										function filtsearch() {
											if (document.getElementById('txtfind').value != 'Search & Filters') {
												if (document.getElementById('txtfind').value.length > 1) {
													filtstring1 = document.getElementById('txtfind').value;
												}
												else {
													filtstring1 = '';
												}
											}
											else {
												filtstring1 = '';
											}
											
											//alert(filtstring2);
											filtscanfilters();
											//alert(filtstring2);
											//alert(roscms_current_page);
											load_frametable(roscms_current_page);
										}
										
										function filtadd() {
											filtscanfilters();
											
											<?php
												if ($roscms_security_level > 1) { 
													$tmp_security_new_filter = "k_is_stable";
												}
												else {
													$tmp_security_new_filter = "a_is_";
												}
											?>
											
											if (filtstring2 == '') {
												filtpopulate('<?php echo $tmp_security_new_filter; ?>');
											}
											else {
												filtpopulate(filtstring2+'|<?php echo $tmp_security_new_filter; ?>');
											}
										}
										
										filtentryclear(); /* add first filter entry (default) */
										
									-->									
									</script>
							  </div>
								<div style="border: 0px dashed red; position: absolute; top: 9px; right: 13px; text-align:right; white-space: nowrap;">
									<select name="favlangopt" id="favlangopt" style="vertical-align: top; width: 22ex;" onchange="setlang(this.value)">
										<?php
											$query_language2 = mysql_query("SELECT * 
																			FROM languages
																			WHERE lang_level > '0'
																			ORDER BY lang_name ASC ;");
											while($result_language2=mysql_fetch_array($query_language2)) {
												echo '<option value="'.$result_language2['lang_id'].'"';
												
												$query_account2 = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
												$result_account2 = @mysql_fetch_array($query_account2);
												
												if ($result_language2['lang_id'] == $result_account2['user_language']) {
													//echo ' selected="selected" style="background:#b5eda3 none repeat scroll 0% 50%;"';
													echo ' selected="selected"';
												}
												
												echo '>'.$result_language2['lang_name'].'</option>';
											}
										?>
									</select>
									<script type="text/javascript" language="javascript">
										<!--
											
											function setlang(favlang) {
												var tmp_regstr;
											
												var transcheck = filtstring2.search(/r_is_/);
												
												if (transcheck != -1) { // translation view
													tmp_regstr = new RegExp('r_is_'+userlang, "g");
													filtstring2 = filtstring2.replace(tmp_regstr, 'r_is_'+favlang);
													
													tmp_regstr = new RegExp('r_is_<?php echo $roscms_standard_language_trans; ?>', "g");
													filtstring2 = filtstring2.replace(tmp_regstr, 'r_is_'+favlang);
												}
												else {
													tmp_regstr = new RegExp('l_is_'+userlang, "g");
													//alert('set: '+userlang+' => '+favlang);
													//alert(tmp_regstr);
													//alert('filtstring2:'+filtstring2);
													//alert(filtstring2.replace(tmp_regstr, 'l_is_'+favlang));
													
													filtstring2 = filtstring2.replace(tmp_regstr, 'l_is_'+favlang);
												}
												
												userlang = favlang;
												filtpopulate(filtstring2);
												load_frametable_cp3(roscms_current_tbl_position);
											}
											
											function getlang() {
												//alert('get: '+userlang);
												return userlang;
											}
										
										-->
									</script>
							</div>
							<div id="tablecmdbar" style="padding-top: 5px;"></div>
							<script type="text/javascript" language="javascript">
								<!--
								
									function tblcmdbar(opt) {
										cmdbarstr = '';
										
										cmdhtml_space = '&nbsp;';
										cmdhtml_diff = '<button type="button" id="cmddiff" onclick="diffentries()">Compare</button>'+cmdhtml_space;
										cmdhtml_preview = '<button type="button" id="cmdpreview" onclick="bpreview()">Preview</button>'+cmdhtml_space;
										cmdhtml_ready = '<button type="button" id="cmdready" onclick="bchangetags(\'mn\')">Ready</button>'+cmdhtml_space;
										<?php 
											if ($roscms_security_level >= 2) {
										?>
											cmdhtml_stable = '<button type="button" id="cmdstable" onclick="bchangetags(\'ms\')">Stable</button>'+cmdhtml_space;
										<?php
											} else {
										?>
											cmdhtml_stable = '';
										<?php
											}
										?>
										cmdhtml_refresh = '<button type="button" id="cmdrefresh" onclick="load_frametable_cp2(roscms_current_tbl_position)">Refresh</button>'+cmdhtml_space;

										cmdhtml_select1 = '<select name="extraopt" id="extraopt" style="vertical-align: top; width: 22ex;" onchange="bchangetags(this.value)">';
										cmdhtml_select1 += '<option value="sel" style="color: rgb(119, 119, 119);">More actions...</option>';
										cmdhtml_select_as = '<option value="as">&nbsp;&nbsp;&nbsp;Add star</option>';
										cmdhtml_select_xs = '<option value="xs">&nbsp;&nbsp;&nbsp;Remove star</option>';
										cmdhtml_select_no = '<option value="no" style="color: rgb(119, 119, 119);">&nbsp;&nbsp;&nbsp;-----</option>';
										cmdhtml_select_mn = '<option value="mn">&nbsp;&nbsp;&nbsp;Mark as new</option>';
										<?php 
											if ($roscms_security_level >= 2) {
										?>
											cmdhtml_select_ms = '<option value="ms">&nbsp;&nbsp;&nbsp;Mark as stable</option>';
											cmdhtml_select_ge = '<option value="va">&nbsp;&nbsp;&nbsp;Generate page</option>';
										<?php
											} else {
										?>
											cmdhtml_select_ms = '';
										<?php
											}
										?>
										<?php 
											if ($roscms_security_level == 3) {
										?>
											cmdhtml_select_va = '<option value="va">&nbsp;&nbsp;&nbsp;Move to archive</option>';
											cmdhtml_select_xe = '<option value="xe">&nbsp;&nbsp;&nbsp;Delete</option>';
										<?php
											} else {
										?>
											cmdhtml_select_va = '';
											cmdhtml_select_xe = '';
										<?php
											}
										?>
										cmdhtml_select_xe2 = '<option value="xe">&nbsp;&nbsp;&nbsp;Delete</option>';
										cmdhtml_select2 = '</select>';


										switch (opt) {
											case 'all':
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_preview;
												cmdbarstr += cmdhtml_ready;
												cmdbarstr += cmdhtml_stable;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_xe;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'trans':
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_preview;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_xe;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'new':
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_preview;
												cmdbarstr += cmdhtml_stable;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_xe2;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'page':
												cmdbarstr += cmdhtml_preview;
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_xe;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'script':
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_xe;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'draft':
												cmdbarstr += cmdhtml_diff;
												cmdbarstr += cmdhtml_preview;
												cmdbarstr += cmdhtml_ready;
												cmdbarstr += cmdhtml_stable;
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_mn;
												cmdbarstr += cmdhtml_select_ms;
												cmdbarstr += cmdhtml_select_va;
												cmdbarstr += cmdhtml_select_no;
												cmdbarstr += cmdhtml_select_xe2;
												cmdbarstr += cmdhtml_select2;
												break;
											case 'archive':
												cmdbarstr += cmdhtml_refresh;
												cmdbarstr += cmdhtml_select1;
												cmdbarstr += cmdhtml_select_as;
												cmdbarstr += cmdhtml_select_xs;
												cmdbarstr += cmdhtml_select2;
												break;
										}
										
										document.getElementById('tablecmdbar').innerHTML = cmdbarstr;
										
										switch (opt) {
											case 'page':
												document.getElementById('cmdpreview').style.fontWeight = 'bold';
												break;
											case 'archive':
												document.getElementById('cmdrefresh').style.fontWeight = 'bold';
												break;
											default:
												document.getElementById('cmddiff').style.fontWeight = 'bold';
												break;
										}
									}		
									
									function tblselectbar(opt) {
										selbarstr = '';
										
										selhtml_space = ', ';
										selhtml_all = '<span class="l" onclick="select_all(1)">All</span>';
										selhtml_none = '<span class="l" onclick="javascript:select_all(0)">None</span>';
										selhtml_inv = '<span class="l" onclick="select_inverse()">Inverse</span>';
										selhtml_star = '<span class="l" onclick="select_stars(1)">Starred</span>';
										selhtml_nostar = '<span class="l" onclick="select_stars(0)">Unstarred</span>';
										selhtml_stable = '<span class="l" onclick="select_nds(\'stable\')">Stable</span>';
										selhtml_new = '<span class="l" onclick="select_nds(\'new\')">New</span>';
										selhtml_draft = '<span class="l" onclick="select_nds(\'draft\')">Draft</span>';
										selhtml_uptodate = '<span class="l" onclick="select_nds(\'transg\')">Current</span>';
										selhtml_outdated = '<span class="l" onclick="select_nds(\'transr\')">Dated</span>';
										selhtml_notrans = '<span class="l" onclick="select_nds(\'transb\')">Missing</span>';
										selhtml_unknown = '<span class="l" onclick="select_nds(\'unknown\')">Unknown</span>';
										selhtml_marked = '<span class="l" onclick="do_something()">Marked?</span>';

										switch (opt) {
											case 'common':
												selbarstr += selhtml_all + selhtml_space;
												selbarstr += selhtml_none + selhtml_space;
												selbarstr += selhtml_inv + selhtml_space;
												selbarstr += selhtml_star + selhtml_space;
												selbarstr += selhtml_nostar + selhtml_space;
												selbarstr += selhtml_stable + selhtml_space;
												selbarstr += selhtml_new + selhtml_space;
												<?php 
													if ($RosCMS_GET_debug) {
												?>
														selbarstr += selhtml_draft + selhtml_space;
														selbarstr += selhtml_unknown + selhtml_space;
														selbarstr += selhtml_marked;
												<?php 
													}
													else {
												?>
														selbarstr += selhtml_draft;
												<?php
													}
												?>
												break;
											case 'trans':
												selbarstr += selhtml_all + selhtml_space;
												selbarstr += selhtml_none + selhtml_space;
												selbarstr += selhtml_inv + selhtml_space;
												selbarstr += selhtml_star + selhtml_space;
												selbarstr += selhtml_nostar + selhtml_space;
												selbarstr += selhtml_uptodate + selhtml_space;
												selbarstr += selhtml_outdated + selhtml_space;
												selbarstr += selhtml_notrans;
												break;
											case 'basic':
												selbarstr += selhtml_all + selhtml_space;
												selbarstr += selhtml_none + selhtml_space;
												selbarstr += selhtml_inv + selhtml_space;
												selbarstr += selhtml_star + selhtml_space;
												selbarstr += selhtml_nostar;
												break;
											case 'bookmark':
												selbarstr += selhtml_all + selhtml_space;
												selbarstr += selhtml_none + selhtml_space;
												selbarstr += selhtml_inv + selhtml_space;
												selbarstr += selhtml_stable + selhtml_space;
												selbarstr += selhtml_new + selhtml_space;
												selbarstr += selhtml_draft;
												break;
											case 'archive':
												selbarstr += selhtml_all + selhtml_space;
												selbarstr += selhtml_none + selhtml_space;
												selbarstr += selhtml_inv;
												break;
										}
										
										document.getElementById('tabselect1').innerHTML = selbarstr;
										document.getElementById('tabselect2').innerHTML = selbarstr;
									}
								-->
							</script>
								<div style="border: 0px dashed red; position: absolute; right: 10px; text-align:right; white-space: nowrap;"><span id="mtblnav">&nbsp;</span></div>
								<div class="tabselect">Select: <span id="tabselect1"></span></div>
								<div id="tablist">&nbsp;</div>
								<div style="border: 0px dashed red; position: absolute; right: 10px; text-align:right; white-space: nowrap;"><span id="mtbl2nav">&nbsp;</span></div>
								<div class="tabselect">Select: <span id="tabselect2"></span></div>
							</div>
							<div id="frameedit" style="display: block; border: 0px dashed red; ">
								<script type="text/javascript" language="javascript">
								<!--
								
									function bshowentry(did, drid) {
										alertbox('Change fields only if you know what you are doing.');
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showentry&d_id='+did+'&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
									}

									function editsaveentrychanges(did, drid) {
										var uf_check = '';
										uf_check = confirm("Please double check your changes.\n\nDo you want to continue?");
										
										if (uf_check == true) {
											var d_lang_str = document.getElementById('cbmentrylang').value;
											var d_revnbr_str = document.getElementById('vernbr').value;
											var d_usr_str = beautifystr2(document.getElementById('verusr').value);
											var d_date_str = document.getElementById('verdate').value;
											var d_time_str = document.getElementById('vertime').value;
											var d_chgdataname_str = document.getElementById('chgdataname').value;
											var d_chgdatatype_str = document.getElementById('cbmchgdatatype').value;
	
											if (d_usr_str.substr(0, 1) == ' ') {
												d_usr_str = d_usr_str.substr(1, d_usr_str.length-1); // remove leading space character
											}
											
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterentry&d_id='+did+'&d_r_id='+drid+'&d_val='+d_lang_str+'&d_val2='+d_revnbr_str+'&d_val3='+d_usr_str+'&d_val4='+d_date_str+'&d_val5='+d_time_str+'&d_val6='+d_chgdataname_str+'&d_val7='+d_chgdatatype_str, 'mef', 'editalterentry', 'html', 'GET', '');
										}
									}

									function bshowsecurity(did, drid) {
										alertbox('Changes will affect all related entries (see \'History\').');
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showsecurity&d_id='+did+'&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
									}

									function editsavesecuritychanges(did, drid) {
										var uf_check = '';
										uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, underscore, comma, dot, plus, minus. \n\nDo you want to continue?");
										
										if (uf_check == true) {
											var d_name_str = beautifystr2(document.getElementById('secdataname').value);
											var d_type_str = document.getElementById('cbmdatatype').value;
											var d_acl_str = document.getElementById('cbmdataacl').value;
											var d_name_update = document.getElementById('chdname').checked;
	
											if (d_name_str.substr(0, 1) == ' ') {
												d_name_str = d_name_str.substr(1, d_name_str.length-1); // remove leading space character
											}
											
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=altersecurity&d_id='+did+'&d_r_id='+drid+'&d_val='+d_name_str+'&d_val2='+d_type_str+'&d_val3='+d_acl_str+'&d_val4='+d_name_update, 'mef', 'editaltersecurity', 'html', 'GET', '');
										}
									}

									function editsavefieldchanges(did, drid) {
										var uf_check = '';
										uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, comma, dot, plus, minus. \n\nDo you want to continue?");
										
										if (uf_check == true) {
											var stext_str = '';
											var text_str = '';
											
											for (i=1; i <= document.getElementById('editaddstextcount').innerHTML; i++) {
												var tmp_str = document.getElementById('editstext'+i).value;
													
												tmp_str = beautifystr(tmp_str);
												
												if (tmp_str.substr(0, 1) == ' ') {
													tmp_str = tmp_str.substr(1, tmp_str.length-1); // remove leading space character
												}
												
												if (document.getElementById('editstextorg'+i).value == 'new' && document.getElementById('editstextdel'+i).checked) {
													// skip
												}
												else {
													stext_str += document.getElementById('editstextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('editstextdel'+i).checked +'|';
												}
											}
											stext_str = stext_str.substr(0, stext_str.length-1);
											//alert(stext_str);

											for (i=1; i <= document.getElementById('editaddtextcount').innerHTML; i++) {	
												var tmp_str = document.getElementById('edittext'+i).value;
													
												tmp_str = beautifystr(tmp_str);
												
												if (tmp_str.substr(0, 1) == ' ') {
													tmp_str = tmp_str.substr(1, tmp_str.length-1); // remove leading space character
												}

												if (document.getElementById('edittextorg'+i).value == 'new' && document.getElementById('edittextdel'+i).checked) {
													// skip
												}
												else {
													text_str += document.getElementById('edittextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('edittextdel'+i).checked +'|';											
												}
											}
											text_str = text_str.substr(0, text_str.length-1);
											//alert(text_str);
											
											//alert(stext_str + '\n\n' + text_str);
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterfields2&d_id='+did+'&d_r_id='+drid+'&d_val='+stext_str+'&d_val2='+text_str, 'mef', 'editalterfields', 'html', 'GET', '');
										}
									}

									function editaddshorttext() {
										var textcount = document.getElementById('editaddstextcount').innerHTML;
										
										textcount++;
										
										document.getElementById('editaddstext').innerHTML += '<input type="text" name="editstext'+textcount+'" id="editstext'+textcount+'" size="25" maxlength="100" value="" />&nbsp;';
										document.getElementById('editaddstext').innerHTML += '<input type="checkbox" name="editstextdel'+textcount+'" id="editstextdel'+textcount+'" value="del" /><label for="editstextdel'+textcount+'">delete?</label>';
										document.getElementById('editaddstext').innerHTML += '<input name="editstextorg'+textcount+'" id="editstextorg'+textcount+'" type="hidden" value="new" /><br /><br />';
										
										document.getElementById('editaddstextcount').innerHTML = textcount;
									}
									
									function editaddtext() {
										var textcount = document.getElementById('editaddtextcount').innerHTML;
										
										textcount++;

										document.getElementById('editaddtext').innerHTML += '<input type="text" name="edittext'+textcount+'" id="edittext'+textcount+'" size="25" maxlength="100" value="" />&nbsp;';
										document.getElementById('editaddtext').innerHTML += '<input type="checkbox" name="edittextdel'+textcount+'" id="edittextdel'+textcount+'" value="del" /><label for="edittextdel'+textcount+'">delete?</label>'
										document.getElementById('editaddtext').innerHTML += '<input name="edittextorg'+textcount+'" id="edittextorg'+textcount+'" type="hidden" value="new" /><br /><br />';
										
										document.getElementById('editaddtextcount').innerHTML = textcount;
									}

									function bshowtag(did, drid) {
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showtag&d_id='+did+'&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
									}

									function bshowhistory(did, drid) {
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showhistory&d_id='+did+'&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
									}

									function balterfields(did, drid, dusr) {
									alertbox('Change fields only if you know what you are doing.');
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterfields&d_id='+did+'&d_r_id='+drid+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
									}

								
									function baddtag(did, drid, dtn, dtv, dusr) {
										var dtna = '';
										var dtva = '';
										
										if (dtn == 'tag') {
											dtna = 'tag';
										}
										else {
											dtna = document.getElementById(dtn).value;
										}
										dtva = document.getElementById(dtv).value;
										
										if (dtna != '' && dtva != '') {
											//alert(dtna+', '+dtva);
											
											<?php if ($RosCMS_GET_debug) { ?>
												debugLog('?page=data_out&d_f=text&d_u=mef&d_fl=addtag&d_id='+did+'&d_r_id='+drid+'&d_val='+dtna+'&d_val2='+dtva);
											<?php } ?>
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=addtag&d_id='+did+'&d_r_id='+drid+'&d_val='+encodeURIComponent(dtna)+'&d_val2='+encodeURIComponent(dtva)+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
										}
									}
									
									function bdeltag(did, drid, dtid, dusr) {								
										if (dtid != '') {
											//alert(dtid);
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=deltag&d_id='+did+'&d_r_id='+drid+'&d_val='+dtid+'&d_val2='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
										}
									}
									
									function bchangetag(did, drid, dtn, dtv, dusr, dtid, objid, dbflag) {	
										if (dtn != '' && dtv != '') {
											//alert(dtid);
											
											<?php if ($RosCMS_GET_debug) { ?>
												debugLog('?page=data_out&d_f=text&d_u=mef&d_fl=changetag'+dbflag+'&d_id='+did+'&d_r_id='+drid+'&d_val='+dtn+'&d_val2='+dtv+'&d_val3='+dusr+'&d_val4='+dtid);
											<?php } ?>
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetag'+encodeURIComponent(dbflag)+'&d_id='+did+'&d_r_id='+drid+'&d_val='+encodeURIComponent(dtn)+'&d_val2='+encodeURIComponent(dtv)+'&d_val3='+dusr+'&d_val4='+dtid, 'mef', objid, 'html', 'GET', '');
										}
									}
									
									function bchangestar(did, drid, dtn, dtv, dusr, objid) {
										//alert(objid);
										//alert('bchangestar(did: '+did+', drid: '+drid+', dtn: '+dtn+', dtv: '+dtv+', dusr: '+dusr+', objid: '+objid+')');
										
										if (did != '' && drid != '') {
											if (document.getElementById(objid).src == '<?php echo $roscms_intern_webserver_roscms; ?>images/star_on_small.gif') {
												document.getElementById(objid).src = '<?php echo $roscms_intern_webserver_roscms; ?>images/star_off_small.gif';
												bchangetag(did, drid, dtn, 'off', dusr, document.getElementById(objid).className, objid, '3');
											}
											else {
												document.getElementById(objid).src = '<?php echo $roscms_intern_webserver_roscms; ?>images/star_on_small.gif';
												bchangetag(did, drid, dtn, 'on', dusr, document.getElementById(objid).className, objid, '3');
											}
										}
									}
									
									function prepair_edit_form_submit() {
										try {
											var poststr = "";
										
											//alert(document.getElementById("estextcount").className);
											//alert(document.getElementById("elmcount").className);
											
											/* short text */
											poststr += "pstextsum="+document.getElementById("estextcount").className;
											for (var i=1; i <= document.getElementById("estextcount").className; i++) {
												poststr += "&pdstext"+i+"=" + encodeURIComponent(document.getElementById("edstext"+i).innerHTML);
												poststr += "&pstext"+i+"=" + encodeURIComponent(document.getElementById("estext"+i).value);
											}
											
											/* text */
											poststr += "&plmsum="+document.getElementById("elmcount").className;
											//alert('textanzahl: '+document.getElementById("elmcount").className);
											var instatinymce;
											for (var i=1; i <= document.getElementById("elmcount").className; i++) {
												poststr += "&pdtext"+i+"=" + encodeURIComponent(document.getElementById("edtext"+i).innerHTML);
												
												instatinymce = ajaxsaveContent("elm"+i); // get the content from TinyMCE
	//											alert(instatinymce);
												if (instatinymce != null) {
													<?php if ($RosCMS_GET_debug) { ?>
														alert('[TinMCE Text] i: '+i+'; mce-content: '+instatinymce);
													<?php } ?>
													poststr += "&plm"+i+"=" + encodeURIComponent(instatinymce);
	//												alert('[TinMCE Text - 2.] i: '+i+'; mce-content: '+poststr);
												}
												else {
													<?php if ($RosCMS_GET_debug) { ?>
														alert('[Plain Text] i: '+i+'; txt-content: '+document.getElementById("elm"+i).value);
													<?php } ?>
													poststr += "&plm"+i+"=" + encodeURIComponent(document.getElementById("elm"+i).value);
												}
											}
											
	//										alert ('content_ready: '+poststr);
											
											return poststr;
										}
										catch (e) {
											rtestop(); // destroy old rich text editor instances
											//window.clearTimeout(autosave_timer);
											//autosave_cache = '';
											//alert('autosave bug stopped');
										}
									}

									function edit_form_submit_draft(did, drid) {
										document.getElementById("bsavedraft").disabled = true;
									
										edit_form_submit_draft_autosave(did, drid);
										
										//alert('a'+roscms_current_tbl_position);
										
										rtestop(); // destroy old rich text editor instances
										
										load_frametable_cp2(roscms_current_tbl_position);
										window.clearTimeout(autosave_timer);
										//autosave_cache = '';
										alertbox('Draft saved');
									}

									function edit_form_submit_draft_autosave(did, drid) {
										var poststr = "";
										var usf_req = '';

										poststr = prepair_edit_form_submit();
										
//										alert('save_draft::::::::\n\n '+poststr.substr(1));
										
										usf_req = '?page=data_out&d_f=text&d_u=asi&d_fl=new&d_id='+encodeURIComponent(did)+'&d_r_id='+encodeURIComponent(drid)+'&d_r_lang='+encodeURIComponent(document.getElementById("mefrlang").innerHTML)+'&d_r_ver='+encodeURIComponent(document.getElementById("mefrverid").innerHTML)+'&d_val='+encodeURIComponent(document.getElementById("estextcount").className)+'&d_val2='+encodeURIComponent(document.getElementById("elmcount").className)+'&d_val3=draft&d_val4='+encodeURIComponent(document.getElementById("entryeditdynnbr").innerHTML);
										<?php if ($RosCMS_GET_debug) { ?>
											debugLog(usf_req);
										<?php } ?>
										makeRequest(usf_req, 'asi', 'mefasi', 'html', 'POST', poststr.substr(1));
									}
									
									function edit_form_submit(did, drid) {
										var usf_req = '';
										var poststr = "";
										document.getElementById("bsavenew").disabled = true;

										poststr = prepair_edit_form_submit();				

//										alert('??edit_form_submit: '+encodeURIComponent(did)+' | '+encodeURIComponent(drid));
										//alert('lang: '+document.getElementById("mefrlang").innerHTML);

										usf_req = '?page=data_out&d_f=text&d_u=asi&d_fl=new&d_id='+encodeURIComponent(did)+'&d_r_id='+encodeURIComponent(drid)+'&d_r_lang='+encodeURIComponent(document.getElementById("mefrlang").innerHTML)+'&d_r_ver='+encodeURIComponent(document.getElementById("mefrverid").innerHTML)+'&d_val='+encodeURIComponent(document.getElementById("estextcount").className)+'&d_val2='+encodeURIComponent(document.getElementById("elmcount").className)+'&d_val3=submit';
										<?php if ($RosCMS_GET_debug) { ?>
											debugLog(usf_req);
										<?php } ?>
										makeRequest(usf_req, 'asi', 'alert', 'html', 'POST', poststr.substr(1));

										rtestop(); // destroy old rich text editor instances

										load_frametable_cp2(roscms_current_tbl_position);
										window.clearTimeout(autosave_timer);
										//autosave_cache = '';
										alertbox('Entry saved');
									}
									
									function autosave_try(t_d_id, t_d_revid) {
										//alert('autosave-try');
										window.clearTimeout(autosave_timer);
										//autosave_cache = '';
										
										try {
											if (document.getElementById("editautosavemode").value == 'false') {
												window.clearTimeout(autosave_timer);
												//autosave_cache = '';
												//alert('autosave-end');
												return;
											}
										} 
										catch (e) {
											window.clearTimeout(autosave_timer);
											//autosave_cache = '';
											//alert('autosave-end2');
											return;
										}
										
										if (autosave_cache != prepair_edit_form_submit() && autosave_cache != '') {
											//alert('auto-save: (txtbox - cache)\n'+prepair_edit_form_submit() +'\n'+ autosave_cache);
											
											//alert('!!!autosave_try: '+t_d_id+' vs. '+document.getElementById("entrydataid").className+' | '+t_d_revid+' vs. '+document.getElementById("entrydatarevid").className);
											edit_form_submit_draft_autosave(document.getElementById("entrydataid").className, document.getElementById("entrydatarevid").className);
										}

										autosave_timer = window.setTimeout("autosave_try()", autosave_coundown); // 10000
									}
									
									function changecreateinterface(menumode) {
										if (menumode == 'single' || menumode == 'dynamic' || menumode == 'template') {
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new&d_val='+menumode, 'mef', 'addnew', 'html', 'GET', '');
										}
									}
																		
									function createentry(menumode) {
										if (menumode == '0') {
											//alert('single');
											if (document.getElementById('txtaddentryname').value != "") {			
												//alert('asasas');							
												makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry2&d_name='+encodeURIComponent(document.getElementById('txtaddentryname').value)+'&d_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&d_r_lang='+encodeURIComponent(document.getElementById('txtaddentrylang').value), 'mef', 'addnew2', 'html', 'GET', '');
											}
											else {
												alertbox('Entry name is requiered');
											}
										}
										else if (menumode == '1') {
											//alert('dynamic');
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry4&d_name='+encodeURIComponent(document.getElementById('txtadddynsource').value)+'&d_type=content&d_r_lang=<?php echo $roscms_standard_language; ?>', 'mef', 'addnew2', 'html', 'GET', '');
										}
										else if (menumode == '2') {
											//alert('template');
											if (document.getElementById('txtaddentryname3').value != "") {										
												makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry3&d_name='+encodeURIComponent(document.getElementById('txtaddentryname3').value)+'&d_type=content&d_r_lang=<?php echo $roscms_standard_language; ?>&d_template='+encodeURIComponent(document.getElementById('txtaddtemplate').value), 'mef', 'addnew2', 'html', 'GET', '');
											}
											else {
												alertbox('Entry name is requiered');
											}
										}
									}
									
									function diffentries() {
										var tentrs = selectedEntries().split("|");
										
										if (tentrs[0] < 2) {
											alertbox("Select two entries to compare them!");
										} 
										else if (tentrs[0] == 2) {
											var tentrs2 = tentrs[1].split("-");
											var tentrs20 = tentrs2[0].split("_");
											var tentrs21 = tentrs2[1].split("_");
											//alert(tentrs[1]+': '+tentrs20[1]+', '+tentrs21[1]);
											var tentrs30, tentrs31;
											
											if (tentrs20[1] < tentrs21[1]) {
												tentrs30 = tentrs20[1];
												tentrs31 = tentrs21[1];
											}
											else { // tentrs20[1] > tentrs21[1]
												tentrs30 = tentrs21[1];
												tentrs31 = tentrs20[1];
											}
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=diff&d_val='+encodeURIComponent(tentrs30)+'&d_val2='+encodeURIComponent(tentrs31), 'mef', 'diff', 'html', 'GET', '');
										}
										else {
											alertbox("Select two entries to compare them!");
										}
									}

									function diffentries2(revid1, revid2) { // called from diff area to update/change entries for diff-process
										if (revid1 != '' && revid2 != '') {
											//alert('rev-ids: '+ revid1 +', '+ revid2);
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=diff&d_val='+encodeURIComponent(revid1)+'&d_val2='+encodeURIComponent(revid2), 'mef', 'diff2', 'html', 'GET', '');
										}
									}

									function diffentries3(revid1, revid2) { // called from diff area to update/change entries for diff-process
										document.getElementById('frmdiff').innerHTML = '';
										
										if (document.getElementById('frmdiff').style.display == 'none') {
											document.getElementById('frmdiff').style.display = 'block';
											document.getElementById('bshowdiffi').src = 'images/tab_open.gif';
											diffentries2(revid1, revid2);
										}
										else {
											document.getElementById('frmdiff').style.display = 'none';
											document.getElementById('bshowdiffi').src = 'images/tab_closed.gif';
										}
									}
									
									function bpreview() {
										var tentrs = selectedEntries().split("|");
										var tentrs2 = tentrs[1].split("_");
										
										if (tentrs[0] == 1) {
											//secwind = window.open("<?php echo $roscms_intern_page_link; ?>data_out&d_f=page&d_u=show&d_val="+tentrs2[1]+"&d_val2=&d_val3=", "RosCMSPagePreview", "location=no,menubar=no,resizable=yes,status=yes,toolbar=no,scrollbars=yes,width=1000,height=800,left=20,top=20");
											secwind = window.open("<?php echo $roscms_intern_page_link; ?>data_out&d_f=page&d_u=show&d_val="+tentrs2[1]+"&d_val2="+userlang+"&d_val3=", "RosCMSPagePreview");
											//secwind.focus();
										}
										else {
											alertbox("Select one entry to preview a page!");
										}
										
										document.getElementById('extraopt').value = 'sel';
										//document.getElementById('cmddiff').focus();
									}


									function bchangetags(ctk) {
										if ((document.getElementById('extraopt').value != 'sel' && document.getElementById('extraopt').value != 'no') || ctk == 'ms' || ctk == 'mn') {
											
											var tentrs = selectedEntries().split("|");
											if (tentrs[0] < 1 || tentrs[0] == '') {
												alertbox("No entry selected.");
											}
											else {
//												alert('==> '+ctk);
												var tmp_obj = 'changetags';
												
												if (ctk == 'ms') {
													tmp_obj = 'changetags2';
													alertbox('Please be patient, related pages get generated ...');
												}
												
												makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetags&d_val='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3='+encodeURIComponent(ctk), 'mef', tmp_obj, 'html', 'GET', '');
											}
										}
										
										document.getElementById('extraopt').value = 'sel';
										
										try {
											document.getElementById('cmddiff').focus();
										}
										catch (e) {}
									}

									
								-->
								</script>
								<style type="text/css">
								<!--
									
										.frmeditheadline {
											color: #5984C3;
											font-weight: bold;
											font-size: 120%;
											padding-right: 25px;
										}
										
										.frmeditheader {
											color: #00681C;
											font-weight: bold;
										}												
										
								-->
								</style>					
								<div id="frmedithead" style="padding-bottom: 10px;">&nbsp;</div>
								<div style="width:100%;">
									<div id="editzone">&nbsp;</div>
								</div>
							</div>
						</div>
					</div>
				</div>
				</div>
				</div>
			</div>
			
<p>&nbsp;</p>
<p><strong>Table Legend</strong></p>
<style>
<!--
	body {
		font-family: Verdana, Arial, Helvetica, sans-serif;
		font-size: 83%;
	}
	
	table#legend {
		display: block;
		width: 500px;
		border: solid 1px black;
	}
	
	table#legend tr td.lbox {
		border: solid 1px black;
		width: 17px;
	}
-->
</style>
	<table id="legend" cellspacing="5">
		<tr>
			<td class="lbox" bgcolor="#dddddd">&nbsp;</td>
			<td width="205" rowspan="2">
				Standard<br />
				<span class="style2">(odd/even)</span>
			</td>
			<td width="50" rowspan="5">&nbsp;</td>
			<td class="lbox" bgcolor="#ffcc99">&nbsp;</td>
			<td width="205">Selected</td>
		</tr>
		<tr>
			<td class="lbox" bgcolor="#eeeeee">&nbsp;</td>
			<td class="lbox" bgcolor="#A3EDB4">&nbsp;</td>
			<td>Translation  up to date</td>
		</tr>
		<tr>
			<td class="lbox" bgcolor="#B5EDA3">&nbsp;</td>
			<td>New</td>
			<td class="lbox" bgcolor="#D6CAE4">&nbsp;</td>
			<td>Translation doesn't exist</td>
		</tr>
		<tr>
			<td class="lbox" bgcolor="#FFE4C1">&nbsp;</td>
			<td>Draft</td>
			<td class="lbox" bgcolor="#FAA5A5">&nbsp;</td>
			<td>Translation  outdated</td>
		</tr>
		<tr>
			<td class="lbox" bgcolor="#ffffcc">&nbsp;</td>
			<td>Mouse hover</td>
			<td class="lbox" bgcolor="#FFCCFF">&nbsp;</td>
			<td>Unknown</td>
		</tr>
	</table>


		<?php
			if ($RosCMS_GET_debug) {
		?>
		<p><b>Ajax-Debug</b></p>
		<p><textarea name="txttabelle" cols="75" rows="10" wrap="off" id="txttabelle"></textarea>
		</p>
		<?php
			}
		?>
      </div>
	</div>
</div>
<p>&nbsp;</p>
<script type="text/javascript" language="javascript">
<!--
	
	function loadingsplash(zeroone) {
		if (zeroone == 1) {
			document.getElementById('ajaxloadinginfo').style.visibility = 'visible';
		}
		else {
			document.getElementById('ajaxloadinginfo').style.visibility = 'hidden';
		}
	}
	
	function makeRequest(url, action, objid, format, kind, parameters) {
		loadingsplash(1); 

		var http_request = false;

		if (window.XMLHttpRequest) { // Mozilla, Safari,...
			http_request = new XMLHttpRequest();
		}
		else if (window.ActiveXObject) { // IE
			try {
				http_request = new ActiveXObject("Msxml2.XMLHTTP");
			} catch (e) {
				try {
				http_request = new ActiveXObject("Microsoft.XMLHTTP");
				} catch (e) {}
			}
		}
		
		if (!http_request) { // stop if browser doesn't support AJAX
			alert('Cannot create an XMLHTTP instance. \nMake sure that your browser does support AJAX. \nMake sure that your browser does support AJAX. \nTry out IE 5.5+ (with ActiveX enabled), IE7+, Mozilla, Opera 9+ or Safari 3+.');
			return false;
		}

		// override mime
		if (http_request.overrideMimeType && format=='xml') {
			http_request.overrideMimeType('text/xml');
		}
		else if (http_request.overrideMimeType && format=='text') {
			http_request.overrideMimeType('text/plain');
		}
		else if (http_request.overrideMimeType && format=='html') {
			http_request.overrideMimeType('text/html');
		}

		if (kind != 'GET' && kind != 'POST') {
			kind = 'GET';
		}
		
		if (roscms_archive == 1) { // enter archive mode
			url = url + '&d_arch=true';
		}
		
		if (kind == 'GET') {
			http_request.onreadystatechange = function() { alertContents(http_request, action, objid); };
			http_request.open('GET', url, true);
			http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
			http_request.send(null);
		}
		else if (kind == 'POST') {
			http_request.onreadystatechange = function() { alertContents(http_request, action, objid); };
			http_request.open('POST', url, true);
//			alert('POST (length: '+parameters.length+'):\n\n:'+ parameters);
			http_request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			http_request.setRequestHeader("Content-length", parameters.length);
			http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
			http_request.setRequestHeader("Connection", "close");
			http_request.send(parameters);
		}
	}

	function alertContents(http_request, action, objid) {
		try {
			if (http_request.readyState == 4) {
				if (http_request.status == 200) {
					loadingsplash(0); 
					
					switch (action) {
						case 'ptm': /* page table main */
							page_table_populate(http_request, objid);
							break;
						case 'mef': /* main edit frame */
							/*if (objid == 'changetags2') {
								alertbox('Please be patient, related pages get generated ...');
							}*/
							main_edit_load(http_request, objid);
							break;
						case 'asi': /* auto save info */
							autosave_info(http_request, objid);
							break;
						case 'ufs': /* user filter storage */
							uf_storage(http_request, objid);
							break;
						case 'uqi': /* user quick info */
							u_quickinfo(http_request, objid);
							break;
						default:
							alert('Unknown-AJAX-LoadAction: '+ action);
							break;
					}
				}
				else {
					alert('There was a problem with the request ['+http_request.status+' / '+http_request.readyState+']. \n\nA client (browser) or server problem. Please make sure you use an up-to-date browser client. \n\nIf this error happens more than once or twice, contact the website admin.');
				}
			}
		}
		catch (e) {
			if (roscms_page_load_finished == true) {
				//alert(roscms_page_load_finished +' , '+ http_request.readyState +' , '+ http_request.status);
				//alert('Info\n\nCaught Exception: \nName: '+ e.name +'\nNumber: '+ e.number +'\nMessage: '+ e.message +'\nDescription: '+ e.description +'\n\nIf this error occur more than once or twice, please reload the page using the reload-link on the top-right of the page. And if you get this info-message on a usual base, please contact the website admin with the exact error message. \n\nIf you use the Safari or Firefox browser, please make sure you run the latest version (some versions have related bugs).');
				alertbox('RosCMS caught an exception to prevent data loss. If you see this message several times, please make sure you use an up-to-date browser client. If the issue still occur, tell the website admin the following information:<br />Name: '+ e.name +'; Number: '+ e.number +'; Message: '+ e.message +'; Description: '+ e.description);
        	}
		}
		
		// to prevent memory leak
		http_request = null;
	}
	
	function page_table_populate(http_request, objid) {
		var lstData = "";
		var temp_counter_loop = 0;
		
		var xmldoc = http_request.responseXML;
		var root_node = xmldoc.getElementsByTagName('root').item(0);
		
		//alert(objid);
		//alert(root_node.firstChild.data);
		
		try {
			if (root_node.firstChild.data) {
				// temp
			}
		}
		catch (e) {
			nres = 0;
			hlRows=false;
			document.getElementById('mtblnav').innerHTML = '&nbsp;';
			document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
			lstData = '<div class="tableswhitespace"><br /><br /><b>No results, due an error in the filter settings or the data metadata.</b><br /><br />If this happens more than a few times, please contact the website admin with the following information:<br />Name: '+ e.name +'<br />Number: '+ e.number +'<br />Message: '+ e.message +'<br />ObjID: '+ objid +'<br />Request: <pre>'+ http_request +'</pre>'+ page_table_space(8)+'</div>';
			document.getElementById(objid).innerHTML = lstData;
			return;
		}
		
		
		if ((root_node.firstChild.data.search(/#none#/)) == -1) {
			lstData = "";
			//alert(xmldoc.getElementsByTagName("row").length);
			var xrow = xmldoc.getElementsByTagName("row");
			var xview = xmldoc.getElementsByTagName("view");
			
			/*
			xrow[0].getAttributeNode("curpos").value
			xrow[0].getAttributeNode("pagelimit").value
			xrow[0].getAttributeNode("pagemax").value
			*/
			
			var mtblnavstr = '';

			roscms_current_tbl_position = xview[0].getAttributeNode("curpos").value;

			//alert(xview[0].getAttributeNode("curpos").value+' >= '+<?php echo $roscms_intern_entry_per_page; ?>*2);
			if (xview[0].getAttributeNode("curpos").value >= <?php echo $roscms_intern_entry_per_page; ?>*2) {
				mtblnavstr += '<span class="l" onclick="load_frametable_cp(0)"><b>&laquo;</b></span>&nbsp;&nbsp;';
			}
			
			//alert(xview[0].getAttributeNode("curpos").value+' >= '+<?php echo $roscms_intern_entry_per_page; ?>);
			if (xview[0].getAttributeNode("curpos").value > 0) {
				mtblnavstr += '<span class="l" onclick="load_frametable_cp(';
				if (xview[0].getAttributeNode("curpos").value-<?php echo $roscms_intern_entry_per_page; ?>*1 >= 0) {
					mtblnavstr += xview[0].getAttributeNode("curpos").value-<?php echo $roscms_intern_entry_per_page; ?>*1;
				}
				else {
					mtblnavstr += '0';
				}
				mtblnavstr += ')"><b>&lsaquo; Previous</b></span>&nbsp;&nbsp;';
			}
			
			var mtblnavfrom = xview[0].getAttributeNode("curpos").value*1+1;
			var mtblnavto = (xview[0].getAttributeNode("curpos").value*1) + (xview[0].getAttributeNode("pagelimit").value*1);
			
			
			/*mtblnavstr += '<select name="cbocurpage" id="cbocurpage">';
			mtblnavstr += '  <option>1 - 25</option>';
			mtblnavstr += '  <option>26 - 50</option>';
			mtblnavstr += '</select>';*/
			
			mtblnavstr += '<b>'+mtblnavfrom+'</b> - <b>';
			
			if (mtblnavto > xview[0].getAttributeNode("pagemax").value) {
				mtblnavstr += xview[0].getAttributeNode("pagemax").value;
			}
			else {
				mtblnavstr += mtblnavto;
			}
			
			mtblnavstr += '</b> of <b>'+xview[0].getAttributeNode("pagemax").value+'</b>';
			
			if (xview[0].getAttributeNode("curpos").value < xview[0].getAttributeNode("pagemax").value-<?php echo $roscms_intern_entry_per_page; ?>*1) {
				mtblnavstr += '&nbsp;&nbsp;<span class="l" onclick="load_frametable_cp(';
				mtblnavstr += xview[0].getAttributeNode("curpos").value*1+<?php echo $roscms_intern_entry_per_page; ?>*1;
				mtblnavstr += ')"><b>Next &rsaquo;</b></span>';
			}
			
			//alert(xview[0].getAttributeNode("curpos").value+' < '+ (xview[0].getAttributeNode("pagemax").value*1-<?php echo $roscms_intern_entry_per_page; ?>*2));
			if (xview[0].getAttributeNode("curpos").value < (xview[0].getAttributeNode("pagemax").value*1-<?php echo $roscms_intern_entry_per_page; ?>*2)) {
				mtblnavstr += '&nbsp;&nbsp;<span class="l" onclick="load_frametable_cp(';
				mtblnavstr += xview[0].getAttributeNode("pagemax").value*1-<?php echo $roscms_intern_entry_per_page; ?>*1;
				mtblnavstr += ')"><b>&raquo;</b></span>';
			}
			mtblnavstr += '&nbsp;&nbsp;';
		
			document.getElementById('mtblnav').innerHTML = mtblnavstr;
			document.getElementById('mtbl2nav').innerHTML = mtblnavstr;
			
			
			//'<a href="#">&laquo;</a></span>&nbsp;&nbsp;<span id="nwr"><a href="#">&lsaquo; Previous</a></span> <strong>101</strong> - <strong>150</strong> of <strong>1524</strong> <span id="odr"><a href="#">Next &rsaquo;</a></span>&nbsp;<span id="ods"> <a href="#">&raquo;</a></span>';
			
			lstData += page_table_header(xview[0].getAttributeNode("tblcols").value);
			//lstData += page_table_header();
			
			for (var i=0; i < xrow.length; i++) {
				/* <item id="5" dname="index" type="layout" rid="3" rver="1" rlang="all" rdate="2007-02-22 18:07:55"> */

				lstData += page_table_body(
												i,
												/*xrow[i].getAttributeNode("color").value,*/
												xrow[i].getAttributeNode("status").value,
												xrow[i].getAttributeNode("id").value, 
												xrow[i].getAttributeNode("dname").value,
												xrow[i].getAttributeNode("type").value,
												xrow[i].getAttributeNode("rid").value,
												xrow[i].getAttributeNode("rver").value,
												xrow[i].getAttributeNode("rlang").value,
												xrow[i].getAttributeNode("rdate").value,
												xrow[i].getAttributeNode("star").value,
												xrow[i].getAttributeNode("starid").value,
												xrow[i].getAttributeNode("rusrid").value,
												xrow[i].getAttributeNode("security").value,
												xrow[i].getAttributeNode("xtrcol").value,
												xrow[i].firstChild.data
											);
				
				//alert(xrow[i].getAttributeNode("id").value);	
				temp_counter_loop = i;		
			}
			lstData += '</tbody></table>';
			
			if (xrow.length < 15) {
				lstData += '<div class="tableswhitespace"><br />'+page_table_space(10-xrow.length)+'</div>';
			}
			
			//document.getElementById('txttabelle').value = lstData;
			nres = (temp_counter_loop+1);
			hlRows=true;
			window.setTimeout("add_js_extras()", 100);
		}
		else {
			nres = 0;
			hlRows=false;
			document.getElementById('mtblnav').innerHTML = '&nbsp;';
			document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
			lstData = '<div class="tableswhitespace"><br /><br /><b>No results.</b>'+page_table_space(10)+'</div>';
		}

		document.getElementById(objid).innerHTML = lstData;


	}
	
	function page_table_header(xtrtblcol) {
		var xtrtblcols2 ='';
		var lstHeader = '';

		xtrtblcols2 = xtrtblcol.split('|');
		
		lstHeader += '<table class="datatable" id="rpmt" name="rpmt" cellpadding="1" cellspacing="0">';
		lstHeader += '<thead><tr class="head">';
		lstHeader += '<th scope="col" class="cMark">&nbsp;</th>';
		lstHeader += '<th scope="col" class="cStar">&nbsp;</th>';
		lstHeader += '<th scope="col" class="cCid">Name</th>';
		/*lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>';
		lstHeader += '<th scope="col" class="cType">Type</th>';*/
		lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>';
		lstHeader += '<th scope="col" class="cRev">Title</th>';

/*
		if (xtrtblcol != '') {
			lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>';
			lstHeader += '<th scope="col" class="cDate">'+xtrtblcol+'</th>';
		}
*/
		
		if (xtrtblcol != '' && xtrtblcols2.length > 1) {
			for (var i=1; i < xtrtblcols2.length-1; i++) {
				lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>';
				lstHeader += '<th scope="col" class="cExtraCol">'+xtrtblcols2[i]+'</th>';
			}
		}

		lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>';
		lstHeader += '<th scope="col" class="cDate">Date</th>';
		lstHeader += '</tr></thead><tbody>\n';
		
	
		return lstHeader;
		lstHeader = null;
	}	
	
	function page_table_body( bnr, bclass, bid, bdname, btype, brid, brver, brlang, brdate, bstar, bstarid, brusrid, security, xtrtblcol, bdesc) {
		var xtrtblcols2 = '';
		var lstBody = '';
		var tmpdesc = '';
		
		if (bstar == 1) {
			bstar = 'cStarOn';
		}
		else {
			bstar = 'cStarOff';
		}

		xtrtblcols2 = xtrtblcol.split('|');
		
		lstBody += '<tr class="'+bclass+'" id="tr'+(bnr+1)+'">';
		lstBody += '<td align="right"><input id="cb'+(bnr+1)+'" type="checkbox" onclick="selcb(this.id)" /></td>';
		lstBody += '<td class="tstar_'+bid+'|'+brid+'-'+bstarid+'"><div id="bstar_'+(bnr+1)+'" class="'+bstar+'">&nbsp;</div></td>';
		/*lstBody += '<td class="rv'+bid+'|'+brid+'"><div id="bstar'+(bnr+1)+'" class="'+bstatus+'">&nbsp;</div></td>';*/
		lstBody += '<td class="rv'+bid+'|'+brid+'">'+bdname+'</td>';
		/*lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
		lstBody += '<td class="rv'+bid+'|'+brid+'">'+btype+'</td>';*/
		lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
		lstBody += '<td class="rv'+bid+'|'+brid+'"><div class="cell-height">';
		if (security.indexOf("write") < 0 ) { // not found -> readonly
			lstBody += '<img src="images/locked.gif" alt="read-only" style="width:11px; height:12px; border:0px;" /> ';
		}
		try {
			tmpdesc = unescape(decodeURI(bdesc));
			tmpdesc = tmpdesc.replace(/\+/g, ' ');
		} catch (e) {
			tmpdesc = '<i>check the title or description field, it contains non UTF-8 chars</i>';
		}

		lstBody += '<span class="tcp">'+tmpdesc+'</span></div></td>';
		/*lstBody += '<span class="tcp">!!</span></div></td>';*/

/*
		if (xtrtblcol != '') {
			lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
			lstBody += '<td class="rv'+bid+'|'+brid+'">'+xtrtblcol+'&nbsp;</td>';
		}
*/	
	
	
		if (xtrtblcol != '' && xtrtblcols2.length > 1) {
			for (var i=1; i < xtrtblcols2.length-1; i++) {
				lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
				lstBody += '<td class="rv'+bid+'|'+brid+'">'+xtrtblcols2[i]+'</td>';
			}
		}

		/*if (xtrtblcol != '' && xtrtblcols2.length > 0) {
			for (var i=0; i < xtrtblcols2.length; i++) {
				lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
				lstBody += '<td class="rv'+bid+'|'+brid+'">'
				switch (xtrtblcols2[i].toLowerCase()) {
					default:
						lstBody += '&nbsp;';
						break;
					case 'type':
						lstBody += btype;
						break;
					case 'language':
						lstBody += brlang;
						break;
					case 'user':
						lstBody += brusrid;
						break;
				}
				lstBody += '</td>';
			}
		}*/
		
		lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
		lstBody += '<td class="rv'+bid+'|'+brid+'">'+brdate.substr(0, 10)+'</td>';
		lstBody += '</tr>';		
		
		return lstBody;
		lstBody = null;
	}
	
	function page_table_space(spacelines) {
		var lstSpace = "";
		//alert(spacelines);
		
		for (var i=0; i < spacelines; i++) {
			lstSpace += "<p>&nbsp;</p>";
		}
		
		return lstSpace;
	}
		
	function main_edit_load(http_request, objid) {
		var tsplits = '';
		var tsplitsa = '';
		
		tsplits = objid.split('_');
				
		//alert(tsplits[0]);
		switch (tsplits[0]) {
			case 'bstar':
				//alert('bstar: tr'+tsplits[1]+' # '+http_request.responseText);
				tsplitsa = document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className.split('-');
				//alert('was ist drin: '+tsplitsa[0]+'---'+tsplitsa[1]+' und das kommt heraus: '+tsplitsa[0]+'-'+http_request.responseText);
				document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className = tsplitsa[0]+'-'+http_request.responseText;
				break;
			case 'editstar':
				document.getElementById(objid).className = http_request.responseText;
				break;
			case 'addnew':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				break;
			case 'addnew2':
//				alert('addnew2');
//				alert(http_request.responseText);
				document.getElementById('editzone').innerHTML = http_request.responseText;
				/*if (http_request.responseText != "") {
					document.getElementById('editzone').innerHTML = '<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p>';
					makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new', 'mef', 'addnew', 'html', 'GET', '');
				}*/
				break;
			case 'diff':
				//alert('!!! diff !!!');
				//alert(http_request.responseText);
				//load_script('http://localhost/reactos.org/roscms/js/diffv3.js');
				document.getElementById('editzone').innerHTML = '<div id="frmdiff">'+ http_request.responseText + '</div>';
				load_frameedit('diffentry');
				document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
				break;
			case 'diff2': // update diff-area with new entries start diff-process; called from within diff-area
				document.getElementById('frmdiff').innerHTML = http_request.responseText;
				document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
				break;
			case 'changetags':
				//alert('!!! changetags !!!'+http_request.responseText);
				<?php
					if ($RosCMS_GET_debug) {
						echo " alert('!!! changetags: '+http_request.responseText); ";
					}
				?>
				load_frametable_cp(0);
				alertbox('Action performed');
				break;
			case 'changetags2':
				<?php
					if ($RosCMS_GET_debug) {
						echo " alert('!!! changetags(*ms*): '+http_request.responseText); ";
					}
				?>
				load_frametable_cp(0);
				alertbox(http_request.responseText);
				break;
			case 'editalterfields':
				//alert('!!! saved?: '+http_request.responseText);
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Entry fields updated');
				break;
			case 'editaltersecurity':
				//alert('!!! saved?: '+http_request.responseText);
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Data fields updated');
				break;
			case 'editalterentry':
				//alert('!!! saved?: '+http_request.responseText);
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Entry updated');
				break;
			default:
				document.getElementById(objid).innerHTML = http_request.responseText;
				autosave_cache = prepair_edit_form_submit();
//				alert('fill cache: (textbox - cache)\n'+prepair_edit_form_submit() +'\n'+ autosave_cache);
				break;
		}
		//alert('load: '+tsplits[0]);
	}
	
	function autosave_info(http_request, objid) {
		switch (objid) {
			case 'mefasi':
				//alert('test');
				<?php
					if ($RosCMS_GET_debug) {
						echo "alertbox('<i>draft:</i> '+http_request.responseText);";
					}
				?>
				
				var tempcache = prepair_edit_form_submit();
				
				if (autosave_cache != tempcache) {
					autosave_cache = tempcache;			
				}
				
//				alert('fill cache: (textbox - cache)\n'+prepair_edit_form_submit() +'\n'+ autosave_cache);
				
				var d = new Date();
				var curr_hour = d.getHours();
				var curr_min = d.getMinutes();
				
				if (curr_hour.length == 1) curr_hour = '0'+curr_hour;
				if (curr_min.length == 1) curr_min = '0'+curr_min;
				
				document.getElementById('mefasi').innerHTML = 'Draft saved at '+ curr_hour +':'+ curr_min;
				break;
			case 'alert':
				window.clearTimeout(autosave_timer);
				<?php
					if ($RosCMS_GET_debug) {
						echo "alertbox('<i>alert:</i> '+ http_request.responseText);";
					}
				?>
				load_frametable(roscms_prev_page);
				break;
			default:
				alert('autosave_info() with no args');
				break;
		}
//		alert('load_2: '+objid);
	}
	
	function uf_storage(http_request, objid) {
		//alert(http_request.responseText);
		//alert(document.getElementById(objid).innerHTML);
		document.getElementById(objid).innerHTML = http_request.responseText;
	}
	
	function u_quickinfo(http_request, objid) {
		document.getElementById('qiload').style.display = 'none';
		document.getElementById(objid).innerHTML = http_request.responseText;
	}
	
	/* selstar(this.className, document.getElementById('bstar'+this.parentNode.id.substr(2,4)).className, <?php echo $roscms_intern_account_id; ?>, 'bstar'+this.parentNode.id.substr(2,4)) */
	function selstar(entryid, dtv, dusr, objid) {
		if (entryid.indexOf("|") > -1) { 
			var devide1 = ''
			var devide2 = ''
			var devideids1 = ''
			var devideids2 = ''
			var devideids3 = ''
			
			devide1 = entryid.split('_');
			devide2 = devide1[1].split('-');
			devideids1 = devide2[0].indexOf("|");
			devideids2 = devide2[0].substr(0, devideids1);
			devideids3 = devide2[0].substr(devideids1+1);
			
			//alert(devideids2+', '+devideids3+'; '+devide2[1]+', '+objid);
			
			//alert(entryid+'; '+dtv+'; '+dusr+'; '+objid+';;  '+devide2+';;  '+devideids2+';;  '+devideids2.substr(0,2));
			
			
			if (devideids2.substr(0,2) == 'tr') {
				alertbox('Cannot bookmark not translated entries.');
			}
			else {
			
				if (dtv == 'cStarOff') {
					dtv = 'on';
					document.getElementById(objid).className = 'cStarOn';
				}
				else {
					dtv = 'off';
					document.getElementById(objid).className = 'cStarOff';
				}
				bchangetag(devideids2, devideids3, 'star', dtv, dusr, devide2[1], objid, '3');
			}
		}
	}
	
	function debugLog (dbg_str) {
		<?php
			if ($RosCMS_GET_debug) {
		?>
				var currentTime = new Date();
				var month = currentTime.getMonth() + 1;
				var day = currentTime.getDate();
				var year = currentTime.getFullYear();
				var datetime = year+'-'+month+'-'+day;
			
			
				var currentTime = new Date();
				var hours = currentTime.getHours();
				var minutes = currentTime.getMinutes();
				if (minutes < 10) minutes = "0" + minutes;
				var seconds = currentTime.getSeconds();
				if (seconds < 10) seconds = "0" + seconds;
				datetime += ' '+hours+':'+minutes+':'+seconds+' - ';
				document.getElementById('txttabelle').value = datetime + dbg_str +'\n'+ document.getElementById('txttabelle').value;
		<?php
			}
			else {
				echo "/* ... */";
			}
		?>
	}
			
-->
</script>

<script type="text/javascript" language="javascript">
	var nres=0;
	var hlRows=true;
	document.getElementById('tablist').style.display = 'block';
	
	//makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl=new&d_cp=0', 'ptm', 'tablist', 'xml', 'GET', '');
	

	document.getElementById('frametable').style.display = 'block';
	document.getElementById('frameedit').style.display = 'none';


	smenutab_open('smenutab8');
	
	
	<?php
		if ($RosCMS_GET_cms_edit != "") {
	?>	
		document.getElementById('frametable').style.display = 'none';
		document.getElementById('frameedit').style.display = 'block';

		//alert('<?php echo $RosCMS_GET_cms_edit; ?>');
		load_frameedit('all', '<?php echo $RosCMS_GET_cms_edit; ?>');
	<?php
		}
	?>	
	
	
	load_user_filter('filter');
	load_user_filter('label');
	
	if (readCookie('labtitel1') == 0) TabOpenCloseEx('labtitel1');
	if (readCookie('labtitel2') == 0) TabOpenCloseEx('labtitel2');
	if (readCookie('labtitel3') == 0) TabOpenCloseEx('labtitel3');


	<?php
		if ($roscms_security_level == 1) {
	?>	
		alertbox('Hint: Check out the "Welcome" page once a day!');
	<?php
		}
	?>	
	
	roscms_page_load_finished = true;

	// window unload blocker
	window.onbeforeunload = unloadMessage;
	function unloadMessage(){
		if (exitmsg != '') {
			return exitmsg;
		}
	}
	
</script>

<script type="text/javascript" language="javascript">
		tinyMCE.init({
			mode : "none",
			theme : "advanced",
			editor_selector : "mceEditor",
			plugins : "table,advhr,advimage,advlink,emotions,iespell,insertdatetime,zoom,searchreplace,print,contextmenu,paste,directionality",
			theme_advanced_buttons1_add_before : "newdocument, separator",
			theme_advanced_buttons1_add : "fontselect,fontsizeselect",
			theme_advanced_buttons2_add : "separator,insertdate,inserttime,zoom,separator,forecolor,backcolor",
			theme_advanced_buttons2_add_before: "cut,copy,paste,pastetext,pasteword,separator,search,replace,separator",
			theme_advanced_buttons3_add_before : "tablecontrols,separator",
			theme_advanced_buttons3_add : "emotions,iespell,media,advhr,separator,print,separator,ltr,rtl",
			theme_advanced_disable : "help, code",
			theme_advanced_toolbar_location : "top",
			theme_advanced_toolbar_align : "left",
			theme_advanced_statusbar_location : "bottom",
			/*content_css : "example_word.css",*/
			plugi2n_insertdate_dateFormat : "%Y-%m-%d",
			plugi2n_insertdate_timeFormat : "%H:%M:%S",
			external_link_list_url : "example_link_list.js",
			external_image_list_url : "example_image_list.js",
			media_external_list_url : "example_media_list.js",
			file_browser_callback : "fileBrowserCallBack",		
			valid_elements : "" +
				"+a[id|style|rel|rev|charset|hreflang|dir|lang|tabindex|accesskey|type|name|href|target|title|class|onfocus|onblur|onclick|" + 
					"ondblclick|onmousedown|onmouseup|onmouseover|onmousemove|onmouseout|onkeypress|onkeydown|onkeyup]," + 
				"-strong/-b[class|style]," + 
				"-em/-i[class|style]," + 
				"-strike[class|style]," + 
				"-u[class|style]," + 
				"#p[id|style|dir|class|align]," + 
				"-ol[class|style]," + 
				"-ul[class|style]," + 
				"-li[class|style]," + 
				 "br," + 
				 "img[id|dir|lang|longdesc|usemap|style|class|src|onmouseover|onmouseout|border|alt=|title|hspace|vspace|width|height|align]," + 
				"-sub[style|class]," + 
				"-sup[style|class]," + 
				"-blockquote[dir|style]," + 
				"-table[border=0|cellspacing|cellpadding|width|height|class|align|summary|style|dir|id|lang|bgcolor|background|bordercolor]," + 
				"-tr[id|lang|dir|class|rowspan|width|height|align|valign|style|bgcolor|background|bordercolor]," + 
				 "tbody[id|class]," + 
				 "thead[id|class]," + 
				 "tfoot[id|class]," + 
				"-td[id|lang|dir|class|colspan|rowspan|width|height|align|valign|style|bgcolor|background|bordercolor|scope]," + 
				"-th[id|lang|dir|class|colspan|rowspan|width|height|align|valign|style|scope]," + 
				 "caption[id|lang|dir|class|style]," + 
				"-div[id|dir|class|align|style]," + 
				"-span[style|class|align]," + 
				"-pre[class|align|style]," + 
				 "address[class|align|style]," + 
				"-h1[id|style|dir|class|align]," + 
				"-h2[id|style|dir|class|align]," + 
				"-h3[id|style|dir|class|align]," + 
				"-h4[id|style|dir|class|align]," + 
				"-h5[id|style|dir|class|align]," + 
				"-h6[id|style|dir|class|align]," + 
				 "hr[class|style]," + 
				"-font[face|size|style|id|class|dir|color]," + 
				 "dd[id|class|title|style|dir|lang]," + 
				 "dl[id|class|title|style|dir|lang]," + 
				 "dt[id|class|title|style|dir|lang]",		
			paste_use_dialog : false,
			theme_advanced_resizing : true,
			theme_advanced_resize_horizontal : false,
			theme_advanced_link_targets : "_something=My somthing;_something2=My somthing2;_something3=My somthing3;",
			paste_auto_cleanup_on_paste : true,
			paste_convert_headers_to_strong : false,
			paste_strip_class_attributes : "all",
			paste_remove_spans : false,
			paste_remove_styles : false,	
			apply_source_formatting : true 
		});
</script>
