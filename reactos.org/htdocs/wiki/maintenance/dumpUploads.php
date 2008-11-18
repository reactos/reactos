<?php
/**
 * @file
 * @ingroup Maintenance
 */

require_once 'commandLine.inc';

class UploadDumper {
	function __construct( $args ) {
		global $IP, $wgUseSharedUploads;
		$this->mAction = 'fetchLocal';
		$this->mBasePath = $IP;
		$this->mShared = false;
		$this->mSharedSupplement = false;
		
		if( isset( $args['help'] ) ) {
			$this->mAction = 'help';
		}
		
		if( isset( $args['base'] ) ) {
			$this->mBasePath = $args['base'];
		}
		
		if( isset( $args['local'] ) ) {
			$this->mAction = 'fetchLocal';
		}
		
		if( isset( $args['used'] ) ) {
			$this->mAction = 'fetchUsed';
		}
		
		if( isset( $args['shared'] ) ) {
			if( isset( $args['used'] ) ) {
				// Include shared-repo files in the used check
				$this->mShared = true;
			} else {
				// Grab all local *plus* used shared
				$this->mSharedSupplement = true;
			}
		}
	}
	
	function run() {
		$this->{$this->mAction}( $this->mShared );
		if( $this->mSharedSupplement ) {
			$this->fetchUsed( true );
		}
	}
	
	function help() {
		echo <<<END
Generates list of uploaded files which can be fed to tar or similar.
By default, outputs relative paths against the parent directory of
\$wgUploadDirectory.

Usage:
php dumpUploads.php [options] > list-o-files.txt

Options:
--base=<path>  Set base relative path instead of wiki include root

--local        List all local files, used or not. No shared files included.
--used         Skip local images that are not used
--shared       Include images used from shared repository

END;
	}
	
	/**
	 * Fetch a list of all or used images from a particular image source.
	 * @param string $table
	 * @param string $directory Base directory where files are located
	 * @param bool $shared true to pass shared-dir settings to hash func
	 */
	function fetchUsed( $shared ) {
		$dbr = wfGetDB( DB_SLAVE );
		$image = $dbr->tableName( 'image' );
		$imagelinks = $dbr->tableName( 'imagelinks' );
		
		$sql = "SELECT DISTINCT il_to, img_name
			FROM $imagelinks
			LEFT OUTER JOIN $image
			ON il_to=img_name";
		$result = $dbr->query( $sql );
		
		foreach( $result as $row ) {
			$this->outputItem( $row->il_to, $shared );
		}
		$dbr->freeResult( $result );
	}
	
	function fetchLocal( $shared ) {
		$dbr = wfGetDB( DB_SLAVE );
		$result = $dbr->select( 'image',
			array( 'img_name' ),
			'',
			__METHOD__ );
		
		foreach( $result as $row ) {
			$this->outputItem( $row->img_name, $shared );
		}
		$dbr->freeResult( $result );
	}
	
	function outputItem( $name, $shared ) {
		$file = wfFindFile( $name );
		if( $file && $this->filterItem( $file, $shared ) ) {
			$filename = $file->getFullPath();
			$rel = wfRelativePath( $filename, $this->mBasePath );
			echo "$rel\n";
		} else {
			wfDebug( __METHOD__ . ": base file? $name\n" );
		}
	}
	
	function filterItem( $file, $shared ) {
		return $shared || $file->isLocal();
	}
}

$dumper = new UploadDumper( $options );
$dumper->run();

