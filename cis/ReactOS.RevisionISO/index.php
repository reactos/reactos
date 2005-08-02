<?php

include ('config.php');

function dm_usort_cmp ($a, $b) {
  if ($a == $b) return 0;
  return ($a > $b) ? -1 : 1;
}

function dm_usort_cmp_desc ($a, $b) {
  if ($a == $b) return 0;
  return ($a > $b) ? 1 : -1;
}

function printHeader()
{
?>
<!doctype html public "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<title>ReactOS Revison ISOs</title>
<meta name="generator" content="Editpad">
<meta name="keywords" content="OS, ReactOS, operating system">
<meta name="author" content="ReactOS Project (ros-dev@reactos.com)">
<style>
.box
{
	padding: 0px;
	background-color: #88aadd;
	border-left: 1px solid #f0f0f0;
	border-right: 1px solid #000000;
	border-top: 1px solid #f0f0f0;
	border-bottom: 1px solid #000000;
}
</style>
</head>
<body bgcolor="#88aadd">
<form method="post" action="">
<?php
}

function printMenu($revision)
{
?>
<table border="0" class="box" cellpadding="5">
<tr>
<td height="2px">
</td>
</tr>
<tr>
<td>

<table border="0" cellpadding="0" cellspacing="0">
<tr>
	<td>
		<b>Branch:</b>
	</td>
	<td>
		<select name="branch" tabindex="1">
<?php

    $d = dir(ISO_PATH);
    $i = 0;
    $dirlist = array();
    while (false !== ($entry = $d->read())) {
      if ((strcasecmp($entry, ".") != 0) && (strcasecmp($entry, "..") != 0) && is_dir(ISO_PATH . "\\" . $entry) == "dir") {
        $dirlist[$i++] = $entry;
      }
    }
    $d->close();

    if (is_array($dirlist)) {
      usort($dirlist, "dm_usort_cmp");
      reset($dirlist);
      while (list($key, $val) = each($dirlist)) {
        $branch = $val;
	if ($branch == $_POST["branch"] || (!isset($_POST["branch"]) && $branch == "trunk"))
		$selected = " selected";
	else
		$selected = "";
	echo "<option$selected>$branch</option>";
      }
    }
	
?>
		</select>
	</td>
	<td>
		&nbsp;
	</td>
	<td>
		<b>Revision:</b>
	</td>
	<td>
<?php
	echo "<input type=\"text\" name=\"revision\" size=\"10\" maxlength=\"10\" tabindex=\"2\" value=\"" . $revision . "\"></input>";
?>
	</td>
	<td>
		&nbsp;
	</td>
	<td>
		<input type="submit" name="getiso" value="Download" tabindex="3" style="border: 1px solid #000000"></input>
	</td>
</tr>
<tr>
	<td colspan="7">
		<hr size="2" width="100%" />
	</td>
</tr>
<tr>
	<td colspan="4">
		<input type="submit" name="getnextiso" value="Next ISO" tabindex="4" style="border: 1px solid #000000"></input>
	</td>
	<td colspan="3" align="right">
		<input type="submit" name="getlatestiso" value="Latest ISO" tabindex="5" style="border: 1px solid #000000"></input>
	</td>
</tr>
</table>

</td>
</tr>
<tr>
<td height="2px">
</td>
</tr>
</table>
<?php
}

function printFooter()
{
?>
</form>

<script>
var revision = document.getElementById('revision');
if (revision) revision.focus();
</script>
</body>
</html>
<?php
}

function locateRevisionISO($branch, $revision, $latest)
{
	$revision = intval($revision);
	$path = ISO_PATH . "\\" . $branch;
	$d = dir($path);
	$i = 0;
	$filelist = array();
	while (false !== ($entry = $d->read())) {
		if (is_dir($path . "\\" . $entry) != "dir")
			$filelist[$i++] = $entry;
	}
	$d->close();
	
	if (is_array($filelist)) {
		$sortFunction = $latest ? "dm_usort_cmp" : "dm_usort_cmp_desc";
		usort($filelist, $sortFunction);
		reset($filelist);
		while (list($key, $filename) = each($filelist)) {
			if (ereg('ReactOS-' . $branch . '-r([0-9]*).iso', $filename, $regs))
			{
				$thisRevision = intval($regs[1]);
				if (($latest) && ($thisRevision < $revision))
					return $regs[1];
				else if ($thisRevision > $revision)
					return $regs[1];
				$lastRevision = $thisRevision;
			}
		}
	}

	return "";
}

function getNextRevisionISO($branch, $revision)
{
	return locateRevisionISO($branch, $revision, false);
}

function getLatestRevisionISO($branch)
{
	return locateRevisionISO($branch, 999999, true);
}

function main()
{
	$branch = $_POST["branch"];
	$revision = $_POST["revision"];

	$filename = "ReactOS-" . $branch . "-r" . $revision . ".iso";
	if (file_exists(ISO_PATH . $branch . "\\" . $filename))
	{
		$location = ISO_BASE_URL . $branch . "/" . $filename;
		header("Location: $location");
		return;
	}
	else
	{
		printHeader();
		printMenu($_POST["revision"]);
		echo "<br><b>No ISO exist for branch '" . $branch . "' and revision " . $revision . ".</b><br><br>";
		printFooter();
	}
}

if (!empty($_POST["getiso"]) && !empty($_POST["branch"]) && !empty($_POST["revision"]) && is_numeric($_POST["revision"]))
	main();
else if (!empty($_POST["getnextiso"]) && !empty($_POST["branch"]) && !empty($_POST["revision"]) && is_numeric($_POST["revision"]))
{
	printHeader();
	printMenu(getNextRevisionISO($_POST["branch"], $_POST["revision"]));
	printFooter();
}
else if (!empty($_POST["getlatestiso"]) && !empty($_POST["branch"]))
{
	printHeader();
	printMenu(getLatestRevisionISO($_POST["branch"]));
	printFooter();
}
else
{
	printHeader();
	printMenu($_POST["revision"]);
	printFooter();
}

?>
