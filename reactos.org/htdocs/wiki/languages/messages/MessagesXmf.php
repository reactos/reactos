<?php
/** Mingrelian (მარგალური)
 *
 * @ingroup Language
 * @file
 *
 * @author Alsandro
 * @author Dato deutschland
 * @author Malafaya
 */

$fallback = 'ka';

$messages = array(
# User preference toggles
'tog-underline'       => 'ხაზ გუსვი ბუნილეფს:',
'tog-highlightbroken' => 'ქააძირი ვაარარსებულ ბუნილეფ <a href="" class="new">მუჭოთ თენა</a> (ალტერნატივა: მუჭოთ თენა<a href="" class="internal">?</a>).',
'tog-justify'         => 'გაასწორი პარაგრაფეფ',
'tog-hideminor'       => 'დოჩული ჭიჭე რედაქტირება ბოლო თირაფეფს',
'tog-showtoolbar'     => 'რედაქტირებაშ ინსტრუმენტეფიშ ძირაფა (ჯავასკრიპტ)',

'underline-always' => 'ირო',
'underline-never'  => 'შურო',

# Dates
'sunday'        => 'ჟაშხა',
'monday'        => 'თუთაშხა',
'tuesday'       => 'თახაშხა',
'wednesday'     => 'ჯუმაშხა',
'thursday'      => 'ცაშხა',
'friday'        => 'ობიშხა',
'saturday'      => 'საბატონი',
'sun'           => 'ჟაშ',
'mon'           => 'თუთ',
'tue'           => 'თახ',
'wed'           => 'ჯუმ',
'thu'           => 'ცაა',
'fri'           => 'ობი',
'sat'           => 'საბ',
'january'       => 'იანარი',
'february'      => 'ფრევალი',
'march'         => 'მარტი',
'april'         => 'აპრილი',
'may_long'      => 'მეესი',
'june'          => 'ივანობა',
'july'          => 'კვირკვე',
'august'        => 'არგუსო',
'september'     => 'ეკენია',
'october'       => 'გიმათუთა',
'november'      => 'გერგებათუთა',
'december'      => 'ქირსეთუთა',
'january-gen'   => 'იანარი',
'february-gen'  => 'ფრევალი',
'march-gen'     => 'მარტიშ',
'april-gen'     => 'აპრილი',
'may-gen'       => 'მეესიშ',
'june-gen'      => 'ივანობაშ',
'july-gen'      => 'კვირკვეშ',
'august-gen'    => 'არგუსო',
'september-gen' => 'ეკენიაშ',
'october-gen'   => 'გიმათუთაშ',
'november-gen'  => 'გერგებათუთაშ',
'december-gen'  => 'ქირსეთუთაშ',
'jan'           => 'იან',
'feb'           => 'ფრე',
'mar'           => 'მარ',
'apr'           => 'აპრ',
'may'           => 'მეე',
'jun'           => 'ივა',
'jul'           => 'კვი',
'aug'           => 'არგ',
'sep'           => 'ეკე',
'oct'           => 'გიმ',
'nov'           => 'გერ',
'dec'           => 'ქირ',

# Categories related messages
'pagecategories'         => '{{PLURAL:$1|კატეგორია|კატეგორიეფ}}',
'category_header'        => 'სტატიეფ კატეგორიას "$1"',
'subcategories'          => 'ქვეკატეგორიეფ',
'category-media-header'  => 'მედია კატეგორიას "$1"',
'category-empty'         => "''თენა კატეგორიას ვარენა ხასილეფ დო მედია''",
'listingcontinuesabbrev' => 'გინძარ.',

'about'          => '-შენი',
'article'        => 'სტატია',
'newwindow'      => '(ახალ ოჭკორიეშა)',
'cancel'         => 'გაუქვება',
'qbfind'         => 'მიგორე',
'qbedit'         => 'რედაქტირება',
'qbpageoptions'  => 'თენა ხასილა',
'qbpageinfo'     => 'კონტექსტ',
'qbmyoptions'    => 'ჩქიმ ხასილეფ',
'qbspecialpages' => 'სპეციალურ ხასილეფ',
'moredotdotdot'  => 'სრულო...',
'mypage'         => 'ჩქიმ ხასილა',
'mytalk'         => 'ჩქიმ სხუნუა',
'navigation'     => 'ნავიგაცია',
'and'            => 'დო',

'errorpagetitle'   => 'შეცთომა',
'returnto'         => 'დირთი $1-შა',
'tagline'          => '{{SITENAME}} ხასილაშე',
'help'             => 'მოხვარა',
'search'           => 'გორუა',
'searchbutton'     => 'გორუა',
'go'               => 'სტატია',
'searcharticle'    => 'სტატია',
'history'          => 'ხასილაშ ისტორია',
'history_short'    => 'ისტორია',
'info_short'       => 'ინფორმაცია',
'printableversion' => 'ობეშტალ ვერსია',
'permalink'        => 'ირიათონ რსხილ',
'print'            => 'დობეჭდი',
'edit'             => 'რედაქტირაფა',
'editthispage'     => 'ხასილაშ რედაქტირება',
'delete'           => 'წაშალი',
'deletethispage'   => 'წაშალ თე ხასილა',
'protect'          => 'დობლოკი',
'newpage'          => 'ახალ ხასილა',
'talkpage'         => 'მოჩამილ ხასილაშ სხუნუა',
'talkpagelinktext' => 'სხუნუა',
'specialpage'      => 'სპეციალურ ხასილა',
'personaltools'    => 'საკუთარ მოღეეფ',
'talk'             => 'სხუნუა',
'views'            => 'ძირაფა',
'toolbox'          => 'ინსტრუმენტეფ',
'otherlanguages'   => 'შხვა ნინალეფს',
'redirectedfrom'   => '(გინოღალულ რე $1-იშე)',
'redirectpagesub'  => 'გინოწურაფა ხასილას',
'jumpto'           => 'გეგნორთ:',
'jumptonavigation' => 'ნავიგაცია',
'jumptosearch'     => 'გორუა',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}}-შენი',
'aboutpage'            => 'Project:შენი',
'copyrightpage'        => '{{ns:project}}:ავტორიშ ულაფეფ',
'currentevents'        => 'მიმალ მოლინეფ',
'currentevents-url'    => 'Project:მიმალ მოლინეფ',
'disclaimers'          => 'გამამინჯალაშ მონწყუმა',
'disclaimerpage'       => 'Project::პასუხისმგებლობაშ ვარება',
'edithelp'             => 'მოხვარა',
'edithelppage'         => 'Help:ტექსტიშ რედაქტირაფა (იოლი)',
'helppage'             => 'Help:მოხვარა',
'mainpage'             => 'დუდ ხასილა',
'mainpage-description' => 'დუდ ხასილა',
'portal'               => 'საზოგადოებაშ ხასილეფ',
'portal-url'           => 'Project:საზოგადოებაშ ხასილეფ',
'privacy'              => 'ანონიმურობაშ პოლიტიკა',
'privacypage'          => 'Project:ანონიმურობაშ პოლიტიკა',

