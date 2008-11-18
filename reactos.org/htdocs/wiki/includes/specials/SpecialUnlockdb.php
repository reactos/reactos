<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 *
 */
function wfSpecialUnlockdb() {
	global $wgUser, $wgOut, $wgRequest;

	if( !$wgUser->isAllowed( 'siteadmin' ) ) {
		$wgOut->permissionRequired( 'siteadmin' );
		return;
	}

	$action = $wgRequest->getVal( 'action' );
	$f = new DBUnlockForm();

	if ( "success" == $action ) {
		$f->showSuccess();
	} else if ( "submit" == $action && $wgRequest->wasPosted() &&
		$wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ) ) ) {
		$f->doSubmit();
	} else {
		$f->showForm( "" );
	}
}

/**
 * @ingroup SpecialPage
 */
class DBUnlockForm {
	function showForm( $err )
	{
		global $wgOut, $wgUser;

		global $wgReadOnlyFile;
		if( !file_exists( $wgReadOnlyFile ) ) {
			$wgOut->addWikiMsg( 'databasenotlocked' );
			return;
		}

		$wgOut->setPagetitle( wfMsg( "unlockdb" ) );
		$wgOut->addWikiMsg( "unlockdbtext" );

		if ( "" != $err ) {
			$wgOut->setSubtitle( wfMsg( "formerror" ) );
			$wgOut->addHTML( '<p class="error">' . htmlspecialchars( $err ) . "</p>\n" );
		}
		$lc = htmlspecialchars( wfMsg( "unlockconfirm" ) );
		$lb = htmlspecialchars( wfMsg( "unlockbtn" ) );
		$titleObj = SpecialPage::getTitleFor( "Unlockdb" );
		$action = $titleObj->escapeLocalURL( "action=submit" );
		$token = htmlspecialchars( $wgUser->editToken() );

		$wgOut->addHTML( <<<END

<form id="unlockdb" method="post" action="{$action}">
<table border="0">
	<tr>
		<td align="right">
			<input type="checkbox" name="wpLockConfirm" />
		</td>
		<td align="left">{$lc}</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td align="left">
			<input type="submit" name="wpLock" value="{$lb}" />
		</td>
	</tr>
</table>
<input type="hidden" name="wpEditToken" value="{$token}" />
</form>
END
);

	}

	function doSubmit() {
		global $wgOut, $wgRequest, $wgReadOnlyFile;

		$wpLockConfirm = $wgRequest->getCheck( 'wpLockConfirm' );
		if ( ! $wpLockConfirm ) {
			$this->showForm( wfMsg( "locknoconfirm" ) );
			return;
		}
		if ( @! unlink( $wgReadOnlyFile ) ) {
			$wgOut->showFileDeleteError( $wgReadOnlyFile );
			return;
		}
		$titleObj = SpecialPage::getTitleFor( "Unlockdb" );
		$success = $titleObj->getFullURL( "action=success" );
		$wgOut->redirect( $success );
	}

	function showSuccess() {
		global $wgOut;
		global $ip;

		$wgOut->setPagetitle( wfMsg( "unlockdb" ) );
		$wgOut->setSubtitle( wfMsg( "unlockdbsuccesssub" ) );
		$wgOut->addWikiMsg( "unlockdbsuccesstext", $ip );
	}
}
