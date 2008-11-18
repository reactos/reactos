<?php

/**
 * Provides the UI through which users can perform editing
 * operations on their watchlist
 *
 * @ingroup Watchlist
 * @author Rob Church <robchur@gmail.com>
 */
class WatchlistEditor {

	/**
	 * Editing modes
	 */
	const EDIT_CLEAR = 1;
	const EDIT_RAW = 2;
	const EDIT_NORMAL = 3;

	/**
	 * Main execution point
	 *
	 * @param $user User
	 * @param $output OutputPage
	 * @param $request WebRequest
	 * @param $mode int
	 */
	public function execute( $user, $output, $request, $mode ) {
		global $wgUser;
		if( wfReadOnly() ) {
			$output->readOnlyPage();
			return;
		}
		switch( $mode ) {
			case self::EDIT_CLEAR:
				// The "Clear" link scared people too much.
				// Pass on to the raw editor, from which it's very easy to clear.
			case self::EDIT_RAW:
				$output->setPageTitle( wfMsg( 'watchlistedit-raw-title' ) );
				if( $request->wasPosted() && $this->checkToken( $request, $wgUser ) ) {
					$wanted = $this->extractTitles( $request->getText( 'titles' ) );
					$current = $this->getWatchlist( $user );
					if( count( $wanted ) > 0 ) {
						$toWatch = array_diff( $wanted, $current );
						$toUnwatch = array_diff( $current, $wanted );
						$this->watchTitles( $toWatch, $user );
						$this->unwatchTitles( $toUnwatch, $user );
						$user->invalidateCache();
						if( count( $toWatch ) > 0 || count( $toUnwatch ) > 0 )
							$output->addHtml( wfMsgExt( 'watchlistedit-raw-done', 'parse' ) );
						if( ( $count = count( $toWatch ) ) > 0 ) {
							$output->addHtml( wfMsgExt( 'watchlistedit-raw-added', 'parse', $count ) );
							$this->showTitles( $toWatch, $output, $wgUser->getSkin() );
						}
						if( ( $count = count( $toUnwatch ) ) > 0 ) {
							$output->addHtml( wfMsgExt( 'watchlistedit-raw-removed', 'parse', $count ) );
							$this->showTitles( $toUnwatch, $output, $wgUser->getSkin() );
						}
					} else {
						$this->clearWatchlist( $user );
						$user->invalidateCache();
						$output->addHtml( wfMsgExt( 'watchlistedit-raw-removed', 'parse', count( $current ) ) );
						$this->showTitles( $current, $output, $wgUser->getSkin() );
					}
				}
				$this->showRawForm( $output, $user );
				break;
			case self::EDIT_NORMAL:
				$output->setPageTitle( wfMsg( 'watchlistedit-normal-title' ) );
				if( $request->wasPosted() && $this->checkToken( $request, $wgUser ) ) {
					$titles = $this->extractTitles( $request->getArray( 'titles' ) );
					$this->unwatchTitles( $titles, $user );
					$user->invalidateCache();
					$output->addHtml( wfMsgExt( 'watchlistedit-normal-done', 'parse',
						$GLOBALS['wgLang']->formatNum( count( $titles ) ) ) );
					$this->showTitles( $titles, $output, $wgUser->getSkin() );
				}
				$this->showNormalForm( $output, $user );
		}
	}

	/**
	 * Check the edit token from a form submission
	 *
	 * @param $request WebRequest
	 * @param $user User
	 * @return bool
	 */
	private function checkToken( $request, $user ) {
		return $user->matchEditToken( $request->getVal( 'token' ), 'watchlistedit' );
	}

	/**
	 * Extract a list of titles from a blob of text, returning
	 * (prefixed) strings; unwatchable titles are ignored
	 *
	 * @param $list mixed
	 * @return array
	 */
	private function extractTitles( $list ) {
		$titles = array();
		if( !is_array( $list ) ) {
			$list = explode( "\n", trim( $list ) );
			if( !is_array( $list ) )
				return array();
		}
		foreach( $list as $text ) {
			$text = trim( $text );
			if( strlen( $text ) > 0 ) {
				$title = Title::newFromText( $text );
				if( $title instanceof Title && $title->isWatchable() )
					$titles[] = $title->getPrefixedText();
			}
		}
		return array_unique( $titles );
	}

