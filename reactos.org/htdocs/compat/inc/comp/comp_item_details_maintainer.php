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
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :comp_id LIMIT 1");
    $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
    $stmt->execute();
		$result_maintainer_item = $stmt->fetch(PDO::FETCH_ASSOC);

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id " . $RSDB_intern_code_db_rsdb_groups . " LIMIT 1");
    $stmt->bindParam('group_id',$result_maintainer_item['comp_groupid'],PDO::PARAM_STR);
    $stmt->execute();
		$result_maintainer_group = $stmt->fetch(PDO::FETCH_ASSOC);



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
		$RSDB_TEMP_appn = "";
		$RSDB_TEMP_apppr = "";
		$RSDB_TEMP_appit = "";
		$RSDB_TEMP_appdesc = "";
		$RSDB_TEMP_version = "";
		$RSDB_TEMP_appinfo = "";
		if (array_key_exists("pmod", $_POST)) $RSDB_TEMP_pmod=htmlspecialchars($_POST["pmod"]);
		if (array_key_exists("txtreq1", $_POST)) $RSDB_TEMP_txtreq1=htmlspecialchars($_POST["txtreq1"]);
		if (array_key_exists("txtreq2", $_POST)) $RSDB_TEMP_txtreq2=htmlspecialchars($_POST["txtreq2"]);
		if (array_key_exists("txtspam", $_POST)) $RSDB_TEMP_txtspam=htmlspecialchars($_POST["txtspam"]);
		if (array_key_exists("verified", $_POST)) $RSDB_TEMP_verified=htmlspecialchars($_POST["verified"]);
		if (array_key_exists("appn", $_POST)) $RSDB_TEMP_appn=htmlspecialchars($_POST["appn"]);
		if (array_key_exists("apppr", $_POST)) $RSDB_TEMP_apppr=htmlspecialchars($_POST["apppr"]);
		if (array_key_exists("appit", $_POST)) $RSDB_TEMP_appit=htmlspecialchars($_POST["appit"]);
		if (array_key_exists("appdesc", $_POST)) $RSDB_TEMP_appdesc=htmlspecialchars($_POST["appdesc"]);
		if (array_key_exists("version", $_POST)) $RSDB_TEMP_version=htmlspecialchars($_POST["version"]);
		if (array_key_exists("appinfo", $_POST)) $RSDB_TEMP_appinfo=htmlspecialchars($_POST["appinfo"]);


		// Edit application group data:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_SET_item != "" && $RSDB_TEMP_appn != "" && $RSDB_TEMP_apppr != "" && $RSDB_TEMP_appit != "" && $RSDB_TEMP_version != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {

      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id ".$RSDB_intern_code_db_rsdb_groups." LIMIT 1");
      $stmt->bindParam('group_id',$RSDB_TEMP_appn,PDO::PARAM_STR);
      $stmt->execute();
			$result_maintainer_group2 = $stmt->fetch(PDO::FETCH_ASSOC);

			// Update item entry:
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_item_comp SET comp_name = :new_name, comp_appversion = :new_appversion, comp_groupid = :new_groupid, comp_description = :new_description, comp_infotext = :new_infotext, comp_osversion = :new_osversion WHERE comp_id = :comp_id");
      $stmt->bindValue('new_name',$result_maintainer_group2['grpentr_name']." ".$RSDB_TEMP_apppr,PDO::PARAM_STR);
      $stmt->bindParam('new_appversion',$RSDB_TEMP_appit,PDO::PARAM_STR);
      $stmt->bindParam('new_groupid',$RSDB_TEMP_appn,PDO::PARAM_STR);
      $stmt->bindParam('new_description',$RSDB_TEMP_appdesc,PDO::PARAM_STR);
      $stmt->bindParam('new_infotext',$RSDB_TEMP_appinfo,PDO::PARAM_STR);
      $stmt->bindParam('new_osversion',$RSDB_TEMP_version,PDO::PARAM_STR);
      $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
      $stmt->execute();
			
			CLog::add("low", "comp_item", "edit", "[App Item] Edit entry", @usrfunc_GetUsername($RSDB_intern_user_id)." changed the group data from: \n\nAppName: ".htmlentities($result_maintainer_item['comp_name'])." - ".$result_maintainer_item['comp_id']."\n\nDesc: ".htmlentities($result_maintainer_item['comp_description'])." \n\GroupID: ".$result_maintainer_item['comp_groupid']." \n\ReactOS version: ".$result_maintainer_item['comp_osversion']." \n\n\nTo: \n\nAppName: ".htmlentities($result_maintainer_group['grpentr_name']." ".$RSDB_TEMP_apppr)." - ".htmlentities($RSDB_TEMP_appn)."\n\nInternVersion: ".htmlentities($RSDB_TEMP_appit)." \n\nDesc: ".htmlentities($RSDB_TEMP_appdesc)." \n\nReactOS version: ".htmlentities($RSDB_TEMP_version), "0");
			?>
			<script language="JavaScript">
				window.setTimeout('window.location.href="<?php echo $RSDB_intern_link_item_item2_both_javascript; ?>"','500')
			</script>
			<?php
		}

		// Special request:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_txtreq1 != "" && $RSDB_TEMP_txtreq2 != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
      $stmt=CDBConnection::getInstance()->prepare("INSERT INTO rsdb_logs ( log_id , log_date , log_usrid , log_usrip , log_level , log_action , log_title , log_description , log_category , log_badusr , log_referrer , log_browseragent , log_read , log_taskdone_usr ) VALUES ('', NOW( ) , :user_id, :ip, 'low', 'request', :title, :description, 'user_moderator', '0', :referrer, :user_agend, ';', '0')");
      $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
      $stmt->bindParam('ip',$RSDB_ipaddr,PDO::PARAM_STR);
      $stmt->bindParam('title',$RSDB_TEMP_txtreq1,PDO::PARAM_STR);
      $stmt->bindParam('description',$RSDB_TEMP_txtreq2,PDO::PARAM_STR);
      $stmt->bindParam('referrer',$RSDB_referrer,PDO::PARAM_STR);
      $stmt->bindParam('user_agent',$RSDB_usragent,PDO::PARAM_STR);
      $stmt->execute();
		}
		// Report spam:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_txtspam != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			$stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_item_comp SET comp_visible = '3' WHERE comp_id = :comp_id");
      $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
      $stmt->execute();
			CLog::add("low", "comp_item", "report_spam", "[App Item] Spam/ads report", @usrfunc_GetUsername($RSDB_intern_user_id)." wrote: \n".htmlentities($RSDB_TEMP_txtspam)." \n\n\n\nUser: ".@usrfunc_GetUsername($result_maintainer_item['comp_usrid'])." - ".$result_maintainer_item['comp_usrid']."\n\nAppName: ".htmlentities($result_maintainer_item['comp_name'])." - ".$result_maintainer_item['comp_id']."\n\nDesc: ".htmlentities($result_maintainer_item['comp_description'])." \n\GroupID: ".$result_maintainer_item['comp_groupid']." \n\ReactOS version: ".$result_maintainer_item['comp_osversion'], $result_maintainer_item['comp_usrid']);
		
		}
		// Verified:
		if ($result_maintainer_item['comp_checked'] == "no") {
			$temp_verified = "1";
		}
		else if ($result_maintainer_item['comp_checked'] == "1") {
			$temp_verified = "yes";
		}
		if ($result_maintainer_item['comp_checked'] == "1" || $result_maintainer_item['comp_checked'] == "no") {
			if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_verified == "done" && usrfunc_IsModerator($RSDB_intern_user_id)) {
				echo "!";
        $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_item_comp SET comp_checked = :checked WHERE comp_id = :comp_id ");
        $stmt->bindParam('checked',$temp_verified,PDO::PARAM_STR);
        $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
        $stmt->execute();
				CLog::add("low", "comp_item", "verified", "[App Item] Verified", @usrfunc_GetUsername($RSDB_intern_user_id)." has verified the following app version: \n\n\n\nUser: ".@usrfunc_GetUsername($result_maintainer_item['comp_usrid'])." - ".$result_maintainer_item['comp_usrid']."\n\nAppName: ".htmlentities($result_maintainer_item['comp_name'])." - ".$result_maintainer_item['comp_id']."\n\nDesc: ".htmlentities($result_maintainer_item['comp_description'])." \n\GroupID: ".$result_maintainer_item['comp_groupid']." \n\ReactOS version: ".$result_maintainer_item['comp_osversion'], "0");
			}
		}
