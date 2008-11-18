<?php
/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @author <brion@pobox.com>
 * @author <mail@tgries.de>
 * @author Tim Starling
 *
 */


/**
 * Stores a single person's name and email address.
 * These are passed in via the constructor, and will be returned in SMTP
 * header format when requested.
 */
class MailAddress {
	/**
	 * @param $address Mixed: string with an email address, or a User object
	 * @param $name String: human-readable name if a string address is given
	 */
	function __construct( $address, $name=null ) {
		if( is_object( $address ) && $address instanceof User ) {
			$this->address = $address->getEmail();
			$this->name = $address->getName();
		} else {
			$this->address = strval( $address );
			$this->name = strval( $name );
		}
	}

	/**
	 * Return formatted and quoted address to insert into SMTP headers
	 * @return string
	 */
	function toString() {
		# PHP's mail() implementation under Windows is somewhat shite, and
		# can't handle "Joe Bloggs <joe@bloggs.com>" format email addresses,
		# so don't bother generating them
		if( $this->name != '' && !wfIsWindows() ) {
			$quoted = wfQuotedPrintable( $this->name );
			if( strpos( $quoted, '.' ) !== false || strpos( $quoted, ',' ) !== false ) {
				$quoted = '"' . $quoted . '"';
			}
			return "$quoted <{$this->address}>";
		} else {
			return $this->address;
		}
	}

	function __toString() {
		return $this->toString();
	}
}


/**
 * Collection of static functions for sending mail
 */
class UserMailer {
	/**
	 * Send mail using a PEAR mailer
	 */
	protected static function sendWithPear($mailer, $dest, $headers, $body)
	{
		$mailResult = $mailer->send($dest, $headers, $body);

		# Based on the result return an error string,
		if( PEAR::isError( $mailResult ) ) {
			wfDebug( "PEAR::Mail failed: " . $mailResult->getMessage() . "\n" );
			return new WikiError( $mailResult->getMessage() );
		} else {
			return true;
		}
	}