	/**
	 * Print out a list of linked titles
	 *
	 * $titles can be an array of strings or Title objects; the former
	 * is preferred, since Titles are very memory-heavy
	 *
	 * @param $titles An array of strings, or Title objects
	 * @param $output OutputPage
	 * @param $skin Skin
	 */
	private function showTitles( $titles, $output, $skin ) {
		$talk = wfMsgHtml( 'talkpagelinktext' );
		// Do a batch existence check
		$batch = new LinkBatch();
		foreach( $titles as $title ) {
			if( !$title instanceof Title )
				$title = Title::newFromText( $title );
			if( $title instanceof Title ) {
				$batch->addObj( $title );
				$batch->addObj( $title->getTalkPage() );
			}
		}
		$batch->execute();
		// Print out the list
		$output->addHtml( "<ul>\n" );
		foreach( $titles as $title ) {
			if( !$title instanceof Title )
				$title = Title::newFromText( $title );
			if( $title instanceof Title ) {
				$output->addHtml( "<li>" . $skin->makeLinkObj( $title )
				. ' (' . $skin->makeLinkObj( $title->getTalkPage(), $talk ) . ")</li>\n" );
			}
		}
		$output->addHtml( "</ul>\n" );
	}

	/**
	 * Count the number of titles on a user's watchlist, excluding talk pages
	 *
	 * @param $user User
	 * @return int
	 */
	private function countWatchlist( $user ) {
		$dbr = wfGetDB( DB_MASTER );
		$res = $dbr->select( 'watchlist', 'COUNT(*) AS count', array( 'wl_user' => $user->getId() ), __METHOD__ );
		$row = $dbr->fetchObject( $res );
		return ceil( $row->count / 2 ); // Paranoia
	}

	/**
	 * Prepare a list of titles on a user's watchlist (excluding talk pages)
	 * and return an array of (prefixed) strings
	 *
	 * @param $user User
	 * @return array
	 */
	private function getWatchlist( $user ) {
		$list = array();
		$dbr = wfGetDB( DB_MASTER );
		$res = $dbr->select(
			'watchlist',
			'*',
			array(
				'wl_user' => $user->getId(),
			),
			__METHOD__
		);
		if( $res->numRows() > 0 ) {
			while( $row = $res->fetchObject() ) {
				$title = Title::makeTitleSafe( $row->wl_namespace, $row->wl_title );
				if( $title instanceof Title && !$title->isTalkPage() )
					$list[] = $title->getPrefixedText();
			}
			$res->free();
		}
		return $list;
	}

	/**
	 * Get a list of titles on a user's watchlist, excluding talk pages,
	 * and return as a two-dimensional array with namespace, title and
	 * redirect status
	 *
	 * @param $user User
	 * @return array
	 */
	private function getWatchlistInfo( $user ) {
		$titles = array();
		$dbr = wfGetDB( DB_MASTER );
		$uid = intval( $user->getId() );
		list( $watchlist, $page ) = $dbr->tableNamesN( 'watchlist', 'page' );
		$sql = "SELECT wl_namespace, wl_title, page_id, page_len, page_is_redirect
			FROM {$watchlist} LEFT JOIN {$page} ON ( wl_namespace = page_namespace
			AND wl_title = page_title ) WHERE wl_user = {$uid}";
		$res = $dbr->query( $sql, __METHOD__ );
		if( $res && $dbr->numRows( $res ) > 0 ) {
			$cache = LinkCache::singleton();
			while( $row = $dbr->fetchObject( $res ) ) {
				$title = Title::makeTitleSafe( $row->wl_namespace, $row->wl_title );
				if( $title instanceof Title ) {
					// Update the link cache while we're at it
					if( $row->page_id ) {
						$cache->addGoodLinkObj( $row->page_id, $title, $row->page_len, $row->page_is_redirect );
					} else {
						$cache->addBadLinkObj( $title );
					}
					// Ignore non-talk
					if( !$title->isTalkPage() )
						$titles[$row->wl_namespace][$row->wl_title] = $row->page_is_redirect;
				}
			}
		}
		return $titles;
	}

