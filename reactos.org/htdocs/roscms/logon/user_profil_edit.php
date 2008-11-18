<?php
    /*
    ReactOS DynamicFrontend (RDF)
    Copyright (C) 2008  Klemens Friedl <frik85@reactos.org>

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
	if ( !defined('ROSCMS_LOGIN') )
	{
		die("Hacking attempt");
	}
	
	session_start();


	create_header("", "logon");
	require("logon/user_profil_menubar.php");
	require("inc/subsys_utils.php");
	
	$message = ""; // error message box text
	$existname = false; // username already exists in the database (true = username exists)
	$existemail = false; // email already exists in the database (true = email exists)
	$safename = ""; // protected username ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)
	$safepwd = ""; // unsafe password, common cracked passwords ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)
	$newpwd = false; // new password

if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6) {
?>
<h1><a href="<?php echo $roscms_SET_path_ex; ?>my/">myReactOS</a> &gt; <a href="<?php echo $roscms_SET_path_ex; ?>my/edit/">Edit My Profile</a> &gt; Activate E-Mail Address</h1>
<div class="u-h1">Activate E-Mail Address</div>
	<div class="u-h2">So you have a new email address and would like to keep your <?php echo $rdf_name; ?> account up-to-date? That's a very good idea. To confirm your email address change, please enter your new email address again.</div>
<?php
}
else {
?>
<h1><a href="<?php echo $roscms_SET_path_ex; ?>my/">myReactOS</a> &gt; Edit My Profile</h1>
<div class="u-h1">Edit My Profile</div>
<div class="u-h2">Update your user account profile data to reflect the current state.</div>
<?php
}
?>
<form action="<?php $roscms_SET_path_ex."my/edit/"; ?>" method="post">
	<div align="center">
		<div style="background: <?php echo $rdf_color_logon_background; ?> none repeat scroll 0%; width: 300px;">
			<div class="corner1">
				<div class="corner2">
					<div class="corner3">
						<div class="corner4">
							<div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
								<?php
									$sql_user_profil = "SELECT user_id, user_name, user_fullname, user_email, user_email_activation, user_website, 
															user_country, user_timezone, user_occupation, user_setting_multisession, 
															user_setting_browseragent, user_setting_ipaddress, user_setting_timeout  
														FROM users 
														WHERE user_id = '".mysql_real_escape_string($rdf_user_id)."'
														LIMIT 1;";
									$query_user_profil = mysql_query($sql_user_profil);
									$result_user_profil = mysql_fetch_array($query_user_profil);

									if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6 &&
										isset($_POST['registerpost']) && 
										isset($_POST['useremail']) && $_POST['useremail'] != "" && 
										preg_match($rdf_register_valid_email_regex, $_POST['useremail']) && /* check if it's a valid email address */
										$result_user_profil['user_email_activation'] == ($_POST['useremail'].$rdf_uri_3))
									{
										$sql_change_email = "UPDATE users 
																SET user_timestamp_touch2 = NOW( ) ,
																user_email_activation = '', 
																user_email = '".mysql_real_escape_string($_POST['useremail'])."' 
																WHERE user_id = ".mysql_real_escape_string($result_user_profil['user_id'])." 
																LIMIT 1;";
										$update_change_email = mysql_query($sql_change_email);
										subsys_update_user($result_user_profil['user_id']);

										echo '<div class="login-title">E-Mail Address Changed</div>';
										echo '<div><a href="'.$roscms_SET_path_ex.'my/" style="color:#FF0000 !important; text-decoration:underline;">My Profile</a></div>';
										return;
									}
									else {									
										if (isset($_POST['registerpost']) && 
											isset($_POST['userpwd3']) && $_POST['userpwd3'] != "" &&
											isset($_POST['userpwd1']) && $_POST['userpwd1'] != "" &&
											isset($_POST['userpwd2']) && $_POST['userpwd2'] != "" &&
											strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max &&
											$_POST['userpwd1'] == $_POST['userpwd2'])
										{
											$sql_unsafe_pwd = "SELECT pwd_name 
																FROM user_unsafepwds 
																WHERE pwd_name = '".mysql_real_escape_string($_POST['userpwd1'])."'
																LIMIT 1;";
											$query_unsafe_pwd = mysql_query($sql_unsafe_pwd);
											$result_unsafe_pwd = mysql_fetch_array($query_unsafe_pwd);
											
											if ($result_unsafe_pwd['pwd_name'] == "") {
												$safepwd = "true";
											}
											else {
												$safepwd = "false";
											}
											
											$newpwd = true;
										}
										
										if (isset($_POST['registerpost']) && 
											isset($_POST['useremail']) && $_POST['useremail'] != "")
										{
											// check if another account with the same email address already exists
											$sql_exist_email = "SELECT user_email  
																FROM users 
																WHERE user_email = '".mysql_real_escape_string($_POST['useremail'])."'
																LIMIT 1;";
											$query_exist_email = mysql_query($sql_exist_email);
											$result_exist_email = mysql_fetch_array($query_exist_email);
											
											if ($result_exist_email['user_email'] != "") {
												if ($result_user_profil['user_email'] != $_POST['useremail']) {
													$existemail = true;
												}
											}
										}
									}
									
									if (($rdf_uri_3 == "" || strlen($rdf_uri_3) < 6) &&
										isset($_POST['registerpost']) && 
										($safepwd == "true" || $safepwd == "") &&
										(isset($_POST['userpwd1']) && ($_POST['userpwd1'] == "" || (strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max))) &&
										isset($_POST['useremail']) && $_POST['useremail'] != "" && 
										preg_match($rdf_register_valid_email_regex, $_POST['useremail']) && /* check if it's a valid email address */
										!$existemail)
									{
										// user language (browser settings)
										$userlang = check_lang($_SERVER["HTTP_ACCEPT_LANGUAGE"]);
										
										if(!$userlang)
											$userlang = "en";
										
										// email address activation code
										$s = "";
										for ($n=0; $n<20; $n++) {
											$s .= chr(rand(0, 255));
										}
										$s = base64_encode($s);   // base64-set, but filter out unwanted chars
										$s = preg_replace("/[+\/=IG0ODQRtl]/i", "", $s);  // strips hard to discern letters, depends on used font type
										//$s = substr($s, 0, $characters);
										$s = substr($s, 0, rand(10, 15));
										$account_act_code = $s;
										

										$updatepwd = "";
										$updatemail = "";
										
										if ($safepwd == "true") {
											$updatepwd = " user_roscms_password = MD5( '".mysql_real_escape_string($_POST['userpwd1'])."' ) , ";
										}
										
										if ($result_user_profil['user_email'] != $_POST['useremail']) {
											$updatemail = " user_email_activation = '".mysql_real_escape_string(htmlspecialchars($_POST['useremail'])).$account_act_code."' , ";
										}
										
										
										if (isset($_POST['loginoption1'])) $updatelogin1 = "true";
										else $updatelogin1 = "false";

										if (isset($_POST['loginoption2'])) $updatelogin2 = "true";
										else $updatelogin2 = "false";

										if (isset($_POST['loginoption3'])) $updatelogin3 = "true";
										else $updatelogin3 = "false";

										if (isset($_POST['loginoption4'])) $updatelogin4 = "true";
										else $updatelogin4 = "false";
										
										
										// update account data
										$sql_profil = "UPDATE users 
														SET ".$updatepwd."
														user_timestamp_touch2 = NOW( ) ,
														user_fullname = '".mysql_real_escape_string($_POST['userfullname'])."',
														".$updatemail."
														user_website = '".mysql_real_escape_string($_POST['userwebsite'])."',
														user_language = '".mysql_real_escape_string($userlang)."',
														user_country = '".mysql_real_escape_string($_POST['country'])."',
														user_timezone = '".mysql_real_escape_string($_POST['tzone'])."',
														user_occupation = '".mysql_real_escape_string($_POST['useroccupation'])."',
														user_setting_multisession = '".mysql_real_escape_string($updatelogin1)."',
														user_setting_browseragent = '".mysql_real_escape_string($updatelogin2)."',
														user_setting_ipaddress = '".mysql_real_escape_string($updatelogin3)."',
														user_setting_timeout = '".mysql_real_escape_string($updatelogin4)."'
														WHERE user_id = ".mysql_real_escape_string($result_user_profil['user_id'])."
														LIMIT 1;";
										$insert_profil = mysql_query($sql_profil);

										echo '<div class="login-title">Profile Changes Saved</div>';
										
										if ($result_user_profil['user_email'] != $_POST['useremail']) {
											// send the email to the users email address
											
											/* email addresses */
											$empfaenger = array(htmlentities($_POST['useremail'], ENT_NOQUOTES, "UTF-8"));
											
											/* CC */
											$empfaengerCC = array("");
											
											/* BCC */
											$empfaengerBCC = array("");
											
											/* sender */
											$absender = $rdf_system_email_str;
											
											/* reply */
											$reply = $rdf_system_email_str;
											
											/* subject */
											$subject = $rdf_name_long." - Email Address Activation";
											
											/* message */
											$message = $rdf_name_long." - Email Address Activation\n\n\nYou have requested an email address change for your account on ".$rdf_name.". The next step in order to enable the new email address for the account is to activate it by using the hyperlink below.";
											$message .= "\n\n\nCurrent E-Mail Address: ".$result_user_profil['user_email']."\nNew E-Mail Address: ".$_POST['useremail']."\n\nActivation-Hyperlink: ".$roscms_SET_path_ex."my/activate/".$account_act_code."/";
											$message .= "\n\n\nBest regards,\nThe ".$rdf_name." Team\n\n\n(please do not reply as this is an auto generated email!)";
											
											/* build the mail header */
											$headers = "";
											$headers .= "From:" . $absender . "\n";
											$headers .= "Reply-To:" . $reply . "\n"; 
											$headers .= "X-Mailer: ".$rdf_system_brand."\n"; 
											$headers .= "X-Sender-IP: ".$_SERVER['REMOTE_ADDR']."\n"; 
											$headers .= "Content-type: text/plain\n";
											
											// extract the email addresses
											$empfaengerString = implode(",",$empfaenger);
											$empfaengerCCString = implode(",",$empfaengerCC);
											$empfaengerBCCString = implode(",",$empfaengerBCC);
											
											$headers .= "Cc: " . $empfaengerCCString . "\n";
											$headers .= "Bcc: " . $empfaengerBCCString . "\n";
											
											/* send the mail */
											@mail($empfaengerString, $subject, $message, $headers);
											
											echo "<div>Check your email inbox (and spam folder) for the <b>email-address activation email</b> that contains the activation hyperlink.</div>";
										}
										
										if ($safepwd == "true") {
											echo "<div>Password changed.</div>";
										}

										echo '<div><a href="'.$roscms_SET_path_ex.'my/" style="color:#FF0000 !important; text-decoration:underline;">My Profile</a></div>';
										
										subsys_update_user($result_user_profil['user_id']);
										
										$message = "";
										unset($_SESSION['rdf_security_code']);
									}
									else if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6) {
								?>
									<div class="login-title">Activate E-Mail Address</div>
									<div class="login-form">
										<label for="useremail"<?php if (isset($_POST['registerpost'])) { echo ' style="color:#FF0000;"'; } ?>>New E-Mail Address</label>
										<input name="useremail" type="text" class="input" tabindex="4" id="useremail" value="" size="50" maxlength="50" />
									</div>
									<div class="login-button">
										<input type="submit" name="submit" value="Save" tabindex="16" />
										<br />
										<input type="button" onclick="window.location='<?php echo $roscms_SET_path_ex; ?>'" tabindex="17" value="Cancel" name="cancel" style="color:#777777;" />
										<input name="registerpost" type="hidden" id="registerpost" value="reg" />
									</div>
								<?php
									}
									else {
								?>
									<div class="login-title">Edit My Profile</div>
									<div><i>* not required</i></div>
									<div class="login-form">
										<label for="username">Username</label>
										<input name="username" type="text" class="input" id="username" <?php echo 'value="'.$result_user_profil['user_name'].'"'; ?> size="50" maxlength="50" disabled="disabled" value="Klemens Friedl" />
										<br /><span style="color:#817A71;">You cannot change your username.</span>
									</div>
									<div class="login-form">
										<label for="userpwd3"<?php if (isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != "") { echo ' style="color:#FF0000;"'; } ?>>Current Password *</label>
										<input name="userpwd3" type="password" class="input" tabindex="1" id="userpwd3" size="50" maxlength="50" />
										<br /><span style="color:#817A71;">Only fill out the three password fields if you want to change your account password!</span>
									</div>
									<div class="login-form">
										<label for="userpwd1"<?php if (isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != "") { echo ' style="color:#FF0000;"'; } ?>>New Password *</label>
										<input name="userpwd1" type="password" class="input" tabindex="2" id="userpwd1" size="50" maxlength="50" />
										<?php 
											if ($safepwd == "false" || (isset($_POST['userpwd1']) && strlen($_POST['userpwd1']) > $rdf_register_user_name_max)) {
												echo "<br /><i>Please use a stronger password! At least ".$rdf_register_user_pwd_min." characters, do not include common words or names, and combine three of these character types: uppercase letters, lowercase letters, numbers, or symbols (ASCII characters).</i>";
											}
											else {
												echo '<br /><span style="color:#817A71;">uppercase letters, lowercase letters, numbers, and symbols (ASCII characters)</span>';
											}
										?>
									</div>
									<div class="login-form">
										<label for="userpwd2"<?php if (isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != "") { echo ' style="color:#FF0000;"'; } ?>>Re-type New Password *</label>
										<input name="userpwd2" type="password" class="input" tabindex="3" id="userpwd2" size="50" maxlength="50" />
									</div>
									<div class="login-form">
										<label for="useremail"<?php if (isset($_POST['registerpost']) && ($_POST['useremail'] == "" || ($existemail && $_POST['useremail'] != $result_user_profil['user_email']) || !preg_match($rdf_register_valid_email_regex, $_POST['useremail']))) { echo ' style="color:#FF0000;"'; } ?>>E-Mail</label>
										<input name="useremail" type="text" class="input" tabindex="4" id="useremail" value="<?php 
											if (isset($_POST['useremail']) && $_POST['useremail'] != "") {
												echo $_POST['useremail'];
											}
											else {
												echo $result_user_profil['user_email'];
											}
										?>" size="50" maxlength="50" />
										<br /><span style="color:#817A71;">Changing the email address involves an activation process.</span>
										<?php
											if (isset($_POST['registerpost']) && $existemail && $_POST['useremail'] != $result_user_profil['user_email']) {
												echo '<br /><i>That email address is already with an account. Do you have several accounts? Please <a href="'.$roscms_SET_path_ex.'login/" style="color:#FF0000 !important; text-decoration:underline;"><b>login</b></a>!</i>';
											}
										?>
									</div>
									<div class="login-form">
										<label for="userfullname">First and Last Name *</label>
										<input name="userfullname" type="text" class="input" tabindex="5" id="userfullname" value="<?php 
											if (isset($_POST['userfullname'])) {
												echo $_POST['userfullname'];
											}
											else {
												echo $result_user_profil['user_fullname'];
											}
										?>" size="50" maxlength="50" />
									</div>
									<div class="login-form">
										<label for="country"<?php if (isset($_POST['registerpost']) && $_POST['country'] == "") { echo ' style="color:#FF0000;"'; } ?>>Country</label>
										<select id="country" name="country" tabindex="6">
											<option value="">Select One</option>
											<?php
												$sql_country = "SELECT coun_id, coun_name  
																FROM user_countries 
																ORDER BY coun_name ASC;";
												$query_country = mysql_query($sql_country);
												while ($result_country = mysql_fetch_array($query_country)) {
													echo '<option value="'.$result_country['coun_id'].'"';
													if (isset($_POST['country']) && $_POST['country'] == $result_country['coun_id']) {
														echo ' selected="selected"'; 
													}
													else if ((!isset($_POST['country']) || (isset($_POST['country']) && $_POST['country'] == "")) && $result_country['coun_id'] == $result_user_profil['user_country']) {
														echo ' selected="selected"'; 
													}
													echo '>'.$result_country['coun_name'].'</option>';
												}
											?>
										</select>
									</div>
									<div class="login-form">
										<label for="tzone"<?php if (isset($_POST['registerpost']) && $_POST['tzone'] == "") { echo ' style="color:#FF0000;"'; } ?>>Timezone</label>
										<select name="tzone" id="tzone" tabindex="7">
											<?php
												$sql_timezone = "SELECT tz_code, tz_name, tz_value2   
																	FROM user_timezone 
																	ORDER BY tz_value ASC;";
												$query_timezone = mysql_query($sql_timezone);
												while ($result_timezone = mysql_fetch_array($query_timezone)) {
													echo '<option value="'.$result_timezone['tz_code'].'"';
													if (isset($_POST['tzone']) && $_POST['tzone'] != "" && $_POST['tzone'] == $result_timezone['tz_code']) {
														echo ' selected="selected"'; 
													}
													else if ((!isset($_POST['tzone']) || (isset($_POST['tzone']) && $_POST['tzone'] == "")) && $result_timezone['tz_code'] == $result_user_profil['user_timezone']) {
														echo ' selected="selected"'; 
													}
													echo '>'.$result_timezone['tz_value2'].' '.$result_timezone['tz_name'].'</option>';
												}
											?>
										</select>									
									</div>
									<div class="login-form">
										<label for="userwebsite">Private Website *</label>
										<input name="userwebsite" type="text" class="input" tabindex="8" id="userwebsite" value="<?php 
											if (isset($_POST['userwebsite']) && $_POST['userwebsite'] != "") {
												echo $_POST['userwebsite'];
											}
											else {
												echo $result_user_profil['user_website'];
											}
										?>" size="50" maxlength="50" />
									</div>
									<div class="login-form">
										<label for="useroccupation">Occupation *</label>
										<input name="useroccupation" type="text" class="input" tabindex="9" id="useroccupation" value="<?php 
											if (isset($_POST['useroccupation']) && $_POST['useroccupation'] != "") {
												echo $_POST['useroccupation'];
											}
											else {
												echo $result_user_profil['user_occupation'];
											}
										?>" size="50" maxlength="50" />
									</div>
							 		<div class="login-form">
										<label for="useroccupation">Location *</label>
										<a href="<?php echo $roscms_intern_path_server; ?>peoplemap/" target="_blank" style="color:#333333 !important; text-decoration:underline; font-weight:bold;">My Location on the Map</a>
									</div> 
									<div class="login-options">
										<fieldset>
											<legend style="color:#817A71;margin-bottom: 10px;">&nbsp;Login Settings&nbsp;</legend>
											<input name="loginoption1" type="checkbox" id="loginoption1" value="true" <?php 
												if (isset($_POST['loginoption1']) || (!isset($_POST['registerpost']) && $result_user_profil['user_setting_multisession'] == "true")) {
													echo 'checked';
												}
											?> tabindex="11" />
											<label for="loginoption1">Multisession</label>
											<br />
											<input name="loginoption2" type="checkbox" id="loginoption2" value="true" <?php 
												if (isset($_POST['loginoption2']) || (!isset($_POST['registerpost']) && $result_user_profil['user_setting_browseragent'] == "true")) {
													echo 'checked';
												}
											?> tabindex="12" /> 
											<label for="loginoption2">Browser Agent Check</label>
											<br />
											<input name="loginoption3" type="checkbox" id="loginoption3" value="true" <?php 
												if (isset($_POST['loginoption3']) || (!isset($_POST['registerpost']) && $result_user_profil['user_setting_ipaddress'] == "true")) {
													echo 'checked';
												}
											?> tabindex="13" /> 
											<label for="loginoption3">IP Address Check</label>
											<br />
											<input name="loginoption4" type="checkbox" id="loginoption4" value="true" <?php 
												if (isset($_POST['loginoption4']) || (!isset($_POST['registerpost']) && $result_user_profil['user_setting_timeout'] == "true")) {
													echo 'checked';
												}
											?> tabindex="14" /> 
											<label for="loginoption4">Log me on automatically</label>
										</fieldset>
									</div>
									<div class="login-button">
										<input type="submit" name="submit" value="Save" tabindex="16" />
										<input type="button" onclick="window.location='<?php echo $roscms_SET_path_ex; ?>'" tabindex="17" value="Cancel" name="cancel" style="color:#777777;" />
										<input name="registerpost" type="hidden" id="registerpost" value="reg" />
									</div>
								<?php
									}
								?>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>
	<?php
		if ($message != "") {
	?>
		<div style="background: #FAD163 none repeat scroll 0%; width: 300px; margin:10px;">
			<div class="corner1">
				<div class="corner2">
					<div class="corner3">
						<div class="corner4">
							<div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
								<b><?php echo $message; ?></b>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>
	<?php
		}
	?>
	</div>	
</form>

