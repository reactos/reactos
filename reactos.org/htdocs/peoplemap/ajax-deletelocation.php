<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    AJAX backend for the People Map for deleting user locations
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

	header("Content-type: text/xml");
	
	if(!$_COOKIE["roscmsusrkey"] || !isset($_GET["delete"]))
		die("<error>Necessary information not specified!</error>");
	

	require_once("config.inc.php");

	try
	{
		$dbh = new PDO("mysql:host=$DB_HOST", $DB_USER, $DB_PASS);
	}
	catch(PDOException $e)
	{
		// Give no exact error message here, so no server internals are exposed
		die("<error>Could not establish the DB connection</error>");
	}
	
	// Get the user ID from the session key
	$stmt = $dbh->prepare("SELECT usersession_user_id FROM $DB_ROSCMS.user_sessions WHERE usersession_id = :usersessionid LIMIT 1");
	$stmt->bindParam(":usersessionid", $_COOKIE["roscmsusrkey"]);
	$stmt->execute() or die("<error>Query failed #1</error>");
	$userid = (int)$stmt->fetchColumn();
	
	if(!$userid)
		die("<error>No user ID!</error>");
	
	// Delete the location entry
	$stmt = $dbh->prepare("DELETE FROM $DB_PEOPLEMAP.user_locations WHERE roscms_user_id = :userid");
	$stmt->bindParam(":userid", $userid);
	$stmt->execute() or die("<error>Query failed #2</error>");
		
	// Just return a success state
	echo "<deleted />";
?>
