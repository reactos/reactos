<?php
	/*
	  Serendipity configuration file
	  Written on Tue, 17 Jan 2006 22:13:03 +0100
	*/

	$serendipity['versionInstalled']  = '0.9.1';
	$serendipity['dbPrefix']          = 'ser_';
	$serendipity['dbType']            = 'mysql';
	$serendipity['dbPersistent']      = true;

	// End of Serendipity configuration file
	// You can place your own special variables after here:

	require_once("blogs-connect.php");
	define("ROOT_PATH", "../");
?>
