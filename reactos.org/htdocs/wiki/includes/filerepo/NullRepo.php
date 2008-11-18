<?php

/**
 * File repository with no files, for performance testing
 * @ingroup FileRepo
 */
class NullRepo extends FileRepo {
	function __construct( $info ) {}

	function storeBatch( $triplets, $flags = 0 ) {
		return false;
	}

	function storeTemp( $originalName, $srcPath ) {
		return false;
	}
	function publishBatch( $triplets, $flags = 0 ) {
		return false;
	}
	function deleteBatch( $sourceDestPairs ) {
		return false;
	}
	function getFileProps( $virtualUrl ) {
		return false;
	}
	function newFile( $title, $time = false ) {
		return false;
	}
	function findFile( $title, $time = false ) {
		return false;
	}
}
