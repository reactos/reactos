<?php
/**
 * Change the password of a given user
 *
 * @file
 * @ingroup Maintenance
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

$optionsWithArgs = array( 'user', 'password' );
require_once 'commandLine.inc';

$USAGE = 
	"Usage: php changePassword.php [--user=user --password=password | --help]\n" .
	"\toptions:\n" .
	"\t\t--help      show this message\n" .
	"\t\t--user      the username to operate on\n" .
	"\t\t--password  the password to use\n";

if( in_array( '--help', $argv ) )
	wfDie( $USAGE );

$cp = new ChangePassword( @$options['user'], @$options['password'] );
$cp->main();

/**
 * @ingroup Maintenance
 */
class ChangePassword {
	var $dbw;
	var $user, $password;

	function ChangePassword( $user, $password ) {
		global $USAGE;
		if( !strlen( $user ) or !strlen( $password ) ) {
			wfDie( $USAGE );
		}

		$this->user = User::newFromName( $user );
		if ( !$this->user->getId() ) {
			die ( "No such user: $user\n" );
		}

		$this->password = $password;

		$this->dbw = wfGetDB( DB_MASTER );
	}

	function main() {
		$fname = 'ChangePassword::main';

		$this->user->setPassword( $this->password );
		$this->user->saveSettings();
	}
}