	/**
	 * Show a message indicating the number of items on the user's watchlist,
	 * and return this count for additional checking
	 *
	 * @param $output OutputPage
	 * @param $user User
	 * @return int
	 */
	private function showItemCount( $output, $user ) {
		if( ( $count = $this->countWatchlist( $user ) ) > 0 ) {
			$output->addHtml( wfMsgExt( 'watchlistedit-numitems', 'parse',
				$GLOBALS['wgLang']->formatNum( $count ) ) );
		} else {
			$output->addHtml( wfMsgExt( 'watchlistedit-noitems', 'parse' ) );
		}
		return $count;
	}

	/**
	 * Remove all titles from a user's watchlist
	 *
	 * @param $user User
	 */
	private function clearWatchlist( $user ) {
		$dbw = wfGetDB( DB_MASTER );
		$dbw->delete( 'watchlist', array( 'wl_user' => $user->getId() ), __METHOD__ );
	}

	/**
	 * Add a list of titles to a user's watchlist
	 *
	 * $titles can be an array of strings or Title objects; the former
	 * is preferred, since Titles are very memory-heavy
	 *
	 * @param $titles An array of strings, or Title objects
	 * @param $user User
	 */
	private function watchTitles( $titles, $user ) {
		$dbw = wfGetDB( DB_MASTER );
		$rows = array();
		foreach( $titles as $title ) {
			if( !$title instanceof Title )
				$title = Title::newFromText( $title );
			if( $title instanceof Title ) {
				$rows[] = array(
					'wl_user' => $user->getId(),
					'wl_namespace' => ( $title->getNamespace() & ~1 ),
					'wl_title' => $title->getDBkey(),
					'wl_notificationtimestamp' => null,
				);
				$rows[] = array(
					'wl_user' => $user->getId(),
					'wl_namespace' => ( $title->getNamespace() | 1 ),
					'wl_title' => $title->getDBkey(),
					'wl_notificationtimestamp' => null,
				);
			}
		}
		$dbw->insert( 'watchlist', $rows, __METHOD__, 'IGNORE' );
	}

	/**
	 * Remove a list of titles from a user's watchlist
	 *
	 * $titles can be an array of strings or Title objects; the former
	 * is preferred, since Titles are very memory-heavy
	 *
	 * @param $titles An array of strings, or Title objects
	 * @param $user User
	 */
	private function unwatchTitles( $titles, $user ) {
		$dbw = wfGetDB( DB_MASTER );
		foreach( $titles as $title ) {
			if( !$title instanceof Title )
				$title = Title::newFromText( $title );
			if( $title instanceof Title ) {
				$dbw->delete(
					'watchlist',
					array(
						'wl_user' => $user->getId(),
						'wl_namespace' => ( $title->getNamespace() & ~1 ),
						'wl_title' => $title->getDBkey(),
					),
					__METHOD__
				);
				$dbw->delete(
					'watchlist',
					array(
						'wl_user' => $user->getId(),
						'wl_namespace' => ( $title->getNamespace() | 1 ),
						'wl_title' => $title->getDBkey(),
					),
					__METHOD__
				);
			}
		}
	}

	/**
	 * Show the standard watchlist editing form
	 *
	 * @param $output OutputPage
	 * @param $user User
	 */
	private function showNormalForm( $output, $user ) {
		global $wgUser;
		if( ( $count = $this->showItemCount( $output, $user ) ) > 0 ) {
			$self = SpecialPage::getTitleFor( 'Watchlist' );
			$form  = Xml::openElement( 'form', array( 'method' => 'post',
				'action' => $self->getLocalUrl( 'action=edit' ) ) );
			$form .= Xml::hidden( 'token', $wgUser->editToken( 'watchlistedit' ) );
			$form .= '<fieldset><legend>' . wfMsgHtml( 'watchlistedit-normal-legend' ) . '</legend>';
			$form .= wfMsgExt( 'watchlistedit-normal-explain', 'parse' );
			foreach( $this->getWatchlistInfo( $user ) as $namespace => $pages ) {
				$form .= '<h2>' . $this->getNamespaceHeading( $namespace ) . '</h2>';
				$form .= '<ul>';
				foreach( $pages as $dbkey => $redirect ) {
					$title = Title::makeTitleSafe( $namespace, $dbkey );
					$form .= $this->buildRemoveLine( $title, $redirect, $wgUser->getSkin() );
				}
				$form .= '</ul>';
			}
			$form .= '<p>' . Xml::submitButton( wfMsg( 'watchlistedit-normal-submit' ) ) . '</p>';
			$form .= '</fieldset></form>';
			$output->addHtml( $form );
		}
	}

