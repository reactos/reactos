<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Configuration settings
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
*/
	
	// DB Settings
	// The user entered here must have:
	//   * SELECT, INSERT, UPDATE and DELETE privileges to $DB_TESTMAN
	//   * SELECT privileges to $DB_ROSCMS
	define("DB_HOST", "localhost");
	define("DB_USER", "root");
	define("DB_PASS", "");
	
	define("DB_ROSCMS", "roscms");
	define("DB_TESTMAN", "testman");
	
	define("RESULTS_PER_PAGE", 100);
	define("MAX_COMPARE_RESULTS", 5);
?>
