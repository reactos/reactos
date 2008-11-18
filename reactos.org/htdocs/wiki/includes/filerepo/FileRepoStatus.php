<?php

/**
 * Generic operation result class for FileRepo-related operations
 * @ingroup FileRepo
 */
class FileRepoStatus extends Status {
	/**
	 * Factory function for fatal errors
	 */
	static function newFatal( $repo /*, parameters...*/ ) {
		$params = array_slice( func_get_args(), 1 );
		$result = new self( $repo );
		call_user_func_array( array( &$result, 'error' ), $params );
		$result->ok = false;
		return $result;
	}

	static function newGood( $repo = false, $value = null ) {
		$result = new self( $repo );
		$result->value = $value;
		return $result;
	}

	function __construct( $repo = false ) {
		if ( $repo ) {
			$this->cleanCallback = $repo->getErrorCleanupFunction();
		}
	}
}
