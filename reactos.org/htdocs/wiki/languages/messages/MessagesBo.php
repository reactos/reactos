<?php
/** Tibetan (བོད་ཡིག)
 *
 * @ingroup Language
 * @file
 *
 */

$digitTransformTable = array(
	'0' => '༠', # &#x0f20;
	'1' => '༡', # &#x0f21;
	'2' => '༢', # &#x0f22;
	'3' => '༣', # &#x0f23;
	'4' => '༤', # &#x0f24;
	'5' => '༥', # &#x0f25;
	'6' => '༦', # &#x0f26;
	'7' => '༧', # &#x0f27;
	'8' => '༨', # &#x0f28;
	'9' => '༩', # &#x0f29;
);

$messages = array(
# Dates
'sunday'        => 'གཟའ་ཉི་མ།',
'monday'        => 'གཟའ་ཟླ་བ།',
'tuesday'       => 'གཟའ་མིག་དམར།',
'wednesday'     => 'གཟའ་ལྷག་པ།',
'thursday'      => 'གཟའ་ཕུར་བུ།',
'friday'        => 'གཟའ་པ་སངས།',
'saturday'      => 'གཟའ་སྤེན་པ།',
'sun'           => 'གཟའ་ཉི་མ།',
'mon'           => 'གཟའ་ཟླ་བ།',
'tue'           => 'གཟའ་མིག་དམར།',
'wed'           => 'གཟའ་ལྷག་པ།',
'thu'           => 'གཟའ་ཕུར་བུ།',
'fri'           => 'གཟའ་པ་སངས།',
'sat'           => 'གཟའ་སྤེན་པ།',
'january'       => 'ཟླ་དང་པོ།',
'february'      => 'ཟླ་གཉིས་པ།',
'march'         => 'ཟླ་གསུམ།',
'april'         => 'ཟླ་བཞི་བ།',
'may_long'      => 'ཟླ་ལྔ་བ།',
'june'          => 'ཟླ་དྲུག་པ།',
'july'          => 'ཟླ་བདུན་པ།',
'august'        => 'ཟླ་བརྒྱད་པ།',
'september'     => 'ཟླ་དགུ་བ།',
'october'       => 'ཟླ་བཅུ་བ།',
'november'      => 'ཟླ་བཅུ་གཅིག་པ།',
'december'      => 'ཟླ་བཅུ་གཉིས་པ།',
'january-gen'   => 'ཟླ་དང་པོ།',
'february-gen'  => 'ཟླ་གཉིས་པ།',
'march-gen'     => 'ཟླ་གསུམ།',
'april-gen'     => 'ཟླ་བཞི་བ།',
'may-gen'       => 'ཟླ་ལྔ་བ།',
'june-gen'      => 'ཟླ་དྲུག་པ།',
'july-gen'      => 'ཟླ་བདུན་པ།',
'august-gen'    => 'ཟླ་བརྒྱད་པ།',
'september-gen' => 'ཟླ་དགུ་བ།',
'october-gen'   => 'ཟླ་བཅུ་བ།',
'november-gen'  => 'ཟླ་བཅུ་གཅིག་པ།',
'december-gen'  => 'ཟླ་བཅུ་གཉིས་པ།',
'jan'           => 'ཟླ་དང་པོ།',
'feb'           => 'ཟླ་གཉིས་པ།',
'mar'           => 'ཟླ་གསུམ།',
'apr'           => 'ཟླ་བཞི་བ།',
'may'           => 'ཟླ་ལྔ་བ།',
'jun'           => 'ཟླ་དྲུག་པ།',
'jul'           => 'ཟླ་བདུན་པ།',
'aug'           => 'ཟླ་བརྒྱད་པ།',
'sep'           => 'ཟླ་དགུ་བ།',
'oct'           => 'ཟླ་བཅུ་བ།',
'nov'           => 'ཟླ་བཅུ་གཅིག་པ།',
'dec'           => 'ཟླ་བཅུ་གཉིས་པ།',

# Categories related messages
'subcategories' => 'རིགས་ཕལ་བ།',

'about'          => 'ཨཱབོཨུཏ་',
'cancel'         => 'དོར་བ།',
'qbedit'         => 'རྩོམ་སྒྲིག',
'qbspecialpages' => 'དམིཊ་བསལ་གྱི་བཟོ་བཅོས།',
'mytalk'         => 'ངའི་གླེང་མོལ།',
'navigation'     => 'དཀར་ཆག',

'help'             => 'རོགས་རམ།',
'search'           => 'འཚོལ།',
'searchbutton'     => 'འཚོལ།',
'go'               => 'སོང་།',
'searcharticle'    => 'སོང་།',
'history_short'    => 'རྗེས་ཐོ།',
'printableversion' => 'དཔར་ཐུབ་པའི་དྲ་ངོས།',
'permalink'        => 'རྟག་བརྟན་གྱི་དྲ་འབྲེལ།',
'edit'             => 'རྩོམ་སྒྲིག',
'delete'           => 'གསུབ་པ།',
'protect'          => 'སྲུང་བ།',
'talk'             => 'གྲོས་བསྡུར།',
'toolbox'          => 'ལག་ཆ།',
'jumptonavigation' => 'དཀར་ཆག',
'jumptosearch'     => 'འཚོལ།',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'ཨཱབོཨུཏ་ {{SITENAME}}',
'aboutpage'            => 'Project:ཨཱབོཨུཏ་',
'currentevents'        => 'ད་ལྟའི་བྱ་བ།',
'currentevents-url'    => 'Project:ད་ལྟའི་བྱ་བ།',
'edithelp'             => 'རྩོམ་སྒྲིག་གི་རོགས་རམ།',
'mainpage'             => 'གཙོ་ངོས།',
'mainpage-description' => 'གཙོ་ངོས།',
'portal'               => 'ཁོངས་མི་འདུ་ར།',

'editsection' => 'རྩོམ་སྒྲིག',
'editold'     => 'རྩོམ་སྒྲིག',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'རྩོམ་ཡིག',
'nstab-special'   => 'ཁྱད་པར་བ།',
'nstab-mediawiki' => 'སྐད་ཆ།',

# General errors
'viewsource' => 'འབྱོང་ཁོངས་ལ་ལྟ་བ།',

# Login and logout pages
'yourname'           => 'དྲ་མིང་།',
'yourpassword'       => 'ལམ་ཡིག',
'yourpasswordagain'  => 'ལམ་ཡིག་ཡང་གཏག་བྱོས།',
'remembermypassword' => 'ངའི་དྲ་མིང་འདིར་གསོག་པ།',
'login'              => 'ནང་འཛུལ།',
'userlogin'          => 'ནང་འཛུལ། / ཐོ་འགོད།',
'logout'             => 'ཕྱིར་འབུད།',
'userlogout'         => 'ཕྱིར་འབུད།',
'notloggedin'        => 'ནང་འཛུལ་བྱས་མེད།',
'nologinlink'        => 'ཐོ་ཞིག་འགོད་པ།',
'createaccount'      => 'ཐོ་འགོད།',
'gotaccountlink'     => 'ནང་འཛུལ།',
'youremail'          => 'དྲ་འཕྲིན། *:',
'username'           => 'དྲ་མིང་།:',
'email'              => 'དྲ་འཕྲིན།',

# Edit pages
'summary'      => 'བསྡུས་དོན།',
'minoredit'    => 'འདི་རྩོམ་སྒྲིག་ཚར་མེད།',
'watchthis'    => 'དྲ་ངོས་འདི་ལ་མཉམ་འཇོག་པ།',
'savearticle'  => 'དྲ་ངོས་ཉར་བ།',
'showpreview'  => 'སྔ་ལྟས་སྟོན།',
'showdiff'     => 'བཟོས་བཅོས་སྟོན།',
'loginreqlink' => 'ནང་འཛུལ།',
'newarticle'   => '(ཎེཝ་)',

# History pages
'cur'  => 'ད་ལྟ།',
'last' => 'མཐའ་མ།',

# Search results
'powersearch' => 'འཚོལ།',

# Preferences page
'mypreferences'     => 'ངའི་ལེགས་སྒྲིག',
'prefsnologin'      => 'ནང་འཛུལ་བྱས་མེད།',
'prefs-rc'          => 'ཉེ་བའི་བཟོ་བཅོས།',
'searchresultshead' => 'འཚོལ།',

# Recent changes
'recentchanges'   => 'ཉེ་བའི་བཟོ་བཅོས།',
'minoreditletter' => 'ཨཾི',
'newpageletter'   => 'ཎེ',

# Recent changes linked
'recentchangeslinked' => 'འབྲེལ་བའི་བཟོ་བཅོས།',

# Upload
'upload'            => 'ཡར་འཇོག',
'uploadbtn'         => 'ཡར་འཇོག',
'uploadnologin'     => 'ནང་འཛུལ་བྱས་མེད།',
'filedesc'          => 'བསྡུས་དོན།',
'fileuploadsummary' => 'བསྡུས་དོན།:',
'watchthisupload'   => 'དྲ་ངོས་འདི་ལ་མཉམ་འཇོག་པ།',

# Random page
'randompage' => 'རང་མོས་ཤོག་ངོས།',

'brokenredirects-edit'   => '(རྩོམ་སྒྲིག )',
'brokenredirects-delete' => '(གསུབ་པ།)',

# Miscellaneous special pages
'newpages-username' => 'དྲ་མིང་།:',
'move'              => 'སྤོར།',

# Book sources
'booksources-go' => 'སོང་།',

# Special:AllPages
'allpages'       => 'དྲ་ངོས་ཡོངས།',
'allpagessubmit' => 'སོང་།',

# E-mail user
'emailuser'    => 'བཀོལ་མི་འདིར་དྲ་འཕྲིན་སྐུར་བ།',
'emailmessage' => 'སྐད་ཆ།',

# Watchlist
'watchlist'     => 'ངའི་མཉམ་འཇོག་ཐོ།',
'watchnologin'  => 'ནང་འཛུལ་བྱས་མེད།',
'watch'         => 'མཉམ་འཇོག',
'watchthispage' => 'དྲ་ངོས་འདི་ལ་མཉམ་འཇོག་པ།',

# Restrictions (nouns)
'restriction-edit' => 'རྩོམ་སྒྲིག',
'restriction-move' => 'སྤོར།',

# Undelete
'undelete-search-submit' => 'འཚོལ།',

# Contributions
'contributions' => 'བཀོལ་མིའི་བྱས་རྗེས།',
'mycontris'     => 'ངའི་བྱས་རྗེས།',

# What links here
'whatlinkshere' => 'དྲ་འབྲེལ་ཅི་ཞིག',

# Block/unblock
'ipbreason'          => 'རྒྱུ་མཚན།',
'ipblocklist-submit' => 'འཚོལ།',

# Move page
'movearticle' => 'སྤོར་ངོས།',
'move-watch'  => 'དྲ་ངོས་འདི་ལ་མཉམ་འཇོག་པ།',
'movereason'  => 'རྒྱུ་མཚན།',

# Namespace 8 related
'allmessages' => 'མ་ལག་གི་སྐད་ཆ།',

# Tooltip help for the actions
'tooltip-pt-preferences' => 'ངའི་ལེགས་སྒྲིག',
'tooltip-pt-logout'      => 'ཕྱིར་འབུད།',
'tooltip-ca-move'        => 'ཨཾོབེ༹་ ཐིས་ པགེ་',
'tooltip-p-logo'         => 'གཙོ་ངོས།',

# Special:NewImages
'ilsubmit' => 'འཚོལ།',

# Multipage image navigation
'imgmultigo' => 'སོང་།!',

# Table pager
'table_pager_limit_submit' => 'སོང་།',

# Special:SpecialPages
'specialpages' => 'དམིཊ་བསལ་གྱི་བཟོ་བཅོས།',

);
