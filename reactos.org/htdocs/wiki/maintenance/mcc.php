<?php
/**
 * memcached diagnostic tool
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
require_once( 'commandLine.inc' );

$mcc = new memcached( array('persistant' => true/*, 'debug' => true*/) );
$mcc->set_servers( $wgMemCachedServers );
#$mcc->set_debug( true );

function mccShowHelp($command) {

	if(! $command ) { $command = 'fullhelp'; }
	$onlyone = true;

	switch ( $command ) {

		case 'fullhelp':
			// will show help for all commands
			$onlyone = false;

		case 'get':
			print "get: grabs something\n";
		if($onlyone) { break; }

		case 'getsock':
			print "getsock: lists sockets\n";
		if($onlyone) { break; }

		case 'set':
			print "set: changes something\n";
		if($onlyone) { break; }

		case 'delete':
			print "delete: deletes something\n";
		if($onlyone) { break; }

		case 'history':
			print "history: show command line history\n";
		if($onlyone) { break; }

		case 'server':
			print "server: show current memcached server\n";
		if($onlyone) { break; }

		case 'dumpmcc':
			print "dumpmcc: shows the whole thing\n";
		if($onlyone) { break; }

		case 'exit':
		case 'quit':
			print "exit or quit: exit mcc\n";
		if($onlyone) { break; }

		case 'help':
			print "help: help about a command\n";
		if($onlyone) { break; }

		default:
			if($onlyone) {
				print "$command: command does not exist or no help for it\n";
			}
	}
}

do {
	$bad = false;
	$showhelp = false;
	$quit = false;

	$line = readconsole( '> ' );
	if ($line === false) exit;

	$args = explode( ' ', $line );
	$command = array_shift( $args );

	// process command
	switch ( $command ) {
		case 'help':
			// show an help message
			mccShowHelp(array_shift($args));
		break;

		case 'get':
			print "Getting {$args[0]}[{$args[1]}]\n";
			$res = $mcc->get( $args[0] );
			if ( array_key_exists( 1, $args ) ) {
				$res = $res[$args[1]];
			}
			if ( $res === false ) {
				#print 'Error: ' . $mcc->error_string() . "\n";
				print "MemCached error\n";
			} elseif ( is_string( $res ) ) {
				print "$res\n";
			} else {
				var_dump( $res );
			}
		break;

		case 'getsock':
			$res = $mcc->get( $args[0] );
			$sock = $mcc->get_sock( $args[0] );
			var_dump( $sock );
			break;

		case 'server':
			$res = $mcc->get( $args[0] );
			$hv = $mcc->_hashfunc( $args[0] );
			for ( $i = 0; $i < 3; $i++ ) {
				print $mcc->_buckets[$hv % $mcc->_bucketcount] . "\n";
				$hv += $mcc->_hashfunc( $i . $args[0] );
			}
			break;

		case 'set':
			$key = array_shift( $args );
			if ( $args[0] == "#" && is_numeric( $args[1] ) ) {
				$value = str_repeat( '*', $args[1] );
			} else {
				$value = implode( ' ', $args );
			}
			if ( !$mcc->set( $key, $value, 0 ) ) {
				#print 'Error: ' . $mcc->error_string() . "\n";
				print "MemCached error\n";
			}
			break;

		case 'delete':
			$key = implode( ' ', $args );
			if ( !$mcc->delete( $key ) ) {
				#print 'Error: ' . $mcc->error_string() . "\n";
				print "MemCached error\n";
			}
			break;

		case 'history':
			if ( function_exists( 'readline_list_history' ) ) {
				foreach( readline_list_history() as $num => $line) {
					print "$num: $line\n";
				}
			} else {
				print "readline_list_history() not available\n";
			}
			break;

		case 'dumpmcc':
			var_dump( $mcc );
			break;

		case 'quit':
		case 'exit':
			$quit = true;
			break;

		default:
			$bad = true;
	} // switch() end

	if ( $bad ) {
		if ( $command ) {
			print "Bad command\n";
		}
	} else {
		if ( function_exists( 'readline_add_history' ) ) {
			readline_add_history( $line );
		}
	}
} while ( !$quit );