'ok'                  => 'ჯგირ',
'retrievedfrom'       => 'გორილ რე "$1"-იშე',
'youhavenewmessages'  => 'თქვა გიღუნა $1 ($2).',
'newmessageslink'     => 'ახალ ტყობინაფეფ',
'newmessagesdifflink' => 'შხვანობა ოწოხოლენ ვერსიაწკუმა',
'editsection'         => 'რედაქტირაფა',
'editold'             => 'რედაქტირაფა',
'editsectionhint'     => 'სექციაშ რედაქტირაფა: $1',
'toc'                 => 'ოსხუნალ',
'showtoc'             => 'ძირაფა',
'hidetoc'             => 'ფულუა',
'site-rss-feed'       => '$1-იშ არხ RSS',
'site-atom-feed'      => '$1-იშ არხი Atom',
'page-rss-feed'       => '$1-იშ  არხი  RSS',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'     => 'სტატია',
'nstab-user'     => 'მახვარებელიშ ხასილა',
'nstab-media'    => 'მედიაშ ხასილა',
'nstab-special'  => 'სპეციალურ',
'nstab-project'  => 'პროექტიშ ხასილა',
'nstab-image'    => 'ფაილი',
'nstab-template' => 'თანგი',
'nstab-category' => 'კატეგორია',

# Main script and global functions
'nosuchspecialpage' => 'სპეციალურ ხასილეფ ვაარსეენც',

# General errors
'badtitle'       => 'ცაგანა სათაური',
'badtitletext'   => 'მოთხილ ხასილაშ ოდუდალე ვა რდუ სწორი, ვარა ცალიორ რდუ, ვარა ხოლო გოხოლუდეს ნინეფშქა დო ინტერ-ვიკი ოდუდალეშ ჩილათირო დოქიმინელ რსხი.',
'viewsource'     => 'ქოძირ წყუ',
'viewsourcefor'  => '$1-იშ',
'viewsourcetext' => 'თქვა შეგილებუნა ქოძირათ თე ხასილაშ დაჭყაფურ ფაილ დო ქუდარსხუათ თიშ მანგი:',

# Login and logout pages
'yourname'                => 'მახვარებელ',
'yourpassword'            => 'პაროლ',
'remembermypassword'      => 'ქიმკომიწონი',
'yourdomainname'          => 'თქვან დომენ',
'login'                   => 'მინულა',
'nav-login-createaccount' => 'მინულა',
'loginprompt'             => 'ვიკიპედიაშა მიშაულარო ოხვილუთ კუკიშ გოაქტირაფას.',
'userlogin'               => 'მინულა',
'logout'                  => 'გუმულა',
'userlogout'              => 'გიშულა',
'nologin'                 => 'დიორდე ვარეთ რეგისტრირებულ? $1.',
'nologinlink'             => 'გონწყით ანგარიშ',
'gotaccount'              => 'უკვე რეგისტრირებულ რეთ? $1',
'gotaccountlink'          => 'მინულა',
'yourrealname'            => 'ნანდულ სახელ *',
'yourlanguage'            => 'ნინა:',
'loginsuccess'            => "'''ასე მიშულირ რეთ {{SITENAME}}-ს მუჭოთ \"\$1\".'''",
'nouserspecified'         => 'საჭირო რე მახვარებელიშ სახელიშ მიშაჭარუა.',
'mailmypassword'          => 'ახალ პაროლიშ მოჯღონა',
'noemail'                 => 'აქ "$1" მახვარებელიშ ელ. ფოსტაშ მისამართ წურაფილ ვარე.',
'loginlanguagelabel'      => 'ნინა: $1',

