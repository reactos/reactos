<?php
if ( ! defined( 'MEDIAWIKI' ) )
	die( 1 );

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
 * Wrapper object for MediaWiki's localization functions,
 * to be passed to the template engine.
 *
 * @private
 * @ingroup Skins
 */
class MediaWiki_I18N {
	var $_context = array();

	function set($varName, $value) {
		$this->_context[$varName] = $value;
	}

	function translate($value) {
		wfProfileIn( __METHOD__ );

		// Hack for i18n:attributes in PHPTAL 1.0.0 dev version as of 2004-10-23
		$value = preg_replace( '/^string:/', '', $value );

		$value = wfMsg( $value );
		// interpolate variables
		$m = array();
		while (preg_match('/\$([0-9]*?)/sm', $value, $m)) {
			list($src, $var) = $m;
			wfSuppressWarnings();
			$varValue = $this->_context[$var];
			wfRestoreWarnings();
			$value = str_replace($src, $varValue, $value);
		}
		wfProfileOut( __METHOD__ );
		return $value;
	}
}

/**
 * Template-filler skin base class
 * Formerly generic PHPTal (http://phptal.sourceforge.net/) skin
 * Based on Brion's smarty skin
 * @copyright Copyright © Gabriel Wicke -- http://www.aulinx.de/
 *
 * @todo Needs some serious refactoring into functions that correspond
 * to the computations individual esi snippets need. Most importantly no body
 * parsing for most of those of course.
 *
 * @ingroup Skins
 */
class SkinTemplate extends Skin {
	/**#@+
	 * @private
	 */

	/**
	 * Name of our skin, set in initPage()
	 * It probably need to be all lower case.
	 */
	var $skinname;

	/**
	 * Stylesheets set to use
	 * Sub directory in ./skins/ where various stylesheets are located
	 */
	var $stylename;

	/**
	 * For QuickTemplate, the name of the subclass which
	 * will actually fill the template.
	 */
	var $template;

	/**
	 * An array of strings representing extra CSS files to load.  May include:
	 * 'IE', 'IE50', 'IE55', 'IE60', 'IE70', 'rtl'.
	 */
	var $cssfiles;

	/**#@-*/

	/**
	 * Setup the base parameters...
	 * Child classes should override this to set the name,
	 * style subdirectory, and template filler callback.
	 *
	 * @param OutputPage $out
	 */
	function initPage( &$out ) {
		parent::initPage( $out );
		$this->skinname  = 'monobook';
		$this->stylename = 'monobook';
		$this->template  = 'QuickTemplate';
		$this->cssfiles = array();
	}

	/**
	 * Create the template engine object; we feed it a bunch of data
	 * and eventually it spits out some HTML. Should have interface
	 * roughly equivalent to PHPTAL 0.7.
	 *
	 * @param string $callback (or file)
	 * @param string $repository subdirectory where we keep template files
	 * @param string $cache_dir
	 * @return object
	 * @private
	 */
	function setupTemplate( $classname, $repository=false, $cache_dir=false ) {
		return new $classname();
	}

