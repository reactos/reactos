<?php
#
# Copyright (C) 2002, 2004 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * Contain log classes
 * @file
 */

/**
 * Class to simplify the use of log pages.
 * The logs are now kept in a table which is easier to manage and trim
 * than ever-growing wiki pages.
 *
 */
class LogPage {
	const DELETED_ACTION = 1;
	const DELETED_COMMENT = 2;
	const DELETED_USER = 4;
    const DELETED_RESTRICTED = 8;
	/* @access private */
	var $type, $action, $comment, $params, $target, $doer;
	/* @acess public */
	var $updateRecentChanges;

	/**
	  * Constructor
	  *
	  * @param string $type One of '', 'block', 'protect', 'rights', 'delete',
	  *               'upload', 'move'
	  * @param bool $rc Whether to update recent changes as well as the logging table
	  */
	function __construct( $type, $rc = true ) {
		$this->type = $type;
		$this->updateRecentChanges = $rc;
	}

	protected function saveContent() {
		global $wgUser, $wgLogRestrictions;
		$fname = 'LogPage::saveContent';

		$dbw = wfGetDB( DB_MASTER );
		$log_id = $dbw->nextSequenceValue( 'log_log_id_seq' );

		$this->timestamp = $now = wfTimestampNow();
		$data = array(
			'log_id' => $log_id,
			'log_type' => $this->type,
			'log_action' => $this->action,
			'log_timestamp' => $dbw->timestamp( $now ),
			'log_user' => $this->doer->getId(),
			'log_namespace' => $this->target->getNamespace(),
			'log_title' => $this->target->getDBkey(),
			'log_comment' => $this->comment,
			'log_params' => $this->params
		);
		$dbw->insert( 'logging', $data, $fname );
		$newId = !is_null($log_id) ? $log_id : $dbw->insertId();

		if( !($dbw->affectedRows() > 0) ) {
			wfDebugLog( "logging", "LogPage::saveContent failed to insert row - Error {$dbw->lastErrno()}: {$dbw->lastError()}" );
		}
		# And update recentchanges
		if( $this->updateRecentChanges ) {
			# Don't add private logs to RC!
			if( !isset($wgLogRestrictions[$this->type]) || $wgLogRestrictions[$this->type]=='*' ) {
				$titleObj = SpecialPage::getTitleFor( 'Log', $this->type );
				$rcComment = $this->getRcComment();
				RecentChange::notifyLog( $now, $titleObj, $this->doer, $rcComment, '',
					$this->type, $this->action, $this->target, $this->comment, $this->params, $newId );
			}
		}
		return true;
	}

	public function getRcComment() {
		$rcComment = $this->actionText;
		if( '' != $this->comment ) {
			if ($rcComment == '')
				$rcComment = $this->comment;
			else
				$rcComment .= ': ' . $this->comment;
		}
		return $rcComment;
	}

	/**
	 * @static
	 */
	public static function validTypes() {
		global $wgLogTypes;
		return $wgLogTypes;
	}

	/**
	 * @static
	 */
	public static function isLogType( $type ) {
		return in_array( $type, LogPage::validTypes() );
	}

	/**
	 * @static
	 */
	public static function logName( $type ) {
		global $wgLogNames, $wgMessageCache;

		if( isset( $wgLogNames[$type] ) ) {
			$wgMessageCache->loadAllMessages();
			return str_replace( '_', ' ', wfMsg( $wgLogNames[$type] ) );
		} else {
			// Bogus log types? Perhaps an extension was removed.
			return $type;
		}
	}

	/**
	 * @todo handle missing log types
	 * @param string $type logtype
	 * @return string Headertext of this logtype
	 */
	static function logHeader( $type ) {
		global $wgLogHeaders;
		return wfMsgExt($wgLogHeaders[$type],array('parseinline'));
	}

