<?php
/** Sanskrit (संस्कृत)
 *
 * @ingroup Language
 * @file
 *
 * @author Kaustubh
 */

$fallback = 'hi';

$digitTransformTable = array(
	'0' => '०', # &#x0966;
	'1' => '१', # &#x0967;
	'2' => '२', # &#x0968;
	'3' => '३', # &#x0969;
	'4' => '४', # &#x096a;
	'5' => '५', # &#x096b;
	'6' => '६', # &#x096c;
	'7' => '७', # &#x096d;
	'8' => '८', # &#x096e;
	'9' => '९', # &#x096f;
);

$linkPrefixExtension = false;

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Special',
	NS_MAIN	            => '',
	NS_TALK	            => 'संभाषणं',
	NS_USER             => 'योजकः',
	NS_USER_TALK        => 'योजकसंभाषणं',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1संभाषणं',
	NS_IMAGE            => 'चित्रं',
	NS_IMAGE_TALK       => 'चित्रसंभाषणं',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_talk',
	NS_TEMPLATE         => 'Template',
	NS_TEMPLATE_TALK    => 'Template_talk',
	NS_HELP             => 'उपकारः',
	NS_HELP_TALK        => 'उपकारसंभाषणं',
	NS_CATEGORY         => 'वर्गः',
	NS_CATEGORY_TALK    => 'वर्गसंभाषणं',
);

$skinNames = array(
	'standard' => 'पूर्व',
	'nostalgia' => 'पुराण',
	'cologneblue' => 'नील',
	'monobook' => 'पुस्तक',
	'myskin' => 'मे चर्मन्',
	'chick' => 'Chick'
);

$messages = array(
# Dates
'sunday'    => 'विश्रामवासरे',
'monday'    => 'सोमवासरे',
'tuesday'   => 'मंगलवासरे',
'wednesday' => 'बुधवासरे',
'thursday'  => 'गुरुवासरे',
'friday'    => 'शुक्रवासरे',
'saturday'  => 'शनिवासरे',
'sun'       => 'विश्राम',
'mon'       => 'सोम',
'tue'       => 'मंगल',
'wed'       => 'बुध',
'thu'       => 'गुरु',
'fri'       => 'शुक्र',
'sat'       => 'शनि',
'january'   => 'पौषमाघे',
'february'  => 'फाल्गुने',
'march'     => 'फाल्गुनचैत्रे',
'april'     => 'मधुमासे',
'may_long'  => 'वैशाखज्येष्ठे',
'june'      => 'ज्येष्ठाषाढके',
'july'      => 'आषाढश्रावणे',
'august'    => 'नभस्ये',
'september' => 'भाद्रपदाश्विने',
'october'   => 'अश्विनकार्तिके',
'november'  => 'कार्तिकमार्गशीर्षे',
'december'  => 'मार्गशीर्षपौषे',

'and' => 'एवम्',

'help' => 'सहायता',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'edithelp'             => 'संपादनार्थं सहायता',
'mainpage'             => 'मुखपृष्ठं',
'mainpage-description' => 'मुखपृष्ठं',

# Miscellaneous special pages
'ancientpages' => 'प्राचीनतम् पृष्ठा',

# Book sources
'booksources-go' => 'प्रस्थानम्',

# Special:AllPages
'allarticles' => 'सर्व लेखा',

# E-mail user
'emailsubject' => 'विषयः',
'emailmessage' => 'सन्देशः',

# Delete/protect/revert
'actioncomplete' => 'कार्य समापनम्',

# Block/unblock
'blocklink' => 'निषेध',

# Namespace 8 related
'allmessages'     => 'व्यवस्था सन्देशानि',
'allmessagesname' => 'नाम',

# Auto-summaries
'autosumm-new' => 'नवीन पृष्ठं: $1',

);