	/**
	 * This function will perform a direct (authenticated) login to
	 * a SMTP Server to use for mail relaying if 'wgSMTP' specifies an
	 * array of parameters. It requires PEAR:Mail to do that.
	 * Otherwise it just uses the standard PHP 'mail' function.
	 *
	 * @param $to MailAddress: recipient's email
	 * @param $from MailAddress: sender's email
	 * @param $subject String: email's subject.
	 * @param $body String: email's text.
	 * @param $replyto String: optional reply-to email (default: null).
	 * @return mixed True on success, a WikiError object on failure.
	 */
	static function send( $to, $from, $subject, $body, $replyto=null ) {
		global $wgSMTP, $wgOutputEncoding, $wgErrorString, $wgEnotifImpersonal;
		global $wgEnotifMaxRecips;

		if ( is_array( $to ) ) {
			wfDebug( __METHOD__.': sending mail to ' . implode( ',', $to ) . "\n" );
		} else {
			wfDebug( __METHOD__.': sending mail to ' . implode( ',', array( $to->toString() ) ) . "\n" );
		}

		if (is_array( $wgSMTP )) {
			require_once( 'Mail.php' );

			$msgid = str_replace(" ", "_", microtime());
			if (function_exists('posix_getpid'))
				$msgid .= '.' . posix_getpid();

			if (is_array($to)) {
				$dest = array();
				foreach ($to as $u)
					$dest[] = $u->address;
			} else
				$dest = $to->address;

			$headers['From'] = $from->toString();

			if ($wgEnotifImpersonal) {
				$headers['To'] = 'undisclosed-recipients:;';
			}
			else {
				$headers['To'] = implode( ", ", (array )$dest );
			}

			if ( $replyto ) {
				$headers['Reply-To'] = $replyto->toString();
			}
			$headers['Subject'] = wfQuotedPrintable( $subject );
			$headers['Date'] = date( 'r' );
			$headers['MIME-Version'] = '1.0';
			$headers['Content-type'] = 'text/plain; charset='.$wgOutputEncoding;
			$headers['Content-transfer-encoding'] = '8bit';
			$headers['Message-ID'] = "<$msgid@" . $wgSMTP['IDHost'] . '>'; // FIXME
			$headers['X-Mailer'] = 'MediaWiki mailer';

			// Create the mail object using the Mail::factory method
			$mail_object =& Mail::factory('smtp', $wgSMTP);
			if( PEAR::isError( $mail_object ) ) {
				wfDebug( "PEAR::Mail factory failed: " . $mail_object->getMessage() . "\n" );
				return new WikiError( $mail_object->getMessage() );
			}

			wfDebug( "Sending mail via PEAR::Mail to $dest\n" );
			$chunks = array_chunk( (array)$dest, $wgEnotifMaxRecips );
			foreach ($chunks as $chunk) {
				$e = self::sendWithPear($mail_object, $chunk, $headers, $body);
				if( WikiError::isError( $e ) )
					return $e;
			}
		} else	{
			# In the following $headers = expression we removed "Reply-To: {$from}\r\n" , because it is treated differently
			# (fifth parameter of the PHP mail function, see some lines below)

			# Line endings need to be different on Unix and Windows due to
			# the bug described at http://trac.wordpress.org/ticket/2603
			if ( wfIsWindows() ) {
				$body = str_replace( "\n", "\r\n", $body );
				$endl = "\r\n";
			} else {
				$endl = "\n";
			}
			$headers =
				"MIME-Version: 1.0$endl" .
				"Content-type: text/plain; charset={$wgOutputEncoding}$endl" .
				"Content-Transfer-Encoding: 8bit$endl" .
				"X-Mailer: MediaWiki mailer$endl".
				'From: ' . $from->toString();
			if ($replyto) {
				$headers .= "{$endl}Reply-To: " . $replyto->toString();
			}

			$wgErrorString = '';
			$html_errors = ini_get( 'html_errors' );
			ini_set( 'html_errors', '0' );
			set_error_handler( array( 'UserMailer', 'errorHandler' ) );
			wfDebug( "Sending mail via internal mail() function\n" );

			if (function_exists('mail')) {
				if (is_array($to)) {
					foreach ($to as $recip) {
						$sent = mail( $recip->toString(), wfQuotedPrintable( $subject ), $body, $headers );
					}
				} else {
					$sent = mail( $to->toString(), wfQuotedPrintable( $subject ), $body, $headers );
				}
			} else {
				$wgErrorString = 'PHP is not configured to send mail';
			}

			restore_error_handler();
			ini_set( 'html_errors', $html_errors );

			if ( $wgErrorString ) {
				wfDebug( "Error sending mail: $wgErrorString\n" );
				return new WikiError( $wgErrorString );
			} elseif (! $sent) {
				//mail function only tells if there's an error
				wfDebug( "Error sending mail\n" );
				return new WikiError( 'mailer error' );
			} else {
				return true;
			}
		}
	}

	/**
	 * Get the mail error message in global $wgErrorString
	 *
	 * @param $code Integer: error number
	 * @param $string String: error message
	 */
	static function errorHandler( $code, $string ) {
		global $wgErrorString;
		$wgErrorString = preg_replace( '/^mail\(\)(\s*\[.*?\])?: /', '', $string );
	}

	/**
	 * Converts a string into a valid RFC 822 "phrase", such as is used for the sender name
	 */
	static function rfc822Phrase( $phrase ) {
		$phrase = strtr( $phrase, array( "\r" => '', "\n" => '', '"' => '' ) );
		return '"' . $phrase . '"';
	}
}

/**
 * This module processes the email notifications when the current page is
 * changed. It looks up the table watchlist to find out which users are watching
 * that page.
 *
 * The current implementation sends independent emails to each watching user for
 * the following reason:
 *
 * - Each watching user will be notified about the page edit time expressed in
 * his/her local time (UTC is shown additionally). To achieve this, we need to
 * find the individual timeoffset of each watching user from the preferences..
 *
 * Suggested improvement to slack down the number of sent emails: We could think
 * of sending out bulk mails (bcc:user1,user2...) for all these users having the
 * same timeoffset in their preferences.
 *
 * Visit the documentation pages under http://meta.wikipedia.com/Enotif
 *
 *
 */
class EmailNotification {
	/**@{{
	 * @private
	 */
	var $to, $subject, $body, $replyto, $from;
	var $user, $title, $timestamp, $summary, $minorEdit, $oldid, $composed_common, $editor;
	var $mailTargets = array();

	/**@}}*/

