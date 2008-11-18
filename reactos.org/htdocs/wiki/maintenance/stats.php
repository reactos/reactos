<?php
/**
 * Show statistics from memcached
 *
 * @file
 * @ingroup Maintenance
 */

require_once('commandLine.inc');

if( get_class( $wgMemc ) == 'FakeMemCachedClient' ) {
	die("You are running FakeMemCachedClient, I can not provide any statistics.\n");
}

print "Requests\n";
$session = intval($wgMemc->get(wfMemcKey('stats','request_with_session')));
$noSession = intval($wgMemc->get(wfMemcKey('stats','request_without_session')));
$total = $session + $noSession;
printf( "with session:      %-10d %6.2f%%\n", $session, $session/$total*100 );
printf( "without session:   %-10d %6.2f%%\n", $noSession, $noSession/$total*100 );
printf( "total:             %-10d %6.2f%%\n", $total, 100 );


print "\nParser cache\n";
$hits = intval($wgMemc->get(wfMemcKey('stats','pcache_hit')));
$invalid = intval($wgMemc->get(wfMemcKey('stats','pcache_miss_invalid')));
$expired = intval($wgMemc->get(wfMemcKey('stats','pcache_miss_expired')));
$absent = intval($wgMemc->get(wfMemcKey('stats','pcache_miss_absent')));
$stub = intval($wgMemc->get(wfMemcKey('stats','pcache_miss_stub')));
$total = $hits + $invalid + $expired + $absent + $stub;
printf( "hits:              %-10d %6.2f%%\n", $hits, $hits/$total*100 );
printf( "invalid:           %-10d %6.2f%%\n", $invalid, $invalid/$total*100 );
printf( "expired:           %-10d %6.2f%%\n", $expired, $expired/$total*100 );
printf( "absent:            %-10d %6.2f%%\n", $absent, $absent/$total*100 );
printf( "stub threshold:    %-10d %6.2f%%\n", $stub, $stub/$total*100 );
printf( "total:             %-10d %6.2f%%\n", $total, 100 );

$hits = intval($wgMemc->get(wfMemcKey('stats','image_cache_hit')));
$misses = intval($wgMemc->get(wfMemcKey('stats','image_cache_miss')));
$updates = intval($wgMemc->get(wfMemcKey('stats','image_cache_update')));
$total = $hits + $misses;
print("\nImage cache\n");
printf( "hits:              %-10d %6.2f%%\n", $hits, $hits/$total*100 );
printf( "misses:            %-10d %6.2f%%\n", $misses, $misses/$total*100 );
printf( "updates:           %-10d\n", $updates );

$hits = intval($wgMemc->get(wfMemcKey('stats','diff_cache_hit')));
$misses = intval($wgMemc->get(wfMemcKey('stats','diff_cache_miss')));
$uncacheable = intval($wgMemc->get(wfMemcKey('stats','diff_uncacheable')));
$total = $hits + $misses + $uncacheable;
print("\nDiff cache\n");
printf( "hits:              %-10d %6.2f%%\n", $hits, $hits/$total*100 );
printf( "misses:            %-10d %6.2f%%\n", $misses, $misses/$total*100 );
printf( "uncacheable:       %-10d %6.2f%%\n", $uncacheable, $uncacheable/$total*100 );


