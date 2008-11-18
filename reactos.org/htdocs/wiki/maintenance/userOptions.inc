<?php
/**
 * @file
 * @ingroup Maintenance
 */

// Options we will use
$options = array( 'list', 'nowarn', 'quiet', 'usage', 'dry' );
$optionsWithArgs = array( 'old', 'new' );

require_once( 'commandLine.inc' );

/**
 * @ingroup Maintenance
 */
class userOptions {
	public $mQuick;
	public $mQuiet;
	public $mDry;
	public $mAnOption;
	public $mOldValue;
	public $mNewValue;

	private $mMode, $mReady ;

	/** Constructor. Will show usage and exit if script options are not correct */
	function __construct( $opts, $args ) {
		if( !$this->checkOpts( $opts, $args ) ) {
			userOptions::showUsageAndExit();
		} else {
			$this->mReady = $this->initializeOpts( $opts, $args );
		}
	}


	/** This is used to check options. Only needed on construction */
	private function checkOpts( $opts, $args ) {
		// The three possible ways to run the script:
		$list   = isset( $opts['list'] );
		$usage  = isset( $opts['usage'] ) && (count($args) <= 1);
		$change = isset( $opts['old']) && isset($opts['new']) && (count($args) <= 1) ;

		// We want only one of them
		$isValid = (($list + $usage + $change) == 1);

		return $isValid;
	}

	/** load script options in the object */
	private function initializeOpts( $opts, $args ) {

		$this->mQuick = isset( $opts['nowarn'] );
		$this->mQuiet = isset( $opts['quiet'] );
		$this->mDry   = isset( $opts['dry'] );

		// Set object properties, specially 'mMode' used by run()
		if( isset($opts['list']) ) {
			$this->mMode = 'LISTER' ;
		} elseif( isset($opts['usage']) ) {
			$this->mMode = 'USAGER' ;
			$this->mAnOption = isset($args[0]) ? $args[0] : false ;
		} elseif( isset($opts['old']) && isset($opts['new']) ) {
			$this->mMode = 'CHANGER' ;
			$this->mOldValue = $opts['old'] ;
			$this->mNewValue = $opts['new'] ;
			$this->mAnOption = $args[0];
		} else {
			die("There is a bug in the software, this should never happen\n");
		}

		return true;
	}

	// Dumb stuff to run a mode.
	public function run() {
		if(!$this->mReady ) {
			return false;
		}

		$this->{$this->mMode}( );

	}

	#
	# Modes.
	# 

	/** List default options and their value */
	private function LISTER( ) {
		$def = User::getDefaultOptions();
		ksort($def);
		$maxOpt = 0;
		foreach( $def as $opt => $value ) {
			$maxOpt = max( $maxOpt, strlen($opt) );
		}
		foreach( $def as $opt => $value ) {
			printf( "%-{$maxOpt}s: %s\n", $opt, $value );
		}
	}

	/** List options usage */
	private function USAGER( ) {
		$ret = array();
		$defaultOptions = User::getDefaultOptions();

		// We list user by user_id from one of the slave database
		$dbr = wfGetDB( DB_SLAVE );
		$result = $dbr->select( 'user',
			array( 'user_id' ),
			array(),
			__METHOD__
			);

		while( $id = $dbr->fetchObject( $result ) ) {

			$user = User::newFromId( $id->user_id );

			// Get the options and update stats
			if( $this->mAnOption ) {

				if(!array_key_exists( $this->mAnOption, $defaultOptions ) ) {
					print "Invalid user option. Use --list to see valid choices\n";
					exit;
				}

				$userValue = $user->getOption( $this->mAnOption );
				if( $userValue <> $defaultOptions[$this->mAnOption] ) {
					@$ret[$this->mAnOption][$userValue]++;
				}

			} else {

				foreach( $defaultOptions as $name => $defaultValue ) {
					$userValue = $user->getOption( $name );
					if( $userValue <> $defaultValue ) {
						@$ret[$name][$userValue]++;
					}
				}
			}
		}

		foreach( $ret as $optionName => $usageStats ) {
			print "Usage for <$optionName> (default: '{$defaultOptions[$optionName]}'):\n";
			foreach( $usageStats as $value => $count ) {
				print " $count user(s): '$value'\n";
			}
			print "\n";
		}
	}


	/** Change our users options */
	private function CHANGER( ) {
		$this->warn();

		// We list user by user_id from one of the slave database
		$dbr = wfGetDB( DB_SLAVE );
		$result = $dbr->select( 'user',
			array( 'user_id' ),
			array(),
			__METHOD__
			);

		while( $id = $dbr->fetchObject( $result ) ) {

			$user = User::newFromId( $id->user_id );

			$curValue = $user->getOption( $this->mAnOption );
			$username = $user->getName();

			if( $curValue == $this->mOldValue ) {

				if(!$this->mQuiet) {
					print "Setting {$this->mAnOption} for $username from '{$this->mOldValue}' to '{$this->mNewValue}'): ";
				}

				// Change value 
				$user->setOption( $this->mAnOption, $this->mNewValue );

				// Will not save the settings if run with --dry
				if(!$this->mDry) {
					$user->saveSettings();
				}
				if( !$this->mQuiet) { print " OK\n"; }

			} elseif( !$this->mQuiet ) {
				print "Not changing '$username' using <{$this->mAnOption}> = '$curValue'\n";
			}
		}
	}


	/** Return an array of option names */
	public static function getDefaultOptionsNames() {
		$def = User::getDefaultOptions();
		$ret = array();
		foreach( $def as $optname => $defaultValue) {
			array_push( $ret, $optname );
		}
		return $ret;
	}


	#
	# Helper methods
	#

	public static function showUsageAndExit() {
print <<<USAGE

This script pass through all users and change one of their options.
The new option is NOT validated.

Usage:
    php userOptions.php --list
    php userOptions.php [user option] --usage
    php userOptions.php [options] <user option> --old <old value> --new <new value>

Switchs:
    --list : list available user options and their default value

    --usage : report all options statistics or just one if you specify it.

    --old <old value> : the value to look for
    --new <new value> : new value to update users with

Options:
    --nowarn: hides the 5 seconds warning
    --quiet : do not print what is happening
    --dry   : do not save user settings back to database

USAGE;
    exit(0);
	}

	/** The warning message and countdown */
	public function warn() {

		if( $this->mQuick ) {
			return true;
		}

print <<<WARN
The script is about to change the skin for ALL USERS in the database.
Users with option <$this->mAnOption> = '$this->mOldValue' will be made to use '$this->mNewValue'.

Abort with control-c in the next five seconds....
WARN;
		require('counter.php');
		for ($i=6;$i>=1;) {
			print_c($i, --$i);
			sleep(1);
		}
		print "\n";

		return true;
	}

}