	/**
	 * Send emails corresponding to the user $editor editing the page $title.
	 * Also updates wl_notificationtimestamp.
	 *
	 * May be deferred via the job queue.
	 *
	 * @param $editor User object
	 * @param $title Title object
	 * @param $timestamp
	 * @param $summary
	 * @param $minorEdit
	 * @param $oldid (default: false)
	 */
	function notifyOnPageChange($editor, $title, $timestamp, $summary, $minorEdit, $oldid = false) {
		global $wgEnotifUseJobQ;

		if( $title->getNamespace() < 0 )
			return;

		if ($wgEnotifUseJobQ) {
			$params = array(
				"editor" => $editor->getName(),
				"editorID" => $editor->getID(),
				"timestamp" => $timestamp,
				"summary" => $summary,
				"minorEdit" => $minorEdit,
				"oldid" => $oldid);
			$job = new EnotifNotifyJob( $title, $params );
			$job->insert();
		} else {
			$this->actuallyNotifyOnPageChange($editor, $title, $timestamp, $summary, $minorEdit, $oldid);
		}

	}

	/*
	 * Immediate version of notifyOnPageChange().
	 *
	 * Send emails corresponding to the user $editor editing the page $title.
	 * Also updates wl_notificationtimestamp.
	 *
	 * @param $editor User object
	 * @param $title Title object
	 * @param $timestamp
	 * @param $summary
	 * @param $minorEdit
	 * @param $oldid (default: false)
	 */
	function actuallyNotifyOnPageChange($editor, $title, $timestamp, $summary, $minorEdit, $oldid=false) {

		# we use $wgPasswordSender as sender's address
		global $wgEnotifWatchlist;
		global $wgEnotifMinorEdits, $wgEnotifUserTalk, $wgShowUpdatedMarker;
		global $wgEnotifImpersonal;

		wfProfileIn( __METHOD__ );

		# The following code is only run, if several conditions are met:
		# 1. EmailNotification for pages (other than user_talk pages) must be enabled
		# 2. minor edits (changes) are only regarded if the global flag indicates so

		$isUserTalkPage = ($title->getNamespace() == NS_USER_TALK);
		$enotifusertalkpage = ($isUserTalkPage && $wgEnotifUserTalk);
		$enotifwatchlistpage = $wgEnotifWatchlist;

		$this->title = $title;
		$this->timestamp = $timestamp;
		$this->summary = $summary;
		$this->minorEdit = $minorEdit;
		$this->oldid = $oldid;
		$this->editor = $editor;
		$this->composed_common = false;

		$userTalkId = false;

		if ( (!$minorEdit || $wgEnotifMinorEdits) ) {
			if ( $wgEnotifUserTalk && $isUserTalkPage ) {
				$targetUser = User::newFromName( $title->getText() );
				if ( !$targetUser || $targetUser->isAnon() ) {
					wfDebug( __METHOD__.": user talk page edited, but user does not exist\n" );
				} elseif ( $targetUser->getId() == $editor->getId() ) {
					wfDebug( __METHOD__.": user edited their own talk page, no notification sent\n" );
				} elseif( $targetUser->getOption( 'enotifusertalkpages' ) ) {
					wfDebug( __METHOD__.": sending talk page update notification\n" );
					$this->compose( $targetUser );
					$userTalkId = $targetUser->getId();
				} else {
					wfDebug( __METHOD__.": talk page owner doesn't want notifications\n" );
				}
			}

			if ( $wgEnotifWatchlist ) {
				// Send updates to watchers other than the current editor
				$userCondition = 'wl_user != ' . $editor->getID();
				if ( $userTalkId !== false ) {
					// Already sent an email to this person
					$userCondition .= ' AND wl_user != ' . intval( $userTalkId );
				}
				$dbr = wfGetDB( DB_SLAVE );

				list( $user ) = $dbr->tableNamesN( 'user' );

				$res = $dbr->select( array( 'watchlist', 'user' ),
					array( "$user.*" ),
					array(
						'wl_user=user_id',
						'wl_title' => $title->getDBkey(),
						'wl_namespace' => $title->getNamespace(),
						$userCondition,
						'wl_notificationtimestamp IS NULL',
					), __METHOD__ );
				$userArray = UserArray::newFromResult( $res );

				foreach ( $userArray as $watchingUser ) {
					if ( $watchingUser->getOption( 'enotifwatchlistpages' ) &&
						( !$minorEdit || $watchingUser->getOption('enotifminoredits') ) &&
						$watchingUser->isEmailConfirmed() )
					{
						$this->compose( $watchingUser );
					}
				}
			}
		}

		global $wgUsersNotifiedOnAllChanges;
		foreach ( $wgUsersNotifiedOnAllChanges as $name ) {
			$user = User::newFromName( $name );
			$this->compose( $user );
		}

		$this->sendMails();

		if ( $wgShowUpdatedMarker || $wgEnotifWatchlist ) {
			# Mark the changed watch-listed page with a timestamp, so that the page is
			# listed with an "updated since your last visit" icon in the watch list. Do
			# not do this to users for their own edits.
			$dbw = wfGetDB( DB_MASTER );
			$dbw->update( 'watchlist',
				array( /* SET */
					'wl_notificationtimestamp' => $dbw->timestamp($timestamp)
				), array( /* WHERE */
					'wl_title' => $title->getDBkey(),
					'wl_namespace' => $title->getNamespace(),
					'wl_notificationtimestamp IS NULL',
					'wl_user != ' . $editor->getID()
				), __METHOD__
			);
		}

		wfProfileOut( __METHOD__ );
	} # function NotifyOnChange