	/**
	 * Get the correct "heading" for a namespace
	 *
	 * @param $namespace int
	 * @return string
	 */
	private function getNamespaceHeading( $namespace ) {
		return $namespace == NS_MAIN
			? wfMsgHtml( 'blanknamespace' )
			: htmlspecialchars( $GLOBALS['wgContLang']->getFormattedNsText( $namespace ) );
	}

	/**
	 * Build a single list item containing a check box selecting a title
	 * and a link to that title, with various additional bits
	 *
	 * @param $title Title
	 * @param $redirect bool
	 * @param $skin Skin
	 * @return string
	 */
	private function buildRemoveLine( $title, $redirect, $skin ) {
		$link = $skin->makeLinkObj( $title );
		if( $redirect )
			$link = '<span class="watchlistredir">' . $link . '</span>';
		$tools[] = $skin->makeLinkObj( $title->getTalkPage(), wfMsgHtml( 'talkpagelinktext' ) );
		if( $title->exists() ) {
			$tools[] = $skin->makeKnownLinkObj( $title, wfMsgHtml( 'history_short' ), 'action=history' );
		}
		if( $title->getNamespace() == NS_USER && !$title->isSubpage() ) {
			$tools[] = $skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Contributions', $title->getText() ), wfMsgHtml( 'contributions' ) );
		}
		return '<li>'
			. Xml::check( 'titles[]', false, array( 'value' => $title->getPrefixedText() ) )
			. $link . ' (' . implode( ' | ', $tools ) . ')' . '</li>';
		}

	/**
	 * Show a form for editing the watchlist in "raw" mode
	 *
	 * @param $output OutputPage
	 * @param $user User
	 */
	public function showRawForm( $output, $user ) {
		global $wgUser;
		$this->showItemCount( $output, $user );
		$self = SpecialPage::getTitleFor( 'Watchlist' );
		$form  = Xml::openElement( 'form', array( 'method' => 'post',
			'action' => $self->getLocalUrl( 'action=raw' ) ) );
		$form .= Xml::hidden( 'token', $wgUser->editToken( 'watchlistedit' ) );
		$form .= '<fieldset><legend>' . wfMsgHtml( 'watchlistedit-raw-legend' ) . '</legend>';
		$form .= wfMsgExt( 'watchlistedit-raw-explain', 'parse' );
		$form .= Xml::label( wfMsg( 'watchlistedit-raw-titles' ), 'titles' );
		$form .= "<br />\n";
		$form .= Xml::openElement( 'textarea', array( 'id' => 'titles', 'name' => 'titles',
			'rows' => $wgUser->getIntOption( 'rows' ), 'cols' => $wgUser->getIntOption( 'cols' ) ) );
		$titles = $this->getWatchlist( $user );
		foreach( $titles as $title )
			$form .= htmlspecialchars( $title ) . "\n";
		$form .= '</textarea>';
		$form .= '<p>' . Xml::submitButton( wfMsg( 'watchlistedit-raw-submit' ) ) . '</p>';
		$form .= '</fieldset></form>';
		$output->addHtml( $form );
	}

	/**
	 * Determine whether we are editing the watchlist, and if so, what
	 * kind of editing operation
	 *
	 * @param $request WebRequest
	 * @param $par mixed
	 * @return int
	 */
	public static function getMode( $request, $par ) {
		$mode = strtolower( $request->getVal( 'action', $par ) );
		switch( $mode ) {
			case 'clear':
				return self::EDIT_CLEAR;
			case 'raw':
				return self::EDIT_RAW;
			case 'edit':
				return self::EDIT_NORMAL;
			default:
				return false;
		}
	}

	/**
	 * Build a set of links for convenient navigation
	 * between watchlist viewing and editing modes
	 *
	 * @param $skin Skin to use
	 * @return string
	 */
	public static function buildTools( $skin ) {
		$tools = array();
		$modes = array( 'view' => false, 'edit' => 'edit', 'raw' => 'raw' );
		foreach( $modes as $mode => $subpage ) {
			$tools[] = $skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Watchlist', $subpage ), wfMsgHtml( "watchlisttools-{$mode}" ) );
		}
		return implode( ' | ', $tools );
	}
}
