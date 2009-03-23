<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Main Page
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
  
  charset=utf-8 without BOM
*/
	
	require_once("config.inc.php");
	require_once("connect.db.php");
	require_once("utils.inc.php");
	require_once("languages.inc.php");
	require_once(SHARED_PATH . "subsys_layout.php");
	require_once(SHARED_PATH . "svn.php");
	
	GetLanguage();
	require_once(SHARED_PATH . "lang/$lang.inc.php");
	require_once("lang/$lang.inc.php");
	
	$rev = GetLatestRevision();
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
	<title><?php echo $testman_langres["index_title"]; ?></title>
	<link rel="stylesheet" type="text/css" href="../shared/css/menu.css" />
	<link rel="stylesheet" type="text/css" href="../shared/css/reactos.css" />
	<link rel="stylesheet" type="text/css" href="css/index.css" />
	<script type="text/javascript">
	//<![CDATA[
		document.write('<style type="text/css">');
		document.write('#js_stuff {display: block;}');
		document.write('<\/style>');
	//]]>
	</script>
	<script type="text/javascript" src="../shared/js/ajax.js"></script>
	<script type="text/javascript">
	//<![CDATA[
		<?php require_once("js/index.js.php"); ?>
	//]]>
	</script>
</head>
<body>

<?php
	BasicLayout($lang);
	LanguageBox($lang);
?>
</td>
<td id="content">

<h1><?php echo $testman_langres["index_header"]; ?></h1>
<h2><?php echo $testman_langres["index_title"]; ?></h2>

<p><?php echo $testman_langres["index_intro"]; ?></p>

<noscript>
	<div class="bubble_bg">
		<div class="rounded_ll">
		<div class="rounded_lr">
		<div class="rounded_ul">
		<div class="rounded_ur">
		
		<div class="bubble">
			<b><?php echo $testman_langres["js_disclaimer"]; ?></b>
		</div>
		
		</div>
		</div>
		</div>
		</div>
	</div>
</noscript>