	/**
	 * @private
	 */
	function composeCommonMailtext() {
		global $wgPasswordSender, $wgNoReplyAddress;
		global $wgEnotifFromEditor, $wgEnotifRevealEditorAddress;
		global $wgEnotifImpersonal;

		$this->composed_common = true;

		$summary = ($this->summary == '') ? ' - ' : $this->summary;
		$medit   = ($this->minorEdit) ? wfMsg( 'minoredit' ) : '';

		# You as the WikiAdmin and Sysops can make use of plenty of
		# named variables when composing your notification emails while
		# simply editing the Meta pages

		$subject = wfMsgForContent( 'enotif_subject' );
		$body    = wfMsgForContent( 'enotif_body' );
		$from    = ''; /* fail safe */
		$replyto = ''; /* fail safe */
		$keys    = array();

		# regarding the use of oldid as an indicator for the last visited version, see also
		# http://bugzilla.wikipeda.org/show_bug.cgi?id=603 "Delete + undelete cycle doesn't preserve old_id"
		# However, in the case of a new page which is already watched, we have no previous version to compare
		if( $this->oldid ) {
			$difflink = $this->title->getFullUrl( 'diff=0&oldid=' . $this->oldid );
			$keys['$NEWPAGE'] = wfMsgForContent( 'enotif_lastvisited', $difflink );
			$keys['$OLDID']   = $this->oldid;
			$keys['$CHANGEDORCREATED'] = wfMsgForContent( 'changed' );
		} else {
			$keys['$NEWPAGE'] = wfMsgForContent( 'enotif_newpagetext' );
			# clear $OLDID placeholder in the message template
			$keys['$OLDID']   = '';
			$keys['$CHANGEDORCREATED'] = wfMsgForContent( 'created' );
		}

		if ($wgEnotifImpersonal && $this->oldid)
			/*
			 * For impersonal mail, show a diff link to the last
			 * revision.
			 */
			$keys['$NEWPAGE'] = wfMsgForContent('enotif_lastdiff',
					$this->title->getFullURL("oldid={$this->oldid}&diff=prev"));

		$body = strtr( $body, $keys );
		$pagetitle = $this->title->getPrefixedText();
		$keys['$PAGETITLE']          = $pagetitle;
		$keys['$PAGETITLE_URL']      = $this->title->getFullUrl();

		$keys['$PAGEMINOREDIT']      = $medit;
		$keys['$PAGESUMMARY']        = $summary;

		$subject = strtr( $subject, $keys );

		# Reveal the page editor's address as REPLY-TO address only if
		# the user has not opted-out and the option is enabled at the
		# global configuration level.
		$editor = $this->editor;
		$name    = $editor->getName();
		$adminAddress = new MailAddress( $wgPasswordSender, 'WikiAdmin' );
		$editorAddress = new MailAddress( $editor );
		if( $wgEnotifRevealEditorAddress
		    && ( $editor->getEmail() != '' )
		    && $editor->getOption( 'enotifrevealaddr' ) ) {
			if( $wgEnotifFromEditor ) {
				$from    = $editorAddress;
			} else {
				$from    = $adminAddress;
				$replyto = $editorAddress;
			}
		} else {
			$from    = $adminAddress;
			$replyto = new MailAddress( $wgNoReplyAddress );
		}

		if( $editor->isIP( $name ) ) {
			#real anon (user:xxx.xxx.xxx.xxx)
			$utext = wfMsgForContent('enotif_anon_editor', $name);
			$subject = str_replace('$PAGEEDITOR', $utext, $subject);
			$keys['$PAGEEDITOR']       = $utext;
			$keys['$PAGEEDITOR_EMAIL'] = wfMsgForContent( 'noemailtitle' );
		} else {
			$subject = str_replace('$PAGEEDITOR', $name, $subject);
			$keys['$PAGEEDITOR']          = $name;
			$emailPage = SpecialPage::getSafeTitleFor( 'Emailuser', $name );
			$keys['$PAGEEDITOR_EMAIL'] = $emailPage->getFullUrl();
		}
		$userPage = $editor->getUserPage();
		$keys['$PAGEEDITOR_WIKI'] = $userPage->getFullUrl();
		$body = strtr( $body, $keys );
		$body = wordwrap( $body, 72 );

		# now save this as the constant user-independent part of the message
		$this->from    = $from;
		$this->replyto = $replyto;
		$this->subject = $subject;
		$this->body    = $body;
	}

