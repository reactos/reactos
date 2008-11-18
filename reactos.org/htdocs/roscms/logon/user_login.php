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
	
	//die("ha");
	//require_once("logon_utils.php");
	require_once("subsys_login.php");
	
	global $roscms_SET_path_ex;
	global $rdf_login_cookie_usrkey;
	global $rdf_login_cookie_seckey;
	global $rdf_login_cookie_usrname;
	global $rdf_login_cookie_usrpwd;
	global $rdf_login_cookie_loginname;


	
function create_login_page($message)
{
	//die("ha");

	global $rpm_page_title;
	global $rpm_logo;
	global $roscms_langres;
	global $rpm_page;
	
	global $roscms_SET_path_ex;
	global $rdf_name;
	global $rdf_login_cookie_usrkey;
	global $rdf_login_cookie_seckey;
	global $rdf_login_cookie_usrname;
	global $rdf_login_cookie_usrpwd;
	global $rdf_login_cookie_loginname;
	global $rdf_color_logon_background;

	$rpm_sec="";
	if (array_key_exists("sec", $_GET)) $rpm_sec=htmlspecialchars($_GET["sec"]);


	$random_string="";
	$random_string_security="";

	if ($rpm_sec == "security") { 
		$random_string_security = make_key();
		setcookie($rdf_login_cookie_seckey, $random_string_security, 0, "/", cookie_domain());
		echo '<script language="javascript" src="js/md5.js"></script>';
	} 
	else {
		$random_string_security = "";
		setcookie($rdf_login_cookie_seckey, $random_string_security, time() - 3600, "/", cookie_domain());
	}

	create_header("", "logon");
	require("logon/user_profil_menubar.php");


	$target_clean = '';
	if (isset($_REQUEST['target'])) {
		$target_tainted = $_REQUEST['target'];
		if (preg_match('/^(\/[a-zA-Z0-9!$%&,\'()*+\-.\/:;=?@_~]+)$/',
		    $target_tainted, $matches)) {
			$target_clean = $matches[1];
		}
	}
?>
<form action="<?php $roscms_SET_path_ex."login/"; ?>" method="post">
    <?php
    if ($target_clean != '' ) {
      echo '<input type="hidden" name="target" value="'.$target_clean.'" />';
    }
    ?>
	<h1>Login</h1>
	<div class="u-h1">Login to <?php echo $rdf_name; ?></div>
	<div class="u-h2">You don't have a <?php echo $rdf_name; ?> account yet? <a href="<?php echo $roscms_SET_path_ex; ?>register/">Join now</a>, it's free and just takes a minute.</div>
	<div align="center">
		<div style="background: <?php echo $rdf_color_logon_background; ?> none repeat scroll 0%; width: 300px;">
			<div class="corner1">
				<div class="corner2">
					<div class="corner3">
						<div class="corner4">
							<div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
								<div class="login-title"><?php if ($rpm_sec == "security") { echo "Secure "; } ?>Login</div>
								<div class="login-form">
									<label for="<?php echo $rdf_login_cookie_usrname; ?>">Username</label>
									<input name="<?php echo $rdf_login_cookie_usrname; ?>" type="text" class="input" tabindex="1" id="<?php echo $rdf_login_cookie_usrname; ?>" <?php 
										if (isset($_POST[$rdf_login_cookie_usrname])) {
											echo 'value="' . $_POST[$rdf_login_cookie_usrname] .  '"';
										}
										else if (isset($_COOKIE[$rdf_login_cookie_loginname])) {
											echo 'value="' . $_COOKIE[$rdf_login_cookie_loginname] .  '"';
										}
									?> size="50" maxlength="50" />
								</div>
								<div class="login-form">
									<label for="<?php echo $rdf_login_cookie_usrpwd; ?>">Password</label>
									<input name="<?php echo $rdf_login_cookie_usrpwd; ?>" type="password" class="input" tabindex="2" id="<?php echo $rdf_login_cookie_usrpwd; ?>" size="50" maxlength="50" />
								</div>
								<?php
									if ($rpm_sec == "" || $rpm_sec == "standard") {
								?>
									<div class="login-options">
										<input name="loginoption1" type="checkbox" id="loginoption1" value="save" <?php 
										if (isset($_COOKIE[$rdf_login_cookie_loginname])) {
										echo 'checked';
										}
										?> tabindex="3" />
										<label for="loginoption1">Save username</label>
										<br />
										<input name="loginoption2" type="checkbox" id="loginoption2" value="notimeout" tabindex="4" /> 
										<label for="loginoption2">Log me on automatically</label>
									</div>
								<?php
									}
								?>
								<div class="login-button">
									<input type="submit" name="submit" value="Login" class="button" tabindex="5" <?php if ($rpm_sec == "security") { echo 'onclick="'.$rdf_login_cookie_usrpwd.'.value = calcMD5(\''.$random_string_security.'\' + calcMD5('.$rdf_login_cookie_usrpwd.'.value))"'; } ?> />
									<input name="logintype" type="hidden" id="logintype" value="<?php if ($rpm_sec == "security") { echo 'security'; } else { echo 'standard'; } ?>" />
								</div>
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
		<div style="margin:10px"><?php
			if ($rpm_sec == "" || $rpm_sec == "standard") {
				echo '<a href="'.$roscms_SET_path_ex.'login/?sec=security';
				if ($target_clean != '') {
					echo '&amp;target=' . urlencode($target_clean);
				}
				echo '">Use enhanced security</a>';
			}
			else {
				echo '<a href="'.$roscms_SET_path_ex.'login/?sec=standard';
				if ($target_clean != '') {
					echo '&amp;target=' . urlencode($target_clean);
				}
				echo '">Use standard security</a>';
			}
		?></div>
		<a href="<?php echo $roscms_SET_path_ex; ?>login/lost/">Lost username or password?</a>
	</div>	
  </form>
  <p>&nbsp;</p>

<?php
}

