<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/

	// As we cannot do cross-site requests with AJAX, we have to add this script, which gets the information from "ajax-getfiles-provider.php" over PHP.
	require_once("config.inc.php");
	header("Content-type: text/xml; charset=utf-8");
	readfile($AJAX_GETFILES_PROVIDER_URL . "?" . $_SERVER["QUERY_STRING"]);
?>
