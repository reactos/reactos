<!--
     Show profiling data.

     Copyright 2005 Kate Turner.

     Permission is hereby granted, free of charge, to any person obtaining a copy
     of this software and associated documentation files (the "Software"), to deal
     in the Software without restriction, including without limitation the rights
     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     copies of the Software, and to permit persons to whom the Software is
     furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in
     all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
     SOFTWARE.

-->
<html>
<head>
<title>Profiling data</title>
<style type="text/css">
	th {
		text-align: left;
		border-bottom: solid 1px black;
	}

	th, td {
		padding-left: 0.5em;
		padding-right: 0.5em;
	}

	td.timep, td.memoryp, td.count, td.cpr, td.tpc, td.mpc, td.tpr, td.mpr {
		text-align: right;
	}
	td.timep, td.tpc, td.tpr {
		background-color: #fffff0;
	}
	td.memoryp, td.mpc, td.mpr {
		background-color: #f0f8ff;
	}
	td.count, td,cpr {
		background-color: #f0fff0;
	}
	td.name {
		background-color: #f9f9f9;
	}
</style>
</head>
<body>
<?php

$wgDBadminuser = $wgDBadminpassword = $wgDBserver = $wgDBname = $wgEnableProfileInfo = $wgDBprefix = false;

define( 'MW_NO_SETUP', 1 );
require_once( './includes/WebStart.php' );
require_once("./AdminSettings.php");
require_once( './includes/GlobalFunctions.php' );

if (!$wgEnableProfileInfo) {
	echo "disabled\n";
	exit( 1 );
}

foreach (array("wgDBadminuser", "wgDBadminpassword", "wgDBserver", "wgDBname") as $var)
	if ($$var === false) {
		echo "AdminSettings.php not correct\n";
		exit( 1 );
	}


$expand = array();
if (isset($_REQUEST['expand']))
	foreach(explode(",", $_REQUEST['expand']) as $f)
		$expand[$f] = true;

class profile_point {
	var $name;
	var $count;
	var $time;
	var $children;

	function profile_point($name, $count, $time, $memory ) {
		$this->name = $name;
		$this->count = $count;
		$this->time = $time;
		$this->memory = $memory;
		$this->children = array();
	}

	function add_child($child) {
		$this->children[] = $child;
	}

	function display($indent = 0.0) {
		global $expand, $totaltime, $totalmemory, $totalcount;
		usort($this->children, "compare_point");

		$extet = '';
		if (isset($expand[$this->name()]))
			$ex = true;
		else	$ex = false;
		if (!$ex) {
			if (count($this->children)) {
				$url = makeurl(false, false, $expand + array($this->name() => true));
				$extet = " <a href=\"$url\">[+]</a>";
			} else $extet = '';
		} else {
			$e = array();
			foreach ($expand as $name => $ep)
				if ($name != $this->name())
					$e += array($name => $ep);

			$extet = " <a href=\"" . makeurl(false, false, $e) . "\">[&ndash;]</a>";
		}
		?>
		<tr>
		<td class="name" style="padding-left: <?php echo $indent ?>em;">
			<?php echo htmlspecialchars($this->name()) . $extet ?>
		</td>
		<td class="timep"><?php echo @wfPercent( $this->time() / $totaltime * 100 ) ?></td>
		<td class="memoryp"><?php echo @wfPercent( $this->memory() / $totalmemory * 100 ) ?></td>
		<td class="count"><?php echo $this->count() ?></td>
		<td class="cpr"><?php echo round( sprintf( '%.2f', $this->callsPerRequest() ), 2 ) ?></td>
		<td class="tpc"><?php echo round( sprintf( '%.2f', $this->timePerCall() ), 2 ) ?></td>
		<td class="mpc"><?php echo round( sprintf( '%.2f' ,$this->memoryPerCall() / 1024 ), 2 ) ?></td>
		<td class="tpr"><?php echo @round( sprintf( '%.2f', $this->time() / $totalcount ), 2 ) ?></td>
		<td class="mpr"><?php echo @round( sprintf( '%.2f' ,$this->memory() / $totalcount / 1024 ), 2 ) ?></td>
		</tr>
		<?php
		if ($ex) {
			foreach ($this->children as $child) {
				$child->display($indent + 2);
			}
		}
	}

	function name() {
		return $this->name;
	}

	function count() {
		return $this->count;
	}

	function time() {
		return $this->time;
	}
	
	function memory() {
		return $this->memory;
	}
	
	function timePerCall() {
		return @($this->time / $this->count);
	}
	
	function memoryPerCall() {
		return @($this->memory / $this->count);
	}
	
	function callsPerRequest() {
		global $totalcount;
		return @($this->count / $totalcount);
	}
	
	function timePerRequest() {
		global $totalcount;
		return @($this->time / $totalcount);
	}
	
	function memoryPerRequest() {
		global $totalcount;
		return @($this->memory / $totalcount);
	}

	function fmttime() {
		return sprintf("%5.02f", $this->time);
	}
};

