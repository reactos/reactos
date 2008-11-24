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
  
		function textbox_hint(objid, objval, objhint, zeroone) {
			
			if (zeroone == 1 && objval == objhint) {
				document.getElementById(objid).value = '';
				document.getElementById(objid).style.color ='#000000';
			}
			if (zeroone == 0 && objval == '') {
				document.getElementById(objid).value = objhint;
				document.getElementById(objid).style.color = '#999999';
			}
		
		}
    
    
			function beautifystr (tmp_str) {
				
				// remove invalid characters
				tmp_str = tmp_str.replace(/\|/g, '');
				tmp_str = tmp_str.replace(/=/g, '');
				tmp_str = tmp_str.replace(/&/g, '');
				tmp_str = tmp_str.replace(/_/g, '');
				
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
				

				usf_req = '?page=data_out&d_f=text&d_u=uqi&d_val=ptm&d_id='+encodeURIComponent(qistr[0].substr(2))+'&d_r_id='+encodeURIComponent(qistr[1]);
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
					
					usf_req = '?page=data_out&d_f=text&d_u=ufs&d_val=add&d_val2='+encodeURIComponent(uf_type)+'&d_val3='+encodeURIComponent(uf_name)+'&d_val4='+encodeURIComponent(uf_str);
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
				makeRequest(usf_req, 'ufs', uf_objid, 'html', 'GET', '');
			}
			
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
		
			function tbl_user_filter(ufiltstr, ufilttype, ufilttitel) {
				var tentrs = selectedEntries().split("|");

				if (ufilttype == 2 && tentrs[0] > 0 && tentrs[0] != '') {
					
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
			
      
			function load_frametable(objevent) {
				if (document.getElementById('frametable').style.display != 'block') {
					document.getElementById('frametable').style.display = 'block';
					document.getElementById('frameedit').style.display = 'none';
					
					window.clearTimeout(alertactiv); /* deactivate alert-timer */
					document.getElementById('alertb').style.visibility = 'hidden';
					document.getElementById('alertbc').innerHTML = '&nbsp;';
					select_all(0); /* deselect all table entries */
				}
				
				if (submenu_button == 'true') {
					roscms_prev_page = roscms_current_page;
					roscms_current_page = objevent;
					filtpopulate(filtstring2);
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

				
				makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+objevent+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0', 'ptm', 'tablist', 'xml', 'GET', '');
			}
			
			function load_frametable_cp(tcp) {
				window.clearTimeout(alertactiv); /* deactivate alert-timer */
				document.getElementById('alertb').style.visibility = 'hidden';
				document.getElementById('alertbc').innerHTML = '&nbsp;';
				select_all(0); /* deselect all table entries */
					
				
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
									break;
								}
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

							
							// loading screen:
							document.getElementById('editzone').innerHTML = '<div style="background:#FFFFFF; border-bottom: 1px solid #bbb; border-bottom-width: 1px; border-right: 1px solid #bbb; border-right-width: 1px;"><div style="margin:10px;"><div style="width:95%;"><br /><br /><center><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" />&nbsp;&nbsp;loading ...</center><br /><br /></div></div></div>';
							
							
							makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl='+objevent+'&d_id='+devideids2+'&d_r_id='+devideids3+'&d_r_lang='+userlang, 'mef', 'editzone', 'html', 'GET', '');
						}
						else {
							alert('bad request: load_frameedit('+objevent+', '+entryid+')');
						}
						break;
				}
				
				
			}
		
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
										}
										
										function filtentryselect(objid) {
											var filtselstr = '';
											var filtselstr2 = '';
											
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
										
										function filtscanfilters() {									
												filtstring2 = '';
												

												for (var i=1; i <= filtercounter; i++) {
													
													if (document.getElementById('sfa'+i)) {
														
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
												}
												
												filtstring2 = filtstring2.substr(0,filtstring2.length-1); /* remove last '|' */
										}
	
										function filtsearchbox() { // standard search box
											if (document.getElementById('filtersc').style.display == 'none') { // without filters visible

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
											
											filtscanfilters();
											load_frametable(roscms_current_page);
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
												selbarstr += selhtml_draft;

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

									function bshowdepencies(did, drid) {
										makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showdepencies&d_id='+did+'&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
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
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=addtag&d_id='+did+'&d_r_id='+drid+'&d_val='+encodeURIComponent(dtna)+'&d_val2='+encodeURIComponent(dtva)+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
										}
									}
									
									function bdeltag(did, drid, dtid, dusr) {								
										if (dtid != '') {
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=deltag&d_id='+did+'&d_r_id='+drid+'&d_val='+dtid+'&d_val2='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
										}
									}
									
									function bchangetag(did, drid, dtn, dtv, dusr, dtid, objid, dbflag) {	
										if (dtn != '' && dtv != '') {
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetag'+encodeURIComponent(dbflag)+'&d_id='+did+'&d_r_id='+drid+'&d_val='+encodeURIComponent(dtn)+'&d_val2='+encodeURIComponent(dtv)+'&d_val3='+dusr+'&d_val4='+dtid, 'mef', objid, 'html', 'GET', '');
										}
									}
									
									function prepair_edit_form_submit() {
										try {
											var poststr = "";
																					
											/* short text */
											poststr += "pstextsum="+document.getElementById("estextcount").className;
											for (var i=1; i <= document.getElementById("estextcount").className; i++) {
												poststr += "&pdstext"+i+"=" + encodeURIComponent(document.getElementById("edstext"+i).innerHTML);
												poststr += "&pstext"+i+"=" + encodeURIComponent(document.getElementById("estext"+i).value);
											}
											
											/* text */
											poststr += "&plmsum="+document.getElementById("elmcount").className;
											var instatinymce;
											for (var i=1; i <= document.getElementById("elmcount").className; i++) {
												poststr += "&pdtext"+i+"=" + encodeURIComponent(document.getElementById("edtext"+i).innerHTML);
												
												instatinymce = ajaxsaveContent("elm"+i); // get the content from TinyMCE
												if (instatinymce != null) {
													poststr += "&plm"+i+"=" + encodeURIComponent(instatinymce);
												}
												else {
													poststr += "&plm"+i+"=" + encodeURIComponent(document.getElementById("elm"+i).value);
												}
											}
											
											
											return poststr;
										}
										catch (e) {
											rtestop(); // destroy old rich text editor instances
										}
									}

									function edit_form_submit_draft(did, drid) {
										document.getElementById("bsavedraft").disabled = true;
									
										edit_form_submit_draft_autosave(did, drid);
										
										
										rtestop(); // destroy old rich text editor instances
										
										load_frametable_cp2(roscms_current_tbl_position);
										window.clearTimeout(autosave_timer);
										alertbox('Draft saved');
									}

									function edit_form_submit_draft_autosave(did, drid) {
										var poststr = "";
										var usf_req = '';

										poststr = prepair_edit_form_submit();
										
										usf_req = '?page=data_out&d_f=text&d_u=asi&d_fl=new&d_id='+encodeURIComponent(did)+'&d_r_id='+encodeURIComponent(drid)+'&d_r_lang='+encodeURIComponent(document.getElementById("mefrlang").innerHTML)+'&d_r_ver='+encodeURIComponent(document.getElementById("mefrverid").innerHTML)+'&d_val='+encodeURIComponent(document.getElementById("estextcount").className)+'&d_val2='+encodeURIComponent(document.getElementById("elmcount").className)+'&d_val3=draft&d_val4='+encodeURIComponent(document.getElementById("entryeditdynnbr").innerHTML);
										makeRequest(usf_req, 'asi', 'mefasi', 'html', 'POST', poststr.substr(1));
									}
									
									function edit_form_submit(did, drid) {
										var usf_req = '';
										var poststr = "";
										document.getElementById("bsavenew").disabled = true;

										poststr = prepair_edit_form_submit();				


										usf_req = '?page=data_out&d_f=text&d_u=asi&d_fl=new&d_id='+encodeURIComponent(did)+'&d_r_id='+encodeURIComponent(drid)+'&d_r_lang='+encodeURIComponent(document.getElementById("mefrlang").innerHTML)+'&d_r_ver='+encodeURIComponent(document.getElementById("mefrverid").innerHTML)+'&d_val='+encodeURIComponent(document.getElementById("estextcount").className)+'&d_val2='+encodeURIComponent(document.getElementById("elmcount").className)+'&d_val3=submit';
										makeRequest(usf_req, 'asi', 'alert', 'html', 'POST', poststr.substr(1));

										rtestop(); // destroy old rich text editor instances

										load_frametable_cp2(roscms_current_tbl_position);
										window.clearTimeout(autosave_timer);
										alertbox('Entry saved');
									}
									
									function autosave_try(t_d_id, t_d_revid) {
										window.clearTimeout(autosave_timer);
										
										try {
											if (document.getElementById("editautosavemode").value == 'false') {
												window.clearTimeout(autosave_timer);
												return;
											}
										} 
										catch (e) {
											window.clearTimeout(autosave_timer);
											return;
										}
										
										if (autosave_cache != prepair_edit_form_submit() && autosave_cache != '') {
											edit_form_submit_draft_autosave(document.getElementById("entrydataid").className, document.getElementById("entrydatarevid").className);
										}

										autosave_timer = window.setTimeout("autosave_try()", autosave_coundown); // 10000
									}
									
									function changecreateinterface(menumode) {
										if (menumode == 'single' || menumode == 'dynamic' || menumode == 'template') {
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new&d_val='+menumode, 'mef', 'addnew', 'html', 'GET', '');
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
									
									function bchangetags(ctk) {
										if ((document.getElementById('extraopt').value != 'sel' && document.getElementById('extraopt').value != 'no') || ctk == 'ms' || ctk == 'mn') {
											
											var tentrs = selectedEntries().split("|");
											if (tentrs[0] < 1 || tentrs[0] == '') {
												alertbox("No entry selected.");
											}
											else {
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
				alertbox('RosCMS caught an exception to prevent data loss. If you see this message several times, please make sure you use an up-to-date browser client. If the issue still occur, tell the website admin the following information:<br />Name: '+ e.name +'; Number: '+ e.number +'; Message: '+ e.message +'; Description: '+ e.description);
        	}
		}
		
		// to prevent memory leak
		http_request = null;
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
		lstBody += '<td class="rv'+bid+'|'+brid+'">'+bdname+'</td>';
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

	
	
		if (xtrtblcol != '' && xtrtblcols2.length > 1) {
			for (var i=1; i < xtrtblcols2.length-1; i++) {
				lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
				lstBody += '<td class="rv'+bid+'|'+brid+'">'+xtrtblcols2[i]+'</td>';
			}
		}

		
		lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>';
		lstBody += '<td class="rv'+bid+'|'+brid+'">'+brdate.substr(0, 10)+'</td>';
		lstBody += '</tr>';		
		
		return lstBody;
		lstBody = null;
	}
	
	function page_table_space(spacelines) {
		var lstSpace = "";
		
		for (var i=0; i < spacelines; i++) {
			lstSpace += "<p>&nbsp;</p>";
		}
		
		return lstSpace;
	}
		
	function main_edit_load(http_request, objid) {
		var tsplits = '';
		var tsplitsa = '';
		
		tsplits = objid.split('_');
				
		switch (tsplits[0]) {
			case 'bstar':
				tsplitsa = document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className.split('-');
				document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className = tsplitsa[0]+'-'+http_request.responseText;
				break;
			case 'editstar':
				document.getElementById(objid).className = http_request.responseText;
				break;
			case 'addnew':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				break;
			case 'addnew2':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				break;
			case 'diff':
				document.getElementById('editzone').innerHTML = '<div id="frmdiff">'+ http_request.responseText + '</div>';
				load_frameedit('diffentry');
				document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
				break;
			case 'diff2': // update diff-area with new entries start diff-process; called from within diff-area
				document.getElementById('frmdiff').innerHTML = http_request.responseText;
				document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
				break;
			case 'changetags':
				load_frametable_cp(0);
				alertbox('Action performed');
				break;
			case 'changetags2':
				load_frametable_cp(0);
				alertbox(http_request.responseText);
				break;
			case 'editalterfields':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Entry fields updated');
				break;
			case 'editaltersecurity':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Data fields updated');
				break;
			case 'editalterentry':
				document.getElementById('editzone').innerHTML = http_request.responseText;
				alertbox('Entry updated');
				break;
			default:
				document.getElementById(objid).innerHTML = http_request.responseText;
				autosave_cache = prepair_edit_form_submit();
				break;
		}
	}
	
	function autosave_info(http_request, objid) {
		switch (objid) {
			case 'mefasi':
				
				var tempcache = prepair_edit_form_submit();
				
				if (autosave_cache != tempcache) {
					autosave_cache = tempcache;			
				}
				
				
				var d = new Date();
				var curr_hour = d.getHours();
				var curr_min = d.getMinutes();
				
				if (curr_hour.length == 1) curr_hour = '0'+curr_hour;
				if (curr_min.length == 1) curr_min = '0'+curr_min;
				
				document.getElementById('mefasi').innerHTML = 'Draft saved at '+ curr_hour +':'+ curr_min;
				break;
			case 'alert':
				window.clearTimeout(autosave_timer);
				load_frametable(roscms_prev_page);
				break;
			default:
				alert('autosave_info() with no args');
				break;
		}
	}
	
	function uf_storage(http_request, objid) {
		document.getElementById(objid).innerHTML = http_request.responseText;
	}
	
	function u_quickinfo(http_request, objid) {
		document.getElementById('qiload').style.display = 'none';
		document.getElementById(objid).innerHTML = http_request.responseText;
	}

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
  
  
	function add_js_extras() { // table mouse events
		if (!document.getElementById) return;
		
		for (i=1; i<=nres; i++) {
			//row highlighting
			if (hlRows) {
				document.getElementById("tr"+i).onmouseover = function() {hlrow(this.id.substr(2,4),1); show_quickinfo(this.getElementsByTagName('td')[3].className);}
				document.getElementById("tr"+i).onmouseout = function() {hlrow(this.id.substr(2,4),2); stop_quickinfo();}
				document.getElementById("tr"+i).getElementsByTagName('td')[0].onclick = function() {hlrow(this.parentNode.id.substr(2,4),3); selcb(this.parentNode.id.substr(2,4));}
				document.getElementById("tr"+i).getElementsByTagName('td')[1].onclick = function() {selstar(this.className, document.getElementById('bstar_'+this.parentNode.id.substr(2,4)).className, roscms_intern_account_id, 'bstar_'+this.parentNode.id.substr(2,4));}
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
  
  
			function smenutab_open(objid) {
        
				var chtabtext = '';
				if (document.getElementById('smenutab'+objid.substring(8)).className != 'subma') {
					
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
				
				if (getlang() == roscms_standard_language) {
					translang = roscms_standard_language_trans;
				}
				else {
					translang = getlang();
				}


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
						filtstring2 = 'y_is_content_0|k_is_stable_0|i_is_default_0|c_is_user_0|l_is_'+roscms_standard_language+'_0|r_is_'+translang+'|o_asc_name';
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
						filtstring2 = 'k_is_draft_0|u_is_'+roscms_intern_login_check_username+'_0|c_is_type_0|o_desc_datetime';
						load_frametable('draft');
						break;
					case '11':
						filtstring2 = 'u_is_'+roscms_intern_login_check_username+'_0|c_is_type_0|o_desc_datetime';
						load_frametable('my');
						break;
					case '12':
						filtstring2 = 'k_is_archive_0|c_is_version_0|c_is_type_0|l_is_'+getlang()+'_0|o_asc_name|o_desc_ver';
						roscms_archive = 1; /* activate archive mode*/
						load_frametable('archive');
						break;
				}
				
			}
      
      
											function setlang(favlang) {
												var tmp_regstr;
											
												var transcheck = filtstring2.search(/r_is_/);
												
												if (transcheck != -1) { // translation view
													tmp_regstr = new RegExp('r_is_'+userlang, "g");
													filtstring2 = filtstring2.replace(tmp_regstr, 'r_is_'+favlang);
													
													tmp_regstr = new RegExp('r_is_'+roscms_standard_language_trans, "g");
													filtstring2 = filtstring2.replace(tmp_regstr, 'r_is_'+favlang);
												}
												else {
													tmp_regstr = new RegExp('l_is_'+userlang, "g");
													
													filtstring2 = filtstring2.replace(tmp_regstr, 'l_is_'+favlang);
												}
												
												userlang = favlang;
												filtpopulate(filtstring2);
												load_frametable_cp3(roscms_current_tbl_position);
											}
											
											function getlang() {
												return userlang;
											}
										
									function bchangestar(did, drid, dtn, dtv, dusr, objid) {
										
										if (did != '' && drid != '') {
											if (document.getElementById(objid).src == roscms_intern_webserver_roscms+'images/star_on_small.gif') {
												document.getElementById(objid).src = roscms_intern_webserver_roscms+'images/star_off_small.gif';
												bchangetag(did, drid, dtn, 'off', dusr, document.getElementById(objid).className, objid, '3');
											}
											else {
												document.getElementById(objid).src = roscms_intern_webserver_roscms+'images/star_on_small.gif';
												bchangetag(did, drid, dtn, 'on', dusr, document.getElementById(objid).className, objid, '3');
											}
										}
									}
									
									function createentry(menumode) {
										if (menumode == '0') {
											if (document.getElementById('txtaddentryname').value != "") {			
												makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry2&d_name='+encodeURIComponent(document.getElementById('txtaddentryname').value)+'&d_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&d_r_lang='+encodeURIComponent(document.getElementById('txtaddentrylang').value), 'mef', 'addnew2', 'html', 'GET', '');
											}
											else {
												alertbox('Entry name is requiered');
											}
										}
										else if (menumode == '1') {
											makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry4&d_name='+encodeURIComponent(document.getElementById('txtadddynsource').value)+'&d_type=content&d_r_lang='+roscms_standard_language, 'mef', 'addnew2', 'html', 'GET', '');
										}
										else if (menumode == '2') {
											if (document.getElementById('txtaddentryname3').value != "") {										
												makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry3&d_name='+encodeURIComponent(document.getElementById('txtaddentryname3').value)+'&d_type=content&d_r_lang='+roscms_standard_language+'&d_template='+encodeURIComponent(document.getElementById('txtaddtemplate').value), 'mef', 'addnew2', 'html', 'GET', '');
											}
											else {
												alertbox('Entry name is requiered');
											}
										}
									}
									
									function bpreview() {
										var tentrs = selectedEntries().split("|");
										var tentrs2 = tentrs[1].split("_");
										
										if (tentrs[0] == 1) {
											secwind = window.open(roscms_intern_page_link+"data_out&d_f=page&d_u=show&d_val="+tentrs2[1]+"&d_val2="+userlang+"&d_val3=", "RosCMSPagePreview");
										}
										else {
											alertbox("Select one entry to preview a page!");
										}
										
										document.getElementById('extraopt').value = 'sel';
									}

	
	function page_table_populate(http_request, objid) {
		var lstData = "";
		var temp_counter_loop = 0;
		
		var xmldoc = http_request.responseXML;
		var root_node = xmldoc.getElementsByTagName('root').item(0);
		
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
			var xrow = xmldoc.getElementsByTagName("row");
			var xview = xmldoc.getElementsByTagName("view");
			
			var mtblnavstr = '';

			roscms_current_tbl_position = xview[0].getAttributeNode("curpos").value;

			if (xview[0].getAttributeNode("curpos").value >= roscms_intern_entry_per_page*2) {
				mtblnavstr += '<span class="l" onclick="load_frametable_cp(0)"><b>&laquo;</b></span>&nbsp;&nbsp;';
			}
			
			if (xview[0].getAttributeNode("curpos").value > 0) {
				mtblnavstr += '<span class="l" onclick="load_frametable_cp(';
				if (xview[0].getAttributeNode("curpos").value-roscms_intern_entry_per_page*1 >= 0) {
					mtblnavstr += xview[0].getAttributeNode("curpos").value-roscms_intern_entry_per_page*1;
				}
				else {
					mtblnavstr += '0';
				}
				mtblnavstr += ')"><b>&lsaquo; Previous</b></span>&nbsp;&nbsp;';
			}
			
			var mtblnavfrom = xview[0].getAttributeNode("curpos").value*1+1;
			var mtblnavto = (xview[0].getAttributeNode("curpos").value*1) + (xview[0].getAttributeNode("pagelimit").value*1);

			
			mtblnavstr += '<b>'+mtblnavfrom+'</b> - <b>';
			
			if (mtblnavto > xview[0].getAttributeNode("pagemax").value) {
				mtblnavstr += xview[0].getAttributeNode("pagemax").value;
			}
			else {
				mtblnavstr += mtblnavto;
			}
			
			mtblnavstr += '</b> of <b>'+xview[0].getAttributeNode("pagemax").value+'</b>';
			
			if (xview[0].getAttributeNode("curpos").value < xview[0].getAttributeNode("pagemax").value-roscms_intern_entry_per_page*1) {
				mtblnavstr += '&nbsp;&nbsp;<span class="l" onclick="load_frametable_cp(';
				mtblnavstr += xview[0].getAttributeNode("curpos").value*1+roscms_intern_entry_per_page*1;
				mtblnavstr += ')"><b>Next &rsaquo;</b></span>';
			}
			
			if (xview[0].getAttributeNode("curpos").value < (xview[0].getAttributeNode("pagemax").value*1-roscms_intern_entry_per_page*2)) {
				mtblnavstr += '&nbsp;&nbsp;<span class="l" onclick="load_frametable_cp(';
				mtblnavstr += xview[0].getAttributeNode("pagemax").value*1-roscms_intern_entry_per_page*1;
				mtblnavstr += ')"><b>&raquo;</b></span>';
			}
			mtblnavstr += '&nbsp;&nbsp;';
		
			document.getElementById('mtblnav').innerHTML = mtblnavstr;
			document.getElementById('mtbl2nav').innerHTML = mtblnavstr;
			
			
			
			lstData += page_table_header(xview[0].getAttributeNode("tblcols").value);
			
			for (var i=0; i < xrow.length; i++) {

				lstData += page_table_body(
												i,
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
				
				temp_counter_loop = i;		
			}
			lstData += '</tbody></table>';
			
			if (xrow.length < 15) {
				lstData += '<div class="tableswhitespace"><br />'+page_table_space(10-xrow.length)+'</div>';
			}
			
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
  
  
	function unloadMessage(){
		if (exitmsg != '') {
			return exitmsg;
		}
	}
  
  
									function tblcmdbar(opt) {
										cmdbarstr = '';
										
										cmdhtml_space = '&nbsp;';
										cmdhtml_diff = '<button type="button" id="cmddiff" onclick="diffentries()">Compare</button>'+cmdhtml_space;
										cmdhtml_preview = '<button type="button" id="cmdpreview" onclick="bpreview()">Preview</button>'+cmdhtml_space;
										cmdhtml_ready = '<button type="button" id="cmdready" onclick="bchangetags(\'mn\')">Ready</button>'+cmdhtml_space;
										
											if (roscms_access_level >= 2) {
                        cmdhtml_stable = '<button type="button" id="cmdstable" onclick="bchangetags(\'ms\')">Stable</button>'+cmdhtml_space;
											} else {
                        cmdhtml_stable = '';
											}
                      
										cmdhtml_refresh = '<button type="button" id="cmdrefresh" onclick="load_frametable_cp2(roscms_current_tbl_position)">Refresh</button>'+cmdhtml_space;

										cmdhtml_select1 = '<select name="extraopt" id="extraopt" style="vertical-align: top; width: 22ex;" onchange="bchangetags(this.value)">';
										cmdhtml_select1 += '<option value="sel" style="color: rgb(119, 119, 119);">More actions...</option>';
										cmdhtml_select_as = '<option value="as">&nbsp;&nbsp;&nbsp;Add star</option>';
										cmdhtml_select_xs = '<option value="xs">&nbsp;&nbsp;&nbsp;Remove star</option>';
										cmdhtml_select_no = '<option value="no" style="color: rgb(119, 119, 119);">&nbsp;&nbsp;&nbsp;-----</option>';
										cmdhtml_select_mn = '<option value="mn">&nbsp;&nbsp;&nbsp;Mark as new</option>';

											if (roscms_access_level >= 2) {
                        cmdhtml_select_ms = '<option value="ms">&nbsp;&nbsp;&nbsp;Mark as stable</option>';
                        cmdhtml_select_ge = '<option value="va">&nbsp;&nbsp;&nbsp;Generate page</option>';
											} else {
                        cmdhtml_select_ms = '';
											}

											if (roscms_access_level == 3) {
                        cmdhtml_select_va = '<option value="va">&nbsp;&nbsp;&nbsp;Move to archive</option>';
                        cmdhtml_select_xe = '<option value="xe">&nbsp;&nbsp;&nbsp;Delete</option>';
											} else {
                        cmdhtml_select_va = '';
                        cmdhtml_select_xe = '';
											}
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
									
										
										function filtadd() {
											filtscanfilters();
											
												if (roscms_access_level > 1) { 
													tmp_security_new_filter = "k_is_stable";
												}
												else {
													tmp_security_new_filter = "a_is_";
												}
											
											if (filtstring2 == '') {
												filtpopulate(tmp_security_new_filter);
											}
											else {
												filtpopulate(filtstring2+'|'+tmp_security_new_filter);
											}
										}