	/**
	 * @static
	 * @return HTML string
	 */
	static function actionText( $type, $action, $title = NULL, $skin = NULL, $params = array(), $filterWikilinks=false ) {
		global $wgLang, $wgContLang, $wgLogActions;

		$key = "$type/$action";

		if( $key == 'patrol/patrol' )
			return PatrolLog::makeActionText( $title, $params, $skin );

		if( isset( $wgLogActions[$key] ) ) {
			if( is_null( $title ) ) {
				$rv=wfMsg( $wgLogActions[$key] );
			} else {
				if( $skin ) {

					switch( $type ) {
						case 'move':
							$titleLink = $skin->makeLinkObj( $title, htmlspecialchars( $title->getPrefixedText() ), 'redirect=no' );
							$params[0] = $skin->makeLinkObj( Title::newFromText( $params[0] ), htmlspecialchars( $params[0] ) );
							break;
						case 'block':
							if( substr( $title->getText(), 0, 1 ) == '#' ) {
								$titleLink = $title->getText();
							} else {
								// TODO: Store the user identifier in the parameters
								// to make this faster for future log entries
								$id = User::idFromName( $title->getText() );
								$titleLink = $skin->userLink( $id, $title->getText() )
									. $skin->userToolLinks( $id, $title->getText(), false, Linker::TOOL_LINKS_NOBLOCK );
							}
							break;
						case 'rights':
							$text = $wgContLang->ucfirst( $title->getText() );
							$titleLink = $skin->makeLinkObj( Title::makeTitle( NS_USER, $text ) );
							break;
						case 'merge':
							$titleLink = $skin->makeLinkObj( $title, $title->getPrefixedText(), 'redirect=no' );
							$params[0] = $skin->makeLinkObj( Title::newFromText( $params[0] ), htmlspecialchars( $params[0] ) );
							$params[1] = $wgLang->timeanddate( $params[1] );
							break;
						default:
							$titleLink = $skin->makeLinkObj( $title );
					}

				} else {
					$titleLink = $title->getPrefixedText();
				}
				if( $key == 'rights/rights' ) {
					if ($skin) {
						$rightsnone = wfMsg( 'rightsnone' );
						foreach ( $params as &$param ) {
							$groupArray = array_map( 'trim', explode( ',', $param ) );
							$groupArray = array_map( array( 'User', 'getGroupName' ), $groupArray );
							$param = $wgLang->listToText( $groupArray );
						}
					} else {
						$rightsnone = wfMsgForContent( 'rightsnone' );
					}
					if( !isset( $params[0] ) || trim( $params[0] ) == '' )
						$params[0] = $rightsnone;
					if( !isset( $params[1] ) || trim( $params[1] ) == '' )
						$params[1] = $rightsnone;
				}
				if( count( $params ) == 0 ) {
					if ( $skin ) {
						$rv = wfMsg( $wgLogActions[$key], $titleLink );
					} else {
						$rv = wfMsgForContent( $wgLogActions[$key], $titleLink );
					}
				} else {
					array_unshift( $params, $titleLink );
					if ( $key == 'block/block' || $key == 'suppress/block' ) {
						if ( $skin ) {
							$params[1] = '<span title="' . htmlspecialchars( $params[1] ). '">' . $wgLang->translateBlockExpiry( $params[1] ) . '</span>';
						} else {
							$params[1] = $wgContLang->translateBlockExpiry( $params[1] );
						}
						$params[2] = isset( $params[2] )
										? self::formatBlockFlags( $params[2], is_null( $skin ) )
										: '';
					}
					$rv = wfMsgReal( $wgLogActions[$key], $params, true, !$skin );
				}
			}
		} else {
			global $wgLogActionsHandlers;
			if( isset( $wgLogActionsHandlers[$key] ) ) {
				$args = func_get_args();
				$rv = call_user_func_array( $wgLogActionsHandlers[$key], $args );
			} else {
				wfDebug( "LogPage::actionText - unknown action $key\n" );
				$rv = "$action";
			}
		}
		if( $filterWikilinks ) {
			$rv = str_replace( "[[", "", $rv );
			$rv = str_replace( "]]", "", $rv );
		}
		return $rv;
	}

	/**
	 * Add a log entry
	 * @param string $action one of '', 'block', 'protect', 'rights', 'delete', 'upload', 'move', 'move_redir'
	 * @param object &$target A title object.
	 * @param string $comment Description associated
	 * @param array $params Parameters passed later to wfMsg.* functions
	 * @param User $doer The user doing the action
	 */
	function addEntry( $action, $target, $comment, $params = array(), $doer = null ) {
		if ( !is_array( $params ) ) {
			$params = array( $params );
		}

		$this->action = $action;
		$this->target = $target;
		$this->comment = $comment;
		$this->params = LogPage::makeParamBlob( $params );
		
		if ($doer === null) {
			global $wgUser;
			$doer = $wgUser;
		} elseif (!is_object( $doer ) ) {
			$doer = User::newFromId( $doer );
		}
		
		$this->doer = $doer;

		$this->actionText = LogPage::actionText( $this->type, $action, $target, NULL, $params );

		return $this->saveContent();
	}

	/**
	 * Create a blob from a parameter array
	 * @static
	 */
	static function makeParamBlob( $params ) {
		return implode( "\n", $params );
	}

	/**
	 * Extract a parameter array from a blob
	 * @static
	 */
	static function extractParams( $blob ) {
		if ( $blob === '' ) {
			return array();
		} else {
			return explode( "\n", $blob );
		}
	}

	/**
	 * Convert a comma-delimited list of block log flags
	 * into a more readable (and translated) form
	 *
	 * @param $flags Flags to format
	 * @param $forContent Whether to localize the message depending of the user
	 *                    language
	 * @return string
	 */
	public static function formatBlockFlags( $flags, $forContent = false ) {
		$flags = explode( ',', trim( $flags ) );
		if( count( $flags ) > 0 ) {
			for( $i = 0; $i < count( $flags ); $i++ )
				$flags[$i] = self::formatBlockFlag( $flags[$i], $forContent );
			return '(' . implode( ', ', $flags ) . ')';
		} else {
			return '';
		}
	}

	/**
	 * Translate a block log flag if possible
	 *
	 * @param $flag Flag to translate
	 * @param $forContent Whether to localize the message depending of the user
	 *                    language
	 * @return string
	 */
	public static function formatBlockFlag( $flag, $forContent = false ) {
		static $messages = array();
		if( !isset( $messages[$flag] ) ) {
			$k = 'block-log-flags-' . $flag;
			if( $forContent )
				$msg = wfMsgForContent( $k );
			else
				$msg = wfMsg( $k );
			$messages[$flag] = htmlspecialchars( wfEmptyMsg( $k, $msg ) ? $flag : $msg );
		}
		return $messages[$flag];
	}
}

/**
 * Aliases for backwards compatibility with 1.6
 */
define( 'MW_LOG_DELETED_ACTION', LogPage::DELETED_ACTION );
define( 'MW_LOG_DELETED_USER', LogPage::DELETED_USER );
define( 'MW_LOG_DELETED_COMMENT', LogPage::DELETED_COMMENT );
define( 'MW_LOG_DELETED_RESTRICTED', LogPage::DELETED_RESTRICTED );
