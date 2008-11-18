<?php
/**
 * @file
 * @ingroup Profiler
 */

require_once(dirname(__FILE__).'/Profiler.php');

/**
 * Simple profiler base class.
 * @todo document methods (?)
 * @ingroup Profiler
 */
class ProfilerSimple extends Profiler {
	var $mMinimumTime = 0;
	var $mProfileID = false;

	function __construct() {
		global $wgRequestTime, $wgRUstart;
		if (!empty($wgRequestTime) && !empty($wgRUstart)) {
			$this->mWorkStack[] = array( '-total', 0, $wgRequestTime,$this->getCpuTime($wgRUstart));

			$elapsedcpu = $this->getCpuTime() - $this->getCpuTime($wgRUstart);
			$elapsedreal = microtime(true) - $wgRequestTime;

			$entry =& $this->mCollated["-setup"];
			if (!is_array($entry)) {
				$entry = array('cpu'=> 0.0, 'cpu_sq' => 0.0, 'real' => 0.0, 'real_sq' => 0.0, 'count' => 0);
				$this->mCollated["-setup"] =& $entry;
			}
			$entry['cpu'] += $elapsedcpu;
			$entry['cpu_sq'] += $elapsedcpu*$elapsedcpu;
			$entry['real'] += $elapsedreal;
			$entry['real_sq'] += $elapsedreal*$elapsedreal;
			$entry['count']++;
		}
	}

	function setMinimum( $min ) {
		$this->mMinimumTime = $min;
	}

	function setProfileID( $id ) {
		$this->mProfileID = $id;
	}

	function getProfileID() {
		if ( $this->mProfileID === false ) {
			return wfWikiID();
		} else {
			return $this->mProfileID;
		}
	}

	function profileIn($functionname) {
		global $wgDebugFunctionEntry;
		if ($wgDebugFunctionEntry) {
			$this->debug(str_repeat(' ', count($this->mWorkStack)).'Entering '.$functionname."\n");
		}
		$this->mWorkStack[] = array($functionname, count( $this->mWorkStack ), microtime(true), $this->getCpuTime());
	}

	function profileOut($functionname) {
		global $wgDebugFunctionEntry;

		if ($wgDebugFunctionEntry) {
			$this->debug(str_repeat(' ', count($this->mWorkStack) - 1).'Exiting '.$functionname."\n");
		}

		list($ofname, /* $ocount */ ,$ortime,$octime) = array_pop($this->mWorkStack);

		if (!$ofname) {
			$this->debug("Profiling error: $functionname\n");
		} else {
			if ($functionname == 'close') {
				$message = "Profile section ended by close(): {$ofname}";
				$functionname = $ofname;
				$this->debug( "$message\n" );
				$this->mCollated[$message] = array(
					'real' => 0.0, 'count' => 1);
			}
			elseif ($ofname != $functionname) {
				$message = "Profiling error: in({$ofname}), out($functionname)";
				$this->debug( "$message\n" );
				$this->mCollated[$message] = array(
					'real' => 0.0, 'count' => 1);
			}
			$entry =& $this->mCollated[$functionname];
			$elapsedcpu = $this->getCpuTime() - $octime;
			$elapsedreal = microtime(true) - $ortime;
			if (!is_array($entry)) {
				$entry = array('cpu'=> 0.0, 'cpu_sq' => 0.0, 'real' => 0.0, 'real_sq' => 0.0, 'count' => 0);
				$this->mCollated[$functionname] =& $entry;
			}
			$entry['cpu'] += $elapsedcpu;
			$entry['cpu_sq'] += $elapsedcpu*$elapsedcpu;
			$entry['real'] += $elapsedreal;
			$entry['real_sq'] += $elapsedreal*$elapsedreal;
			$entry['count']++;

		}
	}

	function getFunctionReport() {
		/* Implement in output subclasses */
	}

	function getCpuTime($ru=null) {
		if ( function_exists( 'getrusage' ) ) {
			if ( $ru == null )
				$ru = getrusage();
			return ($ru['ru_utime.tv_sec'] + $ru['ru_stime.tv_sec'] + ($ru['ru_utime.tv_usec'] +
				$ru['ru_stime.tv_usec']) * 1e-6);
		} else {
			return 0;
		}
	}

	/* If argument is passed, it assumes that it is dual-format time string, returns proper float time value */
	function getTime($time=null) {
		if ($time==null)
			return microtime(true);
		list($a,$b)=explode(" ",$time);
		return (float)($a+$b);
	}
}