?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="maintainer">
	  <tbody>
		<tr>
		  <td><p><b><a name="maintainerbar"></a>Maintainer: </b>
			  <?php if ($result_maintainer_item['comp_checked'] != "yes") { ?><a href="javascript:Show_verify()">Verify entry</a> | <?php  } ?><a href="javascript:Show_groupentry()">Edit application versions data</a> | <a href="javascript:Show_spam()">Report spam/ads</a> | <a href="javascript:Show_requests()">Special requests</a></p>
		    <div id="groupentry" style="display: block">
			<fieldset>
			<legend>Edit application versions data</legend>
				<div align="left">
				  <form name="form1" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#maintainerbar"; ?>">
				      <p><font size="2">Application name: 
				        <select name="appn" id="appn">
                          <?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' ORDER BY grpentr_name ASC");
					while($result_appn = $stmt->fetch(PDO::FETCH_ASSOC)) {
						echo '<option value="'. $result_appn['grpentr_id'] .'"';
						if  ($result_maintainer_item['comp_groupid'] == $result_appn['grpentr_id']) {
							echo ' selected';
						}
						echo '>'. $result_appn['grpentr_name'] .'</option>';
					}
				?>
						</select>
				        <font size="1">			          [<?php echo htmlentities($result_maintainer_group['grpentr_name']); ?>] (this will move the entry to another application group!) </font></font></p>
				      <p><font size="2">Application PR name: 
                      <input name="apppr" type="text" id="apppr" value="<?php echo htmlentities(substr($result_maintainer_item['comp_name'], strlen($result_maintainer_group['grpentr_name'])+1 )); ?>" size="30" maxlength="100">
		              (max. 100 chars) <br>
		              <br>
		              Application intern version:
                      <input name="appit" type="text" id="appit" value="<?php echo $result_maintainer_item['comp_appversion']; ?>" size="10" maxlength="15">
		              (number) </font></p>
				      <p><font size="2">Application description:
                          <input name="appdesc" type="text" id="appdesc" value="<?php echo htmlentities($result_maintainer_item['comp_description']); ?>" size="50" maxlength="255">
					(max. 255 chars) </font></p>
				      <p><font size="2">Additional information: <br>
                          <textarea name="appinfo" cols="70" rows="10" id="appinfo"><?php echo htmlentities($result_maintainer_item['comp_infotext']); ?></textarea>
                      <br>
	                    <br>
                      ReactOS version:
                      <select name="version" id="version">
				  <?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_osversions WHERE ver_visible = '1' ORDER BY ver_value DESC");
          while($result_osvers = $stmt->fetch(PDO::FETCH_ASSOC)) {
						echo '<option value="'. $result_osvers['ver_value'] .'"';
						if  ($result_maintainer_item['comp_osversion'] == $result_osvers['ver_value']) {
							echo ' selected';
						}
						echo '>'. $result_osvers['ver_name'] .'</option>';
					}
				?>
				            </select> 
	                  <font size="1">						[<?php echo "ReactOS ". @show_osversion($result_maintainer_item['comp_osversion']); ?>]</font>
			            <input name="pmod" type="hidden" id="pmod" value="ok">
			            <br>
                      <br>
	                    <br>
	                    <input type="submit" name="Submit" value="Save">	
		                        </font>				  
				                    </p>
				  </form>
				</div>
			</fieldset>
		</div>
		<div id="verify" style="display: block">
			<fieldset><legend>Verify entry</legend>
				<div align="left">
				  <p><font size="2">User &quot;<?php echo @usrfunc_GetUsername($result_maintainer_item['comp_usrid']); ?>&quot; has submitted this application group on &quot;<?php echo $result_maintainer_item['comp_date']; ?>&quot;. </font></p>
				  <p><font size="2"><strong>Application  name:</strong> <?php echo htmlentities($result_maintainer_group['grpentr_name']); ?><br>
		          <br>
			        <strong>Application PR name:</strong>			      <?php if ($result_maintainer_item['comp_name']) { echo htmlentities($result_maintainer_item['comp_name']); } else { echo '""'; } ?>
			      <br>
		          <br>
			        <strong>Application intern version:</strong>			      <?php 
					
						echo htmlentities($result_maintainer_item['comp_appversion']);
					
					 ?>
		          <br>
		          <br>
			        <strong>Application description:</strong>			      <?php 
					
						echo htmlentities($result_maintainer_item['comp_description']);
					
					 ?>
