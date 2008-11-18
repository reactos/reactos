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


	if (usrfunc_IsModerator($RSDB_intern_user_id)) {
	
		$query_maintainer_group = mysql_query("SELECT * 
									FROM `rsdb_groups` 
									WHERE `grpentr_visible` = '1'
									AND `grpentr_id` = '".mysql_real_escape_string($RSDB_SET_group)."'
									" . $RSDB_intern_code_db_rsdb_groups . "
									LIMIT 1 ;") ;
		$result_maintainer_group = mysql_fetch_array($query_maintainer_group);

		$query_maintainer_group_category = mysql_query("SELECT * 
								FROM `rsdb_categories` 
								WHERE `cat_id` = '". mysql_real_escape_string($result_maintainer_group['grpentr_category'])."'
								AND `cat_visible` = '1'
								AND `cat_comp` = '1'
								LIMIT 1 ;") ;
		$result_maintainer_group_category = mysql_fetch_array($query_maintainer_group_category);

		$query_maintainer_group_vendor = mysql_query("SELECT * 
								FROM `rsdb_item_vendor` 
								WHERE `vendor_id` = '". mysql_real_escape_string($result_maintainer_group['grpentr_vendor'])."'
								AND `vendor_visible` = '1'
								LIMIT 1 ;") ;
		$result_maintainer_group_vendor = mysql_fetch_array($query_maintainer_group_vendor);


		$RSDB_referrer="";
		$RSDB_usragent="";
		$RSDB_ipaddr="";
		if (array_key_exists('HTTP_REFERER', $_SERVER)) $RSDB_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
		if (array_key_exists('HTTP_USER_AGENT', $_SERVER)) $RSDB_usragent=htmlspecialchars($_SERVER['HTTP_USER_AGENT']);
		if (array_key_exists('REMOTE_ADDR', $_SERVER)) $RSDB_ipaddr=htmlspecialchars($_SERVER['REMOTE_ADDR']);

		$RSDB_TEMP_pmod = "";
		$RSDB_TEMP_txtreq1 = "";
		$RSDB_TEMP_txtreq2 = "";
		$RSDB_TEMP_txtspam = "";
		$RSDB_TEMP_verified = "";
		$RSDB_TEMP_appgroup = "";
		$RSDB_TEMP_description = "";
		$RSDB_TEMP_category = "";
		$RSDB_TEMP_vendor = "";
		if (array_key_exists("pmod", $_POST)) $RSDB_TEMP_pmod=htmlspecialchars($_POST["pmod"]);
		if (array_key_exists("txtreq1", $_POST)) $RSDB_TEMP_txtreq1=htmlspecialchars($_POST["txtreq1"]);
		if (array_key_exists("txtreq2", $_POST)) $RSDB_TEMP_txtreq2=htmlspecialchars($_POST["txtreq2"]);
		if (array_key_exists("txtspam", $_POST)) $RSDB_TEMP_txtspam=htmlspecialchars($_POST["txtspam"]);
		if (array_key_exists("verified", $_POST)) $RSDB_TEMP_verified=htmlspecialchars($_POST["verified"]);
		if (array_key_exists("appgroup", $_POST)) $RSDB_TEMP_appgroup=htmlspecialchars($_POST["appgroup"]);
		if (array_key_exists("description", $_POST)) $RSDB_TEMP_description=htmlspecialchars($_POST["description"]);
		if (array_key_exists("category", $_POST)) $RSDB_TEMP_category=htmlspecialchars($_POST["category"]);
		if (array_key_exists("vendor", $_POST)) $RSDB_TEMP_vendor=htmlspecialchars($_POST["vendor"]);


		// Edit application group data:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_SET_group != "" && $RSDB_TEMP_appgroup != "" && $RSDB_TEMP_description != "" && $RSDB_TEMP_category != "" && $RSDB_TEMP_vendor != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			// Update group entry:
			$update_group_entry = "UPDATE `rsdb_groups` SET `grpentr_name` = '". mysql_real_escape_string($RSDB_TEMP_appgroup) ."',
								`grpentr_category` = '". mysql_real_escape_string($RSDB_TEMP_category) ."',
								`grpentr_vendor` = '". mysql_real_escape_string($RSDB_TEMP_vendor) ."',
								`grpentr_description` = '". mysql_real_escape_string($RSDB_TEMP_description) ."' 
								WHERE `grpentr_id` = '". mysql_real_escape_string($RSDB_SET_group) ."' LIMIT 1 ;";
			mysql_query($update_group_entry);
			
			// Update related item entries:
			$query_group_update = mysql_query("SELECT * 
												FROM `rsdb_item_comp` 
												WHERE `comp_groupid` = " . mysql_real_escape_string($result_maintainer_group['grpentr_id']) . "
												ORDER BY `comp_id` ASC ;") ;
			while($result_group_update = mysql_fetch_array($query_group_update)) { 
				$update_item_entry = "UPDATE `rsdb_item_comp`
									SET `comp_name` = '". mysql_real_escape_string($RSDB_TEMP_appgroup.substr($result_group_update['comp_name'], strlen($result_maintainer_group['grpentr_name']) )) ."' 
									WHERE `comp_id` = ". mysql_real_escape_string($result_group_update['comp_id']) ." LIMIT 1 ;";
				mysql_query($update_item_entry);
			}
			add_log_entry("low", "comp_group", "edit", "[App Group] Edit entry", @usrfunc_GetUsername($RSDB_intern_user_id)." changed the group data from: \n\nName: ".htmlentities($result_maintainer_group['grpentr_name'])." \n\nDescription: ".htmlentities($result_maintainer_group['grpentr_description'])." \n\nCategory: ".$result_maintainer_group_category['cat_name']."\n\nVendor: ".$result_maintainer_group_vendor['vendor_name']." \n\n\nTo: \n\nName: ".htmlentities($RSDB_TEMP_appgroup)."\n\nDesc: ".htmlentities($RSDB_TEMP_description)." \n\nCategory: ".htmlentities($RSDB_TEMP_category)." \n\nVendor: ".htmlentities($RSDB_TEMP_vendor), "0");
			?>
			<script language="JavaScript">
				window.setTimeout('window.location.href="<?php echo $RSDB_intern_link_group_group2_both_javascript; ?>"','500')
			</script>
			<?php
		}

		// Special request:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_txtreq1 != "" && $RSDB_TEMP_txtreq2 != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			$report_submit="INSERT INTO `rsdb_logs` ( `log_id` , `log_date` , `log_usrid` , `log_usrip` , `log_level` , `log_action` , `log_title` , `log_description` , `log_category` , `log_badusr` , `log_referrer` , `log_browseragent` , `log_read` , `log_taskdone_usr` ) 
							VALUES ('', NOW( ) , '".mysql_real_escape_string($RSDB_intern_user_id)."', '".mysql_escape_string($RSDB_ipaddr)."', 'low', 'request', '".mysql_escape_string($RSDB_TEMP_txtreq1)."', '".mysql_escape_string($RSDB_TEMP_txtreq2)."', 'user_moderator', '0', '".mysql_escape_string($RSDB_referrer)."', '".mysql_escape_string($RSDB_usragent)."', ';', '0');";
			$db_report_submit=mysql_query($report_submit);
		}
		// Report spam:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_txtspam != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			$update_log_entry = "UPDATE `rsdb_groups` SET
									`grpentr_visible` = '3' WHERE `grpentr_id` = '". mysql_real_escape_string($RSDB_SET_group) ."' LIMIT 1 ;";
			mysql_query($update_log_entry);
			add_log_entry("low", "comp_group", "report_spam", "[App Group] Spam/ads report", @usrfunc_GetUsername($RSDB_intern_user_id)." wrote: \n".htmlentities($RSDB_TEMP_txtspam)." \n\n\n\nUser: ".@usrfunc_GetUsername($result_maintainer_group['grpentr_usrid'])." - ".$result_maintainer_group['grpentr_usrid']."\n\nAppName: ".htmlentities($result_maintainer_group['grpentr_name'])." - ".$result_maintainer_group['grpentr_id']."\n\nDesc: ".htmlentities($result_maintainer_group['grpentr_description'])." \n\nCategory: ".$result_maintainer_group_category['cat_name']." \n\nVendor: ".$result_maintainer_group_vendor['vendor_name'], $result_maintainer_group['grpentr_usrid']);
		
		}
		// Verified:
		if ($result_maintainer_group['grpentr_checked'] == "no") {
			$temp_verified = "1";
		}
		else if ($result_maintainer_group['grpentr_checked'] == "1") {
			$temp_verified = "yes";
		}
		if ($result_maintainer_group['grpentr_checked'] == "1" || $result_maintainer_group['grpentr_checked'] == "no") {
			if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_verified == "done" && usrfunc_IsModerator($RSDB_intern_user_id)) {
				$update_log_entry = "UPDATE `rsdb_groups` SET
										`grpentr_checked` = '". mysql_real_escape_string($temp_verified) ."' WHERE `grpentr_id` = '". mysql_real_escape_string($RSDB_SET_group) ."' LIMIT 1 ;";
				mysql_query($update_log_entry);
				add_log_entry("low", "comp_group", "verified", "[App Group] Verified", @usrfunc_GetUsername($RSDB_intern_user_id)." has verified the following app group: \n\n\n\nUser: ".@usrfunc_GetUsername($result_maintainer_group['grpentr_usrid'])." - ".$result_maintainer_group['grpentr_usrid']."\n\nAppName: ".htmlentities($result_maintainer_group['grpentr_name'])." - ".$result_maintainer_group['grpentr_id']."\n\nDesc: ".htmlentities($result_maintainer_group['grpentr_description'])." \n\nCategory: ".$result_maintainer_group_category['cat_name']." \n\nVendor: ".$result_maintainer_group_vendor['vendor_name'], "0");
			}
		}
?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="maintainer">
	  <tbody>
		<tr>
		  <td><p><b><a name="maintainerbar"></a>Maintainer: </b>
			  <?php if ($result_maintainer_group['grpentr_checked'] != "yes") { ?><a href="javascript:Show_verify()">Verify entry</a> | <?php  } ?><?php /*<strike><a href="javascript:Show_medal()">Change award symbol</a></strike> | */ ?><a href="javascript:Show_groupentry()">Edit application data</a> | <a href="javascript:Show_spam()">Report spam/ads</a> | <a href="<?php echo $RSDB_intern_link_submit_appver; ?>">Add Version</a> | <a href="javascript:Show_addbundle()">Add to Bundle</a> | <a href="javascript:Show_requests()">Special requests</a></p>
		    <div id="groupentry" style="display: block">
			<fieldset><legend>Edit application data</legend>
				<div align="left">
				  <form name="form1" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#maintainerbar"; ?>">
				      <font size="2">Application group name: 
				      <input name="appgroup" type="text" id="appgroup" value="<?php echo $result_maintainer_group['grpentr_name']; ?>" size="40" maxlength="100">
				      (changing this field will affect all version entries too!) <br>
				      <br>
				      Application description:
                      <input name="description" type="text" id="description" value="<?php echo $result_maintainer_group['grpentr_description']; ?>" size="70" maxlength="255">
				      (max. 255 chars) <br>
				      <br>
				      Category: 
						<select name="category" id="category">
					      <option value="0">Please select a category</option>
					      <?php 
						  $RSDB_intern_selected = $result_maintainer_group['grpentr_category'];
						  include("inc/comp/sub/tree_category_combobox.php"); ?>
					  </select>
						<font size="1">				        [<?php echo $result_maintainer_group_category['cat_name']; ?>]</font><br>
				        <br>
				      Vendor:
						<select name="vendor" id="vendor">
						    <option value="0">Please select a vendor</option>
						<?php
						$RSDB_intern_selected = $result_maintainer_group['grpentr_vendor'];
						include("inc/comp/sub/tree_vendor_combobox.php"); ?>
					  </select>
						<font size="1">						[<?php echo $result_maintainer_group_vendor['vendor_name']; ?>]</font>
						<input name="pmod" type="hidden" id="pmod" value="ok">
					  </font>
				      <p><font size="2"><em>All fields are requiered!</em></font></p>
				      <font size="2">
				      <input type="submit" name="Submit" value="Save">	
			          </font>				  
				  </form>
				</div>
			</fieldset>
		</div>
		<div id="medal" style="display: block">
			<fieldset><legend>Change award symbol</legend>
				<div align="left">
				  <p><font size="2">Please read the <a href="<?php echo $RSDB_intern_link_db_sec; ?>help#sym" target="_blank">FAQ &amp; Help page</a> about the award/medal symbols before you change something!</font></p>
				  <p>
				  <form name="form3" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#maintainerbar"; ?>">
				    <p>
				      <font size="2">
				      <select name="medal" id="medal">
				          <option value="10">Platinum</option>
				          <option value="9">Gold</option>
				          <option value="8">Silver</option>
				          <option value="7">Bronze</option>
				          <option value="5">Honorable Mention</option>
				          <option value="0">Untested</option>
				          <option value="2">Known not to work</option>
	                  </select>
                      </font> </p>
				    <p>
				      <input type="submit" name="Submit3" value="Save">
	                </p>
				  </form>
				 </p>
				</div>
			</fieldset>
		</div>
		<div id="verify" style="display: block">
			<fieldset><legend>Verify entry</legend>
				<div align="left">
				  <p><font size="2">User &quot;<?php echo @usrfunc_GetUsername($result_maintainer_group['grpentr_usrid']); ?>&quot; has submitted this application group on &quot;<?php echo $result_maintainer_group['grpentr_date']; ?>&quot;. </font></p>
				  <p><font size="2"><strong>Application group name:</strong> <?php echo htmlentities($result_maintainer_group['grpentr_name']); ?><br>
		          <br>
			        <strong>Description:</strong>			      <?php if ($result_maintainer_group['grpentr_description']) { echo htmlentities($result_maintainer_group['grpentr_description']); } else { echo '""'; } ?>
			      <br>
		          <br>
			        <strong>Category:</strong>			      <?php 
					
						echo htmlentities($result_maintainer_group_category['cat_name']);
					
					 ?>
		          <br>
		          <br>
			        <strong>Vendor:</strong>			      <?php 
					
						echo htmlentities($result_maintainer_group_vendor['vendor_name']);
					
					 ?>
				  </font></p>
				  <p><font size="2">			        Please verify the data and choose one of the three available options below:</font></p>
				  <form name="form2" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#maintainerbar"; ?>">
				  <ul>
				    <li><font size="2"><a href="javascript:Show_spam()"><strong>Report spam/ads</strong></a></font></li>
				  </ul>
				  <ul>
				    <li><font size="2"><a href="javascript:Show_groupentry()"><strong>Correct/edit data</strong></a></font></li>
				  </ul>
				  <ul>
			        <li>
			            <font size="2">
			            <input type="submit" name="Submit2" value="I have verified the data and everything is okay!">
						<input name="pmod" type="hidden" id="pmod" value="ok">
                        <input name="verified" type="hidden" id="verified" value="done">
						</font> </li>
				  </ul>
	              </form> 
				</div>
			</fieldset>
		</div>
		<div id="spam" style="display: block">
			<fieldset>
			<legend>Report spam/ads</legend>
				<div align="left">
				  <form name="form4" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#maintainerbar"; ?>">
				    <p><font size="2">Please write a useful description:<br> 
			          <textarea name="txtspam" cols="70" rows="5" id="txtspam"></textarea>
</font><font size="2" face="Arial, Helvetica, sans-serif">
<input name="pmod" type="hidden" id="pmod" value="ok">
</font><font size="2">                    </font></p>
				    <p><font size="2"><strong>Note:</strong><br>
			        When you click on the submit button, the application group will get immediately invisible, and the user who submitted this entry a bad mark. If a user has some bad marks, he will not be able to submit anything for a certain periode.<br>
			        Only administrators can revert this task, so if you made a mistake use the <a href="javascript:Show_requests()">Special requests</a> function.</font></p>
				    <p>
				      <input type="submit" name="Submit4" value="Submit">
	                </p>
				  </form>
				</div>
			</fieldset>
		</div>
		<div id="addbundle" style="display: block">
			<fieldset><legend>Add to bundle</legend>
				<div align="left">
				  <p><font size="2">This interface is currently not available!</font></p>
				  <p><font size="2">Ask a admin to do that task for the meanwhile: <a href="javascript:Show_requests()">Special requests</a></font></p>
				</div>
			</fieldset>
		</div>
		<div id="requests" style="display: block">
			<fieldset><legend>Special requests</legend>
				<div align="left">
				  <form name="form4" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#maintainerbar"; ?>">
				    <p><font size="2">Message title:<br> 
		            <input name="txtreq1" type="text" id="txtreq1" size="40" maxlength="100">
				    </font></p>
				    <p><font size="2">Text:<br> 
		              <textarea name="txtreq2" cols="70" rows="5" id="txtreq2"></textarea>
</font><font size="2" face="Arial, Helvetica, sans-serif">
<input name="pmod" type="hidden" id="pmod" value="ok">
</font><font size="2">                    </font></p>
				    <p><font size="2"><strong>Note:</strong><br>
			        Please do NOT misuse this function. All administrators will be able to see your message and one of them may contact you per forum private message, email or just do the task you suggested/requested.</font></p>
				    <p><font size="2">If you want to ask something, or the task needs (in all the circumstances) a feedback,  use the website forum, the #reactos-web IRC channel, the mailing list or the forum private message system instead. </font></p>
				    <p><font size="2">This form is not a bug tracking tool nor a feature request function! Use <a href="http://www.reactos.org/bugzilla/">bugzilla</a> for such things instead!</font></p>
				    <p><font size="2"><strong>A sample usage for this form:</strong><br>
			        If you need a new category which doesn't exist, then write a request and one of the admins will read it and may add the missing category. Then you will be able to move this application group to the right category (if you have placed the application somewhere else temporary).</font></p>
				    <p>
				      <font size="2">
				      <input type="submit" name="Submit4" value="Submit">
                      </font> </p>
				  </form>
				</div>
			</fieldset>
		</div>
		  </td>
		</tr>
	  </tbody>
	</table>
	<script language="JavaScript1.2">

		document.getElementById('groupentry').style.display = 'none';
		document.getElementById('medal').style.display = 'none';
		document.getElementById('verify').style.display = 'none';
		document.getElementById('spam').style.display = 'none';
		document.getElementById('addbundle').style.display = 'none';
		document.getElementById('requests').style.display = 'none';
	
		function Show_groupentry()
		{
			document.getElementById('groupentry').style.display = (document.getElementById('groupentry').style.display == 'none') ? 'block' : 'none';
			document.getElementById('medal').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_medal()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('medal').style.display = (document.getElementById('medal').style.display == 'none') ? 'block' : 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_verify()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('medal').style.display = 'none';
			document.getElementById('verify').style.display = (document.getElementById('verify').style.display == 'none') ? 'block' : 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}

		function Show_spam()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('medal').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = (document.getElementById('spam').style.display == 'none') ? 'block' : 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_addbundle()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('medal').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = (document.getElementById('addbundle').style.display == 'none') ? 'block' : 'none';
			document.getElementById('requests').style.display = 'none';
		}


		function Show_requests()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('medal').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = (document.getElementById('requests').style.display == 'none') ? 'block' : 'none';
		}

	</script>