# Edit page toolbar
'bold_sample'     => 'რუმე ტექსტ',
'bold_tip'        => 'რუმე ტექსტ',
'italic_sample'   => 'კურსივ',
'italic_tip'      => 'კურსივ',
'link_sample'     => 'რსხილიშ სახელ',
'link_tip'        => 'დინახალენ რსხილ',
'extlink_sample'  => 'http://www.example.com რსხილიშ სათაურ',
'extlink_tip'     => 'გალე რსხილ (ქორშუდათი http:// პრეფიქს)',
'headline_sample' => 'სათაურიშ ტექსტ',
'headline_tip'    => 'ქვესათაურ',
'math_sample'     => 'ქინახუნეთ ფორმულა თაქ',
'math_tip'        => 'მათემატიკურ ფორმულა (LaTeX)',
'nowiki_sample'   => 'ქინახუნეთ უგუფორმატებუ ტექსტ თაქ',
'nowiki_tip'      => 'ვიკიშ ფორმატირებიშ იგნორირება',
'image_tip'       => 'დინოხუნაფილ სურათ',
'media_tip'       => 'რსხილ ფაილს',
'sig_tip'         => 'თქვან ხემოჭარა დო დრო',
'hr_tip'          => 'ჰორიზონტალურ ხაზ (ნუ გამოიყენებთ ხშირას)',

# Edit pages
'summary'                => 'რეზიუმე',
'subject'                => 'თემა/სახელ',
'minoredit'              => 'ჭიჭე რედაქტირაფა',
'watchthis'              => 'თე ხასილაშ კონტროლ',
'savearticle'            => 'ჩუალა',
'preview'                => 'ოწოხიან გინოჯინა',
'showpreview'            => 'ოწოხიან გინოჯინა',
'showdiff'               => 'თირაფეფიშ ძირაფა',
'anoneditwarning'        => "'''გათხილება:''' თქვა ვარეთ რეგისტრირებულ. თქვან IP მისამართ დინოჭარილ იჸიი თე ხასილაშ რედაქტირაფაშ ისტორიას.",
'summary-preview'        => 'რეზიუმეშ ოწოხიან გინოჯინა',
'blockedtext'            => "<big>'''თქვან მახვარებუშ ჯოხო ვარა IP მიოწურაფუქ ბლოკირქ იჸუ. '''</big> 

ბლოკირაფა ღოლუ $-ქ. 
სამანჯელო წუმორინაფილქ იჸუ გეჸვენჯიქ: ''$2''.

* ბლოკუაშ დაჭყაფურ: $8
* ბლოკიშ ვადაშ გულა ბორჯი: $6
* ბლოკირქ იჸუ: $7

