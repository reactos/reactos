<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Miscellaneous utility functions for the Web Service
  COPYRIGHT:  Copyright 2009 Colin Finck <colin@reactos.org>
*/
	
	// What one of these classes has to look like
	interface Test
	{
		public function getTestId($revision, $platform, $comment);
		public function getSuiteId($module, $test);
		public function submit($test_id, $suite_id, $log);
		public function finish($test_id);
	}
	
	// All classes are autoloaded through this magic function
	function __autoload($class)
	{
		require_once("lib/$class.class.php");
	}
	
	function VerifyLogin($username, $password)
	{
		global $dbh;
		
		// Check the login credentials
		$stmt = $dbh->prepare("SELECT id FROM " . DB_ROSCMS . ".roscms_accounts WHERE name = :username AND password = MD5(:password) AND disabled = 0");
		$stmt->bindParam(":username", $username);
		$stmt->bindParam(":password", $password);
		$stmt->execute() or die("SQL failed #1");
		$user_id = (int)$stmt->fetchColumn();
		
		if(!$user_id)
			die("Invalid Login credentials!");
		
		// Check if the user is permitted to submit test results
		$stmt = $dbh->prepare("SELECT COUNT(*) FROM " . DB_TESTMAN . ".permitted_users WHERE user_id = :userid");
		$stmt->bindParam(":userid", $user_id);
		$stmt->execute() or die("SQL failed #2");
		
		if(!$stmt->fetchColumn())
			die("User is not permitted to submit test results");
		
		return $user_id;
	}
?>