if ((! isset($_POST[$rdf_login_cookie_usrname]) || $_POST[$rdf_login_cookie_usrname] == "") &&
    (! isset($_POST[$rdf_login_cookie_usrpwd]) || $_POST[$rdf_login_cookie_usrpwd] == "")) {
	create_login_page("");
}
elseif(! isset($_POST[$rdf_login_cookie_usrname]) || $_POST[$rdf_login_cookie_usrname] == "") {
	create_login_page("Please enter your username!");
}
elseif(! isset($_POST[$rdf_login_cookie_usrpwd]) || $_POST[$rdf_login_cookie_usrpwd] == "") {
	create_login_page("Please enter your password");
}
else { // login process
	$roscms_currentuser_id = "";
	$roscms_ses_id_key="";
	if (isset($_COOKIE[$rdf_login_cookie_seckey]) &&
	    preg_match('/^([a-z0-9]{32})$/', $_COOKIE[$rdf_login_cookie_seckey], $matches)) {
		$roscms_ses_id_key = $matches[1];
	}
	else {
		$roscms_ses_id_key="";
	}
	if ($roscms_ses_id_key == "") {
		$random_string = make_key();
	}
	else {
		$random_string = $roscms_ses_id_key;
	}
		
	/* Check username. It should only contain printable ASCII chars */
	if (preg_match('/^([ !-~]+)$/', $_POST[$rdf_login_cookie_usrname], $matches)) {
		$rdfusername = $matches[1];
	}
	else {
		create_login_page("You have specified an incorrect or inactive username, or an invalid password.");
		exit;
	}
		
	/* Check password. It should only contain printable ASCII chars */
	if (preg_match('/^([ !-~]+)$/', $_POST[$rdf_login_cookie_usrpwd], $matches)) {
		$rdfpwd = $matches[1];
	}
	else {
		create_login_page("You have specified an incorrect or inactive username, or an invalid password.");
		exit;
	}
	
	$query = mysql_query("SELECT * " .
	                     "  FROM users " .
	                     " WHERE user_name = '" .
                                     mysql_escape_string($rdfusername) .
                                     "'")
	         or die('DB error (user login #1)!');
	$result = mysql_fetch_array($query); 
	$roscms_currentuser_id = $result['user_id'];

	$pwdtemp = $result['user_roscms_password'];
	
	if ($roscms_ses_id_key != "") {
		// we simply concatenate the password and random key to create
		// a unique session md5 hash
		// hmac functions are not available on most web servers, but
		// this is near as dammit.
		$pwdtemp = md5($random_string . $pwdtemp);
		$apassword = $rdfpwd;
	}
	else {
		$apassword = md5($rdfpwd);
	}
	
	if ($apassword != $pwdtemp) {
		create_login_page("You have specified an incorrect or inactive username, or an invalid password.");
		exit;
	}

	$rem_adr = $_SERVER['REMOTE_ADDR'];
	$useragent = $_SERVER['HTTP_USER_AGENT'];
	
	// Query DB table 'users' and read the login_counter and settings of
	// the specific user
	$query = "SELECT user_login_counter, " .
	         "       user_account_enabled, " .
		 "       user_setting_multisession, " .
		 "       user_setting_browseragent, " .
	         "       user_setting_ipaddress, " .
	         "       user_setting_timeout " .
	         "  FROM users  " .
	         " WHERE user_id = $roscms_currentuser_id";
	$login_usr_keya_query = mysql_query($query)
	                        or die('DB error (user login #2)!');
	$login_usr_keya_result = mysql_fetch_array($login_usr_keya_query);
	
	$roscms_currentuser_login_counter = $login_usr_keya_result['user_login_counter'];
	$roscms_currentuser_login_user_account_enabled = $login_usr_keya_result['user_account_enabled'];
	$roscms_currentuser_login_user_setting_multisession = $login_usr_keya_result['user_setting_multisession'];
	$roscms_currentuser_login_user_setting_browseragent = $login_usr_keya_result['user_setting_browseragent'];
	$roscms_currentuser_login_user_setting_ipaddress = $login_usr_keya_result['user_setting_ipaddress'];

	// if the account is NOT enabled; e.g. a reason could be that a member
	// of the admin group has disabled this account because of spamming,
	// etc.
	if ($roscms_currentuser_login_user_account_enabled != "yes") { 
		create_login_page("Account is not activated or disabled!<br /><br />\n" .
		                  "Check your email inbox (and spam folder), maybe you have overseen the activation information.\n"
		                  /* ."<br /><i>System message: " .
		                  $roscms_currentuser_login_user_account_enabled . 
						  "</i>"*/);
		exit;
	}
	
	// if the user account setting is "multisession" (a by user setting),
	// it is set to "false" by default
	if ($roscms_currentuser_login_user_setting_multisession != "true") { // for security reasons here stand "!= true" instead of "== false"
		$query = "SELECT COUNT('usersession_user_id') " .
		         "  FROM user_sessions " .
		         " WHERE usersession_user_id = " .
		                 $roscms_currentuser_id;
		
		$login_usr_session_count_query = mysql_query($query)
		                                 or die('DB error (user login #3)!');
		$login_usr_session_count_result = mysql_fetch_array($login_usr_session_count_query);
		$roscms_currentuser_login_user_lastsession_counter = $login_usr_session_count_result[0];

		if ($roscms_currentuser_login_user_lastsession_counter > 0) {
			/*create_login_page("Your account settings only allow you to login once.<br>\n" .
			                  "You are already logged in so you cannot login again. <br>\n");
			exit;*/
			
			$query = "DELETE FROM user_sessions 
						WHERE usersession_user_id ='".mysql_escape_string($roscms_currentuser_id)."';";
			mysql_query($query);
		}
	}

	// At this point, we've passed all checks and we have a valid login
	// Check if there's an existing session, if so, end that session
	if (0 != roscms_subsys_login("roscms", ROSCMS_LOGIN_OPTIONAL, "")) {
		$query = "DELETE FROM user_sessions " .
		         " WHERE usersession_id = '" .
                                 mysql_escape_string($_COOKIE[$rdf_login_cookie_usrkey]) . "'";
		mysql_query($query);
	}
	
	// save username
	if (isset($_POST['loginoption1']) && $_POST['loginoption1'] == "save") {
		// save username (cookie)
		setcookie($rdf_login_cookie_loginname, $rdfusername,
		          time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
	}
	else {
		// delete username (cookie)
		setcookie($rdf_login_cookie_loginname, '',
		          time() - 3600, '/', cookie_domain());
	}

	if (isset($_POST['loginoption2']) &&
	    $_POST['loginoption2'] == "notimeout" &&
	    $login_usr_keya_result['user_setting_timeout'] == 'true') {
		$expire_value = 'NULL';
		$cookie_time = 0x7fffffff;
	}
	else {
		$expire_value = 'DATE_ADD(NOW(), INTERVAL 30 MINUTE)';
		$cookie_time = time() + 60 * 60;
	}

	// Add an entry to the 'user_sessions' table
	$login_key_post="INSERT INTO user_sessions " .
	                "       (usersession_id, " .
	                "        usersession_user_id, " .
	                "        usersession_expires, " .
	                "        usersession_browseragent, " .
	                "        usersession_ipaddress) " .
	                "VALUES ('$random_string', " .
	                "        $roscms_currentuser_id, " .
	                "        $expire_value, " .
	                "        '" .
	                         mysql_escape_string($useragent) . "', " .
	                "        '" .
	                         mysql_escape_string($rem_adr) . "')";
	$login_key_post_list = mysql_query($login_key_post)
	                       or die('DB error (user login #4)!');

	// save session_id (cookie)
	setcookie($rdf_login_cookie_usrkey, $random_string,
		  $cookie_time, '/', cookie_domain());
	
	$now = explode(' ',microtime());
	$time = $now[1].substr($now[0],2,2);
	settype($time, "double");
	
	// Update the login_counter of the specific user
	$roscms_currentuser_login_counter++;
	$login_usr_key_post = "UPDATE users " .
	                      "   SET user_timestamp_touch = '$time', " .
	                      "       user_login_counter = user_login_counter + 1 " .
	                      " WHERE user_id = $roscms_currentuser_id";
	$login_usr_key_post_list=mysql_query($login_usr_key_post);
	
	$roscms_login_next_page="my/";

	$random_string="";

	if (isset($_REQUEST['target'])) {
		header("Location: http://" . $_SERVER['HTTP_HOST'] .
                       $_REQUEST['target']);
		exit;
	}
	header("Location: ".$roscms_SET_path_ex.$roscms_login_next_page);
}

function make_key() {
	$random_string = '';
	for($i=0;$i<32;$i++) {
		// generate a good radom string
		mt_srand((double)microtime()*1000000);
		mt_srand((double)microtime()*65000*mt_rand(1,248374));
		$random_string .= chr(mt_rand(97,122)); //32,126
	} 
	return $random_string;
}

?>  
</div>
<p>&nbsp;</p>

