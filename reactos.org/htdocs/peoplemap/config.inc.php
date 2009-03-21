<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Configuration settings
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/
	
	// DB Settings
	// The user entered here must have:
	//   * SELECT, INSERT, UPDATE and DELETE privileges to $DB_PEOPLEMAP
	//   * SELECT privileges to $DB_ROSCMS
	$DB_HOST = "localhost";
	$DB_USER = "root";
	$DB_PASS = "";
	$DB_PEOPLEMAP = "peoplemap";
	$DB_ROSCMS    = "roscms";
	
	// Google Maps Settings
	$GOOGLE_MAPS_KEY = "";
	
	// Image Settings
	$MARKERS = array("red", "blue", "green", "violet", "yellow", "cyan", "lightgrey");
?>
