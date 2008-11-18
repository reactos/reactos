<?php
/** Scottish Gaelic (Gàidhlig)
 *
 * @ingroup Language
 * @file
 *
 * @author Alison
 * @author Raymond
 * @author Sionnach
 * @author לערי ריינהארט
 */

$messages = array(
# Dates
'thursday' => 'Diardaoin',
'friday'   => 'Di-Haoine',
'august'   => 'An Lùnastal',

# Categories related messages
'category_header' => 'Altan sa ghnè "$1"',
'subcategories'   => 'Fo-ghnethan',

'about'          => 'Mu',
'newwindow'      => "(a'fosgladh ann an uinneag ùr)",
'qbfind'         => 'Lorg',
'qbedit'         => 'Deasaich',
'qbpageoptions'  => 'An duilleag seo',
'qbmyoptions'    => 'Na duilleagan agam',
'qbspecialpages' => 'Duilleagan àraidh',
'moredotdotdot'  => 'Barrachd...',
'mypage'         => 'Mo dhuilleag',
'anontalk'       => 'Labhairt air an IP seo',
'and'            => 'agus',

'errorpagetitle'    => 'Mearachd',
'returnto'          => 'Till gu $1.',
'tagline'           => "As a'{{SITENAME}}",
'help'              => 'Cuideachadh',
'search'            => 'Lorg',
'go'                => 'Rach',
'history'           => 'Eachdraidh dhuilleag',
'history_short'     => 'Eachdraidh',
'info_short'        => 'Fiosrachadh',
'printableversion'  => 'Lethbhreac so-chlòbhualadh',
'edit'              => 'Deasaich',
'editthispage'      => 'Deasaich an duilleag seo',
'deletethispage'    => 'Dubh às an duilleag seo',
'protect'           => 'Dìon',
'protectthispage'   => 'Dìon an duilleag seo',
'unprotect'         => 'Neo-dhìon',
'unprotectthispage' => 'Neo-dìon an duilleag seo',
'newpage'           => 'Duilleag ùr',
'talkpage'          => "Deasbair mu'n duilleig seo",
'talkpagelinktext'  => 'Deasbaireachd',
'specialpage'       => 'Duilleag àraidh',
'personaltools'     => 'Innealan pearsanta',
'talk'              => 'Deasbaireachd',
'userpage'          => 'Seall duilleag cleachdair',
'imagepage'         => 'Seall duilleag ìomhaigh',
'otherlanguages'    => 'Cainntean eile',
'redirectedfrom'    => '(Ath-stiùirte o $1)',
'protectedpage'     => 'Duilleag dìonta',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => "Mu dheidhinn a' {{SITENAME}}",
'aboutpage'            => 'Project:Mu',
'copyright'            => 'Gheibhear brìgh na duilleig seo a-rèir an $1.',
'copyrightpage'        => '{{ns:project}}:Dlighean-sgrìobhaidh',
'currentevents'        => 'Cùisean an latha',
'disclaimers'          => 'Àicheidhean',
'edithelp'             => 'Cobhair deasaichaidh',
'edithelppage'         => 'Help:Deasaicheadh',
'helppage'             => 'Help:Cuideachadh',
'mainpage'             => 'Prìomh-Dhuilleag',
'mainpage-description' => 'Prìomh-Dhuilleag',
'portal'               => 'Doras na Coimhearsnachd',
'portal-url'           => 'Project:Doras na coimhearsnachd',
'privacy'              => 'Polasaidh uaigneachd',

'retrievedfrom'   => 'Air tarraing à "$1"',
'newmessageslink' => 'teachdaireachdan ùra',
'editsection'     => 'deasaich',
'toc'             => 'Clàr-innse',
'showtoc'         => 'nochd',
'hidetoc'         => 'falaich',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Aiste',
'nstab-user'      => 'Duilleag cleachdair',
'nstab-media'     => 'Meadhanan',
'nstab-special'   => 'Àraidh',
'nstab-image'     => 'Ìomhaigh',
'nstab-mediawiki' => 'Teachdaireachd',
'nstab-template'  => 'Cumadair',
'nstab-help'      => 'Cobhair',
'nstab-category'  => 'Gnè',

# General errors
'error'           => 'Mearachd',
'databaseerror'   => 'Mearachd an stor-dàta',
'noconnect'       => "Tha sinn duilich! Tha draghannan teicnòlach aig a'wiki an dràsda, is cha chuirear fios ri fritheiliche an stor-dàta. <br />
$1",
'nodb'            => 'Cha do thaghadh stòr-dàta $1',
'cachederror'     => "Is e lethbhreac taisgte na duilleig a dh'iarr sibh a leanas, agus dh'fhaoite nach eil e nuadh-aimsireil.",
'readonly'        => 'Stor-dàta glaiste',
'badarticleerror' => 'Cha ghabh an gnìomh seo a dhèanamh air an duilleig seo.',
'badtitle'        => 'Droch thiotal',
'perfdisabled'    => "Tha sinn duilich! Tha an goireas seo air a chur bho fheum gu sealadach o chionns gum bi e a'slaodadh an stòr-dàta gus nach urrain do neach sam bith an wiki a chleachdadh.",

# Login and logout pages
'yourname'                   => 'An t-ainm-cleachdair agaibh',
'yourpassword'               => 'Am facal-faire agaibh',
'yourpasswordagain'          => 'Ath-sgrìobh facal-faire',
'login'                      => 'Cuir a-steach',
'userlogin'                  => 'Cuir a-steach',
'logout'                     => 'Cuir a-mach',
'userlogout'                 => 'Clàraich a-mach',
'createaccount'              => 'Cruthaich cùnntas ùr',
'yourrealname'               => "An dearbh ainm a th'oirbh*",
'yournick'                   => 'An leth-ainm agaibh (a chuirear ti teachdaireachdan)',
'noname'                     => 'Chan eil sibh air ainm-cleachdair iomchaidh a chomharrachadh.',
'nosuchusershort'            => 'Chan eil cleachdair leis an ainm "<nowiki>$1</nowiki>" ann; sgrùdaibh an litreachadh agaibh no cleachaibh am billeag gu h-ìseal gus cùnntas ùr a chrùthachadh.',
'wrongpassword'              => "Chan eil am facal-faire a sgrìobh sibh a-steach ceart. Feuchaibh a-rithis, ma's e ur toil e.",
'acct_creation_throttle_hit' => 'Tha sinn duilich; tha sibh air $1 cùnntasan a chruthachadh cheana agus chan fhaod sibh barrachd a dhèanamh.',

# Edit page toolbar
'italic_sample'   => 'Teacsa eadailteach',
'italic_tip'      => 'Teacsa eadailteach',
'headline_sample' => 'Teacsa ceann-loidhne',
'headline_tip'    => 'Ceann-loidhne ìre 2',

# Edit pages
'summary'         => 'Geàrr-chùnntas',
'subject'         => 'Cuspair/ceann-loidhne',
'minoredit'       => 'Seo mùthadh beag',
'watchthis'       => 'Cùm sùil air an aithris seo',
'savearticle'     => 'Sàbhail duilleag',
'preview'         => 'Roi-shealladh',
'showpreview'     => 'Nochd roi-shealladh',
'showdiff'        => 'Seall atharrachaidhean',
'blockedtitle'    => 'Tha an cleachdair air a bhacadh',
'accmailtitle'    => 'Facal-faire air a chur.',
'accmailtext'     => "Tha am facal-faire aig '$1' air a chur ri $2.",
'newarticle'      => '(Ùr)',
'noarticletext'   => '(Chan eil teacsa anns an duilleig seo a-nis)',
'updated'         => '(Nua-dheasaichte)',
'previewnote'     => '<strong>Cuimhnichibh nach e ach roi-shealladh a tha seo, agus chan eil e air a shàbhaladh fhathast!</strong>',
'editing'         => "A'deasaicheadh $1",
'editconflict'    => 'Mì-chòrdadh deasachaidh: $1',
'explainconflict' => "Tha cuideigin eile air an duilleig seo a mhùthadh o'n thòisich sibh fhèin a dheasaicheadh. Tha am bocsa teacsa shuas a'nochdadh na duilleig mar a tha e an dràsda. Tha na mùthaidhean agaibhse anns a'bhocsa shios. Feumaidh sibh na mùthaidhean agaibh a choimeasgachadh leis an teacsa làithreach. Cha tèid <b>ach an teacsa shuas</b> a shàbhaladh an uair a bhriogas sibh \"Sàbhail duilleag\".<p>",
'yourtext'        => 'An teacsa agaibh',
'storedversion'   => 'Lethbhreac taisgte',
'editingold'      => "<strong>RABHADH: Tha sibh a'deasaicheadh lethbhreac sean-aimsireil na duilleig seo. Ma shàbhalas sibh e, bithidh uile na mùthaidhean dèanta as dèidh an lethbhreac seo air chall.</strong>",
'yourdiff'        => 'Caochlaidhean',

# History pages
'nohistory'  => 'Chan eil eachdraidh deasachaidh aig an duilleig seo.',
'currentrev' => 'Lethbhreac làithreach',
'cur'        => 'làith',
'next'       => 'ath',
'last'       => 'mu dheireadh',

# Diffs
'lineno'                  => 'Loidhne $1:',
'compareselectedversions' => 'Coimeas lethbhreacan taghta',

# Search results
'searchresults'     => 'Toraidhean rannsachaidh',
'notitlematches'    => "Chan eil tiotal duilleig a'samhlachadh",
'notextmatches'     => "Chan eil teacsa duilleig a'samhlachadh",
'prevn'             => '$1 mu dheireadh',
'nextn'             => 'an ath $1',
'viewprevnext'      => 'Seall ($1) ($2) ($3).',
'showingresults'    => "A'nochdadh {{PLURAL:$1|'''1''' toradh|'''$1''' toraidhean}} gu h-ìosal a'tòiseachadh le #'''$2'''.",
'showingresultsnum' => "A'nochdadh {{PLURAL:$3|'''1''' toradh|'''$3''' toraidhean}} gu h-ìosal a'tòiseachadh le #'''$2'''.",
'powersearch'       => 'Rannsaich',

# Preferences page
'preferences'        => 'Taghaidhean',
'changepassword'     => 'Atharraich facal-faire',
'skin'               => 'Bian',
'dateformat'         => 'Cruth nan ceann-latha',
'math_unknown_error' => 'mearachd neo-aithnichte',
'prefs-personal'     => "Dàta a'chleachdair",
'saveprefs'          => 'Sàbhail roghainnean',
'resetprefs'         => 'Ath-shuidhich taghaidhean',
'oldpassword'        => 'Seann fhacal-faire',
'newpassword'        => 'Facal-faire ùr',
'retypenew'          => 'Ath-sgrìobh facal-faire ùr',
'rows'               => 'Sreathan',
'columns'            => 'Colbhan',
'savedprefs'         => 'Tha na roghainnean agaibh air an sàbhaladh.',

# Recent changes
'nchanges'          => '$1 {{PLURAL:$1|mùthadh|mùthaidhean}}',
'recentchanges'     => 'Mùthaidhean ùra',
'recentchangestext' => 'Lean mùthaidhean ùra aig an wiki air an duilleag seo.',
'rcnote'            => "Tha na {{PLURAL:$1|'''1''' mùthadh|$1 mùthaidhean}} deireanach air na {{PLURAL:$2|là|'''$2''' laithean}} deireanach gu h-ìosal as  $5, $4.",
'rcnotefrom'        => "Gheibhear na mùthaidhean o chionn <b>$2</b> shios (a'nochdadh suas ri <b>$1</b>).",
'rclistfrom'        => 'Nochd mùthaidhean ùra o chionn $1',
'rclinks'           => 'Nochd na $1 mùthaidhean deireanach air na $2 laithean deireanach<br />$3',
'diff'              => 'diof',
'hist'              => 'eachd',
'hide'              => 'falaich',
'show'              => 'nochd',
'minoreditletter'   => 'b',
'newpageletter'     => 'Ù',
'boteditletter'     => 'r',

# Recent changes linked
'recentchangeslinked' => 'Mùthaidhean buntainneach',

# Upload
'upload'        => 'Cuir ri fhaidhle',
'filename'      => 'Ainm-faidhle',
'filedesc'      => 'Geàrr-chùnntas',
'filestatus'    => 'Cor dlighe-sgrìobhaidh:',
'ignorewarning' => 'Leig an rabhadh seachad agus sàbhail am faidhle codhiù.',
'badfilename'   => 'Ainm ìomhaigh air atharrachadh ri "$1".',
'fileexists'    => 'Tha faidhle leis an ainm seo ann cheana; nach faigh sibh cinnt air <strong><tt>$1</tt></strong> gu bheil sibh ag iarraidh atharrachadh.',
'savefile'      => 'Sàbhail faidhle',

# Special:ImageList
'imagelist' => 'Liosta nan ìomhaigh',

# Random page
'randompage' => 'Duilleag thuairmeach',

# Statistics
'sitestatstext' => "Tha {{PLURAL:\$1|'''1''' duilleag|'''\$1''' duilleagan gu lèir}} anns an stor-dàta, a'cur san àireamh duilleagan-làbhairt, duilleagan mu dheidhinn a'{{SITENAME}} fhèin, duilleagan \"bun\", ath-stiùireidhean, agus feadhainn eile nach eil nan duilleag brìgheil. As aonais sin, tha '''\$2''' duilleagan ann le brìgh.

'''\$8''' {{PLURAL:\$8|fhaidhl|fhaidhle}} a cuir ri.

Tha na duilleagan air an sealladh '''\$3''' {{PLURAL:\$3|uair|uairean}}, agus air an deasaicheadh '''\$4''' {{PLURAL:\$4|uair|uairean}} o'n deach an wiki a shuidheachadh.
Thig sin ri '''\$5''' deasaicheidhean anns a'mheadhan gach duilleag, agus '''\$6''' seallaidhean gach duilleag.

Tha feadh an [http://www.mediawiki.org/wiki/Manual:Job_queue queue tùrn] na '''\$7'''.",

'doubleredirects' => 'Ath-stiùreidhean dùbailte',

'brokenredirects' => 'Ath-stiùireidhean briste',

# Miscellaneous special pages
'nviews'                  => '$1 {{PLURAL:$1|sealladh|seallaidhean}}',
'uncategorizedpages'      => 'Duilleagan neo-ghnethichte',
'uncategorizedcategories' => 'Gnethan neo-ghnethichte',
'unusedimages'            => 'Ìomhaighean neo-chleachdte',
'shortpages'              => 'Duilleagan goirid',
'longpages'               => 'Duilleagan fhada',
'listusers'               => 'Liosta nan cleachdair',
'newpages'                => 'Duilleagan ùra',
'ancientpages'            => 'Duilleagan as sìne',
'move'                    => 'Gluais',
'movethispage'            => 'Caraich an duilleag seo',

# Special:AllPages
'allpages' => 'Duilleagan uile',
'nextpage' => 'An ath dhuilleag ($1)',

# Special:Categories
'categories'         => 'Gnethan',
'categoriespagetext' => "Tha na gnethan a leanas anns a'wiki.",

# E-mail user
'emailfrom'    => 'Bho',
'emailto'      => 'Ri',
'emailsubject' => 'Cuspair',
'emailmessage' => 'Teachdaireachd',
'emailsend'    => 'Cuir',

# Watchlist
'watchlist'          => 'Mo fhaire',
'nowatchlist'        => 'Chan eil altan air ur faire.',
'addedwatch'         => 'Cuirte ri coimheadlìosta',
'addedwatchtext'     => "Tha an duilleag \"[[:\$1]]\" cuirte ri [[Special:Watchlist|ur faire]] agaibh.  Ri teachd, bith chuir an àireamh an-sin mùthadhan na duilleag sin agus a'dhuilleag \"Talk\", agus bith a'dhuilleag '''tromte''' anns an [[Special:RecentChanges|lìosta nan mùthadhan ùra]] a dh'fhurasdaich i a sheall.

Ma bu toil leibh a dhubh a'dhuilleag as ur faire agaibh nas fadalache, cnap air \"Caisg a' coimhead\" air an taobh-colbh.",
'watchthispage'      => 'Cùm sùil air an dhuilleag seo',
'watchnochange'      => "Cha deach na duilleagan air ur faire a dheasachadh anns a'chuairt ùine taisbeanta.",
'watchmethod-recent' => "A'sgrùdadh deasachaidhean ùra airson duilleagan air ur faire",
'watchmethod-list'   => "A'sgrùdadh duilleagan air ur faire airson deasachaidhean ùra",
'watchlistcontains'  => 'Tha $1 {{PLURAL:$1|duilleag|duilleagan}} air an liosta-faire agaibh.',
'wlshowlast'         => 'Nochd $1 uairean $2 laithean mu dheireadh $3',

# Delete/protect/revert
'deletepage'         => 'Dubh às duilleag',
'confirm'            => 'Daingnich',
'excontent'          => "stuth a bh'ann: '$1'",
'exblank'            => 'bha duilleag falamh',
'actioncomplete'     => 'Gnìomh coileanta',
'reverted'           => 'Tillte ri lethbhreac as ùire',
'editcomment'        => 'Bha mìneachadh an deasaicheidh: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'         => 'Tillte deasachadh aig [[Special:Contributions/$2|$2]] ([[User talk:$2|Deasbaireachd]]) ais ri lethbhreac mu dheireadh le [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectedarticle'   => 'dìonta "[[$1]]"',
'unprotectedarticle' => '"[[$1]]" neo-dhìonta',
'protect-title'      => 'A\'dìonadh "$1"',
'protect-legend'     => 'Daingnich dìonadh',
'protectcomment'     => 'Aobhar airson dìonaidh',

# Undelete
'undeleterevisions' => '$1 {{PLURAL:$1|lethbhreac|lethbhreacan}} taisge',

# Namespace form on various pages
'blanknamespace' => '(Prìomh)',

# Contributions
'mycontris' => 'Mo chuideachaidhean',
'uctop'     => ' (bàrr)',

# What links here
'whatlinkshere' => "Dè tha a' ceangal ri seo?",
'isredirect'    => 'duilleag ath-stiùireidh',

# Block/unblock
'blockip'            => 'Bac cleachdair',
'ipaddress'          => 'IP Seòladh/ainm-cleachdair',
'ipbreason'          => 'Aobhar',
'ipbsubmit'          => 'Bac an cleachdair seo',
'badipaddress'       => "Chan eil an seòladh IP aig a'cleachdair seo iomchaidh",
'blockipsuccesssub'  => 'Shoirbhich bacadh',
'blockipsuccesstext' => "Tha [[Special:Contributions/$1|$1]] air a bhacadh.
<br />Faic [[Special:IPBlockList|Liosta nan IP baicte]] na bacaidhean a dh'ath-sgrùdadh.",
'unblockip'          => 'Neo-bhac cleachdair',
'ipusubmit'          => 'Neo-bhac an seòladh seo',
'ipblocklist'        => 'Liosta seòlaidhean IP agus ainmean-cleachdair air am bacadh',
'blocklink'          => 'bac',
'unblocklink'        => 'neo-bhac',
'blocklogentry'      => 'Chaidh [[$1]] a bhacadh le ùine crìochnachaidh de $2 $3',
'unblocklogentry'    => '"$1" air neo-bhacadh',
'ipb_expiry_invalid' => 'Ùine-crìochnaidh neo-iomchaidh.',
'ip_range_invalid'   => 'Raon IP neo-iomchaidh.',
'proxyblocksuccess'  => 'Dèanta.',

# Developer tools
'lockdb'           => 'Glais stor-dàta',
'lockconfirm'      => 'Seadh, is ann a tha mi ag iarraidh an stor-dàta a ghlasadh.',
'lockbtn'          => 'Glais stor-dàta',
'lockdbsuccesssub' => 'Shoirbhich glasadh an stor-dàta',

# Move page
'move-page-legend' => 'Gluais duilleag',
'movearticle'      => 'Gluais duilleag',
'movepagebtn'      => 'Gluais duilleag',
'pagemovedsub'     => 'Gluasad soirbheachail',
'movedto'          => 'air gluasad gu',
'1movedto2'        => '[[$1]] gluaiste ri [[$2]]',
'1movedto2_redir'  => '[[$1]] gluaiste ri [[$2]] thairis air ath-stiùireadh',

# Namespace 8 related
'allmessages'     => 'Uile teachdaireachdan an t-siostam',
'allmessagestext' => 'Seo liosta de\'n a h-uile teachdaireachd an t-siostam ri fhaotainn anns an fhànais-ainm "Mediawiki:".',

# Thumbnails
'thumbnail-more' => 'Meudaich',
'filemissing'    => "Faidhle a dh'easbhaidh",

# Special:Import
'importnotext' => 'Falamh no gun teacsa',

# Tooltip help for the actions
'tooltip-minoredit' => 'Comharraich seo mar meanbh-dheasachadh',
'tooltip-save'      => 'Sàbhail na mùthaidhean agaibh',
'tooltip-preview'   => 'Roi-sheallaibh na mùthaidhean agaibh; cleachdaibh seo mas sàbhail sibh iad!',

# Attribution
'othercontribs' => 'Stèidhichte air obair le $1.',
'others'        => 'eile',

# Info page
'infosubtitle' => 'Fiosrachadh air duilleig',
'numwatchers'  => 'Aireamh luchd-faire: $1',

# Special:NewImages
'ilsubmit' => 'Rannsaich',
'bydate'   => 'air ceann-latha',

# Special:Version
'version' => 'Lethbhreac', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Duilleagan àraidh',

);
