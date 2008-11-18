<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/peoplemap/ajax-deletelocation.php
  PURPOSE:    AJAX backend for the People Map for deleting user locations
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

	header("Content-type: text/xml");
	
	if(!$_COOKIE["roscmsusrkey"] || !isset($_GET["delete"]))
		die("<error>Necessary information not specified!</error>");
	

	require_once("config.inc.php");
	
	$db = mysql_connect($DB_HOST, $DB_USER, $DB_PASS);
	
	// Get the user ID from the session key
	$query = "SELECT usersession_user_id FROM $DB_ROSCMS.user_sessions WHERE usersession_id = '" . mysql_real_escape_string($_COOKIE["roscmsusrkey"]) . "' LIMIT 1;";
	$result = mysql_query($query, $db) or die("<error>Query failed #1!</error>");
	
	if(mysql_num_rows($result) > 0)
		$userid = mysql_result($result, 0);
	
	if(!$userid)
		die("<error>No user ID!</error>");
	
	// Delete the location entry
	$query = "DELETE FROM $DB_PEOPLEMAP.user_locations WHERE roscms_user_id = $userid;";
	mysql_query($query, $db) or die("<error>Query failed #2!</error>");
	mysql_close($db);
	
	// Just return a success state
	echo "<deleted />";
?>
