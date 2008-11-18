<?php
/**
 * See docs/skin.txt
 *
 * @todo document
 * @file
 * @ingroup Skins
 */

if( !defined( 'MEDIAWIKI' ) )
	die( -1 );

/**
 * @todo document
 * @ingroup Skins
 */
class SkinStandard extends Skin {

	/**
	 *
	 */
	function getHeadScripts( $allowUserJs ) {
		global $wgStylePath, $wgJsMimeType, $wgStyleVersion;

		$s = parent::getHeadScripts( $allowUserJs );
		if ( 3 == $this->qbSetting() ) { # Floating left
			$s .= "<script language='javascript' type='$wgJsMimeType' " .
			  "src='{$wgStylePath}/common/sticky.js?$wgStyleVersion'></script>\n";
		}
		return $s;
	}

	/**
	 *
	 */
	function getUserStyles() {
		global $wgStylePath, $wgStyleVersion;
		$s = '';
		if ( 3 == $this->qbSetting() ) { # Floating left
			$s .= "<style type='text/css'>\n" .
			  "@import '{$wgStylePath}/common/quickbar.css?$wgStyleVersion';\n</style>\n";
		} else if ( 4 == $this->qbSetting() ) { # Floating right
			$s .= "<style type='text/css'>\n" .
			  "@import '{$wgStylePath}/common/quickbar-right.css?$wgStyleVersion';\n</style>\n";
		}
		$s .= parent::getUserStyles();
		return $s;
	}

	/**
	 *
	 */
	function doGetUserStyles() {
		global $wgStylePath;

		$s = parent::doGetUserStyles();
		$qb = $this->qbSetting();

		if ( 2 == $qb ) { # Right
			$s .= "#quickbar { position: absolute; top: 4px; right: 4px; " .
			  "border-left: 2px solid #000000; }\n" .
			  "#article { margin-left: 4px; margin-right: 152px; }\n";
		} else if ( 1 == $qb || 3 == $qb ) {
			$s .= "#quickbar { position: absolute; top: 4px; left: 4px; " .
			  "border-right: 1px solid gray; }\n" .
			  "#article { margin-left: 152px; margin-right: 4px; }\n";
		} else if ( 4 == $qb) {
			$s .= "#quickbar { border-right: 1px solid gray; }\n" .
			  "#article { margin-right: 152px; margin-left: 4px; }\n";
		}
		return $s;
	}

	/**
	 *
	 */
	function getBodyOptions() {
		$a = parent::getBodyOptions();

		if ( 3 == $this->qbSetting() ) { # Floating left
			$qb = "setup(\"quickbar\")";
			if($a["onload"]) {
				$a["onload"] .= ";$qb";
			} else {
				$a["onload"] = $qb;
			}
		}
		return $a;
	}

	function doAfterContent() {
		global $wgContLang;
		$fname =  'SkinStandard::doAfterContent';
		wfProfileIn( $fname );
		wfProfileIn( $fname.'-1' );

		$s = "\n</div><br style=\"clear:both\" />\n";
		$s .= "\n<div id='footer'>";
		$s .= '<table border="0" cellspacing="0"><tr>';

		wfProfileOut( $fname.'-1' );
		wfProfileIn( $fname.'-2' );

		$qb = $this->qbSetting();
		$shove = ($qb != 0);
		$left = ($qb == 1 || $qb == 3);
		if($wgContLang->isRTL()) $left = !$left;

		if ( $shove && $left ) { # Left
				$s .= $this->getQuickbarCompensator();
		}
		wfProfileOut( $fname.'-2' );
		wfProfileIn( $fname.'-3' );
		$l = $wgContLang->isRTL() ? 'right' : 'left';
		$s .= "<td class='bottom' align='$l' valign='top'>";

		$s .= $this->bottomLinks();
		$s .= "\n<br />" . $this->mainPageLink()
		  . ' | ' . $this->aboutLink()
		  . ' | ' . $this->specialLink( 'recentchanges' )
		  . ' | ' . $this->searchForm()
		  . '<br /><span id="pagestats">' . $this->pageStats() . '</span>';

		$s .= "</td>";
		if ( $shove && !$left ) { # Right
			$s .= $this->getQuickbarCompensator();
		}
		$s .= "</tr></table>\n</div>\n</div>\n";

		wfProfileOut( $fname.'-3' );
		wfProfileIn( $fname.'-4' );
		if ( 0 != $qb ) { $s .= $this->quickBar(); }
		wfProfileOut( $fname.'-4' );
		wfProfileOut( $fname );
		return $s;
	}

