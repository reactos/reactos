<?php
/** Buginese (ᨅᨔ ᨕᨘᨁᨗ)
 *
 * @ingroup Language
 * @file
 *
 * @author Kurniasan
 */

$fallback = "id";

$messages = array(
# Dates
'sunday'    => 'ᨕᨕᨖ',
'monday'    => 'ᨕᨔᨛᨙᨊ',
'tuesday'   => 'ᨔᨒᨔ',
'wednesday' => 'ᨕᨑᨅ',
'thursday'  => 'ᨀᨆᨗᨔᨗ',
'friday'    => 'ᨍᨘᨆᨕ',
'saturday'  => 'ᨔᨈᨘ',
'january'   => 'ᨙᨍᨊᨘᨕᨑᨗ',
'february'  => 'ᨙᨄᨅᨛᨑᨘᨕᨑᨗ',
'march'     => 'ᨆᨙᨑ',
'april'     => 'ᨕᨄᨛᨑᨗᨒᨗ',
'may_long'  => 'ᨙᨆᨕᨗ',
'june'      => 'ᨍᨘᨊᨗ',
'july'      => 'ᨍᨘᨒᨗ',
'august'    => 'ᨕᨁᨘᨔᨘᨈᨘᨔᨘ',
'september' => 'ᨙᨔᨙᨈᨇᨛᨑᨛ',
'october'   => 'ᨕᨚᨀᨛᨈᨚᨅᨛᨑᨛ',
'november'  => 'ᨊᨚᨙᨅᨇᨛᨑᨛ',
'december'  => 'ᨉᨗᨙᨔᨇᨛᨑᨛ',

# Categories related messages
'category_header' => 'ᨒᨛᨄ ᨑᨗᨒᨒᨛ ᨙᨀᨈᨛᨁᨚᨑᨗ "$1"',
'subcategories'   => 'ᨔᨅᨛᨙᨀᨈᨛᨁᨚᨈᨗ',

'about'      => 'Atajangeng',
'qbedit'     => 'Sunting',
'mytalk'     => 'ᨕᨄᨅᨗᨌᨑᨊ ᨕᨗᨐ',
'anontalk'   => 'Bicara IP',
'navigation' => 'ᨊᨄᨗᨁᨔᨗ',
'and'        => 'éréngé',

'help'             => 'ᨄᨂᨗᨋᨗ',
'search'           => 'ᨔᨄ',
'searchbutton'     => 'ᨔᨄ',
'go'               => 'ᨒᨕᨚ',
'searcharticle'    => 'ᨒᨕᨚ',
'history_short'    => 'ᨔᨛᨍᨑ',
'edit'             => 'ᨙᨕᨉᨗ',
'create'           => 'ᨕᨛᨅᨘ',
'editthispage'     => 'ᨙᨕᨉᨗ ᨙᨕᨙᨉ ᨒᨛᨄ',
'delete'           => 'ᨄᨛᨙᨉ',
'talkpagelinktext' => 'ᨅᨗᨌᨑ',
'specialpage'      => 'ᨒᨛᨄ ᨔᨛᨙᨄᨔᨗᨕᨒ',
'imagepage'        => 'Ita halamang rapang',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Tentang {{SITENAME}}',
'mainpage'             => 'ᨒᨛᨄ ᨕᨗᨉᨚᨙᨕ',
'mainpage-description' => 'ᨒᨛᨄ ᨕᨗᨉᨚᨙᨕ',
'portal'               => 'Portal komunitas',

'editsection' => 'ᨙᨕᨉᨗ',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'     => 'ᨒᨛᨄ',
'nstab-user'     => 'ᨒᨛᨄ ᨄᨁᨘᨊ',
'nstab-special'  => 'ᨔᨛᨙᨄᨔᨗᨕᨒ',
'nstab-image'    => 'Rapang',
'nstab-help'     => 'ᨄᨂᨗᨋᨗ',
'nstab-category' => 'ᨙᨀᨈᨛᨁᨚᨑᨗ',

# General errors
'badtitle'   => 'Judul dek essa',
'viewsource' => 'ᨕᨗᨈ ᨔᨚᨑᨛᨔᨛ',

# Login and logout pages
'login'      => 'ᨒᨚᨁᨛ ᨕᨈᨆ',
'userlogin'  => 'ᨒᨚᨁᨛ ᨕᨈᨆ / ᨕᨛᨅᨘ ᨕᨀᨕᨘᨊᨛ',
'logout'     => 'ᨒᨚᨁᨛ ᨕᨛᨔᨘ',
'userlogout' => 'ᨒᨚᨁᨛ ᨕᨛᨔᨘ',
'nologin'    => 'ᨙᨉᨄ ᨆᨄᨘᨊ ᨕᨀᨕᨘᨊᨛ? $1.',
'gotaccount' => 'ᨄᨘᨑᨊᨗ ᨕᨛᨃ ᨕᨀᨘᨊᨛᨛ? $1.',
'username'   => 'ᨕᨔᨛ ᨄᨁᨘᨊ:',

# Edit page toolbar
'bold_tip'   => 'ᨙᨈᨀᨛᨔᨛ ᨆᨕᨘᨇᨛ',
'italic_tip' => 'ᨙᨈᨀᨛᨔᨛ ᨕᨗᨈᨒᨗᨀᨛ',

# Edit pages
'preview'          => 'ᨄᨛᨑᨗᨅᨗᨐᨘ',
'accmailtitle'     => 'Ada sandi ni riantarak.',
'accmailtext'      => 'Ada sandi "$1" riantarak ri $2.',
'anontalkpagetext' => "----''Ini adalah halaman diskusi untuk pengguna anonim yang belum membuat rekening atau tidak menggunakannya. Karena tidak membuat rekening, kami terpaksa memakai alamat IP untuk mengenalinya. Alamat IP seperti ini dapat dipakai oleh beberapa pengguna yang berbeda. Jika Anda adalah pengguna anonim dan merasa mendapatkan komentar-komentar yang tidak berkaitan dengan anda, kami anjurkan untuk [[Special:UserLogin|membuat rekening atau masuk log]] untuk menghindari kerancuan dengan pengguna anonim lain.''",
'editing'          => 'ᨙᨕᨉᨗᨈᨗ $1',

# Recent changes
'recentchanges' => 'ᨄᨄᨀᨗᨋ ᨈᨊᨄ',

# Recent changes linked
'recentchangeslinked' => 'Pappakapinra terkait',

# Upload
'upload'    => 'Lureng berkas',
'uploadbtn' => 'Lureng berkas',

# Random page
'randompage' => 'Halamang rawak',

# Miscellaneous special pages
'ancientpages' => 'Artikel talloa',
'move'         => 'ᨙᨕᨔᨘ',
'movethispage' => 'ᨙᨕᨔᨘᨀᨗ ᨕᨗᨙᨐᨙᨉ ᨒᨛᨄ',

# Special:AllPages
'allpages'       => 'Maneng halamang',
'alphaindexline' => '$1 ri $2',
'allpagesfrom'   => 'Mappaitang halamang-halamang rimulai:',
'allarticles'    => 'Maneng artikel',
'allinnamespace' => 'Maneng halamang ($1 namespace)',
'allpagesnext'   => 'Selanjutnya',
'allpagessubmit' => 'Lanre',
'allpagesprefix' => 'Mappaitang halamang-halamang éngkalinga awang:',

# Watchlist
'addedwatch'     => 'Tamba ri jagaan',
'addedwatchtext' => "Halamang \"[[:\$1]]\" ni ritamba ri ida [[Special:Watchlist|watchlist]].
Halamang bicara éréngé gabungan halamang bicara pada wettu depan didaftarkan koe,
éréngé halamang akan wessi '''umpek''' ri [[Special:RecentChanges|daftar pinra tanappa]] barak lebih lemmak ita.

Apak ida ronnak mappedde halamang édé ri daftar jagaan, klik \"Mangedda jaga\" pada kolom ri sedde.",

# Delete/protect/revert
'actioncomplete' => 'Proses makkapo',

# Namespace form on various pages
'blanknamespace' => '(Utama)',

# What links here
'whatlinkshere' => 'Pranala ri halamang édé',

# Move page
'articleexists' => 'Halamang béla ida pile ni ujuk, a dek essa.
Silakan pile aseng laing.',
'1movedto2'     => '[[$1]] ésuk ri [[$2]]',

# Namespace 8 related
'allmessages'        => 'Maneng pappaseng',
'allmessagesname'    => 'Aseng',
'allmessagesdefault' => 'Teks totok',
'allmessagescurrent' => 'Teks kokkoro',

# Attribution
'anonymous' => 'Pabbuak anonim {{SITENAME}}',

# Media information
'imagemaxsize' => 'Gangkai rapang pada keterangan rapang ri halamang hingga:',

# Special:NewImages
'ilsubmit' => 'ᨔᨄ',

# 'all' in various places, this might be different for inflected languages
'imagelistall' => 'maneng',

# Special:SpecialPages
'specialpages' => 'Halamang Istimewa',

);