</font></p>
				  <p><font size="2"><strong>ReactOS version:</strong>
                  <?php echo "ReactOS ". @show_osversion($result_maintainer_item['comp_osversion']); ?>                 </font></p>
				  <p><font size="2">			        Please verify the data and choose one of the three available options below:</font></p>
				  <form name="form2" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#maintainerbar"; ?>">
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
				  <form name="form4" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#maintainerbar"; ?>">
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
				  <form name="form4" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#maintainerbar"; ?>">
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
		document.getElementById('verify').style.display = 'none';
		document.getElementById('spam').style.display = 'none';
		document.getElementById('addbundle').style.display = 'none';
		document.getElementById('requests').style.display = 'none';
	
		function Show_groupentry()
		{
			document.getElementById('groupentry').style.display = (document.getElementById('groupentry').style.display == 'none') ? 'block' : 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_verify()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('verify').style.display = (document.getElementById('verify').style.display == 'none') ? 'block' : 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}

		function Show_spam()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = (document.getElementById('spam').style.display == 'none') ? 'block' : 'none';
			document.getElementById('addbundle').style.display = 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_addbundle()
		{
			document.getElementById('groupentry').style.display = 'none';
			document.getElementById('verify').style.display = 'none';
			document.getElementById('spam').style.display = 'none';
			document.getElementById('addbundle').style.display = (document.getElementById('addbundle').style.display == 'none') ? 'block' : 'none';
			document.getElementById('requests').style.display = 'none';
		}


		function Show_requests()
		{
			document.getElementById('groupentry').style.display = 'none';
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
		$RSDB_TEMP_medal = "";
		if (array_key_exists("padmin", $_POST)) $RSDB_TEMP_padmin=htmlspecialchars($_POST["padmin"]);
		if (array_key_exists("done", $_POST)) $RSDB_TEMP_done=htmlspecialchars($_POST["done"]);
		if (array_key_exists("medal", $_POST)) $RSDB_TEMP_medal=htmlspecialchars($_POST["medal"]);

		
		if ($RSDB_TEMP_padmin == "ok" && $RSDB_TEMP_done != "" && usrfunc_IsAdmin($RSDB_intern_user_id)) {
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_logs SET log_taskdone_usr = :user_id WHERE log_id = :log_id");
      $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
      $stmt->bindParam('log_id',$RSDB_TEMP_done,PDO::PARAM_STR);
      $stmt->execute();
		}
		if ($RSDB_TEMP_padmin == "ok" && $RSDB_TEMP_medal != "" && $RSDB_SET_item != "" && usrfunc_IsAdmin($RSDB_intern_user_id)) {
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_item_comp SET comp_award = :award WHERE comp_id = :comp_id");
      $stmt->bindParam('award',$RSDB_TEMP_medal,PDO::PARAM_STR);
      $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
      $stmt->execute();
			CLog::add("medium", "comp_item", "change award", "[App Item] Change Award", @usrfunc_GetUsername($RSDB_intern_user_id)." (".$RSDB_intern_user_id.") has changed the award symbol from: ".$result_maintainer_item['comp_award']." to ".$RSDB_TEMP_medal, "0");
			?>
			<script language="JavaScript">
				window.setTimeout('window.location.href="<?php echo $RSDB_intern_link_item_item2_both; ?>"','500')
			</script>
			<?php
		}
		
?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="admin">
	  <tr>
		<td><b><a name="adminbar"></a>Admin: </b> <a href="javascript:Show_medal()">Change award symbol</a> | <a href="javascript:Show_readrequests()">Read special requests</a> | <font size="1">all other functions are under construction ...
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
					$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_logs WHERE log_level LIKE 'low' AND log_action LIKE 'request' AND log_category LIKE 'user_moderator' ORDER BY log_date DESC LIMIT 30");
          $stmt->execute();
					while($result_entry_sprequest = $stmt->fetch(PDO::FETCH_ASSOC)) {
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
        <form name="form5" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#adminbar"; ?>">
          <input type="submit" name="Submit5" value="Done!">
          <input name="padmin" type="hidden" id="padmin" value="ok">
          <font size="2" face="Arial, Helvetica, sans-serif">
          <input name="done" type="hidden" id="done" value="<?php echo $result_entry_sprequest['log_id']; ?>">
          </font>
        </form>
    <?php
		}
	
	 ?>
        </font></div>
		</td>
  </tr> 
	<?php
		}
	?> 
