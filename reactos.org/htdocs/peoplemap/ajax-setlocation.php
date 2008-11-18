<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/peoplemap/ajax-setlocation.php
  PURPOSE:    AJAX backend for the People Map for saving user locations
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

	header("Content-type: text/xml");
		
	if(!$_COOKIE["roscmsusrkey"] || !isset($_GET["latitude"]) || !isset($_GET["longitude"]))
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
	
	$latitude = (float)$_GET["latitude"];
	$longitude = (float)$_GET["longitude"];

	// Set latitude and longitude
	$query = "INSERT INTO $DB_PEOPLEMAP.user_locations " .
	         "(roscms_user_id, latitude, longitude) " .
	         "VALUES ($userid, $latitude, $longitude) " .
	         "ON DUPLICATE KEY UPDATE " .
	         "latitude=$latitude, longitude=$longitude;";
	
	mysql_query($query, $db) or die("<error>Query failed #2!</error>");
	
	// We succeeded, return all information about our account
	$query = "SELECT user_name, user_fullname FROM $DB_ROSCMS.users WHERE user_id = $userid LIMIT 1;";
	$result = mysql_query($query, $db) or die("<error>Query failed #3!</error>");
	$row = mysql_fetch_row($result);
	mysql_close($db);
	
	echo "<userinformation>";
	echo "<user>";
	printf("<id>%u</id>", $userid);
	printf("<username>%s</username>", $row[0]);
	printf("<fullname>%s</fullname>", $row[1]);
	printf("<latitude>%s</latitude>", $latitude);
	printf("<longitude>%s</longitude>", $longitude);
	echo "</user>";
	echo "</userinformation>";
?>
