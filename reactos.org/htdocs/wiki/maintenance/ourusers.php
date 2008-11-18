<?php
/**
 * Wikimedia specific
 *
 * This script generates SQL used to update MySQL users on a hardcoded
 * list of hosts. It takes care of setting the wikiuser for every
 * database as well as setting up wikiadmin.
 *
 * @todo document
 * @file
 * @ingroup Maintenance
 */

/** */
$wikiuser_pass = `wikiuser_pass`;
$wikiadmin_pass = `wikiadmin_pass`;
$wikisql_pass = `wikisql_pass`;

if ( @$argv[1] == 'yaseo' ) {
	$hosts = array(
		'localhost',
		'211.115.107.158',
		'211.115.107.159',
		'211.115.107.160',
		'211.115.107.138',
		'211.115.107.139',
		'211.115.107.140',
		'211.115.107.141',
		'211.115.107.142',
		'211.115.107.143',
		'211.115.107.144',
		'211.115.107.145',
		'211.115.107.146',
		'211.115.107.147',
		'211.115.107.148',
		'211.115.107.149',
		'211.115.107.150',
		'211.115.107.152',
		'211.115.107.153',
		'211.115.107.154',
		'211.115.107.155',
		'211.115.107.156',
		'211.115.107.157',
	);
} else {
	$hosts = array(
		'localhost',
		'10.0.%',
		'66.230.200.%',
		'208.80.152.%',
	);
}

$databases = array(
	'%wik%',
	'centralauth',
);

print "/*!40100 set old_passwords=1 */;\n";
print "/*!40100 set global old_passwords=1 */;\n";

foreach( $hosts as $host ) {
	print "--\n-- $host\n--\n\n-- wikiuser\n\n";
	print "GRANT REPLICATION CLIENT,PROCESS ON *.* TO 'wikiuser'@'$host' IDENTIFIED BY '$wikiuser_pass';\n";
	print "GRANT ALL PRIVILEGES ON `boardvote%`.* TO 'wikiuser'@'$host' IDENTIFIED BY '$wikiuser_pass';\n";
	foreach( $databases as $db ) {
		print "GRANT SELECT, INSERT, UPDATE, DELETE ON `$db`.* TO 'wikiuser'@'$host' IDENTIFIED BY '$wikiuser_pass';\n";
	}

	print "\n-- wikiadmin\n\n";
	print "GRANT PROCESS, REPLICATION CLIENT ON *.* TO 'wikiadmin'@'$host' IDENTIFIED BY '$wikiadmin_pass';\n";
	print "GRANT ALL PRIVILEGES ON `boardvote%`.* TO wikiadmin@'$host' IDENTIFIED BY '$wikiadmin_pass';\n";
	foreach ( $databases as $db ) {
		print "GRANT ALL PRIVILEGES ON `$db`.* TO wikiadmin@'$host' IDENTIFIED BY '$wikiadmin_pass';\n";
	}
	print "\n";
}