შეგილებუნა დეკავშირათ $1-ს ვარა ნამთინე შხვა [[{{MediaWiki:Grouppage-sysop}}|ადმინისტრატორს]] ბლოკუაშ კილასხუნალო. 
გეთოლწონით, ნამდა თქვა ვა გუმგარინენა ფუნქცია: ''მახვარებუშა ელ-ფოშტაშ ჯღონუა'', ვა გაფუნა მეღანკილ მოქმენდ ელ-ფოშტაშ მიოწურაფუ თქვან [[Special:Preferences|ანგარიშიშ კონფიგურაციას]], ვარა დობლოკუაშ გეშა თე ფუნქციაშ გუმორინაფაშ ნება მიდაღალირ გაფუნა და. 
თქვან ასეიან IP მიოწურაფუ რე $3, დო ბლოკიშ იდენტიფიკატორ #$5. 
რთხინთ ქიმიოღანკათ თე მუნაჩემეფშე ნამდგაიჸინ(ვარა ჟირხოლო) თქვან კორესპონდენციას.",
'newarticle'             => '(ახალ)',
'newarticletext'         => 'რსხილიშ გეშა თქვა ქომოხვადით ხასილას, ნამუთ დიო ვა რე დორცხუაფილ.
ხასილაშ დარცხუაფალო გემიშეჸონით ინფორმაცია თუდონ ოჭკორიეშა.
(ძირ.[[{{MediaWiki:Helppage}}|მოხვარაშ ხასილა]] გეძინელ ინფორმაციაშო).
თე ხასილას ჩილათირო მოხვადით და, დირთით უკახალე თქვან ბრაუზერიშ ხენწყუალათ.',
'noarticletext'          => 'ასე თე ხასილას ტექსტ ვარე, [[Special:Search/{{PAGENAME}}|მიგორეთ თე ხასილაშ სახელ]] შხვა ხასილებს ვარა [{{fullurl:{{FULLPAGENAME}}|action=edit}} დოჭარით თენა ხასილა].',
'previewnote'            => '<strong> ენა ხვალე ნიმუშიე; თირაფეფ დიო ვა ჩუალე!</strong>',
'editing'                => 'რედაქტირაფა - $1',
'editingsection'         => 'რედაქტირაფა - $1 (სექცია)',
'yourtext'               => 'თქვან ტექსტ',
'copyrightwarning'       => 'თოლჸუჯი ქიმეჩით: ნამდგა ვა რდას თიამიშნაღელი ხასილას {{SITENAME}} $2 ლიცენზიას ათოჸუნს(ძირით $1 დეტალეფშოთ). ვა გოკონა თქვან ნახანდიშ დუდშულო გოფაჩუა დო თიშ უდუნდებელ რედაქტირაფა და, თიწკუმა ვა მიშეჸონათ თინა თაქ.<br />
თქვა ხოლო პიჯალას დუთმოდვანთ, ნამდა თენა თქვან ნაჭარა რე, ვარა გინოღალირ რე ოირკოჩე დომენშე, დო ვარა თიშ მანგურ დუდშულ წყუშე. 
<strong> ვა მიშეღათ ოავტორე უფლებებით თხილერ ნახანდი ავტორიშ ქოჸიაშ უმშო!</strong>',
'templatesused'          => 'თე ხასილას გიმორინაფილ თარგეფ:',
'templatesusedpreview'   => 'თარგეფ, ნამუთ ოწოხიან გინოჯინაშ ხასილას რე გიმორინაფილ',
'template-protected'     => '(თხილერი)',
'template-semiprotected' => '(გვერდო თხილერ)',
'nocreatetext'           => 'თე ხასილას ვა შილებე ახალ ხასილაშ გექიმინუა დორცხუაფილ ვარიაშ გეშა. თქვა შეგილებუნა კინორთა დო გექიმინელ ხასილაშ რედაქტირაფა, ვარა [[სპეციალურ:Userlogin|მიშულა დო ანგარიშიშ დორცხუაფა]]',
'recreate-deleted-warn'  => "'''გართხილება: თქვა კინე ახალშო დუთმარცხუანთ ხასილას, ნამუქუთ ორდო ილასუნ'''

რთხინთ, დეფირქათ, მისაღეფ რე დო ვარ თე ხასილაშ რედაქტირაფაშ გოგინძორაფა.
ინფორმაციაშო თუდოლე მოჩამილიე თე ხასილაშ ლასუაშ ისტორია:",

# History pages
'viewpagelogs'        => 'თე ხასილაშა სარეგისტრაციე ჟურნალეფიშ ძირაფა',
'currentrev'          => 'მიმალ გიშანწყუალა',
'revisionasof'        => '$1-იშ ვერსია',
'revision-info'       => '$1-იშ ვერსია, $2-იშ მიშაღალირ',
'previousrevision'    => '←ჯვეშ ვერსია',
'nextrevision'        => 'უკულ ვერსია→',
'currentrevisionlink' => 'მიმალ ვერსია',
'cur'                 => 'მიმალ',
'last'                => 'ბოლო',
'page_first'          => 'პირველი',
'page_last'           => 'ბოლო',
'histlegend'          => '<div style="font-size:95%">
*ვერსიაშ გოთოლორაფა: ქოგეუნჭირით კორნებულ ვერსიაშ ფარანს თიშ გათოლორაფალო.
*ვერსიეფიშ მეზიმაფა: გაგიშაგორით კორნებულ ვერსიეფ რადიო-ქართეფიშ გეღანკუათ დო გეუნჭირით მეზიმაფაშ კონჭის.
*კუნტარაფეფ: (მიმ.) = შხვანობა მიმალ ვერსიაწკუმა, (ეკ.) = შხვაობა ოწოხოლენ ვერსიაწკუმა, ჭ = ჭიჭე გიშანწყუალეფ.</div>',
'histfirst'           => 'პირველი',
'histlast'            => 'ბოლო',
'historysize'         => '($1 ბაიტ)',
'historyempty'        => '(ცალიერ)',

# Revision feed
'history-feed-title'          => 'რედაქტირებიშ ისტორია',
'history-feed-description'    => 'თენა გვერდიშ რედაქტირებეფიშ ისტორია ვიკის',
'history-feed-item-nocomment' => '$1  $2-ს', # user at time

# Diffs
'history-title'           => '"$1" თირაფეფიშ ისტორია',
'difference'              => '(ვერსიეფიშ დარება)',
'lineno'                  => 'ხაზი $1:',
'compareselectedversions' => 'გიშაგორილ ვერსიეფიშ დარება',
'editundo'                => 'გაუქვება',

# Search results
'noexactmatch' => "'''ხასილა თე სახელით \"\$1\" ვაარსეენს.''' თქვა შეილებნა [[:\$1|თე გვერდიშ ღოლამა]].",
'prevn'        => 'წოხლენ $1',
'nextn'        => 'უკულ $1',
'viewprevnext' => 'ქოძირ  ($1) ($2) ($3).',
'powersearch'  => 'გორუა',

# Preferences page
'preferences'   => 'კონფიგურაცია',
'mypreferences' => 'ჩქიმ კონფიგურაცია',