</table>

			</fieldset>
		</div>
		
		<div id="medal" style="display: block">
			<fieldset><legend>Change award symbol</legend>
				<div align="left">
				  <p><font size="2">Please read the <a href="<?php echo $RSDB_intern_link_db_sec; ?>help#sym" target="_blank">FAQ &amp; Help page</a> about the award/medal symbols before you change something!</font></p>
				  <p><font size="2">Please only change the award symbol if you have tested the application yourself. Do NOT forget to submit a compatibility test report (so that at least one test report exist) before you change the award symbol! </font></p>
				  <form name="form3" method="post" action="<?php echo $RSDB_intern_link_item_item2_both."#maintainerbar"; ?>">
				    <p>
				      <font size="2">
				      <select name="medal" id="medal">
				          <option value="10" <?php if ($result_maintainer_item['comp_award'] == "10") { echo "selected"; } ?>>Platinum</option>
				          <option value="9" <?php if ($result_maintainer_item['comp_award'] == "9") { echo "selected"; } ?>>Gold</option>
				          <option value="8" <?php if ($result_maintainer_item['comp_award'] == "8") { echo "selected"; } ?>>Silver</option>
				          <option value="7" <?php if ($result_maintainer_item['comp_award'] == "7") { echo "selected"; } ?>>Bronze</option>
				          <option value="5" <?php if ($result_maintainer_item['comp_award'] == "5") { echo "selected"; } ?>>Honorable Mention</option>
				          <option value="0" <?php if ($result_maintainer_item['comp_award'] == "0") { echo "selected"; } ?>>Untested</option>
				          <option value="2" <?php if ($result_maintainer_item['comp_award'] == "2") { echo "selected"; } ?>>Known not to work</option>
	                  </select>
                      <input name="padmin" type="hidden" id="padmin" value="ok">
			</font> </p>
				    <p>
				      <input type="submit" name="Submit3" value="Save">
	                </p>
				  </form>
				</div>
			</fieldset>
		</div>
		
				</td>
	  </tr>
	</table>
	<script language="JavaScript1.2">

		document.getElementById('readrequests').style.display = 'none';
		document.getElementById('medal').style.display = 'none';
	
		function Show_readrequests()
		{
			document.getElementById('readrequests').style.display = (document.getElementById('readrequests').style.display == 'none') ? 'block' : 'none';
			document.getElementById('medal').style.display = 'none';
		}
		function Show_medal()
		{
			document.getElementById('readrequests').style.display = 'none';
			document.getElementById('medal').style.display = (document.getElementById('medal').style.display == 'none') ? 'block' : 'none';
		}

	</script>
<?php
	}
?>
