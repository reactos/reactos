<?php
/**
 * This script remove all statistics tracking from memcached
 * 
 * @file
 * @ingroup Maintenance
 */

require_once('commandLine.inc');

foreach ( $wgLocalDatabases as $db ) {
	noisyDelete("$db:stats:request_with_session");
	noisyDelete("$db:stats:request_without_session");
	noisyDelete("$db:stats:pcache_hit");
	noisyDelete("$db:stats:pcache_miss_invalid");
	noisyDelete("$db:stats:pcache_miss_expired");
	noisyDelete("$db:stats:pcache_miss_absent");
	noisyDelete("$db:stats:pcache_miss_stub");
	noisyDelete("$db:stats:image_cache_hit");
	noisyDelete("$db:stats:image_cache_miss");
	noisyDelete("$db:stats:image_cache_update");
	noisyDelete("$db:stats:diff_cache_hit");
	noisyDelete("$db:stats:diff_cache_miss");
	noisyDelete("$db:stats:diff_uncacheable");
}

function noisyDelete( $key ) {
	global $wgMemc;
	/*
	print "$key ";
	if ( $wgMemc->delete($key) ) {
		print "deleted\n";
	} else {
		print "FAILED\n";
	}*/
	$wgMemc->delete($key);
}

