<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
	
	require("inc/data_menu.php");
	
	$RosCMS_GET_branch = "website";

?>
<style type="text/css">
<!--
	.frmeditbutton {
		cursor:pointer;
		color:#006090;
	}
-->
</style>

	<p>&nbsp;</p>
	<h2>User</h2>
	<p><b>User Account Management Interface</b></p>
	<p>&nbsp;</p>
	
	<?php
	
		if (roscms_security_grp_member("ros_admin") || roscms_security_grp_member("ros_sadmin")) {
			echo "<h3>Administrator</h3>";
			$tmp_sql_lang = "";
		}
		else if (roscms_security_grp_member("transmaint")) {
			echo "<h3>Language Maintainer</h3>";
			$tmp_sql_lang = " AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'";
		}
		else {
			$tmp_sql_lang = "";
		}

		if (roscms_security_grp_member("transmaint") ||roscms_security_grp_member("ros_admin") || roscms_security_grp_member("ros_sadmin")) {
?>
<div><label for="textfield">Username: </label><input type="text" name="textfield" id="textfield" onkeyup="getuser()" /> <br />
  <input name="searchopt" type="radio" id="searchopt1" value="accountname" checked="checked" onclick="getuser()" />
  <label>account name</label>
  <input name="searchopt" type="radio" id="searchopt2" value="fullname" onclick="getuser()" />
  <label>full name </label>
  <input name="searchopt" type="radio" id="searchopt3" value="email" onclick="getuser()" />
  <label>email address</label>
  <input name="searchopt" type="radio" id="searchopt4" value="website" onclick="getuser()" />
  <label>website</label>
  <input name="searchopt" type="radio" id="searchopt5" value="language" onclick="getuser()" />
  <label>language</label>
 <img id="ajaxloading" style="display:none;" src="images/ajax_loading.gif" width="13" height="13" /><br /><br />
</div>
<div id="userarea"></div>
			
<br />
<br />
			
<?php		
			echo "<h4>Translators</h4>";

			echo "<ul>";
			
			$query_user_history = mysql_query("SELECT d.data_id, u.user_id, u.user_name, u.user_fullname, u.user_language, COUNT(r.data_id) as 'editcounter'
												FROM data_a d, data_revision r, users u  
												WHERE r.data_id = d.data_id 
												AND r.rev_usrid = u.user_id 
												AND rev_version  > 0
												". $tmp_sql_lang ."
												GROUP BY u.user_name
												ORDER BY editcounter DESC, u.user_name;");
			while ($result_user_history = mysql_fetch_array($query_user_history)) {
				echo "<li>".$result_user_history['user_name']." (".$result_user_history['user_fullname']."; ".$result_user_history['user_language'].") ".$result_user_history['editcounter']." stable edits</li>";
			}

			echo "</ul>";

			echo "<br />";
		
/*
			if (roscms_security_grp_member("transmaint")) {
				echo get_content("log_website_en_2007-30", "system", "en", "content", "text", "archive");
			}
*/
		}
	?>
	<script type="text/javascript" language="javascript">
		<!--
		
			function getuser() {
				var soptckd = '';
				if (document.getElementById('searchopt1').checked) soptckd = 'accountname';
				if (document.getElementById('searchopt2').checked) soptckd = 'fullname';
				if (document.getElementById('searchopt3').checked) soptckd = 'email';
				if (document.getElementById('searchopt4').checked) soptckd = 'website';
				if (document.getElementById('searchopt5').checked) soptckd = 'language';
				makeRequest('?page=data_out&d_f=user&d_u=usrtbl&d_fl=list&d_val='+encodeURIComponent(document.getElementById('textfield').value)+'&d_val2='+encodeURIComponent(soptckd), 'usrtbl', 'userarea');
			}
			
			function getuserdetails(userid) {
				makeRequest('?page=data_out&d_f=user&d_u=usrtbl&d_fl=detail&d_val='+encodeURIComponent(userid), 'usrtbl', 'userarea');
			}


			
			function addmembership(userid, membid) {
				//alert(userid+', '+membid);
				makeRequest('?page=data_out&d_f=user&d_u=usrtbl&d_fl=addmembership&d_val='+encodeURIComponent(userid)+'&d_val2='+encodeURIComponent(membid), 'usrtbl', 'userarea');
			}
			
			function delmembership(userid, membid) {
				//alert(userid+', '+membid);
				uf_check = confirm("Be careful! \n\nDo you want to delete this membership?");
				
				if (uf_check == true) {
					makeRequest('?page=data_out&d_f=user&d_u=usrtbl&d_fl=delmembership&d_val='+encodeURIComponent(userid)+'&d_val2='+encodeURIComponent(membid), 'usrtbl', 'userarea');
				}
			}
			
			function updateusrlang(userid, membid) {
				//alert(userid+', '+membid);
				uf_check = confirm("Do you want to continue?");
				
				if (uf_check == true) {
					makeRequest('?page=data_out&d_f=user&d_u=usrtbl&d_fl=updateusrlang&d_val='+encodeURIComponent(userid)+'&d_val2='+encodeURIComponent(membid), 'usrtbl', 'userarea');
				}
			}
			
		
			function makeRequest(url, action, objid) {
				var http_request = false;
				document.getElementById('ajaxloading').style.display = 'block';
		
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
		
				
				if (http_request.overrideMimeType) {
					http_request.overrideMimeType('text/html');
				}
				http_request.onreadystatechange = function() { alertContents(http_request, action, objid); };
				http_request.open('GET', url, true);
				http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
				http_request.send(null);
			}
			
			function alertContents(http_request, action, objid) {
				try {
					if (http_request.readyState == 4) {
						if (http_request.status == 200) {
							document.getElementById('ajaxloading').style.display = 'none';
							//alert(http_request.responseText);
							document.getElementById('userarea').innerHTML = http_request.responseText;
						}
						else {
							alert('There was a problem with the request ['+http_request.status+' / '+http_request.readyState+']. \n\nA client (browser) or server problem. Please check and try to update your browser. \n\nIf this error happens more than once or twice, contac the website admin.');
						}
					}
				}
				catch( e ) {
					alert('Caught Exception: ' + e.description +'\n\nIf this error occur more than once or twice, please contact the website admin with the exact error message. \n\nIf you use the Safari browser, please make sure you run the latest version.');
				}
				
				// to prevent memory leak
				http_request = null;
			}
			
		-->
	</script>
