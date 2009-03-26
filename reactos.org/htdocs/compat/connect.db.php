<?php

	/*****************************************
	 *
	 *	ReactOS RSDB System
	 *	
	 *	(c) by Klemens Friedl <frik85>
	 *	
	 *****************************************/
	
	
	// Database:
	$dbHost = "localhost";
	$dbUser = "root";
	$dbPass = "";
	$dbName = "rsdb";
	
	
	$connect = @mysql_connect($dbHost, $dbUser, $dbPass) or die("ERROR: Cannot connect to the database!");
	$selectDB = @mysql_select_db($dbName, $connect) or die("Cannot find and select <b>$dbName</b>!");
	
	// Delete (set nothing) the vars after usage:
	$dbHost = "";
	$dbUser = "";
	$dbPass = "";
	$dbName = "";

?>