	/**
	 * Compose a mail to a given user and either queue it for sending, or send it now,
	 * depending on settings.
	 *
	 * Call sendMails() to send any mails that were queued.
	 */
	function compose( $user ) {
		global $wgEnotifImpersonal;

		if ( !$this->composed_common )
			$this->composeCommonMailtext();

		if ( $wgEnotifImpersonal ) {
			$this->mailTargets[] = new MailAddress( $user );
		} else {
			$this->sendPersonalised( $user );
		}
	}

	/**
	 * Send any queued mails
	 */
	function sendMails() {
		global $wgEnotifImpersonal;
		if ( $wgEnotifImpersonal ) {
			$this->sendImpersonal( $this->mailTargets );
		}
	}

	/**
	 * Does the per-user customizations to a notification e-mail (name,
	 * timestamp in proper timezone, etc) and sends it out.
	 * Returns true if the mail was sent successfully.
	 *
	 * @param User $watchingUser
	 * @param object $mail
	 * @return bool
	 * @private
	 */
	function sendPersonalised( $watchingUser ) {
		global $wgLang;
		// From the PHP manual:
		//     Note:  The to parameter cannot be an address in the form of "Something <someone@example.com>".
		//     The mail command will not parse this properly while talking with the MTA.
		$to = new MailAddress( $watchingUser );
		$body = str_replace( '$WATCHINGUSERNAME', $watchingUser->getName() , $this->body );

		$timecorrection = $watchingUser->getOption( 'timecorrection' );

		# $PAGEEDITDATE is the time and date of the page change
		# expressed in terms of individual local time of the notification
		# recipient, i.e. watching user
		$body = str_replace('$PAGEEDITDATE',
			$wgLang->timeanddate( $this->timestamp, true, false, $timecorrection ),
			$body);

		return UserMailer::send($to, $this->from, $this->subject, $body, $this->replyto);
	}

	/**
	 * Same as sendPersonalised but does impersonal mail suitable for bulk
	 * mailing.  Takes an array of MailAddress objects.
	 */
	function sendImpersonal( $addresses ) {
		global $wgLang;

		if (empty($addresses))
			return;

		$body = str_replace(
				array(	'$WATCHINGUSERNAME',
					'$PAGEEDITDATE'),
				array(	wfMsgForContent('enotif_impersonal_salutation'),
					$wgLang->timeanddate($this->timestamp, true, false, false)),
				$this->body);

		return UserMailer::send($addresses, $this->from, $this->subject, $body, $this->replyto);
	}

} # end of class EmailNotification

/**
 * Backwards compatibility functions
 */
function wfRFC822Phrase( $s ) {
	return UserMailer::rfc822Phrase( $s );
}

function userMailer( $to, $from, $subject, $body, $replyto=null ) {
	return UserMailer::send( $to, $from, $subject, $body, $replyto );
}
