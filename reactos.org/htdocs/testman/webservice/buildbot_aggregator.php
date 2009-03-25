<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Aggregator for the Debug Log of ReactOS BuildBot Buildslaves
  COPYRIGHT:  Copyright 2009 Colin Finck <colin@reactos.org>
*/
	
	require_once("config.inc.php");
	require_once(TESTMAN_PATH . "connect.db.php");
	require_once("utils.inc.php");
	
	if(!isset($_GET["username"]) || !isset($_GET["password"]) || !isset($_GET["slavename"]) || !is_numeric($_GET["platform"]) || !is_numeric($_GET["build"]))
		die("Necessary information not specified!");
	
	try
	{
		$dbh = new PDO("mysql:host=" . DB_HOST, DB_USER, DB_PASS);
	}
	catch(PDOException $e)
	{
		// Give no exact error message here, so no server internals are exposed
		die("Could not establish the DB connection");
	}
	
	$user_id = VerifyLogin($_GET["username"], $_GET["password"]);
	
	// Make sure nobody runs this script multiple times for the same build
	$stmt = $dbh->prepare("SELECT COUNT(*) FROM " . DB_TESTMAN . ".winetest_runs WHERE comment = :comment");
	$stmt->bindValue(":comment", "Build " . $_GET["build"]);
	$stmt->execute() or die("SQL failed #1");
	
	if($stmt->fetchColumn())
		die("The script already processed this build before!");
	
	// Read the Buildslave test log
	$fp = @fopen("http://build.reactos.org:8010/builders/" . $_GET["slavename"] . "/builds/" . $_GET["build"] . "/steps/test/logs/stdio/text", "r");
	
	if(!$fp)
		die("Could not open the test log!");
	
	// Get the revision
	do
	{
		$line = fgets($fp);
	}
	while(!preg_match("#.+ ReactOS .+ \(Build [0-9]+-r([0-9]+)\)#", $line, $matches) && !feof($fp));
	
	$revision = $matches[1];
	
	if(!$revision)
		die("Got no revision");
	
	// Create a WineTest object for accessing the database
	$t = new WineTest();
	

	// Get the log for each test
	$line = "";
	$test_id = 0;
	
	while(!feof($fp))
	{
		// Find the line with the test information
		while(substr($line, 0, 27) != "Running Wine Test, Module: " && !feof($fp))
			$line = fgets($fp);
		
		// We might reach end of file here, we're done in this case
		if(feof($fp))
			break;
		
		// Parse the test line
		if(!preg_match("#Running Wine Test, Module: (.+), Test: (.+)#", $line, $matches))
			die("Wine Test line is invalid!");
		
		// Get a Suite ID for this combination
		$suite_id = $t->getSuiteId($matches[1], $matches[2]);
		
		// If an error occured, $suite_id will contain the error message
		if(!is_numeric($suite_id))
			die($suite_id);
		
		// Now get the real log
		$log = "";
		
		do
		{
			$line = fgets($fp);
			
			// If this test misses the summary line for some reason, check whether we reached the next test already.
			// If so, already break the loop here, so that this line won't be on the log for this test.
			if(substr($line, 0, 27) == "Running Wine Test, Module: ")
				break;
			
			$log .= $line;
		}
		while(strpos($line, " tests executed (") === false &&	substr($line, 0, 9) != "[SYSREG] " &&	!feof($fp));
		
		// Did we already get a Test ID for this run?
		if(!$test_id)
		{
			$test_id = $t->getTestId($revision, "reactos." . $_GET["platform"], "Build " . $_GET["build"]);
			
			// If an error occured, $test_id will contain the error message
			if(!is_numeric($test_id))
				die($test_id);
		}
		
		// Finally submit the log
		$return = $t->submit($test_id, $suite_id, $log);
		
		// If an error occured, $return will contain the error message
		if($return != "OK")
			die($return);
	}
	
	// If we have a Test ID, finish this test run and terminate with the return message from that function
	// Otherwise we couldn't find any test information in this log
	if($test_id)
		die($t->finish($test_id));
	else
		die("Found no test information in this log!");
?>