'grouppage-sysop' => '{{ns:project}}:ადმინისტრატორეფ',

# User rights log
'rightslog' => 'მახვარებელიშ ულაფეფიშ ჟურნალ',

# Recent changes
'nchanges'                       => '$1 თირუა',
'recentchanges'                  => 'ბოლო თირაფეფ',
'recentchanges-feed-description' => 'ვიკიშ უახალაშ თირაფეფიშ თოლჸუჯ თე არხის.',
'rcnote'                         => "თუდოლე ძირით ეკონია {{PLURAL:$1|'''1''' თირაფა|$1 თირაფა}} ბოლო {{PLURAL:$2|დღაშ|'''$2''' დღაშ}} მალობას, $3 რენჯობათ",
'rcnotefrom'                     => 'თუდო მოჸუნაფილიე თირაფეფ, ”’$2””-შე (ძირაფილიე ”’$1”’)',
'rclistfrom'                     => 'ახალ თირაფეფიშ ძირაფა დოჭყაფულ $1-იშე',
'rcshowhideminor'                => 'ჭიჭე რედაქტირებაშ $1',
'rcshowhidebots'                 => 'რობოტეფიშ  $1',
'rcshowhideliu'                  => 'რეგისტრირებულ მახვარებელეფიშ $1',
'rcshowhideanons'                => 'ანონიმურ მახვარებელეფიშ $1',
'rcshowhidepatr'                 => 'გოკონტროლაფირ თირაფეფიშ $1',
'rcshowhidemine'                 => 'ჩქიმ რედაქტირებაშ $1',
'rclinks'                        => 'ბოლო $1 თირუეფიშ ძირაფა უკანასკნელი $2 დღა გარგვალებურს<br />$3',
'diff'                           => 'შხვანერობა',
'hist'                           => 'ისტ.',
'hide'                           => 'ტყობინაფა',
'show'                           => 'ძირაფა',
'minoreditletter'                => 'ჭ',
'newpageletter'                  => 'ა',
'boteditletter'                  => 'რ',

# Recent changes linked
'recentchangeslinked'          => 'აკოხვალამირ თირაფეფ',
'recentchangeslinked-title'    => '"$1"-წკუმა მებუნაფილ თირაფეფი',
'recentchangeslinked-noresult' => 'წურაფილ პერიოდს თირაფეფ აკოხვალამირ ხასილეფს ვა ჸოფერენ.',

# Upload
'upload'        => 'ფაილიშ გეძინა',
'uploadbtn'     => 'ფაილიშ გეთება',
'uploadlogpage' => 'გიმოხარგუაშ რეგისტრაცია',
'uploadedimage' => 'გეთებაა "[[$1]]"',

# Special:ImageList
'imagelist'      => 'სურათეფიშ ერკებულ',
'imagelist_name' => 'სახელ',

# Image description page
'filehist'            => 'ფაილიშ ისტორია',
'filehist-help'       => 'ქოგეუნჭირით ბორჯის, ნამდა ქოძირათ ფაილი თეშ, მუჭოთ თინა თიწკუმა კილეძირედუ.',
'filehist-current'    => 'მიმალ',
'filehist-datetime'   => 'ბორჯი',
'filehist-user'       => 'მახვარებელ',
'filehist-dimensions' => 'განზომილებეფ',
'filehist-filesize'   => 'ფაილიშ ზომა',
'filehist-comment'    => 'კომენტარ',
'imagelinks'          => 'რსხილეფ',
'linkstoimage'        => 'გეჸვენჯი {{PLURAL:$1|ხასილა|ხასილეფ}} მორცხუ თე ფაილს',
'nolinkstoimage'      => 'ვა რე თე ფაილწკუმა მერსხილ ხასილეფ.',
'sharedupload'        => 'თენა ფაილ გეთებულ რე საართო სარგებლობაშოთ დო შილებე თიში გიმორინაფა შხვა პროექტეფს.',
'noimage'             => 'ფაილ თე სახელით ვაარსეენც, თქვან შეგილებუნთ $1.',

# MIME search
'mimesearch' => 'MIME გორუა',

# Random page
'randompage' => 'ნამდგარენ ხასილა',

# Statistics
'statistics' => 'სტატისტიკა',

'withoutinterwiki' => 'ხასილეფ ნინაშ რსხილეფიშ გარეშე',

# Miscellaneous special pages
'nbytes'                  => '$1 ბაიტ',
'nlinks'                  => '$1 რსხილ',
'nmembers'                => '$1 მაკათურ',
'uncategorizedpages'      => 'უკატეგორიე ხასილეფ',
'uncategorizedcategories' => 'კატეგორიეფ კატეგორიეფიშ გარეშე',
'uncategorizedimages'     => 'სურათეფ კატეგორიაშ უმიშო',
'mostlinked'              => 'ხასილეფ, ნამუდგა არძას ბრალ ბუნილეფ უღუნა',
'mostlinkedcategories'    => 'კატეგორიეფ, ნამუდგა არძას ბრალ რსხილეფ უღუნა',
'mostcategories'          => 'სტატიეფ, ნამუდგა არძას ბრალ კატეგორიეფ უღუნა',
'prefixindex'             => 'ოგორალ ზიტყვაშ დაჭყაფურიშ მეჯინათ',
'shortpages'              => 'ჭიჭე ხასილეფ',
'longpages'               => 'გინძე ხასილეფ',
'newpages'                => 'ახალ ხასილეფ',
'ancientpages'            => 'ჯვეშ ხასილეფ',
'move'                    => 'გინოღალა',
'movethispage'            => 'თე გვერდიშ გინოღალა',

