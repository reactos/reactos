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
	
	global $roscms_intern_page_link;

	if ($roscms_security_level == 3) {
?>
		<p>&nbsp;</p>
		<h2>Maintain</h2>
		<p><b>RosCMS Maintainer Interface</b></p>
		<p>&nbsp;</p>
<?php

/*	
		require("inc/data_export_page.php");

		echo generate_page_output_update("55", "en", "");
		echo "<hr />";
		echo generate_page_output_update("55", "de", "");

		//echo generate_page_output("sitemap", "all", "", "single");
		
		echo "<hr />";
		
		echo generate_page_output_update("143", "en", "");
		
		echo "<hr />";
		
		echo generate_page_output_update("160", "en", "");
		
		//echo "<hr />";
		
		//echo generate_page_output_update("148", "en", "");

		echo "<hr />";
		
		echo generate_page_output_update("51", "en", "3");
		
		echo "<hr />";
		
		echo generate_page_output_update("177", "en", "");
*/

?>
		<p><a href="javascript:optimizedb()">Optimize Database Tables</a></p>
		<p>&nbsp;</p>
		<p><a href="javascript:ppreview()">Page Preview</a></p>
		<div><label for="textfield">Entry-Name:</label> <input name="textfield" type="text" id="textfield" size="20" maxlength="100" />
		<select id="txtaddentrytype" name="txtaddentrytype">
			<option value="page" selected="selected">Page</option>
			<option value="content">Content</option>
			<option value="template">Template</option>
			<option value="script">Script</option>
			<option value="system">System</option>
		</select>
		<select id="txtaddentrylang" name="txtaddentrylang">
		<?php
			$query_language = mysql_query("SELECT * 
											FROM languages
											WHERE lang_level > '0'
											ORDER BY lang_name ASC ;");
			while($result_language=mysql_fetch_array($query_language)) {
				echo '<option value="';
				echo $result_language['lang_id'];
				echo '">'.$result_language['lang_name'].'</option>';
				
			}
		?>
		</select>						
		<input name="dynnbr" type="text" id="dynnbr" size="3" maxlength="5" />
		<input name="entryupdate" type="button" value="generate" onclick="pupdate()" /></div>

		<p><a href="javascript:genpages()">Generate All Pages</a></p>
		<div id="maintainarea" style="border: 1px dashed red;" style="display:none;"></div>
		<img id="ajaxloading" style="display:none;" src="images/ajax_loading.gif" width="13" height="13" />
		<p>&nbsp;</p>
<?php
	
		if (roscms_security_grp_member("ros_sadmin")) {
		
			echo "<p>&nbsp;</p>";

			echo "<h2>RosCMS Global Log</h2>";
			echo "<h3>High Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewerhigh" cols="75" rows="7" wrap="off">'.get_content("log_website_".date("Y-W"), "system", "en", "high_security_log", "text", "archive").'</textarea><br /><br />';
			echo "<h3>Medium Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewermed" cols="75" rows="5" wrap="off">'.get_content("log_website_".date("Y-W"), "system", "en", "medium_security_log", "text", "archive").'</textarea><br /><br />';
			echo "<h3>Low Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewerlow" cols="75" rows="3" wrap="off">'.get_content("log_website_".date("Y-W"), "system", "en", "low_security_log", "text", "archive").'</textarea><br /><br />';

			echo "<p>&nbsp;</p>";
			echo "<p>&nbsp;</p>";

			echo "<h2>RosCMS Generator Log</h2>";
			echo "<h3>High Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewerhigh2" cols="75" rows="7" wrap="off">'.get_content("log_website_generate_".date("Y-W"), "system", "en", "high_security_log", "text", "archive").'</textarea><br /><br />';
			echo "<h3>Medium Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewermed2" cols="75" rows="5" wrap="off">'.get_content("log_website_generate_".date("Y-W"), "system", "en", "medium_security_log", "text", "archive").'</textarea><br /><br />';
			echo "<h3>Low Security Log - ".date("Y-W")."</h3>";
			echo '<textarea name="logviewerlow2" cols="75" rows="3" wrap="off">'.get_content("log_website_generate_".date("Y-W"), "system", "en", "low_security_log", "text", "archive").'</textarea><br /><br />';

			echo "<p>&nbsp;</p>";
			echo "<p>&nbsp;</p>";

			echo "<h2>RosCMS Language Group Logs</h2>";
			$query_l_lang = mysql_query("SELECT lang_id, lang_name  
									FROM languages  
									ORDER BY lang_name ASC;");
			while ($result_l_lang = mysql_fetch_array($query_l_lang)) {
				echo "<h3>".$result_l_lang['lang_name']."</h3>";
				echo "<h4>High Security Log - ".date("Y-W")."</h4>";
				echo '<textarea name="logviewerhigh'.$result_l_lang['lang_id'].'" cols="75" rows="5" wrap="off">'.get_content("log_website_".$result_l_lang['lang_id']."_".date("Y-W"), "system", "en", "high_security_log", "text", "archive").'</textarea><br /><br />';
				echo "<h4>Medium Security Log - ".date("Y-W")."</h4>";
				echo '<textarea name="logviewermed'.$result_l_lang['lang_id'].'" cols="75" rows="4" wrap="off">'.get_content("log_website_".$result_l_lang['lang_id']."_".date("Y-W"), "system", "en", "medium_security_log", "text", "archive").'</textarea><br /><br />';
				echo "<h4>Low Security Log - ".date("Y-W")."</h4>";
				echo '<textarea name="logviewerlow'.$result_l_lang['lang_id'].'" cols="75" rows="3" wrap="off">'.get_content("log_website_".$result_l_lang['lang_id']."_".date("Y-W"), "system", "en", "low_security_log", "text", "archive").'</textarea><br /><br />';
				echo "<p>&nbsp;</p>";
			}

		}
?>
	<script type="text/javascript" language="javascript">
		<!--
		
			function optimizedb() {
				makeRequest('?page=data_out&d_f=maintain&d_u=optimize', 'optimize', 'maintainarea');
			}
			
			function analyzedb() {
				makeRequest('?page=data_out&d_f=maintain&d_u=analyze', 'analyze', 'maintainarea');
			}

			function genpages() {
				uf_check = confirm("Do you want to continue?");
				
				if (uf_check == true) {
					document.getElementById('maintainarea').style.display = 'block';
					document.getElementById('maintainarea').innerHTML = 'generating all pages, may take several seconds ...';
					makeRequest('?page=data_out&d_f=maintain&d_u=genpages', 'genpages', 'maintainarea');
				}
			}
			
			function ppreview() {
				secwind = window.open("<?php echo $roscms_intern_page_link; ?>data_out&d_f=page&d_u=show&d_val=index&d_val2=en", "RosCMSPagePreview");
			}
			
			function pupdate() {
				document.getElementById('maintainarea').style.display = 'block';
				makeRequest('?page=data_out&d_f=maintain&d_u=pupdate&d_val='+encodeURIComponent(document.getElementById('textfield').value)+'&d_val2='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&d_val3='+encodeURIComponent(document.getElementById('txtaddentrylang').value)+'&d_val4='+encodeURIComponent(document.getElementById('dynnbr').value), 'pupdate', 'maintainarea');
			}

					
			function makeRequest(url, action, objid) {
				var http_request = false;

				document.getElementById('ajaxloading').style.display = 'block';
				document.getElementById(objid).innerHTML = '';

		
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
							document.getElementById(objid).innerHTML = http_request.responseText;
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
<?php
	}
?>