	function quickBar() {
		global $wgOut, $wgTitle, $wgUser, $wgRequest, $wgContLang;
		global $wgEnableUploads, $wgRemoteUploads;

		$fname =  'Skin::quickBar';
		wfProfileIn( $fname );

		$action = $wgRequest->getText( 'action' );
		$wpPreview = $wgRequest->getBool( 'wpPreview' );
		$tns=$wgTitle->getNamespace();

		$s = "\n<div id='quickbar'>";
		$s .= "\n" . $this->logoText() . "\n<hr class='sep' />";

		$sep = "\n<br />";

		# Use the first heading from the Monobook sidebar as the "browse" section
		$bar = $this->buildSidebar();
		unset( $bar['SEARCH'] );
		unset( $bar['LANGUAGES'] );
		unset( $bar['TOOLBOX'] );
		$browseLinks = reset( $bar );

		foreach ( $browseLinks as $link ) {
			if ( $link['text'] != '-' ) {
				$s .= "<a href=\"{$link['href']}\">" .
					htmlspecialchars( $link['text'] ) . '</a>' . $sep;
			}
		}

		if( $wgUser->isLoggedIn() ) {
			$s.= $this->specialLink( 'watchlist' ) ;
			$s .= $sep . $this->makeKnownLink( $wgContLang->specialPage( 'Contributions' ),
				wfMsg( 'mycontris' ), 'target=' . wfUrlencode($wgUser->getName() ) );
		}
		// only show watchlist link if logged in
		$s .= "\n<hr class='sep' />";
		$articleExists = $wgTitle->getArticleId();
		if ( $wgOut->isArticle() || $action =='edit' || $action =='history' || $wpPreview) {
			if($wgOut->isArticle()) {
				$s .= '<strong>' . $this->editThisPage() . '</strong>';
			} else { # backlink to the article in edit or history mode
				if($articleExists){ # no backlink if no article
					switch($tns) {
						case NS_TALK:
						case NS_USER_TALK:
						case NS_PROJECT_TALK:
						case NS_IMAGE_TALK:
						case NS_MEDIAWIKI_TALK:
						case NS_TEMPLATE_TALK:
						case NS_HELP_TALK:
						case NS_CATEGORY_TALK:
							$text = wfMsg('viewtalkpage');
							break;
						case NS_MAIN:
							$text = wfMsg( 'articlepage' );
							break;
						case NS_USER:
							$text = wfMsg( 'userpage' );
							break;
						case NS_PROJECT:
							$text = wfMsg( 'projectpage' );
							break;
						case NS_IMAGE:
							$text = wfMsg( 'imagepage' );
							break;
						case NS_MEDIAWIKI:
							$text = wfMsg( 'mediawikipage' );
							break;
						case NS_TEMPLATE:
							$text = wfMsg( 'templatepage' );
							break;
						case NS_HELP:
							$text = wfMsg( 'viewhelppage' );
							break;
						case NS_CATEGORY:
							$text = wfMsg( 'categorypage' );
							break;
						default:
							$text= wfMsg( 'articlepage' );
					}

					$link = $wgTitle->getText();
					if ($nstext = $wgContLang->getNsText($tns) ) { # add namespace if necessary
						$link = $nstext . ':' . $link ;
					}

					$s .= $this->makeLink( $link, $text );
				} elseif( $wgTitle->getNamespace() != NS_SPECIAL ) {
					# we just throw in a "New page" text to tell the user that he's in edit mode,
					# and to avoid messing with the separator that is prepended to the next item
					$s .= '<strong>' . wfMsg('newpage') . '</strong>';
				}

			}

			# "Post a comment" link
			if( ( $wgTitle->isTalkPage() || $wgOut->showNewSectionLink() ) && $action != 'edit' && !$wpPreview )
				$s .= '<br />' . $this->makeKnownLinkObj( $wgTitle, wfMsg( 'postcomment' ), 'action=edit&section=new' );
			
			#if( $tns%2 && $action!='edit' && !$wpPreview) {
				#$s.= '<br />'.$this->makeKnownLink($wgTitle->getPrefixedText(),wfMsg('postcomment'),'action=edit&section=new');
			#}

			/*
			watching could cause problems in edit mode:
			if user edits article, then loads "watch this article" in background and then saves
			article with "Watch this article" checkbox disabled, the article is transparently
			unwatched. Therefore we do not show the "Watch this page" link in edit mode
			*/
			if ( $wgUser->isLoggedIn() && $articleExists) {
				if($action!='edit' && $action != 'submit' )
				{
					$s .= $sep . $this->watchThisPage();
				}
				if ( $wgTitle->userCan( 'edit' ) )
					$s .= $sep . $this->moveThisPage();
			}
			if ( $wgUser->isAllowed('delete') and $articleExists ) {
				$s .= $sep . $this->deleteThisPage() .
				$sep . $this->protectThisPage();
			}
			$s .= $sep . $this->talkLink();
			if ($articleExists && $action !='history') {
				$s .= $sep . $this->historyLink();
			}
			$s.=$sep . $this->whatLinksHere();

			if($wgOut->isArticleRelated()) {
				$s .= $sep . $this->watchPageLinksLink();
			}

			if ( NS_USER == $wgTitle->getNamespace()
				|| $wgTitle->getNamespace() == NS_USER_TALK ) {

				$id=User::idFromName($wgTitle->getText());
				$ip=User::isIP($wgTitle->getText());

				if($id||$ip) {
					$s .= $sep . $this->userContribsLink();
				}
				if( $this->showEmailUser( $id ) ) {
					$s .= $sep . $this->emailUserLink();
				}
			}
			$s .= "\n<br /><hr class='sep' />";
		}

		if ( $wgUser->isLoggedIn() && ( $wgEnableUploads || $wgRemoteUploads ) ) {
			$s .= $this->specialLink( 'upload' ) . $sep;
		}
		$s .= $this->specialLink( 'specialpages' )
		  . $sep . $this->bugReportsLink();

		global $wgSiteSupportPage;
		if( $wgSiteSupportPage ) {
			$s .= "\n<br /><a href=\"" . htmlspecialchars( $wgSiteSupportPage ) .
			  '" class="internal">' . wfMsg( 'sitesupport' ) . '</a>';
		}

		$s .= "\n<br /></div>\n";
		wfProfileOut( $fname );
		return $s;
	}


}