function compare_point($a, $b) {
	global $sort;
	switch ($sort) {
	case "name":
		return strcmp($a->name(), $b->name());
	case "time":
		return $a->time() > $b->time() ? -1 : 1;
	case "memory":
		return $a->memory() > $b->memory() ? -1 : 1;
	case "count":
		return $a->count() > $b->count() ? -1 : 1;
	case "time_per_call":
		return $a->timePerCall() > $b->timePerCall() ? -1 : 1;
	case "memory_per_call":
		return $a->memoryPerCall() > $b->memoryPerCall() ? -1 : 1;
	case "calls_per_req":
		return $a->callsPerRequest() > $b->callsPerRequest() ? -1 : 1;
	case "time_per_req":
		return $a->timePerRequest() > $b->timePerRequest() ? -1 : 1;
	case "memory_per_req":
		return $a->memoryPerRequest() > $b->memoryPerRequest() ? -1 : 1;
	}
}

$sorts = array("time","memory","count","calls_per_req","name","time_per_call","memory_per_call","time_per_req","memory_per_req");
$sort = 'time';
if (isset($_REQUEST['sort']) && in_array($_REQUEST['sort'], $sorts))
	$sort = $_REQUEST['sort'];

$dbh = mysql_connect($wgDBserver, $wgDBadminuser, $wgDBadminpassword)
	or die("mysql server failed: " . mysql_error());
mysql_select_db($wgDBname, $dbh) or die(mysql_error($dbh));
$res = mysql_query("
	SELECT pf_count, pf_time, pf_memory, pf_name
	FROM {$wgDBprefix}profiling
	ORDER BY pf_name ASC
", $dbh) or die("query failed: " . mysql_error());

if (isset($_REQUEST['filter']))
	$filter = $_REQUEST['filter'];
else	$filter = '';

?>
<form method="profiling.php">
<p>
<input type="text" name="filter" value="<?php echo htmlspecialchars($filter)?>"/>
<input type="hidden" name="sort" value="<?php echo htmlspecialchars($sort)?>"/>
<input type="hidden" name="expand" value="<?php echo htmlspecialchars(implode(",", array_keys($expand)))?>"/>
<input type="submit" value="Filter" />
</p>
</form>

<table cellspacing="0" border="1">
<tr id="top">
<th><a href="<?php echo makeurl(false, "name") ?>">Name</a></th>
<th><a href="<?php echo makeurl(false, "time") ?>">Time (%)</a></th>
<th><a href="<?php echo makeurl(false, "memory") ?>">Memory (%)</a></th>
<th><a href="<?php echo makeurl(false, "count") ?>">Count</a></th>
<th><a href="<?php echo makeurl(false, "calls_per_req") ?>">Calls/req</a></th>
<th><a href="<?php echo makeurl(false, "time_per_call") ?>">ms/call</a></th>
<th><a href="<?php echo makeurl(false, "memory_per_call") ?>">kb/call</a></th>
<th><a href="<?php echo makeurl(false, "time_per_req") ?>">ms/req</a></th>
<th><a href="<?php echo makeurl(false, "memory_per_req") ?>">kb/req</a></th>
</tr>
<?php
$totaltime = 0.0;
$totalcount = 0;
$totalmemory = 0.0;

function makeurl($_filter = false, $_sort = false, $_expand = false) {
	global $filter, $sort, $expand;

	if ($_expand === false)
		$_expand = $expand;

	$nfilter = $_filter ? $_filter : $filter;
	$nsort = $_sort ? $_sort : $sort;
	$exp = urlencode(implode(',', array_keys($_expand)));
	return "?filter=$nfilter&amp;sort=$nsort&amp;expand=$exp";
}

$points = array();
$queries = array();
$sqltotal = 0.0;

$last = false;
while (($o = mysql_fetch_object($res)) !== false) {
	$next = new profile_point($o->pf_name, $o->pf_count, $o->pf_time, $o->pf_memory);
	if( $next->name() == '-total' ) {
		$totaltime = $next->time();
		$totalcount = $next->count();
		$totalmemory = $next->memory();
	}
	if ($last !== false) {
		if (preg_match("/^".preg_quote($last->name(), "/")."/", $next->name())) {
			$last->add_child($next);
			continue;
		}
	}
	$last = $next;
	if (preg_match("/^query: /", $next->name()) || preg_match("/^query-m: /", $next->name())) {
		$sqltotal += $next->time();
		$queries[] = $next;
	} else {
		$points[] = $next;
	}
}

$s = new profile_point("SQL Queries", 0, $sqltotal, 0, 0);
foreach ($queries as $q)
	$s->add_child($q);
$points[] = $s;

usort($points, "compare_point");

foreach ($points as $point) {
	if (strlen($filter) && !strstr($point->name(), $filter))
		continue;

	$point->display();
}
?>
</table>

<p>Total time: <tt><?php printf("%5.02f", $totaltime) ?></tt></p>
<p>Total memory: <tt><?php printf("%5.02f", $totalmemory / 1024 ) ?></tt></p>
<?php

mysql_free_result($res);
mysql_close($dbh);

?>
</body>
</html>
