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
	$existactcode = ""; // pwd-id exists in the database (true = pwd-id exists)


?>
<h1>Activate myReactOS Account</h1>
<div class="u-h1">Activate myReactOS Account</div>
<div class="u-h2">Already a member? <a href="<?php echo $roscms_SET_path_ex; ?>login/">Login now</a>! <br />
	Don't have a <?php $rdf_logon_system_name; ?> account yet? <a href="<?php echo $roscms_SET_path_ex; ?>register/">Join now</a>, it's free and just takes a minute.</div>
<form action="<?php $roscms_SET_path_ex."login/activate/"; ?>" method="post">
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
											//echo "email fine".$result_exist_email['user_email'];
										}
										else {
											//echo "email error".$sql_exist_email;
										}
									}
									if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6)
									{
										// check if an account with the pwd-id exists
										$sql_exist_pwdid = "SELECT user_id  
															FROM users 
															WHERE user_register_activation = '".mysql_real_escape_string($rdf_uri_3)."'
															LIMIT 1;";
										$query_exist_pwdid = mysql_query($sql_exist_pwdid);
										$result_exist_pwdid = mysql_fetch_array($query_exist_pwdid);
										
										if ($result_exist_pwdid['user_id'] != "") {
											$existactcode = "true";
											//echo "pwd-id fine".$result_exist_pwdid['user_id'];
										}
										else {
											$existactcode = "false";
											echo '<div class="login-title">Invalid Code</div>';
											$message = 'Nothing for you to see here. <br />Please move along.';
											//echo "pwd-id error";//.$sql_exist_pwdid;
										}
									}
									else {
										$existactcode = "false";
										echo '<div class="login-title">Invalid Code</div>';
										$message = 'Nothing for you to see here. <br />Please move along.';
									}
									
									if ($rdf_uri_3 != "" && strlen($rdf_uri_3) > 6 && 
										isset($_POST['registerpost']) && 
										isset($_POST['useremail']) && $_POST['useremail'] != "" && 
										preg_match($rdf_register_valid_email_regex, $_POST['useremail']) && /* check if it's a valid email address */
										$existactcode == "true" && $existemail)
									{
										$sql_exist_pwdid2 = "SELECT user_id  
															FROM users 
															WHERE user_register_activation = '".mysql_real_escape_string($rdf_uri_3)."'
															LIMIT 1;";
										$query_exist_pwdid2 = mysql_query($sql_exist_pwdid2);
										$result_exist_pwdid2 = mysql_fetch_array($query_exist_pwdid2);

										// set new account password
										$sql_new_pwd = "UPDATE users 
															SET user_register_activation = '',
															user_account_enabled = 'yes',
															user_timestamp_touch2 = NOW( ) 
															WHERE user_id = ".mysql_real_escape_string($result_exist_pwdid2['user_id'])." 
															LIMIT 1;";
										$update_new_pwd = mysql_query($sql_new_pwd);
										
										echo '<div class="login-title">Account activated</div>';
										echo '<div><a href="'.$roscms_SET_path_ex.'login/" style="color:#FF0000 !important; text-decoration:underline;">Login now</a>!</div>';
									}							
									else if ($existactcode == "true") {
										echo '<div class="login-title">Activate your Account</div>';
								?>
									</div>
									<div class="login-form">
										<label for="useremail"<?php if (isset($_POST['registerpost']) && ($_POST['useremail'] == "" || !$existemail || !preg_match($rdf_register_valid_email_regex, $_POST['useremail']))) { echo ' style="color:#FF0000;"'; } ?>>Your E-Mail Address</label>
										<input name="useremail" type="text" class="input" tabindex="4" id="useremail" <?php 
											if (isset($_POST['useremail'])) {
												echo 'value="' . $_POST['useremail'] .  '"';
											}
										?> size="50" maxlength="50" />
									</div>
									<div class="login-button">
										<input type="submit" name="submit" value="Activate Account" tabindex="8" /><br />
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

