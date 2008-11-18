<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Constructor
 */
function wfSpecialLockdb() {
	global $wgUser, $wgOut, $wgRequest;

	if( !$wgUser->isAllowed( 'siteadmin' ) ) {
		$wgOut->permissionRequired( 'siteadmin' );
		return;
	}

	# If the lock file isn't writable, we can do sweet bugger all
	global $wgReadOnlyFile;
	if( !is_writable( dirname( $wgReadOnlyFile ) ) ) {
		DBLockForm::notWritable();
		return;
	}

	$action = $wgRequest->getVal( 'action' );
	$f = new DBLockForm();

	if ( 'success' == $action ) {
		$f->showSuccess();
	} else if ( 'submit' == $action && $wgRequest->wasPosted() &&
		$wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ) ) ) {
		$f->doSubmit();
	} else {
		$f->showForm( '' );
	}
}

/**
 * A form to make the database readonly (eg for maintenance purposes).
 * @ingroup SpecialPage
 */
class DBLockForm {
	var $reason = '';

	function DBLockForm() {
		global $wgRequest;
		$this->reason = $wgRequest->getText( 'wpLockReason' );
	}

	function showForm( $err ) {
		global $wgOut, $wgUser;

		$wgOut->setPagetitle( wfMsg( 'lockdb' ) );
		$wgOut->addWikiMsg( 'lockdbtext' );

		if ( "" != $err ) {
			$wgOut->setSubtitle( wfMsg( 'formerror' ) );
			$wgOut->addHTML( '<p class="error">' . htmlspecialchars( $err ) . "</p>\n" );
		}
		$lc = htmlspecialchars( wfMsg( 'lockconfirm' ) );
		$lb = htmlspecialchars( wfMsg( 'lockbtn' ) );
		$elr = htmlspecialchars( wfMsg( 'enterlockreason' ) );
		$titleObj = SpecialPage::getTitleFor( 'Lockdb' );
		$action = $titleObj->escapeLocalURL( 'action=submit' );
		$reason = htmlspecialchars( $this->reason );
		$token = htmlspecialchars( $wgUser->editToken() );

		$wgOut->addHTML( <<<END
<form id="lockdb" method="post" action="{$action}">
{$elr}:
<textarea name="wpLockReason" rows="10" cols="60" wrap="virtual">{$reason}</textarea>
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
		global $wgOut, $wgUser, $wgLang, $wgRequest;
		global $wgReadOnlyFile;

		if ( ! $wgRequest->getCheck( 'wpLockConfirm' ) ) {
			$this->showForm( wfMsg( 'locknoconfirm' ) );
			return;
		}
		$fp = @fopen( $wgReadOnlyFile, 'w' );

		if ( false === $fp ) {
			# This used to show a file not found error, but the likeliest reason for fopen()
			# to fail at this point is insufficient permission to write to the file...good old
			# is_writable() is plain wrong in some cases, it seems...
			self::notWritable();
			return;
		}
		fwrite( $fp, $this->reason );
		fwrite( $fp, "\n<p>(by " . $wgUser->getName() . " at " .
		  $wgLang->timeanddate( wfTimestampNow() ) . ")\n" );
		fclose( $fp );

		$titleObj = SpecialPage::getTitleFor( 'Lockdb' );
		$wgOut->redirect( $titleObj->getFullURL( 'action=success' ) );
	}

	function showSuccess() {
		global $wgOut;

		$wgOut->setPagetitle( wfMsg( 'lockdb' ) );
		$wgOut->setSubtitle( wfMsg( 'lockdbsuccesssub' ) );
		$wgOut->addWikiMsg( 'lockdbsuccesstext' );
	}

	public static function notWritable() {
		global $wgOut;
		$wgOut->showErrorPage( 'lockdb', 'lockfilenotwritable' );
	}
}
