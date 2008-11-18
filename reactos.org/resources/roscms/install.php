<?php
	/* RosCMS install script */

	// This function was taken from the sourceStream() function of "Database.php" of MediaWiki 1.10.0 and modified
	function import_sql_dump( $file )
	{
		$ret = TRUE;
		$fp = fopen( $file, "r" );
		
		while( !feof( $fp ) )
		{
			$line = trim( fgets( $fp, 1024 ) );
			$sl = strlen( $line ) - 1;
			
			// Skip empty and comment lines
			if( $sl < 0 || ( $line{0} == "-" && $line{1} == "-" ) )
				continue;
			
			// If this lines ends with a semicolon, it terminates the command
			if( $line{$sl} == ";" )
			{
				$done = true;
				$line = substr( $line, 0, $sl );
			}
			
			$cmd .= "$line\n";
			
			if( $done )
			{
				if( !mysql_query( $cmd ) )
				{
					$ret = FALSE;
					echo "<br />" . mysql_errno() . ": " . mysql_error();
				}
				
				$done = false;
				$cmd = "";
			}
		}
		
		fclose( $fp );
		
		if( $ret )
			echo "OK";
		
		return $ret;
	}

	$rpm_ready = "";
	if (array_key_exists("ready", $_GET)) $rpm_ready=htmlspecialchars($_GET["ready"]);
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd">
<html>
<head>
	<title>RosCMS v3 - Install Script</title>
	<style type="text/css">
	<!--
	
		body {
			font-family: Verdana;
			font-size: 12px;
		}
		 
		hr {
			margin: 5px 0 5px 0;
			color: #8D8D8D;
		}
		 
		h1 {
			font-size: 24px;
			color: #5984C3;
			font-weight: bold;
		}

		h2 {
			font-size: 22px;
			color: #5984C3;
			font-weight: bold;
		}
		 
		h3 {
			font-size: 18px;
			color: #5984C3;
			font-weight: bold;
		}
		 
		a {
			font-size: 12px;
			color: #006090;
		}
		 
		a:hover {
			background-color: #DDEEFF;
			color: #000000;
		}
	-->
	</style>