# Book sources
'booksources' => 'წინგიშ წყუეფ',

# Special:Log
'specialloguserlabel'  => 'მახვარებელ:',
'speciallogtitlelabel' => 'სათაურ:',
'log'                  => 'ჟურნალეფ',
'all-logs-page'        => 'ირ ჟურნალ',

# Special:AllPages
'allpages'       => 'ირ ხასილა',
'alphaindexline' => '$1-იშე $2-შა',
'nextpage'       => 'უკულ ხასილა ($1)',
'prevpage'       => 'წოხლენ ხასილა ($1)',
'allpagesfrom'   => 'ხასილეფიშ ძირაფა დოჭყაფულ:',
'allarticles'    => 'ირ სტატია',
'allpagessubmit' => 'ძირა',

# Special:Categories
'categories' => 'კატეგორიეფ',

# E-mail user
'emailuser' => 'მიდუჯღონით ელ.ფოშტა ათე მახვარებუს',

# Watchlist
'watchlist'            => 'ჩქიმ კონტროლიშ ხასილეფ',
'mywatchlist'          => 'ჩქიმ კონტროლიშ ერკებულ',
'watchlistfor'         => "('''$1'''-შენი)",
'removedwatchtext'     => 'ასე გვერდ "[[:$1]]" ვარე თქვან კონტროლიშ გვერდეფს.',
'watch'                => 'კონტროლ',
'watchthispage'        => 'თე ხასილაშ კონტროლ',
'unwatch'              => 'კონტროლიშ გაუქვება',
'watchlist-details'    => '$1 ხასილას კილმაჯინეთ სხუნუაშ ხასილაშ უკოროცხუო.',
'wlshowlast'           => 'ძირაფა ბოლო $1 საათიშ $2 დღაშ $3',
'watchlist-hide-bots'  => 'რობოტიშ თირუეფიშ ფულუა',
'watchlist-hide-own'   => 'ჩქიმ რედაქტირებაშ ფულუა',
'watchlist-hide-minor' => 'ჭიჭე რედაქტირებებაშ ფულუა',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'კონტროლირაფონი...',
'unwatching' => 'ვაკონტროლირაფონი...',

# Delete/protect/revert
'deletepage'                  => 'ხასილაშ შალუა',
'actioncomplete'              => 'მოქმედალა რსულებულ რე',
'deletedarticle'              => 'წაშალულ რე "[[$1]]"',
'dellogpage'                  => 'ლასირეფიშ ერკებულ',
'deleteotherreason'           => 'შხვა/დამატებით ბაძაძი:',
'deletereasonotherlist'       => 'შხვა ბაძაძი',
'rollbacklink'                => 'გაუქვება',
'protectlogpage'              => 'თხილუაშ ისტორია',
'protectcomment'              => 'კომენტარ:',
'protectexpiry'               => 'ვადა',
'protect-unchain'             => 'გინოღალას ალობას ბლოკიშ მონწყუმა',
'protect-default'             => '(სტანდარტულ)',
'protect-fallback'            => 'საჭიროე "$1"-იშ ულაფა',
'protect-level-autoconfirmed' => 'ვარეგისტრირებულ მახვარებელეფიშ დაბლოკვა',
'protect-level-sysop'         => 'ხვალე ადმინისტრატორეფ',
'protect-summary-cascade'     => 'იერარქიულ',
'restriction-type'            => 'ულაფა',

# Undelete
'undeletebtn'            => 'ახალშო ეკონწყუალა',
'undelete-search-submit' => 'გორუა',

# Namespace form on various pages
'namespace'      => 'ჯოხოეფიშ ოფირჩა:',
'invert'         => 'არძა, გიშაგორილი გარდა',
'blanknamespace' => '(დუდ)',

# Contributions
'contributions' => 'მახვარებელიშ ნახანდ',
'mycontris'     => 'ჩქიმ ნახანდ',
'contribsub2'   => '$1 ($2) შენი',
'uctop'         => '(დუდ)',
'month'         => 'თუთა:',
'year'          => 'წანა:',

'sp-contributions-newbies-sub' => 'ახალეფშოთ',
'sp-contributions-blocklog'    => 'ბლოკირებაშ ისტორია',

