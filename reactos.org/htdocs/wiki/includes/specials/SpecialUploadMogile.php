<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * You will need the extension MogileClient to use this special page.
 */
require_once( 'MogileFS.php' );

/**
 * Entry point
 */
function wfSpecialUploadMogile() {
	global $wgRequest;
	$form = new UploadFormMogile( $wgRequest );
	$form->execute();
}

/**
 * Extends Special:Upload with MogileFS.
 * @ingroup SpecialPage
 */
class UploadFormMogile extends UploadForm {
	/**
	 * Move the uploaded file from its temporary location to the final
	 * destination. If a previous version of the file exists, move
	 * it into the archive subdirectory.
	 *
	 * @todo If the later save fails, we may have disappeared the original file.
	 *
	 * @param string $saveName
	 * @param string $tempName full path to the temporary file
	 * @param bool $useRename  Not used in this implementation
	 */
	function saveUploadedFile( $saveName, $tempName, $useRename = false ) {
		global $wgOut;
		$mfs = MogileFS::NewMogileFS();

		$this->mSavedFile = "image!{$saveName}";

		if( $mfs->getPaths( $this->mSavedFile )) {
			$this->mUploadOldVersion = gmdate( 'YmdHis' ) . "!{$saveName}";
			if( !$mfs->rename( $this->mSavedFile, "archive!{$this->mUploadOldVersion}" ) ) {
				$wgOut->showFileRenameError( $this->mSavedFile,
				  "archive!{$this->mUploadOldVersion}" );
				return false;
			}
		} else {
			$this->mUploadOldVersion = '';
		}

		if ( $this->mStashed ) {
			if (!$mfs->rename($tempName,$this->mSavedFile)) {
				$wgOut->showFileRenameError($tempName, $this->mSavedFile );
				return false;
			}
		} else {
			if ( !$mfs->saveFile($this->mSavedFile,'normal',$tempName )) {
				$wgOut->showFileCopyError( $tempName, $this->mSavedFile );
				return false;
			}
			unlink($tempName);
		}
		return true;
	}

	/**
	 * Stash a file in a temporary directory for later processing
	 * after the user has confirmed it.
	 *
	 * If the user doesn't explicitly cancel or accept, these files
	 * can accumulate in the temp directory.
	 *
	 * @param string $saveName - the destination filename
	 * @param string $tempName - the source temporary file to save
	 * @return string - full path the stashed file, or false on failure
	 * @access private
	 */
	function saveTempUploadedFile( $saveName, $tempName ) {
		global $wgOut;

		$stash = 'stash!' . gmdate( "YmdHis" ) . '!' . $saveName;
		$mfs = MogileFS::NewMogileFS();
		if ( !$mfs->saveFile( $stash, 'normal', $tempName ) ) {
			$wgOut->showFileCopyError( $tempName, $stash );
			return false;
		}
		unlink($tempName);
		return $stash;
	}

	/**
	 * Stash a file in a temporary directory for later processing,
	 * and save the necessary descriptive info into the session.
	 * Returns a key value which will be passed through a form
	 * to pick up the path info on a later invocation.
	 *
	 * @return int
	 * @access private
	 */
	function stashSession() {
		$stash = $this->saveTempUploadedFile(
			$this->mUploadSaveName, $this->mUploadTempName );

		if( !$stash ) {
			# Couldn't save the file.
			return false;
		}

		$key = mt_rand( 0, 0x7fffffff );
		$_SESSION['wsUploadData'][$key] = array(
			'mUploadTempName' => $stash,
			'mUploadSize'     => $this->mUploadSize,
			'mOname'          => $this->mOname );
		return $key;
	}

	/**
	 * Remove a temporarily kept file stashed by saveTempUploadedFile().
	 * @access private
	 * @return success
	 */
	function unsaveUploadedFile() {
		global $wgOut;
		$mfs = MogileFS::NewMogileFS();
		if ( ! $mfs->delete( $this->mUploadTempName ) ) {
			$wgOut->showFileDeleteError( $this->mUploadTempName );
			return false;
		} else {
			return true;
		}
	}
}
