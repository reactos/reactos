<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Compare Page
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
  
  charset=utf-8 without BOM
*/

	require_once("config.inc.php");
	require_once("connect.db.php");
	require_once("utils.inc.php");
	require_once("languages.inc.php");
	require_once(SHARED_PATH . "subsys_layout.php");
	
	GetLanguage();
	require_once("lang/$lang.inc.php");
	

	function GetValueForResult($result)
	{
		// If a test crashed, return a numeric value of 0, so that the comparison is accurate
		if($result == -1)
			return 0;
		
		return $result;
	}
	
	function GetDifference(&$current_result_row, &$prev_result_row, $subject)
	{
		if(!$prev_result_row["id"] || $current_result_row[$subject] == $prev_result_row[$subject])
			return "&nbsp;";
		
		$diff = GetValueForResult($current_result_row[$subject]) - GetValueForResult($prev_result_row[$subject]);
		
		if($diff > 0)
			return "(+$diff)";
		else
			return "($diff)";
	}
	
	function CheckIfChanged(&$changed, &$temp, &$current)
	{
		if($changed)
			return;
		
		if($temp == -2)
			$temp = $current;
		else if($current != $temp)
			$changed = true;
	}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
	<title><?php echo $testman_langres["compare_title"]; ?></title>
	<link rel="stylesheet" type="text/css" href="../shared/css/basic.css" />
	<link rel="stylesheet" type="text/css" href="../shared/css/reactos.css" />
	<link rel="stylesheet" type="text/css" href="css/compare.css" />
	<script type="text/javascript">
	//<![CDATA[
		<?php require_once("js/compare.js.php"); ?>
	//]]>
	</script>
</head>
<body onload="Load()">

<h2><?php echo $testman_langres["compare_title"]; ?></h2>

<?php
	if(!isset($_GET["ids"]))
		die("Necessary information not specified");
	
	$id_array = explode(",", $_GET["ids"]);
	
	if(!$id_array)
		die("<i>ids</i> parameter is no array");
	
	// Verify that the array only contains numeric values to prevent SQL injections
	for($i = 0; $i < count($id_array); $i++)
		if(!is_numeric($id_array[$i]))
			die("<i>ids</i> parameter is not entirely numeric!");
	
	if(count($id_array) > MAX_COMPARE_RESULTS)
		die(sprintf($testman_langres["maxselection"], MAX_COMPARE_RESULTS));
	
	if(count($id_array) > 1)
	{
		echo '<div>';
		printf('<input type="checkbox" id="showchanged" onclick="ShowChangedCheckbox_OnClick(this)" /> %s', $testman_langres["showchanged"]);
		echo '</div><br />';
	}
?>

<div id="legend">
	<div id="intro"><?php echo $testman_langres["legend"]; ?>:</div>
	
	<div class="box totaltests"></div>
	<div class="desc"><?php echo $testman_langres["totaltests"]; ?></div>
	
	<div class="zero_failedtests" style="border: solid 1px black; border-right: none; margin: 0; width: 8px;"></div>
	<div class="real_failedtests" style="border: solid 1px black; border-left: none; width: 7px;"></div>
	<div class="desc"><?php echo $testman_langres["failedtests"]; ?></div>
	
	<div class="box todotests"></div>
	<div class="desc"><?php echo $testman_langres["todotests"]; ?></div>
	
	<div class="box skippedtests"></div>
	<div class="desc"><?php echo $testman_langres["skippedtests"]; ?></div>
	
	<div class="box diff_legend"></div>
	<div class="desc"><?php echo $testman_langres["difference"]; ?></div>
</div>