# What links here
'whatlinkshere'       => 'სო რე თენა ხასილა წურაფილ',
'whatlinkshere-title' => 'ხასილეფ, სოდეთ რენა რსხილეფ $1-ს',
'whatlinkshere-page'  => 'გვერდ:',
'linklistsub'         => '(რსხილეფ)',
'linkshere'           => "გეჸვენჯ ხასილეფს ოხოლუ რსხილეფ '''[[:$1]]'''-შენ",
'nolinkshere'         => "'''[[:$1]]''', თე ხასილას ვარე რსხილ.",
'isredirect'          => 'გინოწურაფაშ ხასილა',
'istemplate'          => 'ჩართება',
'whatlinkshere-prev'  => '{{PLURAL:$1|წოხოლენ|წოხოლენ $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|უკულიან|უკულიან $1}}',
'whatlinkshere-links' => '← რსხილეფ',

# Block/unblock
'blockip'            => 'მახვარებელიშ ვარა IP მისამართიშ ბლოკირება',
'ipboptions'         => '2 საათი:2 hours,1 დღა:1 დღა,3 დღა:3 დღალეფ,1 მარა:1 week,2 მარა:2 weeks,1 თუთა:1 month,3 თუთა:3 months,6 თუთა:6 months,1 წანა:1 year,განუსაზღვრელი ვადით:infinite', # display1:time1,display2:time2,...
'ipbotheroption'     => 'შხვა',
'ipblocklist-submit' => 'გორუა',
'blocklink'          => 'ბლოკირაფა',
'unblocklink'        => 'ბლოკიშ მონწყუმა',
'contribslink'       => 'ნახანდ',
'blocklogpage'       => 'ბლოკირეფიშ ერკება',
'blocklogentry'      => '"[[$1]]" ბლოკირაფაშ ვადაშ გულა ბორჯი $2 $3.',

# Move page
'move-page-legend' => 'გვერდიშ გინოღალა',
'movearticle'      => 'ხასილაშ გინოღალა',
'newtitle'         => 'ახალ სათაურ',
'move-watch'       => 'თე ხასილაშ კონტროლ',
'movepagebtn'      => 'ხასილაშ გინოღალა',
'pagemovedsub'     => 'გინოღალა რსულებულ რე',
'movepage-moved'   => '<big>\'\'\'"$1" გინოღალულ რე "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'movedto'          => 'გინაღალულ რე',
'movetalk'         => 'აკოხვალამირ სხუნუაშ გინოღალა',
'1movedto2'        => '[[$1]] გინოღალირიე ხასილაშა [[$2]]',
'movelogpage'      => 'გინოღალაშ რეგისტრაცია',
'movereason'       => 'საბაბი',
'revertmove'       => 'გაუქვება',

# Export
'export' => 'ხასილეფიშ ექსპორტ',

# Namespace 8 related
'allmessages' => 'ირ სისტემურ შეტყვინაფა',

# Thumbnails
'thumbnail-more'  => 'მორდი',
'thumbnail_error' => 'ესკიზიშ ქიმინუაშ ჩილათა: $1',

# Import log
'importlogpage' => 'იმპორტიშ ჟურნალ',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'ჩქიმ ხასილა',
'tooltip-pt-mytalk'               => 'ჩქიმ სხუნუაშ გვერდ',
'tooltip-pt-preferences'          => 'ჩქიმ კონფიგურაცია',
'tooltip-pt-watchlist'            => 'ხასილეფიშ ერკებულ, ნამუშ თირაფას თქვა ითოლორუანთ',
'tooltip-pt-mycontris'            => 'ირ ჩქიმ ნახანდ',
'tooltip-pt-login'                => 'ჯგირ იჸიი თქვან რეგისტრაცია, მორო აუცილებელი ვარე.',
'tooltip-pt-logout'               => 'გუმულა',
'tooltip-ca-talk'                 => 'შინაარსიშ ხასილაშ სხუნუა',
'tooltip-ca-edit'                 => 'თქვა შეილებნა თე ხასილაშ რედაქტირება. რთხიინთ გეუნჭირით ოწმახ რწყებაშ ღილაკიშ გვერდიშ შენახაშა.',
'tooltip-ca-addsection'           => 'ქოგეუძინით კომენტარ თე სხუნუას.',
'tooltip-ca-viewsource'           => 'ხასილა თხილერ რე, შეგილებუნა ძირათ თიშ წყუ.',
'tooltip-ca-protect'              => 'ხასილაშ თხილუა',
'tooltip-ca-delete'               => 'თე ხასილაშ შალუა',
'tooltip-ca-move'                 => 'თე ხასილაშ გინოღალა',
'tooltip-ca-watch'                => 'თე ხასილაშ მინოთება თქვან კონტროლირაფონ ხასილეფს',
'tooltip-ca-unwatch'              => 'მონწყუმეთ თენა ხასილა თქვან კონტროლიშ ხასილეფიშე',
'tooltip-search'                  => 'გორუა {{SITENAME}}',
'tooltip-n-mainpage'              => 'დუდ ხასილაშ ძირაფა',
'tooltip-n-portal'                => 'პროექტიშენი, მუშ ქიმინუა შეილებჷნა, სოდეთ ძირჷნთ',
'tooltip-n-currentevents'         => 'ქიჩინით რსული ინფორმაცია ასეიან მოლინეფიშენ',
'tooltip-n-recentchanges'         => 'ვიკიშ ეკონია თირაფეფ',
'tooltip-n-randompage'            => 'ქუმოძირ ნებისმიერ ხასილა',
'tooltip-n-help'                  => 'ხასილა, სოდეთ ძირჷნთ.',
'tooltip-t-whatlinkshere'         => 'არძო ვიკი ხასილაშ სია ნამუდგა მიკოკირილ რე თაქ.',
'tooltip-t-contributions'         => 'ქოძირეთ თე მახვარებელიშ ნახანდ',
'tooltip-t-emailuser'             => 'მიდუჯღონით ელ.ფოშტა ათე მახვარებუს',
'tooltip-t-upload'                => 'ქიმკაკირ ფაილ',
'tooltip-t-specialpages'          => 'ირ სპეციალურ ხასილა',
'tooltip-ca-nstab-user'           => 'მახვარებელიშ ხასილაშ ძირაფა',
'tooltip-ca-nstab-project'        => 'პროექტიშ ხასილა',
'tooltip-ca-nstab-image'          => 'სურათიშ ხასილაშ ძირაფა',
'tooltip-ca-nstab-template'       => 'თარგიშ ძირაფა',
'tooltip-ca-nstab-help'           => 'ქოძირეთ დახვარებაშ გვერდ',
'tooltip-ca-nstab-category'       => 'ხასილაშ კატეგორიაშ ძირა',
'tooltip-minoredit'               => 'ქიმიოღანკი, მუჭოთ ჭიჭე რედაქტირაფა [alt-i]',
'tooltip-save'                    => 'თირაფაშ ჩუალა',
'tooltip-preview'                 => 'ოწმახ გეგნაჯინ თირაფეფის, რთხიინთ, იხვარით თენა ჩუალაშახ! [alt-p]',
'tooltip-diff'                    => 'ტექსტს არსებულ თირაფეფიშ ძირა. [alt-v]',
'tooltip-compareselectedversions' => 'ქოძირეთ თე ხასილაშ ჟირ გიშაგორილ ვერსიაშ შხვანერობა.',
'tooltip-watch'                   => 'თე ხასილაშ გეძინა თქვან ოკონტროლე ერკებულშა [alt-w]',

