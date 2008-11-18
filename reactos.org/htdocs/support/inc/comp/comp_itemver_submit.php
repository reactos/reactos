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


	$query_page_group = mysql_query("SELECT * 
								FROM `rsdb_groups` 
								WHERE `grpentr_visible` = '1'
								AND `grpentr_id` = '" . mysql_escape_string($RSDB_SET_group) . "'
								ORDER BY `grpentr_id` DESC LIMIT 1") ;
	
	$result_page_group = mysql_fetch_array($query_page_group);		

?>

<h1>Submit new &quot;<?php echo htmlentities($result_page_group['grpentr_name']); ?>&quot; version</h1> 
<h2>Submit new &quot;<?php echo htmlentities($result_page_group['grpentr_name']); ?>&quot; version</h2>

<?php 
if ($RSDB_intern_user_id <= 0) {
	please_register(); 
}
else {
?>
<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">If you want to <b>submit a new application</b> to the compatibility database, use the <a href="<?php echo $RSDB_intern_link_db_sec; ?>submit"><b>Submit Application Wizard - Step 1</b></a> instead. </font></p>
<fieldset><legend>&nbsp;<b><font color="#000000">Submit an Application in 3 Steps</font></b>&nbsp;</legend>
	<table width="100%" border="0" cellpadding="1" cellspacing="5">
      <tr>
        <td width="33%"><h4><font color="#999999">Step 1</font></h4></td>
        <td width="34%"><h4>Step 2</h4></td>
        <td width="33%"><h4><font color="#999999">Step 3</font></h4></td>
      </tr>
      <tr>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>general information</b>:</font></p>
          <ul>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">application name</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">short  decription</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">category</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">vendor </font></li>
            </ul></td>
        <td valign="top"><p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>version information</b></font></p>
            <ul>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">application version</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">ReactOS</font> <font size="1" face="Verdana, Arial, Helvetica, sans-serif">version</font></li>
            </ul></td>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>test results &amp; screenshots</b></font></p>
            <ul>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><font color="#999999">What works</font></font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">What does not work</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Describe what you have tested and what not</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Application function</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Installation routine</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Conclusion</font></li>
            </ul></td>
      </tr>
    </table>
</fieldset>
<br />
<?php

   function is_num($var) {
       for ($i=0;$i<strlen($var);$i++) {
           $ascii_code=ord($var[$i]);
           if (intval($ascii_code) >=48 && intval($ascii_code) <=57) {
               continue;
           } else {          
               return false;
           }
       } 
       return true;
   } 

	$RSDB_TEMP_subok = "";
	$RSDB_TEMP_postgroupid = "";
	$RSDB_TEMP_postgroupname = "";
	if (array_key_exists("subok", $_POST)) $RSDB_TEMP_subok=htmlspecialchars($_POST["subok"]);
	if (array_key_exists("postgroupid", $_POST)) $RSDB_TEMP_postgroupid=htmlspecialchars($_POST["postgroupid"]);
	if (array_key_exists("postgroupname", $_POST)) $RSDB_TEMP_postgroupname=htmlspecialchars($_POST["postgroupname"]);


	$RSDB_TEMP_txtverint = "";
	$RSDB_TEMP_txtappver = "";
	$RSDB_TEMP_cboversion = "";
	if (array_key_exists("txtappver", $_POST)) $RSDB_TEMP_txtappver=htmlspecialchars($_POST["txtappver"]);
	if (array_key_exists("txtverint", $_POST)) $RSDB_TEMP_txtverint=htmlspecialchars($_POST["txtverint"]);
	if (array_key_exists("version", $_POST)) $RSDB_TEMP_cboversion=htmlspecialchars($_POST["version"]);



	$RSDB_TEMP_sApp = "";
	$RSDB_TEMP_sOSver = "";
	if (array_key_exists("sApp", $_GET)) $RSDB_TEMP_sApp=htmlspecialchars($_GET["sApp"]);
	if (array_key_exists("sOSver", $_GET)) $RSDB_TEMP_sOSver=htmlspecialchars($_GET["sOSver"]);
	if ($RSDB_TEMP_sApp != "" && $RSDB_TEMP_sOSver != "") {
		if (is_num($RSDB_TEMP_sOSver)) {
			$query_app_entry=mysql_query("SELECT * 
												FROM `rsdb_item_comp` 
												WHERE `comp_appversion` = '".mysql_real_escape_string($RSDB_TEMP_sApp)."'
												AND `comp_visible` = '1'
												AND `comp_groupid` = '".mysql_real_escape_string($RSDB_SET_group)."'
												LIMIT 1 ;");	
			$result_app_entry = mysql_fetch_array($query_app_entry);
			
			$query_app_entry_checking=mysql_query("SELECT COUNT('comp_id')
												FROM `rsdb_item_comp` 
												WHERE `comp_appversion` = '".mysql_real_escape_string($RSDB_TEMP_sApp)."'
												AND `comp_visible` = '1'
												AND `comp_groupid` = '".mysql_real_escape_string($RSDB_SET_group)."'
												AND `comp_osversion` = '".mysql_real_escape_string($RSDB_TEMP_sOSver)."' ;");	
			$result_app_entry_checking = mysql_fetch_array($query_app_entry_checking);
			
			if ($result_app_entry_checking[0] == 0) {
				$RSDB_TEMP_txtappver = substr($result_app_entry['comp_name'], strlen($result_page_group['grpentr_name'])+1 );
				$RSDB_TEMP_txtverint = $result_app_entry['comp_appversion'];
				$RSDB_TEMP_cboversion = $RSDB_TEMP_sOSver;
				$RSDB_TEMP_postgroupname = $result_page_group['grpentr_name'];
				$RSDB_TEMP_subok = "okay";
				$RSDB_TEMP_postgroupid = $RSDB_SET_group;
			}
			else {
				msg_bar("This combination of application version and ReactOS version is already available.");
				add_log_entry("low", "comp_item_submit", "submit", "Double Entry Prevention! (light)", "Double Entry Prevention! (light) \n\nThis combination of application version and ReactOS version is already available.", $RSDB_intern_user_id);
			}
		}
	}
	
	
	
	$RSDB_TEMP_SUBMIT_valid = false;
	if ($RSDB_TEMP_subok == "okay") {
		$RSDB_TEMP_SUBMIT_valid = true;
		if ($RSDB_TEMP_cboversion == "" || $RSDB_TEMP_cboversion == "0") {
			msg_bar("Invalid Version!");
			echo "<br />";
			$RSDB_TEMP_SUBMIT_valid = false;
		}
		if (strlen($RSDB_TEMP_txtappver) < 1) {
			msg_bar("The 'Version' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
	}

	if ($RSDB_TEMP_subok != "okay" || $RSDB_TEMP_SUBMIT_valid != true || $RSDB_TEMP_postgroupid != $RSDB_SET_group) {
	
			$query_page_vendor = mysql_query("SELECT * 
										FROM `rsdb_item_vendor` 
										WHERE `vendor_id` = '" . mysql_escape_string($result_page_group['grpentr_vendor']) . "'
										ORDER BY `vendor_id` DESC LIMIT 1") ;
			
			$result_page_vendor = mysql_fetch_array($query_page_vendor);		
?>
<noscript>
	Sorry, currently the submit function is only usable with ECMAScript enabled! The noscript methode will be available soon!
</noscript>

<form name="RSDB_comp_submitapp" method="post" action="<?php echo $RSDB_intern_link_submit_appver; ?>">





<table width="100%" border="0">
    <tr>
      <td width="5%">&nbsp;</td>
      <td width="90%"><table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag1" style="display:table ">
          <tr>
            <td> 
              <table width="100%" border="0">
                <tr>
                  <td width="54"><img src="media/icons/info/submitapp.png" width="56" height="56"></td>
                  <td width="20" height="56">&nbsp;</td>
                  <td><h3> Welcome to the Submit new &quot;<?php echo htmlentities($result_page_group['grpentr_name']); ?>&quot; version Wizard - Step2 </h3></td>
                </tr>
              </table>
              
              <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">This Submit new &quot;<?php echo htmlentities($result_page_group['grpentr_name']); ?>&quot; version Wizard<sup>&dagger;</sup> wizard will guide you through the submission process. </font><font size="2" face="Verdana, Arial, Helvetica, sans-serif">The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds<sup>&Dagger;</sup>. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</font></p>              

	<?php
		$query_date_entry_records=mysql_query("SELECT COUNT('comp_id')
											FROM `rsdb_item_comp`
											WHERE `comp_visible` = '1' 
											AND `comp_groupid` = '". mysql_escape_string($RSDB_SET_group) ."' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		if ($result_date_entry_records[0] != 0) {
	?>
	<p>&nbsp;</p>
	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
	Please <strong>click on  a link </strong> in the <strong>list</strong>, then you will be able to <strong>submit a compatibility test report</strong>:</font></p>
	<table border="0" cellpadding="1" cellspacing="1">
      <tr bgcolor="#5984C3">
        <td width="100" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>ReactOS:</strong></font></div></td>
	<?php
		$query_appnames=mysql_query("SELECT * 
										FROM `rsdb_object_osversions` 
										WHERE `ver_visible` = '1'
										ORDER BY `ver_value` DESC
										LIMIT 0, 6  ; ;");	
		while($result_appnames = mysql_fetch_array($query_appnames)) {
	?>
	    <td width="100" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong><?php echo $result_appnames['ver_name']; ?></strong></font></div></td>
	<?php
		}
	?>
	  </tr>
      <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

	/*$temp_result=0;
	$query_appsum=mysql_query("SELECT DISTINCT (
											`comp_appversion` 
											)
										FROM `rsdb_item_comp` 
										WHERE `comp_visible` = '1' 
										AND `comp_groupid` = '". mysql_escape_string($RSDB_SET_group) ."'
										GROUP BY `comp_appversion` 
										ORDER BY `comp_appversion` ASC ;");	
	while($result_appsum = mysql_fetch_array($query_appsum)) {
		$temp_result++;
	}
	echo $temp_result;*/
 
	$query_appsum=mysql_query("SELECT DISTINCT (
											`comp_appversion` 
											), `comp_name`, `comp_osversion`
										FROM `rsdb_item_comp` 
										WHERE `comp_visible` = '1' 
										AND `comp_groupid` = '". mysql_escape_string($RSDB_SET_group) ."'
										GROUP BY `comp_appversion` 
										ORDER BY `comp_appversion` ASC ;");	
	while($result_appsum = mysql_fetch_array($query_appsum)) {
?>
      <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$color = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$color = $cellcolor2;
									}
								 ?>">
        <td><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<?php
		echo "<b>".$result_appsum['comp_name']."</b>";

    ?>
			</font></div>
		</td>
		<?php
			$query_osvers=mysql_query("SELECT * 
										FROM `rsdb_object_osversions` 
										WHERE `ver_visible` = '1'
										ORDER BY `ver_value` DESC
										LIMIT 0, 6  ;");	
			while($result_osvers = mysql_fetch_array($query_osvers)) {
					$query_appvers=mysql_query("SELECT * 
													FROM `rsdb_item_comp` 
													WHERE `comp_visible` = '1'
													AND `comp_groupid` = '". mysql_escape_string($RSDB_SET_group) ."'
													AND `comp_osversion` = '". mysql_escape_string($result_osvers['ver_value']) ."'
													AND `comp_appversion` = '". mysql_escape_string($result_appsum['comp_appversion']) ."'
													GROUP BY `comp_appversion` 
													ORDER BY `comp_appversion` ASC ;");	
					$result_appvers = mysql_fetch_array($query_appvers);
				if ($result_osvers['ver_value'] == $result_appvers['comp_osversion']) {
		?>
        	<td>
				<div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
				<?php


					$query_apptests=mysql_query("SELECT COUNT('comp_id')
														FROM `rsdb_item_comp_testresults` 
														WHERE `test_comp_id` = '". mysql_escape_string($result_appvers['comp_id']) ."'
														AND `test_visible` = '1' ;");	
					$result_apptests = mysql_fetch_array($query_apptests);
					if ($result_apptests[0] == 1) {
						echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_appvers['comp_id']."&amp;item2=tests&amp;addbox=add\">1 report</a></b>";
					}
					else {
						echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_appvers['comp_id']."&amp;item2=tests&amp;addbox=add\">".$result_apptests[0]." reports</a></b>";
					}
					//echo $result_appvers['comp_appversion']."-".$result_appvers['comp_osversion'];
				}
				else {
					echo '<td bgcolor="#FFFFFF"><center><i><a href="javascript:AddVerEntry(\''.$result_appsum['comp_name'].'\', \''.$result_appsum['comp_appversion'].'\', \''.$result_osvers['ver_name'].'\', \''.$result_osvers['ver_value'].'\')">add</a></i></center></td>';
				}
				?>
				</font></div>
			</td>
		<?php 
			}
		?>
	  </tr>
	<?php 
		}
	?>
    </table>
    <br>
	<font size="1"><i>	Click on an &quot;add&quot; link to add that entry.</i></font>
    <p>&nbsp;</p>
	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">If the Application or ReactOS version  is <strong>not in the list</strong> above then please click on the Next button.</font></p>	
	<?php 
		}
		else {
	?>
    <p>&nbsp;</p>
	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">To continue, click Next.</font></p>	
	<?php 
		}
	?>
              <p>&nbsp;</p>
              <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><sup>&dagger;</sup></font><strong><font size="1" face="Verdana, Arial, Helvetica, sans-serif"> </font></strong><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><em>&quot;application&quot; means in this context &quot;application or driver&quot;. </em></font><br />
                <font size="2" face="Verdana, Arial, Helvetica, sans-serif"><sup>&Dagger;</sup></font> <font size="1" face="Verdana, Arial, Helvetica, sans-serif"><i>development builds are marked with: SVN, RC1, RC2, etc. </i></font></p>
			  <hr size="1" noshade color="#CCCCCC" id="sdsd">
			  <table width="100%"  border="0">
                <tr>
                  <td width="50%" align="left"><button name="helpwizp" type="button" value="Help" onclick="WizHelp()">&nbsp;Help&nbsp;</button></td>
                  <td width="50%" align="right"><button name="nextwizp2" id="nextwizp2" type="button" value="Next Page" onclick="WizPag2()">&nbsp;Next&nbsp;&nbsp;&gt;&nbsp;</button></td>
                </tr>
              </table>			  </td>
          </tr>
        </table>
		
		
		<table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag2" style="display:<?php 

	if (htmlentities($RSDB_TEMP_cboversion) == "") {
		echo "none";
	}
	else {
		echo "table";
	}
		
?> ">
          <tr>
            <td><table width="100%" height="106" border="0">
              <tr>
                <td align="left" valign="bottom"><p><font size="4" face="Verdana, Arial, Helvetica, sans-serif"><strong>Submit new &quot;<?php echo htmlentities($result_page_group['grpentr_name']); ?>&quot; version Wizard</strong></font><br />
                    <font size="3" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Input the versions data </font></p></td>
                <td width="10%" height="56" valign="top"><div align="right"><img src="media/icons/info/submitapp.png" width="56" height="56"></div></td>
              </tr>
              <tr>
                <td height="21" colspan="2" align="left" valign="top"><hr size="1" noshade color="#CCCCCC" id="sdsd">
                    <p align="right"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">2/3</font></p></td>
              </tr>
              <tr>
                <td height="21" colspan="2" align="left" valign="top">

				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><strong>Application name:</strong><br />
				  <?php echo htmlentities($result_page_group['grpentr_name']); ?>
				  <input name="postgroupid" type="hidden" value="<?php echo $result_page_group['grpentr_id']; ?>">
                  <input name="postgroupname" type="hidden" value="<?php echo $result_page_group['grpentr_name']; ?>">
</font></p>
				<p><table width="300" border="0" align="right">
                  <tr>
                    <td><font size="1"><fieldset><legend>&nbsp;<b>examples:</b>&nbsp;</legend>
					&quot;1.5&quot; instead of &quot;Firefox 1.5.0.2 German&quot;<br />
					&quot;2003&quot; instead of &quot;MS Office 2003 SP1&quot;<br />
					&quot;2004&quot; instead of &quot;Unreal Tournament 2004&quot;<br />
					&quot;1&quot; instead of &quot;Deus Ex&quot; (version 1)</fieldset></font> </td>
                  </tr>
                </table><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><strong>Application version:</strong><br />
				    <input name="txtappver" type="text" id="txtappver" onkeyup="checkSubmit()" value="<?php echo htmlentities($RSDB_TEMP_txtappver); ?>" size="20" maxlength="50" />
                    <em><font size="1">(xx.xx or year number)</font></em></font></p>				<p><font size="2"><strong><font face="Verdana, Arial, Helvetica, sans-serif">Sequel number:</font></strong><font face="Verdana, Arial, Helvetica, sans-serif"> (optional) <br>
				      <input name="txtverint" type="text" id="txtverint" size="10" maxlength="25" value="<?php echo htmlentities($RSDB_TEMP_txtverint); ?>" onkeyup="checkSubmit()" /> 
				      <em><font size="1">(number)</font></em> </font></font></p>
				<p><font size="2"><strong><font face="Verdana, Arial, Helvetica, sans-serif">ReactOS Version:</font></strong>
                    <font face="Verdana, Arial, Helvetica, sans-serif"><br>
                    <select name="version" id="version" onchange="checkSubmit()">
                        <option value="0" <?php 
					if  ($RSDB_TEMP_cboversion == "") {
						echo "selected";
					}
				?>>Please select a version</option>
                        <?php
					$query_osvers=mysql_query("SELECT * 
												FROM `rsdb_object_osversions` 
												WHERE `ver_visible` = '1'
												ORDER BY `ver_value` DESC  ;");	
					while($result_osvers = mysql_fetch_array($query_osvers)) {
						echo '<option value="'. $result_osvers['ver_value'] .'"';
						if  (htmlentities($RSDB_TEMP_cboversion) == $result_osvers['ver_value']) {
							echo ' selected';
						}
						echo '>'. $result_osvers['ver_name'] .'</option>';
					}
				?>
                    </select>
</font></font></p>
				<p>&nbsp;</p>
				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</font></p>
				<p>&nbsp;</p>
				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">By clicking &quot;Submit&quot; below you agree to be bound by the <a href="<?php echo $RSDB_intern_index_php; ?>?page=conditions" target="_blank">submit conditions</a>.</font></p>
				<hr size="1" noshade color="#CCCCCC" id="sdsd">
				<div align="right">
				  <table width="100%"  border="0" cellpadding="0" cellspacing="0">
                    <tr>
                      <td width="50%" align="left"><button name="backwizp1" type="button" value="Back to Page 1" onclick="WizPag1()">&nbsp;&lt;&nbsp;&nbsp;Back&nbsp;</button>						</td>
					<td width="50%" align="right">	
					<input name="subok" type="hidden" value="okay">
						<input type="submit" name="Submit" id="Submit" value="&nbsp;Submit&nbsp;&nbsp;&gt;&nbsp;" disabled>
					</td></tr>
                  </table>		
				</div></td>
              </tr>
            </table>
		
          </tr>
        </table>
      </td>
      <td width="5%">&nbsp;</td>
    </tr>
  </table>
</form>
<script language="JavaScript1.2">
<!--

	var brow = navigator.appName;
	
	function WizPag1() {
		if (brow=="Microsoft Internet Explorer") { // work-around for IE CSS bug
			document.getElementById('pag1').style.display = 'block';
		}
		else {
			document.getElementById('pag1').style.display = 'table';
		}
		document.getElementById('pag2').style.display = 'none';
	}
	
	function WizPag2() {
		document.getElementById('pag1').style.display = 'none';
		if (brow=="Microsoft Internet Explorer") { // work-around for IE CSS bug
			document.getElementById('pag2').style.display = 'block';
		}
		else {
			document.getElementById('pag2').style.display = 'table';
		}
	}	

	function WizHelp() {
		alert("This Submit new application version wizard will guide you through the submission process.\n\nPlease read the 'Help & FAQ' page (see menu bar) before you submit anything.\n\nIf you have problems with this wizard or suggestions/ideas, please visit the ReactOS Forum (htttp://www.reactos.org/forum/ -> Website Sub-Forum) and report problems or let us know your suggestions, ideas to improve the website.\n\nThank you for using the ReactOS Compatibility Database!");
	}
		
-->
</script>

<script language="JavaScript1.2">
<!--
	var subStep;
	
	function checkSubmit() {
		document.getElementById('txtappver').style.background = '#FFFFFF';
		document.getElementById('txtverint').style.background = '#FFFFFF';
		document.getElementById('version').style.background = '#FFFFFF';
		checking = false;
		if (document.getElementById("txtappver").value == "") {
			checking = true;
			document.getElementById('txtappver').style.background = '#FF9900';
		}
		if ( !IsNumeric(document.getElementById("txtverint").value) ) {
			checking = true;
			document.getElementById('txtverint').style.background = '#FF9900';
		}
		if (document.getElementById('version').value == "0") {
			checking = true;
			document.getElementById('version').style.background = '#FF9900';
		}
		if (checking == false) {
			document.getElementById('Submit').disabled=false;
		}
		else {
			document.getElementById('Submit').disabled=true;
		}
	}
	
	function IsNumeric(sText) {
	   var ValidChars = "0123456789.";
	   var IsNumber=true;
	   var Char;
	
		for (i = 0; i < sText.length && IsNumber == true; i++) { 
			Char = sText.charAt(i); 
			if (ValidChars.indexOf(Char) == -1) {
				IsNumber = false;
			}
		}
	   return IsNumber;
	}
   
	function checkVer(sText) {
		if (IsNumeric(sText)) {
			document.getElementById('txtverint').style.background = '#FFFFFF';
		}
		else {
			document.getElementById('txtverint').style.background = '#FF9900';
		}
	}

	function checkVer2(sText) {
		if (sText != "") {
			document.getElementById('txtappver').style.background = '#FFFFFF';
		}
	}

	function AddVerEntry(sAppv1, sAppv2, sOSv1, sOSv2) {
		var chk = window.confirm("'"+sAppv1+"' - 'ReactOS "+sOSv1+"'\n\n If you want to submit a compatibility test report then and '"+sAppv1+"' is the application which you have tested in 'ReactOS "+sOSv1+"', then click 'OK'.");
		if (chk == true) {
			parent.location.href = "<?php echo $RSDB_intern_link_submit_appver_javascript; ?>&sApp="+sAppv2+"&sOSver="+sOSv2;
		}
	}


-->
</script>

<?php
	}
	else {
		
		$query_app_entry_checking2=mysql_query("SELECT COUNT('comp_id')
											FROM `rsdb_item_comp` 
											WHERE `comp_name` = '".mysql_real_escape_string($RSDB_TEMP_postgroupname." ".$RSDB_TEMP_txtappver)."'
											AND `comp_groupid` = '".mysql_real_escape_string($RSDB_SET_group)."'
											AND `comp_osversion` = '".mysql_real_escape_string($RSDB_TEMP_cboversion)."' ;");	
		$result_app_entry_checking2 = mysql_fetch_array($query_app_entry_checking2);
		
		if ($RSDB_TEMP_txtverint == "") {
			$RSDB_TEMP_txtverint = "0";
		}

		if ($result_app_entry_checking2[0] == 0 && $RSDB_TEMP_subok == "okay" && $RSDB_TEMP_SUBMIT_valid == true && $RSDB_TEMP_postgroupid == $RSDB_SET_group) {
			$report_submit="INSERT INTO `rsdb_item_comp` ( `comp_id` , `comp_name` , `comp_visible` , `comp_groupid` , `comp_appversion` , `comp_osversion` , `comp_description` , `comp_usrid` , `comp_date` ) 
							VALUES ('', '".mysql_real_escape_string($RSDB_TEMP_postgroupname." ".$RSDB_TEMP_txtappver)."', '1', '".mysql_real_escape_string($RSDB_SET_group)."', '".mysql_real_escape_string($RSDB_TEMP_txtverint)."', '".mysql_real_escape_string($RSDB_TEMP_cboversion)."', '', '".mysql_real_escape_string($RSDB_intern_user_id)."', NOW() );";
			$db_report_submit=mysql_query($report_submit);
			
			$query_page = mysql_query("SELECT * 
										FROM `rsdb_item_comp` 
										WHERE `comp_visible` = '1'
										AND `comp_name` = '" . mysql_real_escape_string($RSDB_TEMP_postgroupname." ".$RSDB_TEMP_txtappver) . "'
										ORDER BY `comp_id` DESC LIMIT 1") ;
			
			$result_page = mysql_fetch_array($query_page);		
			
			// Stats update:
			$update_stats_entry = "UPDATE `rsdb_stats` SET
									`stat_s_icomp` = (stat_s_icomp + 1) 
									WHERE `stat_date` = '". date("Y-m-d") ."' LIMIT 1 ;";
			mysql_query($update_stats_entry);
?>
	<table width="100%" border="0">
		<tr>
		  <td width="5%">&nbsp;</td>
		  <td width="90%">
	
	<table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag3">
	  <tr>
		<td><table width="100%" height="106" border="0">
			<tr>
			  <td align="left" valign="bottom"><p><font size="4" face="Verdana, Arial, Helvetica, sans-serif"><strong>Submit new application version Wizard</strong></font><br />
					  <font size="3" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Further information </font></p></td>
			  <td width="10%" height="56" valign="top"><div align="right"><img src="media/icons/info/submitapp.png" width="56" height="56"></div></td>
			</tr>
			<tr>
			  <td height="21" colspan="2" align="left" valign="top"><hr size="1" noshade color="#CCCCCC" id="sdsd">
				  <p align="right"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">3/3</font></p></td>
			</tr>
			<tr>
			  <td height="21" colspan="2" align="left" valign="top">	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><?php echo $RSDB_TEMP_postgroupname." ".$RSDB_TEMP_txtappver; ?> has been saved to the database!</font></p>
		<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Thank you for your help! This was part two of three, the next step will be to submit compatibility test report. </font></p>
		<p>&nbsp;</p>
		<ul>
		  <li><a href="<?php echo $RSDB_intern_link_item.$result_page['comp_id']; ?>&amp;item2=tests&amp;addbox=add">Please <strong>submit</strong> a <strong>compatibility report</strong> to the &quot;<b><?php echo $result_page['comp_name']; ?></b>&quot; entry!</a></li>
		</ul>
				<p>&nbsp;</p>
				<p align="center"><img src="images/progress-end.gif" style="width:1px; height:20px;" /><img src="images/progress-todo.gif" name="p1" id="p1" style="width:20px; height:20px;" /><img src="images/progress-todo.gif" name="p2"id="p2" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p3"id="p3" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p4"id="p4" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p5"id="p5" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p6"id="p6" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p7"id="p7" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p8"id="p8" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p9"id="p9" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p10"id="p10" style="width:20; height:20px;" /><img src="images/progress-end.gif"
				   style="width:1px; height:20px;" /></p>
				  <p align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">You are being redirected in <b><span id="counter">10</span></b> seconds. </font></p>             
			<p>&nbsp;</p>
		 <hr size="1" noshade color="#CCCCCC" id="sdsd">
				   <div align="right">
					<button name="nextwiz" id="nextwiz" type="button" value="NextWizard" onclick="NextBigStep()"  >&nbsp;Next&nbsp;&nbsp;&gt;&nbsp;</button>
				</div></td>
			</tr>
		  </table>
	  </tr>
	</table></td>
		  <td width="5%">&nbsp;</td>
		</tr>
	</table>
	<script language="JavaScript">
	<!--
	var start=new Date();
		start=Date.parse(start)/1000;
		var counts=10;
		var count2="0";
		
		function CountDown(){
			var now=new Date();
			now=Date.parse(now)/1000;
			var x=parseInt(counts-(now-start),10);
			document.getElementById('counter').innerHTML = x;
			switch(x) {
				case 9:
					count2="1";
					break;
				case 8:
					count2="2";
					break;
				case 7:
					count2="3";
					break;
				case 6:
					count2="4";
					break;
				case 5:
					count2="5";
					break;
				case 4:
					count2="6";
					break;
				case 3:
					count2="7";
					break;
				case 2:
					count2="8";
					break;
				case 1:
					count2="9";
					break;
				case 0:
					count2="10";
					break;
			}
			if (count2 != "0") {
				document.getElementById("p"+count2).src = "images/progress-done.gif";
			}
			if(x>0){
				timerID=setTimeout("CountDown()", 100);
			}else{
				location.href="<?php echo $RSDB_intern_link_item_javascript.$result_page['comp_id']; ?>&item2=tests&addbox=add";
			}
		}
	
		function NextBigStep(){
			location.href="<?php echo $RSDB_intern_link_item_javascript.$result_page['comp_id']; ?>&item2=tests&addbox=add";
		}
		
		window.setTimeout('CountDown()',100);
	-->
	</script>
<?php
		}
		else {
			echo "<p>";
			msg_bar("An error occur while checking the data. The data is invalid! Please submit the data again, thx.");
			echo "<br />";
			msg_bar("If this message appear more then one times and you are sure everything is valid and okay, then please report this to the ReactOS forum.");
			echo "</p>";
			if ($result_app_entry_checking2[0] != 0) {
				msg_bar("Double Entry Prevention: please check the data!");
				add_log_entry("low", "comp_item_submit", "submit", "Double Entry Prevention!", "Double Entry Prevention! \n\nAn error occur while checking the data. The data is invalid! Please submit the data again, thx. \n\nIf this message appear more then one times and you are sure everything is valid and okay, then please report this to the ReactOS forum.", $RSDB_intern_user_id);
			}
			echo '<p><a href="javascript:history.go(-1)">Click here to go back!</a></p>';
		}
	}
}
?>
