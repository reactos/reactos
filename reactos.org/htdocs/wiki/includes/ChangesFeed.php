<?php

class ChangesFeed {

	public $format, $type, $titleMsg, $descMsg;

	public function __construct( $format, $type ) {
		$this->format = $format;
		$this->type = $type;
	}

	public function getFeedObject( $title, $description ) {
		global $wgSitename, $wgContLanguageCode, $wgFeedClasses, $wgTitle;
		$feedTitle = "$wgSitename  - {$title} [$wgContLanguageCode]";

		return new $wgFeedClasses[$this->format](
			$feedTitle, htmlspecialchars( $description ), $wgTitle->getFullUrl() );
	}

	public function execute( $feed, $rows, $limit = 0 , $hideminor = false, $lastmod = false ) {
		global $messageMemc, $wgFeedCacheTimeout;
		global $wgFeedClasses, $wgTitle, $wgSitename, $wgContLanguageCode;

		if ( !FeedUtils::checkFeedOutput( $this->format ) ) {
			return;
		}

		$timekey = wfMemcKey( $this->type, $this->format, 'timestamp' );
		$key = wfMemcKey( $this->type, $this->format, 'limit', $limit, 'minor', $hideminor );

		FeedUtils::checkPurge($timekey, $key);

		/*
		* Bumping around loading up diffs can be pretty slow, so where
		* possible we want to cache the feed output so the next visitor
		* gets it quick too.
		*/
		$cachedFeed = $this->loadFromCache( $lastmod, $timekey, $key );
		if( is_string( $cachedFeed ) ) {
			wfDebug( "RC: Outputting cached feed\n" );
			$feed->httpHeaders();
			echo $cachedFeed;
		} else {
			wfDebug( "RC: rendering new feed and caching it\n" );
			ob_start();
			self::generateFeed( $rows, $feed );
			$cachedFeed = ob_get_contents();
			ob_end_flush();
			$this->saveToCache( $cachedFeed, $timekey, $key );
		}
		return true;
	}

	public function saveToCache( $feed, $timekey, $key ) {
		global $messageMemc;
		$expire = 3600 * 24; # One day
		$messageMemc->set( $key, $feed );
		$messageMemc->set( $timekey, wfTimestamp( TS_MW ), $expire );
	}

	public function loadFromCache( $lastmod, $timekey, $key ) {
		global $wgFeedCacheTimeout, $messageMemc;
		$feedLastmod = $messageMemc->get( $timekey );

		if( ( $wgFeedCacheTimeout > 0 ) && $feedLastmod ) {
			/*
			* If the cached feed was rendered very recently, we may
			* go ahead and use it even if there have been edits made
			* since it was rendered. This keeps a swarm of requests
			* from being too bad on a super-frequently edited wiki.
			*/

			$feedAge = time() - wfTimestamp( TS_UNIX, $feedLastmod );
			$feedLastmodUnix = wfTimestamp( TS_UNIX, $feedLastmod );
			$lastmodUnix = wfTimestamp( TS_UNIX, $lastmod );

			if( $feedAge < $wgFeedCacheTimeout || $feedLastmodUnix > $lastmodUnix) {
				wfDebug( "RC: loading feed from cache ($key; $feedLastmod; $lastmod)...\n" );
				return $messageMemc->get( $key );
			} else {
				wfDebug( "RC: cached feed timestamp check failed ($feedLastmod; $lastmod)\n" );
			}
		}
		return false;
	}

	/**
	* @todo document
	* @param $rows Database resource with recentchanges rows
	* @param $feed Feed object
	*/
	public static function generateFeed( $rows, &$feed ) {
		wfProfileIn( __METHOD__ );

		$feed->outHeader();

		# Merge adjacent edits by one user
		$sorted = array();
		$n = 0;
		foreach( $rows as $obj ) {
			if( $n > 0 &&
				$obj->rc_namespace >= 0 &&
				$obj->rc_cur_id == $sorted[$n-1]->rc_cur_id &&
				$obj->rc_user_text == $sorted[$n-1]->rc_user_text ) {
				$sorted[$n-1]->rc_last_oldid = $obj->rc_last_oldid;
			} else {
				$sorted[$n] = $obj;
				$n++;
			}
		}

		foreach( $sorted as $obj ) {
			$title = Title::makeTitle( $obj->rc_namespace, $obj->rc_title );
			$talkpage = $title->getTalkPage();
			$item = new FeedItem(
				$title->getPrefixedText(),
				FeedUtils::formatDiff( $obj ),
				$title->getFullURL( 'diff=' . $obj->rc_this_oldid . '&oldid=prev' ),
				$obj->rc_timestamp,
				($obj->rc_deleted & Revision::DELETED_USER) ? wfMsgHtml('rev-deleted-user') : $obj->rc_user_text,
				$talkpage->getFullURL()
				);
			$feed->outItem( $item );
		}
		$feed->outFooter();
		wfProfileOut( __METHOD__ );
	}

}