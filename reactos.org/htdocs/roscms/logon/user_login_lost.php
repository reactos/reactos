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
	
	$message = ""; // error message box text
	$existemail = false; // email already exists in the database (true = email exists)
	$existpwdid = ""; // pwd-id exists in the database (true = pwd-id exists)
	$safepwd = ""; // unsafe password, common cracked passwords ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)


if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6) {
?>
	<h1><a href="<?php echo $roscms_SET_path_ex; ?>login/">Login</a> &gt; Reset your Password</h1>
	<div class="u-h1">Reset your Password</div>
	<div class="u-h2">Have you forgotten your password of your <?php echo $rdf_name; ?> account? Don't panic. You have already requested us that we reset your password. Now it's your turn to enter a new password for your <?php echo $rdf_name; ?> account.</div>
<?php
}
else {
?>
	<h1><a href="<?php echo $roscms_SET_path_ex; ?>login/">Login</a> &gt; Lost Username or Password?</h1>
	<div class="u-h1">Lost Username or Password?</div>
	<div class="u-h2">Have you forgotten your username and/or password of your <?php echo $rdf_name; ?> account? Don't panic. We can send you your username and let you reset your password. All you need is your email address.</div>
<?php
}
?>
<form action="<?php $roscms_SET_path_ex."login/lost/"; ?>" method="post">
	<div align="center">
		<div style="background: <?php echo $rdf_color_logon_background; ?> none repeat scroll 0%; width: 300px;">
			<div class="corner1">
				<div class="corner2">
					<div class="corner3">
						<div class="corner4">
							<div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
								<?php
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
											$existemail = true;
										}
									}
									if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6)
									{
										// check if an account with the pwd-id exists
										$sql_exist_pwdid = "SELECT user_id  
															FROM users 
															WHERE user_roscms_getpwd_id = '".mysql_real_escape_string($rdf_uri_3)."'
															LIMIT 1;";
										$query_exist_pwdid = mysql_query($sql_exist_pwdid);
										$result_exist_pwdid = mysql_fetch_array($query_exist_pwdid);
										
										if ($result_exist_pwdid['user_id'] != "") {
											$existpwdid = "true";
										}
										else {
											$existpwdid = "false";
											echo '<div class="login-title">Invalid Code</div>';
											$message = "Nothing for you to see here. <br />Please move along.";
										}
									}
									
									if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6 && 
										isset($_POST['registerpost']) && 
										isset($_POST['userpwd1']) && $_POST['userpwd1'] != "" && isset($_POST['userpwd2']) && $_POST['userpwd2'] != "" && 
										strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max &&
										$_POST['userpwd1'] == $_POST['userpwd2'] &&
										isset($_POST['usercaptcha']) && $_POST['usercaptcha'] != "" &&
										!empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) &&										
										$existpwdid == "true")
									{
										$sql_exist_pwdid2 = "SELECT user_id 
															FROM users 
															WHERE user_roscms_getpwd_id = '".mysql_real_escape_string($rdf_uri_3)."'
															LIMIT 1;";
										$query_exist_pwdid2 = mysql_query($sql_exist_pwdid2);
										$result_exist_pwdid2 = mysql_fetch_array($query_exist_pwdid2);

										// set new account password
										$sql_new_pwd = "UPDATE users 
															SET user_roscms_getpwd_id = '',
															user_roscms_password = MD5( '".mysql_real_escape_string($_POST['userpwd1'])."' ),
															user_timestamp_touch2 = NOW( ) 
															WHERE user_id = ".mysql_real_escape_string($result_exist_pwdid2['user_id'])." 
															LIMIT 1;";
										$update_new_pwd = mysql_query($sql_new_pwd);
										
										echo '<div class="login-title">Password changed</div>';
										echo '<div><a href="'.$roscms_SET_path_ex.'login/" style="color:#FF0000 !important; text-decoration:underline;">Login now</a>!</div>';
									}							
									else if (($rdf_uri_3 == "" || strlen($rdf_uri_3) < 6) &&
										isset($_POST['registerpost']) && 
										isset($_POST['useremail']) && $_POST['useremail'] != "" && 
										preg_match($rdf_register_valid_email_regex, $_POST['useremail']) && /* check if it's a valid email address */
										isset($_POST['usercaptcha']) && $_POST['usercaptcha'] != "" &&
										!empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && 
										$existemail)
									{
										// password activation code
										$s = "";
										for ($n=0; $n<20; $n++) {
											$s .= chr(rand(0, 255));
										}
										$s = base64_encode($s);   // base64-set, but filter out unwanted chars
										$s = preg_replace("/[+\/=IG0ODQRtl]/i", "", $s);  // strips hard to discern letters, depends on used font type
										//$s = substr($s, 0, $characters);
										$s = substr($s, 0, rand(10, 15));
										$account_act_code = $s;

										$sql_exist_email2 = "SELECT user_id, user_name  
															FROM users 
															WHERE user_email = '".mysql_real_escape_string($_POST['useremail'])."'
															LIMIT 1;";
										$query_exist_email2 = mysql_query($sql_exist_email2);
										$result_exist_email2 = mysql_fetch_array($query_exist_email2);
										
										// add pwd-id to account
										$sql_lost_pwd = "UPDATE users 
															SET user_roscms_getpwd_id = '".mysql_real_escape_string($account_act_code)."',
															user_timestamp_touch2 = NOW( ) 
															WHERE user_id = ".mysql_real_escape_string($result_exist_email2['user_id'])." 
															LIMIT 1;";
										$update_lost_pwd = mysql_query($sql_lost_pwd);

										
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
										$subject = $rdf_name_long." - Lost username or password?";
										
										/* message */
										$message = $rdf_name_long." - Lost username or password?\n\n\nYou have requested your ".$rdf_name." account login data.";
										$message .= "\n\nYou haven't requested your account login data? Oops, then someone has tried the 'Lost username or password?' function with your email address, just ignore this email.";
										$message .= "\n\n\nUsername: ".$result_exist_email2['user_name']."\n\n\nLost your password? Reset your password: ".$roscms_SET_path_ex."login/lost/".$account_act_code."/";
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
										
										
										echo '<div class="login-title">Account login data sent</div>';
										echo "<div>Check your email inbox (and spam folder) for the email that contains your account login data.</div>";
										
										$message = "";
										unset($_SESSION['rdf_security_code']);
									}
									else if ($existpwdid == "" || $existpwdid == "true") {
										if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6) {
											echo '<div class="login-title">Reset your Password</div>';
										}
										else {
											echo '<div class="login-title">Lost Username or Password?</div>';
										}
								?>
									</div>
									<?php
										if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6) {
									?>
										<div class="login-form">
											<label for="userpwd1"<?php if (isset($_POST['registerpost'])) { echo ' style="color:#FF0000;"'; } ?>>New Password</label>
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
											<label for="userpwd2"<?php if (isset($_POST['registerpost'])) { echo ' style="color:#FF0000;"'; } ?>>Re-type Password</label>
											<input name="userpwd2" type="password" class="input" tabindex="3" id="userpwd2" size="50" maxlength="50" />
										</div>
									<?php
										}
										else {
									?>
										<div class="login-form">
											<label for="useremail"<?php if (isset($_POST['registerpost']) && ($_POST['useremail'] == "" || !preg_match($rdf_register_valid_email_regex, $_POST['useremail']))) { echo ' style="color:#FF0000;"'; } ?>>E-Mail</label>
											<input name="useremail" type="text" class="input" tabindex="4" id="useremail" <?php 
												if (isset($_POST['useremail'])) {
													echo 'value="' . $_POST['useremail'] .  '"';
												}
											?> size="50" maxlength="50" />
										</div>
									<?php
										}
									?>
									<div class="login-form">
										<label for="usercaptcha"<?php if (isset($_POST['registerpost'])) { echo ' style="color:#FF0000;"'; } ?>>Type the code shown</label>
										<input name="usercaptcha" type="text" class="input" tabindex="7" id="usercaptcha" size="50" maxlength="50" />
										
										<script type="text/javascript">
											var BypassCacheNumber = 0;
											
											function CaptchaReload()
											{
												++BypassCacheNumber;
												document.getElementById("captcha").src = "<?php echo $roscms_SET_path_ex; ?>register/captcha/?" + BypassCacheNumber;
											}
											
											document.write('<br /><span style="color:#817A71;">If you can\'t read this, try <a href="javascript:CaptchaReload()">another one</a>.</span>');
										</script>
										
										<img id="captcha" src="<?php echo $roscms_SET_path_ex; ?>register/captcha/" style="padding-top:10px;" alt="If you can't read this, try another one or email <?php echo $rdf_support_email_str; ?> for help." title="Are you human?" /><br />
										<?php 
											if (isset($_POST['registerpost'])) { 
												echo "<br /><i>Captcha code is case insensitive. <br />If you can't read it, try another one.</i>";
											}
										?>
									</div>
									<div class="login-button">
										<input type="submit" name="submit" value="Send" tabindex="8" />
										<input type="button" onclick="window.location='<?php echo $roscms_SET_path_ex; ?>'" tabindex="9" value="Cancel" name="cancel" style="color:#777777;" />
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

