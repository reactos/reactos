<?php

include ('config.php');

function dm_usort_cmp ($a, $b) {
  if ($a == $b) return 0;
  return ($a > $b) ? -1 : 1;
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

function printMenu()
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
      if ((strcasecmp($entry, ".") != 0) && (strcasecmp($entry, "..") != 0) && is_dir(ISO_PATH . "\\" . $entry)=="dir") {
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
	$revision = $_POST["revision"];
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
		printMenu();
		echo "<br><b>No ISO exist for branch '" . $branch . "' and revision " . $revision . ".</b><br><br>";
		printFooter();
	}
}

if (!empty($_POST["getiso"]) && !empty($_POST["branch"]) && !empty($_POST["revision"]) && is_numeric($_POST["revision"]))
	main();
else
{
	printHeader();
	printMenu();
	printFooter();
}

?>
