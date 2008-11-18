<?php
/**
 * Optional upgrade script to populate the img_sha1 field
 *
 * @file
 * @ingroup MaintenanceArchive
 */

$optionsWithArgs = array( 'method' );
require_once( dirname(__FILE__).'/../commandLine.inc' );
$method = isset( $options['method'] ) ? $options['method'] : 'normal';

$t = -microtime( true );
$fname = 'populateSha1.php';
$dbw = wfGetDB( DB_MASTER );
$res = $dbw->select( 'image', array( 'img_name' ), array( 'img_sha1' => '' ), $fname );
$imageTable = $dbw->tableName( 'image' );
$oldimageTable = $dbw->tableName( 'oldimage' );
$batch = array();

$cmd = 'mysql -u' . wfEscapeShellArg( $wgDBuser ) . 
	' -h' . wfEscapeShellArg( $wgDBserver ) .
	' -p' . wfEscapeShellArg( $wgDBpassword, $wgDBname );
if ( $method == 'pipe' ) {
	echo "Using pipe method\n";
	$pipe = popen( $cmd, 'w' );
}

$numRows = $res->numRows();
$i = 0;
foreach ( $res as $row ) {
	if ( $i % 100 == 0 ) {
		printf( "Done %d of %d, %5.3f%%  \r", $i, $numRows, $i / $numRows * 100 );
		wfWaitForSlaves( 5 );
	}
	$file = wfLocalFile( $row->img_name );
	if ( !$file ) {
		continue;
	}
	$sha1 = File::sha1Base36( $file->getPath() );
	if ( strval( $sha1 ) !== '' ) {
		$sql = "UPDATE $imageTable SET img_sha1=" . $dbw->addQuotes( $sha1 ) .
			" WHERE img_name=" . $dbw->addQuotes( $row->img_name );
		if ( $method == 'pipe' ) {
			fwrite( $pipe, "$sql;\n" );
		} else {
			$dbw->query( $sql, $fname );
		}
	}
	$i++;
}
if ( $method == 'pipe' ) {
	fflush( $pipe );
	pclose( $pipe );
}
$t += microtime( true );
printf( "\nDone %d files in %.1f seconds\n", $numRows, $t );

?>