	/**
	 * initialize various variables and generate the template
	 *
	 * @param OutputPage $out
	 * @public
	 */
	function outputPage( &$out ) {
		global $wgTitle, $wgArticle, $wgUser, $wgLang, $wgContLang, $wgOut;
		global $wgScript, $wgStylePath, $wgContLanguageCode;
		global $wgMimeType, $wgJsMimeType, $wgOutputEncoding, $wgRequest;
		global $wgXhtmlDefaultNamespace, $wgXhtmlNamespaces;
		global $wgDisableCounters, $wgLogo, $action, $wgFeedClasses, $wgHideInterlanguageLinks;
		global $wgMaxCredits, $wgShowCreditsIfMax;
		global $wgPageShowWatchingUsers;
		global $wgUseTrackbacks;
		global $wgArticlePath, $wgScriptPath, $wgServer, $wgLang, $wgCanonicalNamespaceNames;

		wfProfileIn( __METHOD__ );

		$oldid = $wgRequest->getVal( 'oldid' );
		$diff = $wgRequest->getVal( 'diff' );

		wfProfileIn( __METHOD__."-init" );
		$this->initPage( $out );

		$this->mTitle =& $wgTitle;
		$this->mUser =& $wgUser;

		$tpl = $this->setupTemplate( $this->template, 'skins' );

		#if ( $wgUseDatabaseMessages ) { // uncomment this to fall back to GetText
		$tpl->setTranslator(new MediaWiki_I18N());
		#}
		wfProfileOut( __METHOD__."-init" );

		wfProfileIn( __METHOD__."-stuff" );
		$this->thispage = $this->mTitle->getPrefixedDbKey();
		$this->thisurl = $this->mTitle->getPrefixedURL();
		$this->loggedin = $wgUser->isLoggedIn();
		$this->iscontent = ($this->mTitle->getNamespace() != NS_SPECIAL );
		$this->iseditable = ($this->iscontent and !($action == 'edit' or $action == 'submit'));
		$this->username = $wgUser->getName();
		$userPage = $wgUser->getUserPage();
		$this->userpage = $userPage->getPrefixedText();

		if ( $wgUser->isLoggedIn() || $this->showIPinHeader() ) {
			$this->userpageUrlDetails = self::makeUrlDetails( $this->userpage );
		} else {
			# This won't be used in the standard skins, but we define it to preserve the interface
			# To save time, we check for existence
			$this->userpageUrlDetails = self::makeKnownUrlDetails( $this->userpage );
		}

		$this->usercss =  $this->userjs = $this->userjsprev = false;
		$this->setupUserCss();
		$this->setupUserJs( $out->isUserJsAllowed() );
		$this->titletxt = $this->mTitle->getPrefixedText();
		wfProfileOut( __METHOD__."-stuff" );

		wfProfileIn( __METHOD__."-stuff2" );
		$tpl->set( 'title', $wgOut->getPageTitle() );
		$tpl->set( 'pagetitle', $wgOut->getHTMLTitle() );
		$tpl->set( 'displaytitle', $wgOut->mPageLinkTitle );
		$tpl->set( 'pageclass', Sanitizer::escapeClass( 'page-'.$this->mTitle->getPrefixedText() ) );

		$nsname = isset( $wgCanonicalNamespaceNames[ $this->mTitle->getNamespace() ] ) ?
		          $wgCanonicalNamespaceNames[ $this->mTitle->getNamespace() ] :
		          $this->mTitle->getNsText();

		$tpl->set( 'nscanonical', $nsname );
		$tpl->set( 'nsnumber', $this->mTitle->getNamespace() );
		$tpl->set( 'titleprefixeddbkey', $this->mTitle->getPrefixedDBKey() );
		$tpl->set( 'titletext', $this->mTitle->getText() );
		$tpl->set( 'articleid', $this->mTitle->getArticleId() );
		$tpl->set( 'currevisionid', isset( $wgArticle ) ? $wgArticle->getLatest() : 0 );

		$tpl->set( 'isarticle', $wgOut->isArticle() );

		$tpl->setRef( "thispage", $this->thispage );
		$subpagestr = $this->subPageSubtitle();
		$tpl->set(
			'subtitle',  !empty($subpagestr)?
			'<span class="subpages">'.$subpagestr.'</span>'.$out->getSubtitle():
			$out->getSubtitle()
		);
		$undelete = $this->getUndeleteLink();
		$tpl->set(
			"undelete", !empty($undelete)?
			'<span class="subpages">'.$undelete.'</span>':
			''
		);

		$tpl->set( 'catlinks', $this->getCategories());
		if( $wgOut->isSyndicated() ) {
			$feeds = array();
			foreach( $wgOut->getSyndicationLinks() as $format => $link ) {
				$feeds[$format] = array(
					'text' => wfMsg( "feed-$format" ),
					'href' => $link );
			}
			$tpl->setRef( 'feeds', $feeds );
		} else {
			$tpl->set( 'feeds', false );
		}
		if ($wgUseTrackbacks && $out->isArticleRelated()) {
			$tpl->set( 'trackbackhtml', $wgTitle->trackbackRDF() );
		} else {
			$tpl->set( 'trackbackhtml', null );
		}

		$tpl->setRef( 'xhtmldefaultnamespace', $wgXhtmlDefaultNamespace );
		$tpl->set( 'xhtmlnamespaces', $wgXhtmlNamespaces );
		$tpl->setRef( 'mimetype', $wgMimeType );
		$tpl->setRef( 'jsmimetype', $wgJsMimeType );
		$tpl->setRef( 'charset', $wgOutputEncoding );
		$tpl->set( 'headlinks', $out->getHeadLinks() );
		$tpl->set('headscripts', $out->getScript() );
		$tpl->setRef( 'wgScript', $wgScript );
		$tpl->setRef( 'skinname', $this->skinname );
		$tpl->set( 'skinclass', get_class( $this ) );
		$tpl->setRef( 'stylename', $this->stylename );
		$tpl->set( 'printable', $wgRequest->getBool( 'printable' ) );
		$tpl->setRef( 'loggedin', $this->loggedin );
		$tpl->set('nsclass', 'ns-'.$this->mTitle->getNamespace());
		$tpl->set('notspecialpage', $this->mTitle->getNamespace() != NS_SPECIAL);
		/* XXX currently unused, might get useful later
		$tpl->set( "editable", ($this->mTitle->getNamespace() != NS_SPECIAL ) );
		$tpl->set( "exists", $this->mTitle->getArticleID() != 0 );
		$tpl->set( "watch", $this->mTitle->userIsWatching() ? "unwatch" : "watch" );
		$tpl->set( "protect", count($this->mTitle->isProtected()) ? "unprotect" : "protect" );
		$tpl->set( "helppage", wfMsg('helppage'));
		*/
		$tpl->set( 'searchaction', $this->escapeSearchLink() );
		$tpl->set( 'search', trim( $wgRequest->getVal( 'search' ) ) );
		$tpl->setRef( 'stylepath', $wgStylePath );
		$tpl->setRef( 'articlepath', $wgArticlePath );
		$tpl->setRef( 'scriptpath', $wgScriptPath );
		$tpl->setRef( 'serverurl', $wgServer );
		$tpl->setRef( 'logopath', $wgLogo );
		$tpl->setRef( "lang", $wgContLanguageCode );
		$tpl->set( 'dir', $wgContLang->isRTL() ? "rtl" : "ltr" );
		$tpl->set( 'rtl', $wgContLang->isRTL() );
		$tpl->set( 'langname', $wgContLang->getLanguageName( $wgContLanguageCode ) );
		$tpl->set( 'showjumplinks', $wgUser->getOption( 'showjumplinks' ) );
		$tpl->set( 'username', $wgUser->isAnon() ? NULL : $this->username );
		$tpl->setRef( 'userpage', $this->userpage);
		$tpl->setRef( 'userpageurl', $this->userpageUrlDetails['href']);
		$tpl->set( 'userlang', $wgLang->getCode() );
		$tpl->set( 'pagecss', $this->setupPageCss() );
		$tpl->set( 'printcss', $this->getPrintCss() );
		$tpl->setRef( 'usercss', $this->usercss);
		$tpl->setRef( 'userjs', $this->userjs);
		$tpl->setRef( 'userjsprev', $this->userjsprev);
		global $wgUseSiteJs;
		if ($wgUseSiteJs) {
			$jsCache = $this->loggedin ? '&smaxage=0' : '';
			$tpl->set( 'jsvarurl',
				self::makeUrl('-',
					"action=raw$jsCache&gen=js&useskin=" .
						urlencode( $this->getSkinName() ) ) );
		} else {
			$tpl->set('jsvarurl', false);
		}
		$newtalks = $wgUser->getNewMessageLinks();

		if (count($newtalks) == 1 && $newtalks[0]["wiki"] === wfWikiID() ) {
			$usertitle = $this->mUser->getUserPage();
			$usertalktitle = $usertitle->getTalkPage();
			if( !$usertalktitle->equals( $this->mTitle ) ) {
				$ntl = wfMsg( 'youhavenewmessages',
					$this->makeKnownLinkObj(
						$usertalktitle,
						wfMsgHtml( 'newmessageslink' ),
						'redirect=no'
					),
					$this->makeKnownLinkObj(
						$usertalktitle,
						wfMsgHtml( 'newmessagesdifflink' ),
						'diff=cur'
					)
				);
				# Disable Cache
				$wgOut->setSquidMaxage(0);
			}
		} else if (count($newtalks)) {
			$sep = str_replace("_", " ", wfMsgHtml("newtalkseperator"));
			$msgs = array();
			foreach ($newtalks as $newtalk) {
				$msgs[] = wfElement("a",
					array('href' => $newtalk["link"]), $newtalk["wiki"]);
			}
			$parts = implode($sep, $msgs);
			$ntl = wfMsgHtml('youhavenewmessagesmulti', $parts);
			$wgOut->setSquidMaxage(0);
		} else {
			$ntl = '';
		}
		wfProfileOut( __METHOD__."-stuff2" );

		wfProfileIn( __METHOD__."-stuff3" );
		$tpl->setRef( 'newtalk', $ntl );
		$tpl->setRef( 'skin', $this);
		$tpl->set( 'logo', $this->logoText() );
		if ( $wgOut->isArticle() and (!isset( $oldid ) or isset( $diff )) and
			$wgArticle and 0 != $wgArticle->getID() )
		{
			if ( !$wgDisableCounters ) {
				$viewcount = $wgLang->formatNum( $wgArticle->getCount() );
				if ( $viewcount ) {
					$tpl->set('viewcount', wfMsgExt( 'viewcount', array( 'parseinline' ), $viewcount ) );
				} else {
					$tpl->set('viewcount', false);
				}
			} else {
				$tpl->set('viewcount', false);
			}

			if ($wgPageShowWatchingUsers) {
				$dbr = wfGetDB( DB_SLAVE );
				$watchlist = $dbr->tableName( 'watchlist' );
				$sql = "SELECT COUNT(*) AS n FROM $watchlist
					WHERE wl_title='" . $dbr->strencode($this->mTitle->getDBkey()) .
					"' AND  wl_namespace=" . $this->mTitle->getNamespace() ;
				$res = $dbr->query( $sql, 'SkinTemplate::outputPage');
				$x = $dbr->fetchObject( $res );
				$numberofwatchingusers = $x->n;
				if ($numberofwatchingusers > 0) {
					$tpl->set('numberofwatchingusers',
						wfMsgExt('number_of_watching_users_pageview', array('parseinline'),
						$wgLang->formatNum($numberofwatchingusers))
					);
				} else {
					$tpl->set('numberofwatchingusers', false);
				}
			} else {
				$tpl->set('numberofwatchingusers', false);
			}

			$tpl->set('copyright',$this->getCopyright());

			$this->credits = false;

			if (isset($wgMaxCredits) && $wgMaxCredits != 0) {
				require_once("Credits.php");
				$this->credits = getCredits($wgArticle, $wgMaxCredits, $wgShowCreditsIfMax);
			} else {
				$tpl->set('lastmod', $this->lastModified());
			}

			$tpl->setRef( 'credits', $this->credits );

		} elseif ( isset( $oldid ) && !isset( $diff ) ) {
			$tpl->set('copyright', $this->getCopyright());
			$tpl->set('viewcount', false);
			$tpl->set('lastmod', false);
			$tpl->set('credits', false);
			$tpl->set('numberofwatchingusers', false);
		} else {
			$tpl->set('copyright', false);
			$tpl->set('viewcount', false);
			$tpl->set('lastmod', false);
			$tpl->set('credits', false);
			$tpl->set('numberofwatchingusers', false);
		}
		wfProfileOut( __METHOD__."-stuff3" );

		wfProfileIn( __METHOD__."-stuff4" );
		$tpl->set( 'copyrightico', $this->getCopyrightIcon() );
		$tpl->set( 'poweredbyico', $this->getPoweredBy() );
		$tpl->set( 'disclaimer', $this->disclaimerLink() );
		$tpl->set( 'privacy', $this->privacyLink() );
		$tpl->set( 'about', $this->aboutLink() );

		$tpl->setRef( 'debug', $out->mDebugtext );
		$tpl->set( 'reporttime', wfReportTime() );
		$tpl->set( 'sitenotice', wfGetSiteNotice() );
		$tpl->set( 'bottomscripts', $this->bottomScripts() );

		$printfooter = "<div class=\"printfooter\">\n" . $this->printSource() . "</div>\n";
		$out->mBodytext .= $printfooter ;
		$tpl->setRef( 'bodytext', $out->mBodytext );

		# Language links
		$language_urls = array();

		if ( !$wgHideInterlanguageLinks ) {
			foreach( $wgOut->getLanguageLinks() as $l ) {
				$tmp = explode( ':', $l, 2 );
				$class = 'interwiki-' . $tmp[0];
				unset($tmp);
				$nt = Title::newFromText( $l );
				$language_urls[] = array(
					'href' => $nt->getFullURL(),
					'text' => ($wgContLang->getLanguageName( $nt->getInterwiki()) != ''?$wgContLang->getLanguageName( $nt->getInterwiki()) : $l),
					'class' => $class
				);
			}
		}
		if(count($language_urls)) {
			$tpl->setRef( 'language_urls', $language_urls);
		} else {
			$tpl->set('language_urls', false);
		}
		wfProfileOut( __METHOD__."-stuff4" );

		# Personal toolbar
		$tpl->set('personal_urls', $this->buildPersonalUrls());
		$content_actions = $this->buildContentActionUrls();
		$tpl->setRef('content_actions', $content_actions);

		// XXX: attach this from javascript, same with section editing
		if($this->iseditable &&	$wgUser->getOption("editondblclick") )
		{
			$encEditUrl = wfEscapeJsString( $this->mTitle->getLocalUrl( $this->editUrlOptions() ) );
			$tpl->set('body_ondblclick', 'document.location = "' . $encEditUrl . '";');
		} else {
			$tpl->set('body_ondblclick', false);
		}
		$tpl->set( 'body_onload', false );
		$tpl->set( 'sidebar', $this->buildSidebar() );
		$tpl->set( 'nav_urls', $this->buildNavUrls() );

		// original version by hansm
		if( !wfRunHooks( 'SkinTemplateOutputPageBeforeExec', array( &$this, &$tpl ) ) ) {
			wfDebug( __METHOD__ . ': Hook SkinTemplateOutputPageBeforeExec broke outputPage execution!' );
		}

		// execute template
		wfProfileIn( __METHOD__."-execute" );
		$res = $tpl->execute();
		wfProfileOut( __METHOD__."-execute" );

		// result may be an error
		$this->printOrError( $res );
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Output the string, or print error message if it's
	 * an error object of the appropriate type.
	 * For the base class, assume strings all around.
	 *
	 * @param mixed $str
	 * @private
	 */
	function printOrError( $str ) {
		echo $str;
	}

	/**
	 * build array of urls for personal toolbar
	 * @return array
	 * @private
	 */
	function buildPersonalUrls() {
		global $wgTitle, $wgRequest;

		$pageurl = $wgTitle->getLocalURL();
		wfProfileIn( __METHOD__ );

		/* set up the default links for the personal toolbar */
		$personal_urls = array();
		if ($this->loggedin) {
			$personal_urls['userpage'] = array(
				'text' => $this->username,
				'href' => &$this->userpageUrlDetails['href'],
				'class' => $this->userpageUrlDetails['exists']?false:'new',
				'active' => ( $this->userpageUrlDetails['href'] == $pageurl )
			);
			$usertalkUrlDetails = $this->makeTalkUrlDetails($this->userpage);
			$personal_urls['mytalk'] = array(
				'text' => wfMsg('mytalk'),
				'href' => &$usertalkUrlDetails['href'],
				'class' => $usertalkUrlDetails['exists']?false:'new',
				'active' => ( $usertalkUrlDetails['href'] == $pageurl )
			);
			$href = self::makeSpecialUrl( 'Preferences' );
			$personal_urls['preferences'] = array(
				'text' => wfMsg( 'mypreferences' ),
				'href' => $href,
				'active' => ( $href == $pageurl )
			);
			$href = self::makeSpecialUrl( 'Watchlist' );
			$personal_urls['watchlist'] = array(
				'text' => wfMsg( 'mywatchlist' ),
				'href' => $href,
				'active' => ( $href == $pageurl )
			);

			# We need to do an explicit check for Special:Contributions, as we
			# have to match both the title, and the target (which could come
			# from request values or be specified in "sub page" form. The plot
			# thickens, because $wgTitle is altered for special pages, so doesn't
			# contain the original alias-with-subpage.
			$title = Title::newFromText( $wgRequest->getText( 'title' ) );
			if( $title instanceof Title && $title->getNamespace() == NS_SPECIAL ) {
				list( $spName, $spPar ) =
					SpecialPage::resolveAliasWithSubpage( $title->getText() );
				$active = $spName == 'Contributions'
					&& ( ( $spPar && $spPar == $this->username )
						|| $wgRequest->getText( 'target' ) == $this->username );
			} else {
				$active = false;
			}

			$href = self::makeSpecialUrlSubpage( 'Contributions', $this->username );
			$personal_urls['mycontris'] = array(
				'text' => wfMsg( 'mycontris' ),
				'href' => $href,
				'active' => $active
			);
			$personal_urls['logout'] = array(
				'text' => wfMsg( 'userlogout' ),
				'href' => self::makeSpecialUrl( 'Userlogout',
					$wgTitle->isSpecial( 'Preferences' ) ? '' : "returnto={$this->thisurl}"
				),
				'active' => false
			);
		} else {
			global $wgUser;
			$loginlink = $wgUser->isAllowed( 'createaccount' )
				? 'nav-login-createaccount'
				: 'login';
			if( $this->showIPinHeader() ) {
				$href = &$this->userpageUrlDetails['href'];
				$personal_urls['anonuserpage'] = array(
					'text' => $this->username,
					'href' => $href,
					'class' => $this->userpageUrlDetails['exists']?false:'new',
					'active' => ( $pageurl == $href )
				);
				$usertalkUrlDetails = $this->makeTalkUrlDetails($this->userpage);
				$href = &$usertalkUrlDetails['href'];
				$personal_urls['anontalk'] = array(
					'text' => wfMsg('anontalk'),
					'href' => $href,
					'class' => $usertalkUrlDetails['exists']?false:'new',
					'active' => ( $pageurl == $href )
				);
				$personal_urls['anonlogin'] = array(
					'text' => wfMsg( $loginlink ),
					'href' => self::makeSpecialUrl( 'Userlogin', 'returnto=' . $this->thisurl ),
					'active' => $wgTitle->isSpecial( 'Userlogin' )
				);
			} else {

				$personal_urls['login'] = array(
					'text' => wfMsg( $loginlink ),
					'href' => self::makeSpecialUrl( 'Userlogin', 'returnto=' . $this->thisurl ),
					'active' => $wgTitle->isSpecial( 'Userlogin' )
				);
			}
		}

		wfRunHooks( 'PersonalUrls', array( &$personal_urls, &$wgTitle ) );
		wfProfileOut( __METHOD__ );
		return $personal_urls;
	}

	function tabAction( $title, $message, $selected, $query='', $checkEdit=false ) {
		$classes = array();
		if( $selected ) {
			$classes[] = 'selected';
		}
		if( $checkEdit && !$title->isAlwaysKnown() && $title->getArticleId() == 0 ) {
			$classes[] = 'new';
			$query = 'action=edit';
		}

		$text = wfMsg( $message );
		if ( wfEmptyMsg( $message, $text ) ) {
			global $wgContLang;
			$text = $wgContLang->getFormattedNsText( MWNamespace::getSubject( $title->getNamespace() ) );
		}

		$result = array();
		if( !wfRunHooks('SkinTemplateTabAction', array(&$this,
				$title, $message, $selected, $checkEdit,
				&$classes, &$query, &$text, &$result)) ) {
			return $result;
		}

		return array(
			'class' => implode( ' ', $classes ),
			'text' => $text,
			'href' => $title->getLocalUrl( $query ) );
	}

	function makeTalkUrlDetails( $name, $urlaction = '' ) {
		$title = Title::newFromText( $name );
		if( !is_object($title) ) {
			throw new MWException( __METHOD__." given invalid pagename $name" );
		}
		$title = $title->getTalkPage();
		self::checkTitle( $title, $name );
		return array(
			'href' => $title->getLocalURL( $urlaction ),
			'exists' => $title->getArticleID() != 0 ? true : false
		);
	}

	function makeArticleUrlDetails( $name, $urlaction = '' ) {
		$title = Title::newFromText( $name );
		$title= $title->getSubjectPage();
		self::checkTitle( $title, $name );
		return array(
			'href' => $title->getLocalURL( $urlaction ),
			'exists' => $title->getArticleID() != 0 ? true : false
		);
	}

	/**
	 * an array of edit links by default used for the tabs
	 * @return array
	 * @private
	 */
	function buildContentActionUrls () {
		global $wgContLang, $wgLang, $wgOut;
		wfProfileIn( __METHOD__ );

		global $wgUser, $wgRequest;
		$action = $wgRequest->getText( 'action' );
		$section = $wgRequest->getText( 'section' );
		$content_actions = array();

		$prevent_active_tabs = false ;
		wfRunHooks( 'SkinTemplatePreventOtherActiveTabs', array( &$this , &$prevent_active_tabs ) )	;

		if( $this->iscontent ) {
			$subjpage = $this->mTitle->getSubjectPage();
			$talkpage = $this->mTitle->getTalkPage();

			$nskey = $this->mTitle->getNamespaceKey();
			$content_actions[$nskey] = $this->tabAction(
				$subjpage,
				$nskey,
				!$this->mTitle->isTalkPage() && !$prevent_active_tabs,
				'', true);

			$content_actions['talk'] = $this->tabAction(
				$talkpage,
				'talk',
				$this->mTitle->isTalkPage() && !$prevent_active_tabs,
				'',
				true);

			wfProfileIn( __METHOD__."-edit" );
			if ( $this->mTitle->quickUserCan( 'edit' ) && ( $this->mTitle->exists() || $this->mTitle->quickUserCan( 'create' ) ) ) {
				$istalk = $this->mTitle->isTalkPage();
				$istalkclass = $istalk?' istalk':'';
				$content_actions['edit'] = array(
					'class' => ((($action == 'edit' or $action == 'submit') and $section != 'new') ? 'selected' : '').$istalkclass,
					'text' => $this->mTitle->exists()
						? wfMsg( 'edit' )
						: wfMsg( 'create' ),
					'href' => $this->mTitle->getLocalUrl( $this->editUrlOptions() )
				);

				if ( $istalk || $wgOut->showNewSectionLink() ) {
					$content_actions['addsection'] = array(
						'class' => $section == 'new'?'selected':false,
						'text' => wfMsg('addsection'),
						'href' => $this->mTitle->getLocalUrl( 'action=edit&section=new' )
					);
				}
			} elseif ( $this->mTitle->exists() || $this->mTitle->isAlwaysKnown() ) {
				$content_actions['viewsource'] = array(
					'class' => ($action == 'edit') ? 'selected' : false,
					'text' => wfMsg('viewsource'),
					'href' => $this->mTitle->getLocalUrl( $this->editUrlOptions() )
				);
			}
			wfProfileOut( __METHOD__."-edit" );

			wfProfileIn( __METHOD__."-live" );
			if ( $this->mTitle->getArticleId() ) {

				$content_actions['history'] = array(
					'class' => ($action == 'history') ? 'selected' : false,
					'text' => wfMsg('history_short'),
					'href' => $this->mTitle->getLocalUrl( 'action=history')
				);

				if($wgUser->isAllowed('delete')){
					$content_actions['delete'] = array(
						'class' => ($action == 'delete') ? 'selected' : false,
						'text' => wfMsg('delete'),
						'href' => $this->mTitle->getLocalUrl( 'action=delete' )
					);
				}
				if ( $this->mTitle->quickUserCan( 'move' ) ) {
					$moveTitle = SpecialPage::getTitleFor( 'Movepage', $this->thispage );
					$content_actions['move'] = array(
						'class' => $this->mTitle->isSpecial( 'Movepage' ) ? 'selected' : false,
						'text' => wfMsg('move'),
						'href' => $moveTitle->getLocalUrl()
					);
				}

				if ( $this->mTitle->getNamespace() !== NS_MEDIAWIKI && $wgUser->isAllowed( 'protect' ) ) {
					if(!$this->mTitle->isProtected()){
						$content_actions['protect'] = array(
							'class' => ($action == 'protect') ? 'selected' : false,
							'text' => wfMsg('protect'),
							'href' => $this->mTitle->getLocalUrl( 'action=protect' )
						);

					} else {
						$content_actions['unprotect'] = array(
							'class' => ($action == 'unprotect') ? 'selected' : false,
							'text' => wfMsg('unprotect'),
							'href' => $this->mTitle->getLocalUrl( 'action=unprotect' )
						);
					}
				}
			} else {
				//article doesn't exist or is deleted
				if( $wgUser->isAllowed( 'deletedhistory' ) && $wgUser->isAllowed( 'undelete' ) ) {
					if( $n = $this->mTitle->isDeleted() ) {
						$undelTitle = SpecialPage::getTitleFor( 'Undelete' );
						$content_actions['undelete'] = array(
							'class' => false,
							'text' => wfMsgExt( 'undelete_short', array( 'parsemag' ), $wgLang->formatNum($n) ),
							'href' => $undelTitle->getLocalUrl( 'target=' . urlencode( $this->thispage ) )
							#'href' => self::makeSpecialUrl( "Undelete/$this->thispage" )
						);
					}
				}

				if ( $this->mTitle->getNamespace() !== NS_MEDIAWIKI && $wgUser->isAllowed( 'protect' ) ) {
					if( !$this->mTitle->getRestrictions( 'create' ) ) {
						$content_actions['protect'] = array(
							'class' => ($action == 'protect') ? 'selected' : false,
							'text' => wfMsg('protect'),
							'href' => $this->mTitle->getLocalUrl( 'action=protect' )
						);

					} else {
						$content_actions['unprotect'] = array(
							'class' => ($action == 'unprotect') ? 'selected' : false,
							'text' => wfMsg('unprotect'),
							'href' => $this->mTitle->getLocalUrl( 'action=unprotect' )
						);
					}
				}
			}

			wfProfileOut( __METHOD__."-live" );

			if( $this->loggedin ) {
				if( !$this->mTitle->userIsWatching()) {
					$content_actions['watch'] = array(
						'class' => ($action == 'watch' or $action == 'unwatch') ? 'selected' : false,
						'text' => wfMsg('watch'),
						'href' => $this->mTitle->getLocalUrl( 'action=watch' )
					);
				} else {
					$content_actions['unwatch'] = array(
						'class' => ($action == 'unwatch' or $action == 'watch') ? 'selected' : false,
						'text' => wfMsg('unwatch'),
						'href' => $this->mTitle->getLocalUrl( 'action=unwatch' )
					);
				}
			}


			wfRunHooks( 'SkinTemplateTabs', array( &$this , &$content_actions ) )	;
		} else {
			/* show special page tab */

			$content_actions[$this->mTitle->getNamespaceKey()] = array(
				'class' => 'selected',
				'text' => wfMsg('nstab-special'),
				'href' => $wgRequest->getRequestURL(), // @bug 2457, 2510
			);

			wfRunHooks( 'SkinTemplateBuildContentActionUrlsAfterSpecialPage', array( &$this, &$content_actions ) );
		}

		/* show links to different language variants */
		global $wgDisableLangConversion;
		$variants = $wgContLang->getVariants();
		if( !$wgDisableLangConversion && sizeof( $variants ) > 1 ) {
			$preferred = $wgContLang->getPreferredVariant();
			$vcount=0;
			foreach( $variants as $code ) {
				$varname = $wgContLang->getVariantname( $code );
				if( $varname == 'disable' )
					continue;
				$selected = ( $code == $preferred )? 'selected' : false;
				$content_actions['varlang-' . $vcount] = array(
						'class' => $selected,
						'text' => $varname,
						'href' => $this->mTitle->getLocalURL('',$code)
					);
				$vcount ++;
			}
		}

		wfRunHooks( 'SkinTemplateContentActions', array( &$content_actions ) );

		wfProfileOut( __METHOD__ );
		return $content_actions;
	}



	/**
	 * build array of common navigation links
	 * @return array
	 * @private
	 */
	function buildNavUrls () {
		global $wgUseTrackbacks, $wgTitle, $wgUser, $wgRequest;
		global $wgEnableUploads, $wgUploadNavigationUrl;

		wfProfileIn( __METHOD__ );

		$action = $wgRequest->getText( 'action' );

		$nav_urls = array();
		$nav_urls['mainpage'] = array( 'href' => self::makeMainPageUrl() );
		if( $wgEnableUploads ) {
			if ($wgUploadNavigationUrl) {
				$nav_urls['upload'] = array( 'href' => $wgUploadNavigationUrl );
			} else {
				$nav_urls['upload'] = array( 'href' => self::makeSpecialUrl( 'Upload' ) );
			}
		} else {
			if ($wgUploadNavigationUrl)
				$nav_urls['upload'] = array( 'href' => $wgUploadNavigationUrl );
			else
				$nav_urls['upload'] = false;
		}
		$nav_urls['specialpages'] = array( 'href' => self::makeSpecialUrl( 'Specialpages' ) );

		// default permalink to being off, will override it as required below.
		$nav_urls['permalink'] = false;

		// A print stylesheet is attached to all pages, but nobody ever
		// figures that out. :)  Add a link...
		if( $this->iscontent && ($action == '' || $action == 'view' || $action == 'purge' ) ) {
			$nav_urls['print'] = array(
				'text' => wfMsg( 'printableversion' ),
				'href' => $wgRequest->appendQuery( 'printable=yes' )
			);

			// Also add a "permalink" while we're at it
			if ( $this->mRevisionId ) {
				$nav_urls['permalink'] = array(
					'text' => wfMsg( 'permalink' ),
					'href' => $wgTitle->getLocalURL( "oldid=$this->mRevisionId" )
				);
			}

			// Copy in case this undocumented, shady hook tries to mess with internals
			$revid = $this->mRevisionId;
			wfRunHooks( 'SkinTemplateBuildNavUrlsNav_urlsAfterPermalink', array( &$this, &$nav_urls, &$revid, &$revid ) );
		}

		if( $this->mTitle->getNamespace() != NS_SPECIAL ) {
			$wlhTitle = SpecialPage::getTitleFor( 'Whatlinkshere', $this->thispage );
			$nav_urls['whatlinkshere'] = array(
				'href' => $wlhTitle->getLocalUrl()
			);
			if( $this->mTitle->getArticleId() ) {
				$rclTitle = SpecialPage::getTitleFor( 'Recentchangeslinked', $this->thispage );
				$nav_urls['recentchangeslinked'] = array(
					'href' => $rclTitle->getLocalUrl()
				);
			} else {
				$nav_urls['recentchangeslinked'] = false;
			}
			if ($wgUseTrackbacks)
				$nav_urls['trackbacklink'] = array(
					'href' => $wgTitle->trackbackURL()
				);
		}

		if( $this->mTitle->getNamespace() == NS_USER || $this->mTitle->getNamespace() == NS_USER_TALK ) {
			$id = User::idFromName($this->mTitle->getText());
			$ip = User::isIP($this->mTitle->getText());
		} else {
			$id = 0;
			$ip = false;
		}

		if($id || $ip) { # both anons and non-anons have contribs list
			$nav_urls['contributions'] = array(
				'href' => self::makeSpecialUrlSubpage( 'Contributions', $this->mTitle->getText() )
			);

			if( $id ) {
				$logPage = SpecialPage::getTitleFor( 'Log' );
				$nav_urls['log'] = array( 'href' => $logPage->getLocalUrl( 'user='
					. $this->mTitle->getPartialUrl() ) );
			} else {
				$nav_urls['log'] = false;
			}

			if ( $wgUser->isAllowed( 'block' ) ) {
				$nav_urls['blockip'] = array(
					'href' => self::makeSpecialUrlSubpage( 'Blockip', $this->mTitle->getText() )
				);
			} else {
				$nav_urls['blockip'] = false;
			}
		} else {
			$nav_urls['contributions'] = false;
			$nav_urls['log'] = false;
			$nav_urls['blockip'] = false;
		}
		$nav_urls['emailuser'] = false;
		if( $this->showEmailUser( $id ) ) {
			$nav_urls['emailuser'] = array(
				'href' => self::makeSpecialUrlSubpage( 'Emailuser', $this->mTitle->getText() )
			);
		}
		wfProfileOut( __METHOD__ );
		return $nav_urls;
	}

	/**
	 * Generate strings used for xml 'id' names
	 * @return string
	 * @private
	 */
	function getNameSpaceKey () {
		return $this->mTitle->getNamespaceKey();
	}

	/**
	 * @private
	 */
	function setupUserCss() {
		wfProfileIn( __METHOD__ );

		global $wgRequest, $wgAllowUserCss, $wgUseSiteCss, $wgContLang, $wgSquidMaxage, $wgStylePath, $wgUser;

		$sitecss = '';
		$usercss = '';
		$siteargs = '&maxage=' . $wgSquidMaxage;
		if( $this->loggedin ) {
			// Ensure that logged-in users' generated CSS isn't clobbered
			// by anons' publicly cacheable generated CSS.
			$siteargs .= '&smaxage=0';
		}

		# Add user-specific code if this is a user and we allow that kind of thing

		if ( $wgAllowUserCss && $this->loggedin ) {
			$action = $wgRequest->getText('action');

			# if we're previewing the CSS page, use it
			if( $this->mTitle->isCssSubpage() and $this->userCanPreview( $action ) ) {
				$siteargs = "&smaxage=0&maxage=0";
				$usercss = $wgRequest->getText('wpTextbox1');
			} else {
				$usercss = '@import "' .
				  self::makeUrl($this->userpage . '/'.$this->skinname.'.css',
								 'action=raw&ctype=text/css') . '";' ."\n";
			}

			$siteargs .= '&ts=' . $wgUser->mTouched;
		}

		if( $wgContLang->isRTL() && in_array( 'rtl', $this->cssfiles ) ) {
			global $wgStyleVersion;
			$sitecss .= "@import \"$wgStylePath/$this->stylename/rtl.css?$wgStyleVersion\";\n";
		}

		# If we use the site's dynamic CSS, throw that in, too
		if ( $wgUseSiteCss ) {
			$query = "usemsgcache=yes&action=raw&ctype=text/css&smaxage=$wgSquidMaxage";
			$skinquery = "&useskin=" . urlencode( $this->getSkinName() );
			$sitecss .= '@import "' . self::makeNSUrl( 'Common.css', $query, NS_MEDIAWIKI) . '";' . "\n";
			$sitecss .= '@import "' . self::makeNSUrl( ucfirst( $this->skinname ) . '.css', $query, NS_MEDIAWIKI ) . '";' . "\n";
			$sitecss .= '@import "' . self::makeUrl( '-', "action=raw&gen=css$siteargs$skinquery" ) . '";' . "\n";
		}

		# If we use any dynamic CSS, make a little CDATA block out of it.

		if ( !empty($sitecss) || !empty($usercss) ) {
			$this->usercss = "/*<![CDATA[*/\n" . $sitecss . $usercss . '/*]]>*/';
		}
		wfProfileOut( __METHOD__ );
	}

	/**
	 * @private
	 */
	function setupUserJs( $allowUserJs ) {
		wfProfileIn( __METHOD__ );

		global $wgRequest, $wgJsMimeType;
		$action = $wgRequest->getText('action');

		if( $allowUserJs && $this->loggedin ) {
			if( $this->mTitle->isJsSubpage() and $this->userCanPreview( $action ) ) {
				# XXX: additional security check/prompt?
				$this->userjsprev = '/*<![CDATA[*/ ' . $wgRequest->getText('wpTextbox1') . ' /*]]>*/';
			} else {
				$this->userjs = self::makeUrl($this->userpage.'/'.$this->skinname.'.js', 'action=raw&ctype='.$wgJsMimeType);
			}
		}
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Code for extensions to hook into to provide per-page CSS, see
	 * extensions/PageCSS/PageCSS.php for an implementation of this.
	 *
	 * @private
	 */
	function setupPageCss() {
		wfProfileIn( __METHOD__ );
		$out = false;
		wfRunHooks( 'SkinTemplateSetupPageCss', array( &$out ) );

		wfProfileOut( __METHOD__ );
		return $out;
	}

	/**
	 * returns css with user-specific options
	 */
	public function getUserStylesheet() {
		wfProfileIn( __METHOD__ );

		$s = "/* generated user stylesheet */\n";
		$s .= $this->reallyDoGetUserStyles();
		wfProfileOut( __METHOD__ );
		return $s;
	}

	/**
	 * Returns the print stylesheet for this skin.  In all default skins this
	 * is just commonPrint.css, but third-party skins may want to modify it.
	 *
	 * @return string
	 */
	protected function getPrintCss() {
		global $wgStylePath;
		return $wgStylePath . "/common/commonPrint.css";
	}

	/**
	 * This returns MediaWiki:Common.js and MediaWiki:[Skinname].js concate-
	 * nated together.  For some bizarre reason, it does *not* return any
	 * custom user JS from subpages.  Huh?
	 *
	 * There's absolutely no reason to have separate Monobook/Common JSes.
	 * Any JS that cares can just check the skin variable generated at the
	 * top.  For now Monobook.js will be maintained, but it should be consi-
	 * dered deprecated.
	 *
	 * @return string
	 */
	public function getUserJs() {
		wfProfileIn( __METHOD__ );

		$s = parent::getUserJs();
		$s .= "\n\n/* MediaWiki:".ucfirst($this->skinname).".js */\n";

		// avoid inclusion of non defined user JavaScript (with custom skins only)
		// by checking for default message content
		$msgKey = ucfirst($this->skinname).'.js';
		$userJS = wfMsgForContent($msgKey);
		if ( !wfEmptyMsg( $msgKey, $userJS ) ) {
			$s .= $userJS;
		}

		wfProfileOut( __METHOD__ );
		return $s;
	}
}

/**
 * Generic wrapper for template functions, with interface
 * compatible with what we use of PHPTAL 0.7.
 * @ingroup Skins
 */
class QuickTemplate {
	/**
	 * @public
	 */
	function QuickTemplate() {
		$this->data = array();
		$this->translator = new MediaWiki_I18N();
	}

	/**
	 * @public
	 */
	function set( $name, $value ) {
		$this->data[$name] = $value;
	}

	/**
	 * @public
	 */
	function setRef($name, &$value) {
		$this->data[$name] =& $value;
	}

	/**
	 * @public
	 */
	function setTranslator( &$t ) {
		$this->translator = &$t;
	}

	/**
	 * @public
	 */
	function execute() {
		echo "Override this function.";
	}


	/**
	 * @private
	 */
	function text( $str ) {
		echo htmlspecialchars( $this->data[$str] );
	}

	/**
	 * @private
	 */
	function jstext( $str ) {
		echo Xml::escapeJsString( $this->data[$str] );
	}

	/**
	 * @private
	 */
	function html( $str ) {
		echo $this->data[$str];
	}

	/**
	 * @private
	 */
	function msg( $str ) {
		echo htmlspecialchars( $this->translator->translate( $str ) );
	}

	/**
	 * @private
	 */
	function msgHtml( $str ) {
		echo $this->translator->translate( $str );
	}

	/**
	 * An ugly, ugly hack.
	 * @private
	 */
	function msgWiki( $str ) {
		global $wgParser, $wgTitle, $wgOut;

		$text = $this->translator->translate( $str );
		$parserOutput = $wgParser->parse( $text, $wgTitle,
			$wgOut->parserOptions(), true );
		echo $parserOutput->getText();
	}

	/**
	 * @private
	 */
	function haveData( $str ) {
		return isset( $this->data[$str] );
	}

	/**
	 * @private
	 */
	function haveMsg( $str ) {
		$msg = $this->translator->translate( $str );
		return ($msg != '-') && ($msg != ''); # ????
	}
}