<?php
	// Establish a DB connection
	try
	{
		$dbh = new PDO("mysql:host=" . DB_HOST, DB_USER, DB_PASS);
	}
	catch(PDOException $e)
	{
		// Give no exact error message here, so no server internals are exposed
		die("Could not establish the DB connection");
	}
	
	// Get all Suite IDs linked to our Test IDs
	$stmt = $dbh->query(
		"SELECT s.id " .
		"FROM " . DB_TESTMAN . ".winetest_suites s " .
		"JOIN " . DB_TESTMAN . ".winetest_results e ON e.suite_id = s.id " .
		"WHERE e.test_id IN (" . $_GET["ids"] . ")"
	) or die("Query failed #1");
	$suite_ids = $stmt->fetchAll(PDO::FETCH_COLUMN);
	$suite_idlist = implode(",", $suite_ids);
	
	// Get the test results for each column
	$result_stmt = array();
	
	for($i = 0; $i < count($id_array); $i++)
	{
		$result_stmt[$i] = $dbh->prepare(
			"SELECT e.id, e.count, e.todo, e.failures, e.skipped " .
			"FROM " . DB_TESTMAN . ".winetest_suites s " .
			"LEFT JOIN " . DB_TESTMAN . ".winetest_results e ON e.suite_id = s.id AND e.test_id = :testid " .
			"WHERE s.id IN (" . $suite_idlist . ")" .
			"ORDER BY s.module, s.test"
		);
		$result_stmt[$i]->bindParam(":testid", $id_array[$i]);
		$result_stmt[$i]->execute() or die("Query failed #2 for statement $i");
	}
	
	echo '<table id="comparetable" class="datatable" cellspacing="0" cellpadding="0">';
	echo '<thead><tr class="head">';
	printf('<th class="TestSuite">%s</th>', $testman_langres["testsuite"]);
	
	$stmt = $dbh->prepare(
		"SELECT UNIX_TIMESTAMP(r.timestamp) timestamp, a.name, r.revision, r.platform " .
		"FROM " . DB_TESTMAN . ".winetest_runs r " .
		"JOIN " . DB_ROSCMS . ".roscms_accounts a ON r.user_id = a.id " .
		"WHERE r.id = :id"
	);
	
	for($i = 0; $i < count($id_array); $i++)
	{
		$stmt->bindParam(":id", $id_array[$i]);
		$stmt->execute() or die("Query failed #3");
		$row = $stmt->fetch(PDO::FETCH_ASSOC);
		
		echo '<th onmousedown="ResultHead_OnMouseDown(this)">';
		printf($testman_langres["resulthead"], $row["revision"], GetDateString($row["timestamp"]), $row["name"], GetPlatformString($row["platform"]));
		echo '</th>';
	}
	
	echo '</tr></thead>';
	echo '<tbody>';
	
	$oddeven = false;
	$unchanged = array();
	
	// Get all test suites for which we have at least one result in our ID list
	$stmt = $dbh->query(
		"SELECT DISTINCT s.id, s.module, s.test " .
		"FROM " . DB_TESTMAN . ".winetest_suites s " .
		"JOIN " . DB_TESTMAN . ".winetest_results e ON e.suite_id = s.id " .
		"WHERE test_id IN (" . $_GET["ids"] . ") " .
		"ORDER BY s.module ASC, s.test ASC"
	) or die("Query failed #3");
	
	while($suites_row = $stmt->fetch(PDO::FETCH_ASSOC))
	{
		printf('<tr id="suite_%s" class="%s">', $suites_row["id"], ($oddeven ? "odd" : "even"));
		printf('<td onmouseover="Cell_OnMouseOver(this)" onmouseout="Cell_OnMouseOut(this)">%s:%s</td>', $suites_row["module"], $suites_row["test"]);
		
		$changed = false;
		$prev_result_row = null;
		$temp_totaltests = -2;
		$temp_failedtests = -2;
		$temp_todotests = -2;
		$temp_skippedtests = -2;
		
		for($i = 0; $i < count($result_stmt); $i++)
		{
			$result_row = $result_stmt[$i]->fetch(PDO::FETCH_ASSOC);
			
			echo '<td onmouseover="Cell_OnMouseOver(this)" onmouseout="Cell_OnMouseOut(this)"';
			
			if($result_row["id"])
				printf(' class="clickable" onclick="Result_OnClick(%d)"', $result_row["id"]);
			
			echo '>';
			
			// Check whether there are any changes within the test results of several runs
			CheckIfChanged($changed, $temp_totaltests, $result_row["count"]);
			CheckIfChanged($changed, $temp_failedtests, $result_row["failures"]);
			CheckIfChanged($changed, $temp_todotests, $result_row["todo"]);
			CheckIfChanged($changed, $temp_skippedtests, $result_row["skipped"]);
			
			if($result_row["id"])
			{
				echo '<table class="celltable">';
				echo '<tr>';
				printf('<td colspan="3" title="%s" class="totaltests">%s <span class="diff">%s</span></td>', $testman_langres["totaltests"], GetTotalTestsString($result_row["count"]), GetDifference($result_row, $prev_result_row, "count"));
				echo '</tr><tr>';
				printf('<td title="%s" class="%s_failedtests">%d <span class="diff">%s</span></td>', $testman_langres["failedtests"], ($result_row["failures"] > 0 ? 'real' : 'zero'), $result_row["failures"], GetDifference($result_row, $prev_result_row, "failures"));
				printf('<td title="%s" class="todotests">%d <span class="diff">%s</span></td>', $testman_langres["todotests"], $result_row["todo"], GetDifference($result_row, $prev_result_row, "todo"));
				printf('<td title="%s" class="skippedtests">%d <span class="diff">%s</span></td>', $testman_langres["skippedtests"], $result_row["skipped"], GetDifference($result_row, $prev_result_row, "skipped"));
				echo '</tr></table>';
			}
			else
			{
				// Bloody IE Hack
				echo "&nbsp;";
			}
			
			echo '</td>';
			
			$prev_result_row = $result_row;
		}
		
		echo '</tr>';
		
		if(!$changed)
			$unchanged[] = $suites_row["id"];
		
		$oddeven = !$oddeven;
	}
	
	echo '</tbody></table>';
	
	// Prepare the array containing all "unchanged" rows
	echo "<script type=\"text/javascript\">\n";
	echo "//<![CDATA[\n";
	echo "var UnchangedRows = Array(";
	
	// Cut the last comma from the string
	echo implode(",", $unchanged);
	
	echo ");\n";
	echo "//]]>\n";
	echo "</script>";
?>

</body>
</html>