<?php
	}
?>

<br />

<?php
	if (usrfunc_IsAdmin($RSDB_intern_user_id)) {
	
		$RSDB_TEMP_padmin = "";
		$RSDB_TEMP_done = "";
		if (array_key_exists("padmin", $_POST)) $RSDB_TEMP_padmin=htmlspecialchars($_POST["padmin"]);
		if (array_key_exists("done", $_POST)) $RSDB_TEMP_done=htmlspecialchars($_POST["done"]);
		
		if ($RSDB_TEMP_padmin == "ok" && $RSDB_TEMP_done != "" && usrfunc_IsAdmin($RSDB_intern_user_id)) {
			$update_log_entry = "UPDATE `rsdb_logs` SET 
									`log_taskdone_usr` = '". mysql_real_escape_string($RSDB_intern_user_id) ."' WHERE `log_id` = '". mysql_real_escape_string($RSDB_TEMP_done) ."' LIMIT 1 ;";
			mysql_query($update_log_entry);
		}
		
?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="admin">
	  <tr>
		<td><b><a name="adminbar"></a>Admin: </b><a href="javascript:Show_readrequests()">Read special requests</a> | <font size="1">all other functions are under construction ...
        </font>
		<div id="readrequests" style="display: block">
			<fieldset><legend>Read special requests</legend>

 <table width="100%" border="1">  
    <tr><td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Date</font></strong></font></div></td> 
    <td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">User</font></strong></font></div></td> 
    <td width="25%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Title</font></strong></font></div></td> 
    <td width="45%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Request</font></strong></font></div></td> 
    <td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Done?</font></strong></font></div></td>
    </tr> <?php
					$cellcolor1="#E2E2E2";
					$cellcolor2="#EEEEEE";
					$cellcolorcounter="0";
					$query_entry_sprequest = mysql_query("SELECT * 
							FROM `rsdb_logs` 
							WHERE `log_level` LIKE 'low'
							AND `log_action` LIKE 'request'
							AND `log_category` LIKE 'user_moderator'
							ORDER BY `log_date` DESC
							LIMIT 0, 30;") ;
					while($result_entry_sprequest = mysql_fetch_array($query_entry_sprequest)) {
				?> 
  <tr valign="top" bgcolor="<?php
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
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo $result_entry_sprequest['log_date'];  if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></div></td> 
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo @usrfunc_GetUsername($result_entry_sprequest['log_usrid']); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></div></td> 
    <td><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo htmlentities($result_entry_sprequest['log_title']); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></td> 
    <td><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo wordwrap(nl2br(htmlentities($result_entry_sprequest['log_description'], ENT_QUOTES))); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></td> 
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo @usrfunc_GetUsername($result_entry_sprequest['log_taskdone_usr']); } 
		
		else {
	?>
        <form name="form5" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#adminbar"; ?>">
          <input type="submit" name="Submit5" value="Done!">
          <input name="padmin" type="hidden" id="padmin" value="ok">
          <font size="2" face="Arial, Helvetica, sans-serif">
          <input name="done" type="hidden" id="done" value="<?php echo $result_entry_sprequest['log_id']; ?>">
          </font>
        </form>
    <?php
		}
	
	 ?>
        </font></div></td>
  </tr> 
	<?php
		}
	?> 
</table>

			</fieldset>
		</div>		</td>
	  </tr>
	</table>
	<script language="JavaScript1.2">

		document.getElementById('readrequests').style.display = 'none';
	
		function Show_readrequests()
		{
			document.getElementById('readrequests').style.display = (document.getElementById('readrequests').style.display == 'none') ? 'block' : 'none';
		}
	</script>
<?php
	}
?>