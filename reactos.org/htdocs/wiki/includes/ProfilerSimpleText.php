<?php
/**
 * @file
 * @ingroup Profiler
 */

require_once( dirname( __FILE__ ) . '/ProfilerSimple.php' );

/**
 * The least sophisticated profiler output class possible, view your source! :)
 *
 * Put it to StartProfiler.php like this:
 *
 * require_once( dirname( __FILE__ ) . '/includes/ProfilerSimpleText.php' );
 * $wgProfiler = new ProfilerSimpleText;
 * $wgProfiler->visible=true;
 *
 * @ingroup Profiler
 */
class ProfilerSimpleText extends ProfilerSimple {
	public $visible=false; /* Show as <PRE> or <!-- ? */

	function getFunctionReport() {
		if ($this->visible) print "<pre>";
			else print "<!--\n";
		uasort($this->mCollated,array('self','sort'));
		array_walk($this->mCollated,array('self','format'));
		if ($this->visible) print "</pre>\n";
			else print "-->\n";
	}

	/* dense is good */
	static function sort($a,$b) { return $a['real']<$b['real']; /* sort descending by time elapsed */ }
	static function format($item,$key) { printf("%3.6f %6d - %s\n",$item['real'],$item['count'], $key); }
}