<div id="js_stuff">
	<table width="100%" cellspacing="0" cellpadding="0">
		<tr>
			<td id="mainbox_td">
				<div class="bubble_bg">
					<div class="rounded_ll">
					<div class="rounded_lr">
					<div class="rounded_ul">
					<div class="rounded_ur">
					
					<div class="bubble">
						<h1><?php echo $testman_langres["lastresults_header"]; ?></h1>
						
						<table class="datatable" cellspacing="0" cellpadding="0">
							<thead>
								<tr class="head">
									<th class="TestCheckbox"></th>
									<th><?php echo $testman_langres["revision"]; ?></th>
									<th><?php echo $testman_langres["date"]; ?></th>
									<th><?php echo $testman_langres["user"]; ?></th>
									<th><?php echo $testman_langres["platform"]; ?></th>
									<th><?php echo $testman_langres["comment"]; ?></th>
								</tr>
							</thead>
							<tbody>
								<?php
									try
									{
										$dbh = new PDO("mysql:host=" . DB_HOST, DB_USER, DB_PASS);
									}
									catch(PDOException $e)
									{
										// Give no exact error message here, so no server internals are exposed
										die("<error>Could not establish the DB connection</error>");
									}
									
									$stmt = $dbh->query(
										"SELECT r.id, UNIX_TIMESTAMP(r.timestamp) timestamp, a.name, r.revision, r.platform, r.comment " .
										"FROM " . DB_TESTMAN . ".winetest_runs r " .
										"JOIN " . DB_ROSCMS . ".roscms_accounts a ON r.user_id = a.id " .
										"ORDER BY revision DESC, r.id DESC " .
										"LIMIT 10"
									) or die("Query failed #1");
									
									$oddeven = false;
									$ids = array();
									
									while($row = $stmt->fetch(PDO::FETCH_ASSOC))
									{
										$ids[] = $row["id"];
										
										printf('<tr class="%s" onmouseover="Result_OnMouseOver(this)" onmouseout="Result_OnMouseOut(this)">', ($oddeven ? "odd" : "even"));
										printf('<td><input onclick="Result_OnCheckboxClick(this)" type="checkbox" name="test_%s" /></td>', $row["id"]);
										printf('<td onclick="Result_OnCellClick(this)">%s</td>', $row["revision"]);
										printf('<td onclick="Result_OnCellClick(this)">%s</td>', GetDateString($row["timestamp"]));
										printf('<td onclick="Result_OnCellClick(this)">%s</td>', htmlspecialchars($row["name"]));
										printf('<td onclick="Result_OnCellClick(this)">%s</td>', GetPlatformString($row["platform"]));
										printf('<td onclick="Result_OnCellClick(this)">%s</td>', htmlspecialchars($row["comment"]));
										echo "</tr>";
										
										$oddeven = !$oddeven;
									}
								?>
							</tbody>
						</table>
						
						<?php
							// Ensure that all checkboxes are unchecked with a JavaScript (some browsers keep them checked after a reload)
							echo "<script type=\"text/javascript\">\n";
							echo "//<![CDATA[\n";
							
							foreach($ids as $id)
								printf('document.getElementsByName("test_%s")[0].checked = false;', $id);
							
							echo "\n//]]>\n";
							echo "</script>";
						?>
					</div>
					
					</div>
					</div>
					</div>
					</div>
				</div>
				
				<div class="bubble_bg">
					<div class="rounded_ll">
					<div class="rounded_lr">
					<div class="rounded_ul">
					<div class="rounded_ur">
					
					<div class="bubble">
						<h1><?php echo $testman_langres["search_header"]; ?></h1>
						
						<table id="searchform">
							<tr>
								<td><?php echo $testman_langres["revision"]; ?>:</td>
								<td>
									<input type="text" id="search_revision" value="" size="12" onkeypress="SearchInputs_OnKeyPress(event)" onkeyup="SearchRevisionInput_OnKeyUp(this)" /><br />
									
									<img src="../shared/images/info.gif" alt="" /> <?php printf($shared_langres["rangeinfo"], $rev, ($rev - 50), $rev); ?>
								</td>
							</tr>
							<tr>
								<td><?php echo $testman_langres["user"]; ?>:</td>
								<td>
									<input type="text" id="search_user" value="" size="24" onkeypress="SearchInputs_OnKeyPress(event)" />
								</td>
							</tr>
							<tr>
								<td><?php echo $testman_langres["platform"]; ?>:</td>
								<td>
									<select id="search_platform" size="1" onkeypress="SearchInputs_OnKeyPress(event)">
										<option></option>
										<option value="reactos">ReactOS</option>
										<option value="5.0">Windows 2000</option>
										<option value="5.1">Windows XP</option>
										<option value="5.2">Windows XP x64/Server 2003</option>
										<option value="6.0">Windows Vista/Server 2008</option>
										<option value="6.1">Windows 7</option>
									</select>
								</td>
							</tr>
						</table><br />
						
						<button onclick="SearchButton_OnClick()"><?php echo $testman_langres["search_button"]; ?></button>
						
						<span id="ajax_loading_search">
							<img src="../shared/images/ajax_loading.gif" alt="" /> <?php echo $testman_langres["searching"]; ?>...
						</span>
						
						<div id="searchtable">
							<!-- Filled by the JavaScript -->
						</div>
					</div>
					
					</div>
					</div>
					</div>
					</div>
				</div>
			</td>
			
			<td id="rightbox_td">
				<div class="bubble_bg">
					<div class="rounded_ll">
					<div class="rounded_lr">
					<div class="rounded_ul">
					<div class="rounded_ur">
					
					<div class="bubble">
						<div id="status"><?php printf($testman_langres["status"], "<b>0</b>"); ?></div><br />
						
						<button onclick="CompareButton_OnClick()"><?php echo $testman_langres["compare_button"]; ?></button>
					</div>
					
					</div>
					</div>
					</div>
					</div>
				</div>
			</td>
		</tr>
	</table>
</div>

</td>
</tr>
</table>

</body>
</html>
