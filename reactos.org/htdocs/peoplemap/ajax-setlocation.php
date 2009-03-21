<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    AJAX backend for the People Map for saving user locations
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

	header("Content-type: text/xml");
		
	if(!$_COOKIE["roscmsusrkey"] || !isset($_GET["latitude"]) || !isset($_GET["longitude"]))
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
		
	$latitude = (float)$_GET["latitude"];
	$longitude = (float)$_GET["longitude"];

	// Set latitude and longitude
	$stmt = $dbh->prepare("INSERT INTO $DB_PEOPLEMAP.user_locations(roscms_user_id, latitude, longitude) VALUES (:userid, :latitude, :longitude) ON DUPLICATE KEY UPDATE latitude=:latitude, longitude=:longitude");
	$stmt->bindParam(":userid", $userid);
	$stmt->bindParam(":latitude", $latitude);
	$stmt->bindParam(":longitude", $longitude);
	$stmt->execute() or die("<error>Query failed #2</error>");
	
	// We succeeded, return all information about our account
	$stmt = $dbh->prepare("SELECT user_name, user_fullname FROM $DB_ROSCMS.users WHERE user_id = :userid LIMIT 1");
	$stmt->bindParam(":userid", $userid);
	$stmt->execute() or die("<error>Query failed #3</error>");
	$row = $stmt->fetch(PDO::FETCH_NUM);
		
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