# Browsing diffs
'previousdiff' => '← ჯვეშ თირაფაშა',
'nextdiff'     => 'უკულ თირაფა →',

# Media information
'file-info-size'       => '($1 × $2 პიქსელ, ფაილიშ ზომა: $3, MIME ტიპ: $4)',
'file-nohires'         => '<small> უმოს მაღალ გიშაგორადალა ვა რე შელებუან.</small>',
'svg-long-desc'        => '(ფაილ SVG, ნომინალურო $1×$2 პიქსელ, ფაილიშ ზიმა: $3)',
'show-big-image'       => 'რსული გარჩევადობა',
'show-big-image-thumb' => '<small>ზომა ოწმახ ძირაშ დროს: $1 × $2 პიქსელ</small>',

# Special:NewImages
'newimages' => 'ახალ სურათეფ',

# Bad image list
'bad_image_list' => 'ფორმატ რე უკულიანიშნერო:

ხვალე გიშნაგორეფს (ბჭკარეფ ნამდგა იჭყაფუ *) მიკიჯინუ.
ბწკარიშ პირლველ რსხილ ოკო რდას რსხილ გლახა ფალიშა.
ნებისმიერ უკულიან რსხილეფ კინ თი ბწკარს გენიხილებ მუჭოთ გამონაკლის, მუდგა ნიშნენს ნამუდა გვერდეფ სოდგა ფაილეფ შილებე რდას ინლაინს.',

# Metadata
'metadata'          => 'მეტამონაცემეფ',
'metadata-help'     => 'თე ფაილს ოხოლუ გეძინელ ინფორმაცია, საეგებიოთ ციფრულ კამერაშე ვარა სკანერშე, ნამუთ თიშ ოქიმინალო გიმირინეს. ფაილიშ ორიგინალ თირელ ქორენ და, შილებე კანკალე დეტალ ვა გიშაძირუანდას ფაილშა მიშაღალირ თირაფეფს.',
'metadata-expand'   => 'დეტალეფიშ ძირაფა რსულო',
'metadata-collapse' => 'დეტალეფიშ ტყობინაფა',

# External editor support
'edit-externally'      => 'თე ფაილიშ რედაქტირაფა ბორჯის გიმირინეთ გალენ პროგრამა.',
'edit-externally-help' => 'რსულ ინფორმაციაშენ ქოძირეთ [http://www.mediawiki.org/wiki/Manual:External_editors ჩადგმიშ ინსტრუქციეფ].',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'არძა',
'namespacesall' => 'არძა',
'monthsall'     => 'არძა',

# Watchlist editing tools
'watchlisttools-view' => 'მერსხილ თირაფეფიშ ძირაფა',
'watchlisttools-edit' => 'კონტროლიშ ხასილეფიშ ძირაფა დო რედაქტირება',
'watchlisttools-raw'  => 'კონტროლიშ ერკებულიშ რედაქტირაფა ტექსტიშ ფორმატის',

# Special:Version
'version' => 'ვერსია', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'სპეციალურ ხასილეფ',

);
