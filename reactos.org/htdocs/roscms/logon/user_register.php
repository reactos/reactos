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
	$existname = false; // username already exists in the database (true = username exists)
	$existemail = false; // email already exists in the database (true = email exists)
	$safename = ""; // protected username ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)
	$safepwd = ""; // unsafe password, common cracked passwords ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)
	


?>
<h1>Register to <?php echo $rdf_name; ?></h1>
<div class="u-h1">Register to <?php echo $rdf_name; ?></div>
<span class="u-h2">Become a member of the <?php echo $rdf_name; ?> Community. </span>  The  <span class="u-h2"><?php echo $rdf_logon_system_name; ?></span> account offers single sign-on for all <span class="u-h2"><?php echo $rdf_name; ?></span> web services.
<ul>
	<li>Already a member? <a href="<?php echo $roscms_SET_path_ex; ?>login/">Login now</a>! </li>
	<li><a href="<?php echo $roscms_SET_path_ex; ?>login/lost/">Lost username or password?</a></li>
</ul>

<form action="<?php $roscms_SET_path_ex."register/"; ?>" method="post">
	<div align="center">
		<div style="background: <?php echo $rdf_color_logon_background; ?> none repeat scroll 0%; width: 300px;">
			<div class="corner1">
				<div class="corner2">
					<div class="corner3">
						<div class="corner4">
							<div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
								<?php
									if (isset($_POST['registerpost']) && $_POST['username'] != "" && strlen($_POST['username']) >= $rdf_register_user_name_min) {
										// check if another account with the same username already exists
										$sql_exist_name = "SELECT user_name 
															FROM users 
															WHERE REPLACE(user_name, '_', ' ') = REPLACE('" . mysql_real_escape_string($_POST['username'])."', '_', ' ')
															LIMIT 1;";
										$query_exist_name = mysql_query($sql_exist_name);
										$result_exist_name = mysql_fetch_array($query_exist_name);
										
										if ($result_exist_name['user_name'] != "") {
											$existname = true;
										}
										
										// check if the username is equal to a protected name
										$sql_unsafe_name = "SELECT unsafe_name 
															FROM user_unsafenames 
															WHERE unsafe_name = '".mysql_real_escape_string($_POST['username'])."'
															LIMIT 1;";
										$query_unsafe_name = mysql_query($sql_unsafe_name);
										$result_unsafe_name = mysql_fetch_array($query_unsafe_name);
										
										if ($result_unsafe_name['unsafe_name'] == "") {
											// check if the username contains part(s) of a protected name
											$sql_unsafe_name2 = "SELECT unsafe_name 
																	FROM user_unsafenames
																	WHERE unsafe_part = 1;";
											$query_unsafe_name2 = mysql_query($sql_unsafe_name2);
											while ($result_unsafe_name2 = mysql_fetch_array($query_unsafe_name2)) {
												$pos = strpos(strtolower($_POST['username']), $result_unsafe_name2['unsafe_name']);
												
												// Note our use of ===.  Simply == would not work as expected
												// because the position of 'a' was the 0th (first) character.
												if ($pos !== false) {
													$safename = "false";
													break;
												}
											}
											if ($safename == "") {
												$safename = "true";
											}
										}
										else {
											$safename = "false";
										}
									}
								
									if (isset($_POST['registerpost']) && 
										isset($_POST['userpwd1']) && $_POST['userpwd1'] != "" &&
										isset($_POST['userpwd2']) && $_POST['userpwd2'] != "" &&
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
											$existemail = true;
										}
									}
																		
									if (isset($_POST['registerpost']) && 
										isset($_POST['username']) && $_POST['username'] != "" && substr_count($_POST['username'], ' ') < 4 &&
										strlen($_POST['username']) >= $rdf_register_user_name_min && strlen($_POST['username']) < $rdf_register_user_name_max && 
										isset($_POST['userpwd1']) && $_POST['userpwd1'] != "" && isset($_POST['userpwd2']) && $_POST['userpwd2'] != "" && 
										strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max &&
										$_POST['userpwd1'] == $_POST['userpwd2'] &&
										isset($_POST['useremail']) && $_POST['useremail'] != "" && 
										preg_match($rdf_register_valid_email_regex, $_POST['useremail']) && /* check if it's a valid email address */
										isset($_POST['usercaptcha']) && $_POST['usercaptcha'] != "" &&
										!empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && 
										$safename == "true" && $safepwd == "true" && !$existname && !$existemail)
									{										
										// user language (browser settings)
										$userlang = check_lang($_SERVER["HTTP_ACCEPT_LANGUAGE"]);
										
										if(!$userlang)
											$userlang = "en";
										
										// account activation code
										$s = "";
										for ($n=0; $n<20; $n++) {
											$s .= chr(rand(0, 255));
										}
										$s = base64_encode($s);   // base64-set, but filter out unwanted chars
										$s = preg_replace("/[+\/=IG0ODQRtl]/i", "", $s);  // strips hard to discern letters, depends on used font type
										//$s = substr($s, 0, $characters);
										$s = substr($s, 0, rand(10, 15));
										$account_act_code = $s;
										
										// add new account
										$sql_new_account = "INSERT INTO users ( user_name , 
																				user_roscms_password ,
																				user_register ,
																				user_register_activation ,
																				user_email , 
																				user_language
															) 
															VALUES (
																'".mysql_real_escape_string($_POST['username'])."', 
																MD5( '".mysql_real_escape_string($_POST['userpwd1'])."' ) , 
																NOW( ) , 
																'".mysql_real_escape_string($account_act_code)."', 
																'".mysql_real_escape_string($_POST['useremail'])."', 
																'".mysql_real_escape_string($userlang)."');";
										$insert_new_account = mysql_query($sql_new_account);


										require("inc/subsys_utils.php");
										
										$sql_user_id_new = "SELECT user_id  
															FROM users 
															WHERE user_name = '".mysql_real_escape_string($_POST['username'])."'
															ORDER BY user_id DESC 
															LIMIT 1";
										$query_user_id_new = mysql_query($sql_user_id_new);
										$result_user_id_new = mysql_fetch_array($query_user_id_new);

										$reg_account_post2="INSERT INTO usergroup_members (usergroupmember_userid, usergroupmember_usergroupid) 
															VALUES ('". mysql_escape_string($result_user_id_new["user_id"]) ."', 'user');";
										$reg_account_list2=mysql_query($reg_account_post2);

										subsys_update_user($result_user_id_new["user_id"]);


										
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
										$subject = $rdf_name_long." - Account Activation";
										
										/* message */
										$message = $rdf_name_long." - Account Activation\n\n\nYou have registered an account on ".$rdf_name.". The next step in order to enable the account is to activate it by using the hyperlink below.";
										$message .= "\n\nYou haven't registered an account? Oops, then someone has tried to register an account with your email address. Just ignore this email, no one can use it anyway as it is not activated and the account will get deleted soon.";
										$message .= "\n\n\nUsername: ".$_POST['username']."\nPassword: ".$_POST['userpwd1']."\n\nActivation-Hyperlink: ".$roscms_SET_path_ex."login/activate/".$account_act_code."/";
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
										
										//echo "<p>SQL:<br />$sql_new_account</p>";
										//echo "<p>MSG:<br />$message</p>";
										
										echo '<div class="login-title">Account registered</div>';
										echo "<div>Check your email inbox (and spam folder) for the <b>account activation email</b> that contains the activation hyperlink.</div>";
										
										$message = "";
										unset($_SESSION['rdf_security_code']);
									}
									else {
								
								?>
									<div class="login-title">Register Account</div>
									<div class="login-form">
										<label for="username"<?php if (isset($_POST['registerpost']) && (strlen($_POST['username']) < $rdf_register_user_name_min || strlen($_POST['username']) > $rdf_register_user_name_max || $safename == "false"  || substr_count($_POST['username'], ' ') >= 4 || $existname)) { echo ' style="color:#FF0000;"'; } ?>>Username</label>
										<input name="username" type="text" class="input" tabindex="1" id="username" <?php 
											if (isset($_POST['username'])) {
												echo 'value="' . $_POST['username'] .  '"';
											}
										?> size="50" maxlength="50" />
										<br /><span style="color:#817A71;">uppercase letters, lowercase letters, numbers, and symbols (ASCII characters)</span>
										<?php 
											if (isset($_POST['registerpost'])) {
												if (strlen($_POST['username']) < $rdf_register_user_name_min || $existname || $safename == "false" || strlen($_POST['username']) > $rdf_register_user_name_max || substr_count($_POST['username'], ' ') >= 4) {
													echo "<br /><i>Please try another username with at least ".$rdf_register_user_name_min." characters.</i>";
												}
											}
										?>
									</div>
									<div class="login-form">
										<label for="userpwd1"<?php if (isset($_POST['registerpost'])) { echo ' style="color:#FF0000;"'; } ?>>Password</label>
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
									<div class="login-form">
										<label for="useremail"<?php if (isset($_POST['registerpost']) && ($_POST['useremail'] == "" || $existemail || !preg_match($rdf_register_valid_email_regex, $_POST['useremail']))) { echo ' style="color:#FF0000;"'; } ?>>E-Mail</label>
										<input name="useremail" type="text" class="input" tabindex="4" id="useremail" <?php 
											if (isset($_POST['useremail'])) {
												echo 'value="' . $_POST['useremail'] .  '"';
											}
										?> size="50" maxlength="50" />
										<?php
											if (isset($_POST['registerpost']) && $existemail) {
												echo '<br /><i>That email address is already with an account. Please <a href="'.$roscms_SET_path_ex.'login/" style="color:#FF0000 !important; text-decoration:underline;"><b>login</b></a>!</i>';
											}
										?>
									</div>
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
									<?php 
										/*
											<div class="register-accept">
												<label for="submit">Clicking <strong>I accept</strong> means that you agree <br />
												to the <a href="#"><?php echo $rdf_name; ?> Service Agreement</a> <br />
												and <a href="#">Privacy Statement</a>.</label>
											</div>
										*/ ?>
									<div class="login-button">
										<input type="submit" name="submit" value="Register" tabindex="8" />
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