</head>
<body>
<?php
	if ($rpm_ready == "step2") {
?>
	<h1>RosCMS v3 - Install Script - Step2</h1>
	<form id="form1" method="post" action="install.php?ready=step3">
		<h3>Database Information</h3>
		
		<table>
			<tr>
				<td>Database Host Name:</td>
				<td><input name="dbHost" type="text" id="dbHost" value="localhost" size="25" maxlength="255" /></td>
			</tr>
			<tr>
				<td>Database User Name:</td>
				<td><input name="dbUser" type="text" id="dbUser" value="root" size="25" maxlength="255" /></td>
			</tr>
			<tr>
				<td>Database User Password:</td>
				<td><input name="dbPass" type="text" id="dbPass" size="25" maxlength="255" /></td>
			</tr>
			<tr>
				<td>Database  Name:</td>
			  <td><input name="dbName" type="text" id="dbName" value="roscms" size="25" maxlength="255" /></td>
			</tr>
		</table>
		
		<h3>Server Information</h3>
		<p>Complete Server Path: <input name="path_server" type="text" id="path_server" value="http://localhost/domain.com/" size="50" maxlength="255" />
		</p>
		
		<h3>RosCMS Settings</h3>
	  <p>RosCMS directory :
          <input name="roscmsdir" type="text" id="roscmsdir" value="roscms" size="20" maxlength="50" />
	  </p>
		<p>Default Language: 
          <input name="lang_std" type="text" id="lang_std" value="en" size="5" maxlength="2" />
  	  </p>
		<p>Default Language Name:
          <input name="lang_std_full" type="text" id="lang_std_full" value="English" size="20" maxlength="50" />
		</p>
		<p>Default Translation Language:
          <input name="lang_std_trans" type="text" id="lang_std_trans" value="de" size="5" maxlength="2" />
		</p>

		<h3>Super Administrator Account</h3>
		<p>Account-Name: 
		  <input name="admin_name" type="text" id="admin_name" value="Admin" size="20" maxlength="50" />
		</p>
		<p>Fullname:
          <input name="admin_fullname" type="text" id="admin_fullname" value="Administrator" size="20" maxlength="50" />
</p>
		<p>Password:
          <input name="admin_pwd" type="password" id="admin_pwd" size="20" maxlength="50" />
		  <font size="1">(6 or more characters)</font></p>
		<p>E-Mail Address:
          <input name="admin_email" type="text" id="admin_email" value="admin@domain.com" size="50" maxlength="255" />
</p>
		<p>&nbsp;</p>
		<input type="submit" name="Submit" value="Install RosCMS ..." />
	</form>
<?php
	}
	else if ($rpm_ready == "step3") {
		echo "<h1>RosCMS v3 - Install Script - Step3</h1>";
		
		$dbHost = "localhost";
		$dbUser = "root";
		$dbPass = "";
		$dbName = "roscms";
		$path_server = "";
		$path_roscms = "";
		$lang_std = "";
		$lang_std_full = "";
		$lang_std_trans = "";
		$admin_name = "";
		$admin_fullname = "";
		$admin_pwd = "";
		$admin_email = "";
		
		if (array_key_exists("dbHost", $_POST)) $dbHost=htmlspecialchars($_POST["dbHost"]);
		if (array_key_exists("dbUser", $_POST)) $dbUser=htmlspecialchars($_POST["dbUser"]);
		if (array_key_exists("dbPass", $_POST)) $dbPass=htmlspecialchars($_POST["dbPass"]);
		if (array_key_exists("dbName", $_POST)) $dbName=htmlspecialchars($_POST["dbName"]);
		if (array_key_exists("path_server", $_POST)) $path_server=htmlspecialchars($_POST["path_server"]);
		if (array_key_exists("roscmsdir", $_POST)) $path_roscms=htmlspecialchars($_POST["roscmsdir"]);
		if (array_key_exists("lang_std", $_POST)) $lang_std=htmlspecialchars($_POST["lang_std"]);
		if (array_key_exists("lang_std_full", $_POST)) $lang_std_full=htmlspecialchars($_POST["lang_std_full"]);
		if (array_key_exists("lang_std_trans", $_POST)) $lang_std_trans=htmlspecialchars($_POST["lang_std_trans"]);
		if (array_key_exists("admin_name", $_POST)) $admin_name=htmlspecialchars($_POST["admin_name"]);
		if (array_key_exists("admin_fullname", $_POST)) $admin_fullname=htmlspecialchars($_POST["admin_fullname"]);
		if (array_key_exists("admin_pwd", $_POST)) $admin_pwd=htmlspecialchars($_POST["admin_pwd"]);
		if (array_key_exists("admin_email", $_POST)) $admin_email=htmlspecialchars($_POST["admin_email"]);


		if ($path_server == "") {
			die("Invalid server-path!");
		}
		
		if ($path_roscms == "" || $lang_std == "" || $lang_std_full == "" || lang_std_trans == "") {
			die("Invalid RosCMS config settings!");
		}

		if ($admin_name == "" || $admin_pwd == "" || $admin_email == "") {
			die("Invalid account settings!");
		}

		
		echo "Connecting to the MySQL Server... ";		
		$connect = mysql_connect($dbHost, $dbUser, $dbPass) or die("Cannot connect to the MySQL Server!");
		echo "OK<br />";
		
		echo "Creating the Database... ";
		mysql_query("CREATE DATABASE `". mysql_real_escape_string($dbName) ."`") or die( mysql_errno() . ": " . mysql_error() );
		echo "OK<br />";
		
		echo "Selecting the Database... ";
		mysql_select_db($dbName, $connect) or die("Cannot find and select <b>".$dbName."</b>!");
		echo "OK<br />";
		
		echo "Importing the SQL Data... ";
		if( !import_sql_dump( "roscms.sql" ) ) {
			die("Error: import not successfull!");
		}
		echo "<br />";
		
		echo "Adding first account... ";
		$insert_account = mysql_query("INSERT INTO `users` ( `user_id` , `user_name` , `user_roscms_password` , `user_roscms_getpwd_id` , `user_timestamp_touch` , `user_timestamp_touch2` , `user_login_counter` , `user_account_enabled` , `user_account_hidden` , `user_register` , `user_fullname` , `user_email` , `user_website` , `user_language` , `user_country` , `user_timezone` , `user_occupation` , `user_description` , `user_setting_multisession` , `user_setting_browseragent` , `user_setting_ipaddress` , `user_setting_timeout` ) 
										VALUES (
											NULL , 
											'".mysql_real_escape_string($admin_name)."', 
											MD5( '".mysql_real_escape_string($admin_pwd)."' ) , 
											'', 
											'', 
											NOW( ) , 
											'0', 
											'yes', 
											'no', 
											NOW( ) , 
											'".mysql_real_escape_string($admin_fullname)."', 
											'".mysql_real_escape_string($admin_email)."', 
											'".mysql_real_escape_string($path_server)."', 
											'".mysql_real_escape_string($lang_std)."', 
											'', 
											'', 
											'', 
											'', 
											'true', 
											'true', 
											'true', 
											'true'
										);") or die("Cannot insert first account!");
		echo "OK<br />";
		
		echo "Adding account memberships... ";
		$insert_membership = mysql_query("INSERT INTO `usergroup_members` ( `usergroupmember_userid` , `usergroupmember_usergroupid` ) 
											VALUES (
												'1', 
												'user'
											), 
											(
												'1', 
												'ros_sadmin'
											);") or die("Cannot insert account memberships!");
		echo "OK<br />";
		
		
		// RosCMS settings
		echo "Creating roscms config file... ";
		$fp = fopen("custom.php", "w");
		flock($fp,2);
		fputs($fp, '<?php 
					
					/*
					RosCMS - ReactOS Content Management System
					Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
				
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
					
					
					global $rpm_page;
					
					$roscms_extern_brand = "RosCMS"; 
					$roscms_extern_version = "v3"; 
					$roscms_extern_version_detail = "v3.0731"; 
					
					$roscms_intern_webserver_pages = "'.$path_server.'"; 
		
					$roscms_intern_roscms_dir = "'.$path_roscms.'";
					$roscms_intern_webserver_roscms = "'.$path_server.'".$roscms_intern_roscms_dir."/";
					$roscms_intern_page_link = $roscms_intern_webserver_roscms . "?page=";
					$roscms_intern_script_name =  $roscms_intern_page_link . $rpm_page; 
					
					$roscms_intern_script_branch = $roscms_intern_script_name. "&branch="; 
	
					$roscms_standard_language="'.$lang_std.'"; // en/de/fr/... 
					$roscms_standard_language_full="'.$lang_std_full.'"; // English/German/... 
					$roscms_standard_language_trans="'.$lang_std_trans.'"; // en/de/fr/...');
					
		fputs($fp, "\n ?>");
		flock($fp, 3);
		fclose($fp);		
		echo "OK<br />";
		
		// RosCMS database login config
		echo "Creating roscms database config file... ";
		$fp = fopen("connect.db.php", "w");
		flock($fp,2);
		fputs($fp, '<?php 
					/*
					RosCMS - ReactOS Content Management System
					Copyright (C) 2005-2007  Klemens Friedl <frik85@reactos.org>
					
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
						

					// Database:
					$dbHost = "'.$dbHost.'";
					$dbUser = "'.$dbUser.'";
					$dbPass = "'.$dbPass.'";
					$dbName = "'.$dbName.'";
					$roscms_database = $dbName;					
					
					$connect = @mysql_connect($dbHost, $dbUser, $dbPass) or die("ERROR: Cannot connect to the database!");
					$selectDB = @mysql_select_db($dbName, $connect) or die("Cannot find and select <b>".$dbName."</b>!");
					
					// Delete (set nothing) the vars after usage:
					$dbHost = "";
					$dbUser = "";
					$dbPass = "";
					$dbName = "";');
		
		fputs($fp, "\n ?>");
		flock($fp, 3);
		fclose($fp);		
		echo "OK<br />";
		
		
		echo "<p>&nbsp;</p>";
		echo "<p><b>RosCMS was set up successfully!</b></p>";
		
		echo "<p>&nbsp;</p>";
		echo "<p>Delete this \"install.php\" file on the server and <a href=\"index.php?page=login\">then login into RosCMS</a> with your account.</p>";
		
	}
	else {
?>
		<h1>RosCMS v3 - Install Script - Step1</h1>
		<p><i>&copy; Klemens Friedl 2005-2007, GNU GPL 2 license</i></p>
		<p>&nbsp;</p>
<?php
		if (get_magic_quotes_gpc()) {
			die("<p>ERROR: Disable 'magic quotes' in php.ini (=Off)</p>");
		}
		
		$phpver = explode('-', phpversion());
		$phpver = $phpver[0];
		
		if (substr($phpver, 0, 1) <= 3) {
			die("<p>ERROR: RosCMS v3 don't work with versions prior to PHP 4!</p>");
		}		
		else if (substr($phpver, 0, 1) == 4) {
			echo "<p>RosCMS v3 should work fine with PHP 4, make sure you use an up-to-date PHP 4 version. If possible and reasonable, upgrade to PHP 5 or later.</p>";
		}		
?>
		<p>Your PHP installation is configured correctly. RosCMS has been tested with MySQL 4 and 5. </p>
		<p>Click <i>Next</i> to continue!</p>
		
		<p><a href="install.php?ready=step2"><b>Next &gt;</b></a></p>
<?php
	}
?>
</body>
</html>