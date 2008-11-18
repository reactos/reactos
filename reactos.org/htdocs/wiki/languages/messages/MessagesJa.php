<?php
/** Japanese (日本語)
 *
 * @ingroup Language
 * @file
 *
 * @author Alexsh
 * @author Aotake
 * @author Broad-Sky
 * @author Emk
 * @author Fievarsty
 * @author Hatukanezumi
 * @author Iwai.masaharu
 * @author JtFuruhata
 * @author Kahusi
 * @author Kkkdc
 * @author Marine-Blue
 * @author Muttley
 * @author Mzm5zbC3
 * @author Suisui
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard' => "標準",
	'nostalgia' => "ノスタルジア",
	'cologneblue' => "ケルンブルー",
);

$datePreferences = array(
	'default',
	'ISO 8601',
);

$defaultDateFormat = 'ja';

$dateFormats = array(
	'ja time' => 'H:i',
	'ja date' => 'Y年n月j日 (D)',
	'ja both' => 'Y年n月j日 (D) H:i',
);

$namespaceNames = array(
	NS_MEDIA          => "Media", /* Media */
	NS_SPECIAL        => "特別", /* Special */
	NS_MAIN           => "",
	NS_TALK           => "ノート", /* Talk */
	NS_USER           => "利用者", /* User */
	NS_USER_TALK      => "利用者‐会話", /* User_talk */
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1‐ノート', /* Wikipedia_talk */
	NS_IMAGE          => "画像", /* Image */
	NS_IMAGE_TALK     => "画像‐ノート", /* Image_talk */
	NS_MEDIAWIKI      => "MediaWiki", /* MediaWiki */
	NS_MEDIAWIKI_TALK => "MediaWiki‐ノート", /* MediaWiki_talk */
	NS_TEMPLATE       => "Template", /* Template */
	NS_TEMPLATE_TALK  => "Template‐ノート", /* Template_talk */
	NS_HELP           => "Help", /* Help */
	NS_HELP_TALK      => "Help‐ノート", /* Help_talk */
	NS_CATEGORY       => "Category", /* Category */
	NS_CATEGORY_TALK  => "Category‐ノート" /* Category_talk */
);

$messages = array(
# User preference toggles
'tog-underline'               => 'リンクの下線:',
'tog-highlightbroken'         => '未作成のページへのリンクをハイライトする',
'tog-justify'                 => '段落を均等割り付けする',
'tog-hideminor'               => '最近更新したページから細部の編集を隠す',
'tog-extendwatchlist'         => 'ウォッチリストを拡張する',
'tog-usenewrc'                => '最近更新したページを拡張する（ブラウザによっては使えないことがあります）',
'tog-numberheadings'          => '見出しに番号を振る',
'tog-showtoolbar'             => '編集ボタンを表示する',
'tog-editondblclick'          => 'ダブルクリックで編集する (JavaScript)',
'tog-editsection'             => 'セクション編集用リンクを有効にする',
'tog-editsectiononrightclick' => 'セクションタイトルの右クリックでセクション編集を行えるようにする (JavaScript)',
'tog-showtoc'                 => '目次を表示する (4つ以上の見出しがあるページ)',
'tog-rememberpassword'        => 'セッションを越えてパスワードを記憶する',
'tog-editwidth'               => 'テキストボックスを横幅いっぱいに表示する',
'tog-watchcreations'          => '自分で作成したページをウォッチリストに追加する',
'tog-watchdefault'            => '編集したページをウォッチリストに追加する',
'tog-watchmoves'              => '自分が移動したページをウォッチリストに追加する',
'tog-watchdeletion'           => '自分が削除したページをウォッチリストに追加する',
'tog-minordefault'            => '細部の編集をデフォルトでチェックする',
'tog-previewontop'            => 'プレビューをテキストボックスの前に配置する',
'tog-previewonfirst'          => '編集開始時にもプレビューを表示する',
'tog-nocache'                 => 'ページをキャッシュしない',
'tog-enotifwatchlistpages'    => 'ウォッチリストにあるページが更新されたときにメールを受け取る',
'tog-enotifusertalkpages'     => '自分の会話ページが更新されたときにメールを受け取る',
'tog-enotifminoredits'        => '細部の編集でもメールを受け取る',
'tog-enotifrevealaddr'        => 'あなた以外に送られる通知メールにあなたのメールアドレスを記載する',
'tog-shownumberswatching'     => 'ページをウォッチしている利用者数を表示する',
'tog-fancysig'                => '署名に利用者ページへの自動的なリンクを付けない（このチェックを付ける場合でも利用者ページへのリンクを外さないようにしましょう）',
'tog-externaleditor'          => '編集に外部アプリケーションを使う',
'tog-externaldiff'            => '差分表示に外部アプリケーションを使う',
'tog-showjumplinks'           => 'アクセシビリティのための "{{int:jumpto}}" リンクを有効にする',
'tog-uselivepreview'          => 'ライブプレビューを使用する (JavaScript, 試験中の機能)',
'tog-forceeditsummary'        => '要約欄が空欄の場合に警告する',
'tog-watchlisthideown'        => '自分の編集を表示しない',
'tog-watchlisthidebots'       => 'ボットによる編集を表示しない',
'tog-watchlisthideminor'      => '細部の編集を表示しない',
'tog-nolangconversion'        => '字形変換を無効にする',
'tog-ccmeonemails'            => '他の利用者に送信したメールの控えを自分にも送る',
'tog-diffonly'                => '差分表示の下に記事本文を表示しない',
'tog-showhiddencats'          => '隠しカテゴリを表示する',

'underline-always'  => '常に付ける',
'underline-never'   => '常に付けない',
'underline-default' => 'WWWブラウザ既定',

'skinpreview' => '（プレビュー）',

# Dates
'sunday'        => '日曜日',
'monday'        => '月曜日',
'tuesday'       => '火曜日',
'wednesday'     => '水曜日',
'thursday'      => '木曜日',
'friday'        => '金曜日',
'saturday'      => '土曜日',
'sun'           => '日',
'mon'           => '月',
'tue'           => '火',
'wed'           => '水',
'thu'           => '木',
'fri'           => '金',
'sat'           => '土',
'january'       => '1月',
'february'      => '2月',
'march'         => '3月',
'april'         => '4月',
'may_long'      => '5月',
'june'          => '6月',
'july'          => '7月',
'august'        => '8月',
'september'     => '9月',
'october'       => '10月',
'november'      => '11月',
'december'      => '12月',
'january-gen'   => '1月',
'february-gen'  => '2月',
'march-gen'     => '3月',
'april-gen'     => '4月',
'may-gen'       => '5月',
'june-gen'      => '6月',
'july-gen'      => '7月',
'august-gen'    => '8月',
'september-gen' => '9月',
'october-gen'   => '10月',
'november-gen'  => '11月',
'december-gen'  => '12月',
'jan'           => '1月',
'feb'           => '2月',
'mar'           => '3月',
'apr'           => '4月',
'may'           => '5月',
'jun'           => '6月',
'jul'           => '7月',
'aug'           => '8月',
'sep'           => '9月',
'oct'           => '10月',
'nov'           => '11月',
'dec'           => '12月',

# Categories related messages
'pagecategories'                 => 'カテゴリ',
'category_header'                => 'カテゴリ “$1” にあるページ',
'subcategories'                  => 'サブカテゴリ',
'category-media-header'          => 'カテゴリ “$1” にあるメディア',
'category-empty'                 => 'このカテゴリにはページまたはメディアがひとつもありません。',
'hidden-categories'              => '隠しカテゴリ',
'hidden-category-category'       => '隠しカテゴリ', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|以下にこのカテゴリへ所属するサブカテゴリ 1 件を表示しています。|以下にこのカテゴリへ所属するサブカテゴリ $2 件中 $1 件を表示しています。}}',
'category-subcat-count-limited'  => '以下にこのカテゴリへ所属するサブカテゴリ $1 件を表示しています。',
'category-article-count'         => '以下にこのカテゴリへ所属している記事 $2 件中 $1 件を表示しています。',
'category-article-count-limited' => '以下にこのカテゴリへ所属している記事 $1 件を表示しています。',
'category-file-count'            => '以下にこのカテゴリへ所属しているファイル $2 個中 $1 個を表示しています。',
'category-file-count-limited'    => '以下にこのカテゴリへ所属するファイル $1 個を表示しています。',
'listingcontinuesabbrev'         => 'の続き',

'mainpagetext'      => 'MediaWikiが正常にインストールされました。',
'mainpagedocfooter' => '使い方・設定に関しては[http://meta.wikimedia.org/wiki/%E3%83%98%E3%83%AB%E3%83%97:%E7%9B%AE%E6%AC%A1 ユーザーズガイド]を参照してください。

== はじめましょう ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings/ja 設定リスト]
* [http://www.mediawiki.org/wiki/Manual:FAQ/ja MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki リリース情報メーリングリスト]',

'about'          => '解説',
'article'        => '本文',
'newwindow'      => '（新しいウィンドウが開きます）',
'cancel'         => '中止',
'qbfind'         => '検索',
'qbbrowse'       => '閲覧',
'qbedit'         => '編集',
'qbpageoptions'  => '個人用ツール',
'qbpageinfo'     => 'ページ情報',
'qbmyoptions'    => 'オプション',
'qbspecialpages' => '特別ページ',
'moredotdotdot'  => 'すべて表示する',
'mypage'         => 'マイ・ページ',
'mytalk'         => 'マイ・トーク',
'anontalk'       => 'このIP利用者の会話',
'navigation'     => 'ナビゲーション',
'and'            => 'および',

# Metadata in edit box
'metadata_help' => 'メタデータ:',

'errorpagetitle'    => 'エラー',
'returnto'          => '$1 に戻る。',
'tagline'           => '出典: {{SITENAME}}',
'help'              => 'ヘルプ',
'search'            => '検索',
'searchbutton'      => '検索',
'go'                => '表示',
'searcharticle'     => '表示',
'history'           => '履歴',
'history_short'     => '履歴',
'updatedmarker'     => '最後の訪問から更新されています',
'info_short'        => 'ページ情報',
'printableversion'  => '印刷用バージョン',
'permalink'         => 'この版への固定リンク',
'print'             => '印刷',
'edit'              => '編集',
'create'            => '作成',
'editthispage'      => 'このページを編集',
'create-this-page'  => 'このページを作成する',
'delete'            => '削除',
'deletethispage'    => 'このページを削除',
'undelete_short'    => '削除済$1版',
'protect'           => '保護',
'protect_change'    => '保護・解除',
'protectthispage'   => 'このページを保護',
'unprotect'         => '保護解除',
'unprotectthispage' => 'ページ保護解除',
'newpage'           => '新規ページ',
'talkpage'          => 'このページのノート',
'talkpagelinktext'  => '会話',
'specialpage'       => '特別ページ',
'personaltools'     => '個人用ツール',
'postcomment'       => '新規にコメントを投稿',
'articlepage'       => '項目を表示',
'talk'              => 'ノート',
'views'             => '表示',
'toolbox'           => 'ツールボックス',
'userpage'          => '利用者ページを表示',
'projectpage'       => 'プロジェクトページを表示',
'imagepage'         => '画像のページを表示',
'mediawikipage'     => 'メッセージページを表示',
'templatepage'      => 'テンプレートページを表示',
'viewhelppage'      => 'ヘルプページを表示',
'categorypage'      => 'カテゴリページを表示',
'viewtalkpage'      => 'ノートを表示',
'otherlanguages'    => '他の言語',
'redirectedfrom'    => '（$1 から転送）',
'redirectpagesub'   => 'リダイレクトページ',
'lastmodifiedat'    => '最終更新 $1 $2', # $1 date, $2 time
'viewcount'         => 'このページは $1 回アクセスされました。',
'protectedpage'     => '保護されたページ',
'jumpto'            => '移動:',
'jumptonavigation'  => 'ナビゲーション',
'jumptosearch'      => '検索',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}}について',
'aboutpage'            => 'Project:{{SITENAME}}について',
'bugreports'           => 'バグの報告',
'bugreportspage'       => 'Project:バグの報告',
'copyright'            => 'コンテンツは$1のライセンスで利用することができます。',
'copyrightpagename'    => '{{SITENAME}}の著作権',
'copyrightpage'        => '{{ns:project}}:著作権',
'currentevents'        => '最近の出来事',
'currentevents-url'    => 'Project:最近の出来事',
'disclaimers'          => '免責事項',
'disclaimerpage'       => 'Project:免責事項',
'edithelp'             => '編集の仕方',
'edithelppage'         => 'Help:編集の仕方',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:目次',
'mainpage'             => 'メインページ',
'mainpage-description' => 'メインページ',
'policy-url'           => 'Project:方針',
'portal'               => 'コミュニティ・ポータル',
'portal-url'           => 'Project:コミュニティ・ポータル',
'privacy'              => 'プライバシー・ポリシー',
'privacypage'          => 'Project:プライバシー・ポリシー',

'badaccess'        => '権限がありません',
'badaccess-group0' => 'あなたはこの処理を行う権限を持っていません。',
'badaccess-group1' => 'この処理は $1 の権限を持った利用者のみが実行できます。',
'badaccess-group2' => 'この処理は $1 のうちどちらかの権限を持った利用者のみが実行できます。',
'badaccess-groups' => 'この処理は $1 のうちいずれかの権限を持った利用者のみが実行できます。',

'versionrequired'     => 'MediaWiki バージョン $1 が必要',
'versionrequiredtext' => 'このページの利用には MediaWiki バージョン $1 が必要です。[[Special:Version|バージョン情報]]を確認してください。',

'ok'                      => 'OK',
'retrievedfrom'           => ' "$1" より作成',
'youhavenewmessages'      => 'あなた宛の$1が届いています。（$2）',
'newmessageslink'         => '新しいメッセージ',
'newmessagesdifflink'     => '差分',
'youhavenewmessagesmulti' => '$1 に新しいメッセージが届いています',
'editsection'             => '編集',
'editold'                 => '編集',
'viewsourceold'           => 'ソースを表示',
'editsectionhint'         => '節を編集: $1',
'toc'                     => '目次',
'showtoc'                 => '表示',
'hidetoc'                 => '非表示',
'thisisdeleted'           => '$1 を参照または復帰する。',
'viewdeleted'             => '$1の削除記録と履歴を確認する',
'restorelink'             => '削除された $1 編集',
'feedlinks'               => 'フィード:',
'feed-invalid'            => 'フィード形式の指定が間違っています。',
'feed-unavailable'        => '{{SITENAME}} は、フィードの配信に対応していません。',
'site-rss-feed'           => '$1 をRSSフィード',
'site-atom-feed'          => '$1 をAtomフィード',
'page-rss-feed'           => '"$1" をRSSフィード',
'page-atom-feed'          => '"$1" をAtomフィード',
'red-link-title'          => '$1 （未作成ページ）',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '本文',
'nstab-user'      => '利用者ページ',
'nstab-media'     => 'メディア',
'nstab-special'   => '特別ページ',
'nstab-project'   => '解説',
'nstab-image'     => '画像',
'nstab-mediawiki' => 'メッセージ',
'nstab-template'  => 'テンプレート',
'nstab-help'      => 'ヘルプ',
'nstab-category'  => 'カテゴリ',

# Main script and global functions
'nosuchaction'      => 'そのような動作はありません',
'nosuchactiontext'  => 'このURIで指定された動作は{{SITENAME}}で認識できません。',
'nosuchspecialpage' => 'そのような特別ページはありません',
'nospecialpagetext' => '要求された特別ページは存在しません。有効な特別ページの一覧は[[Special:SpecialPages]]にあります。',

# General errors
'error'                => 'エラー',
'databaseerror'        => 'データベース・エラー',
'dberrortext'          => 'データベース検索の文法エラー。これは恐らくソフトウェアのバグを表しています。

最後に実行を試みた問い合わせ:
<blockquote><tt>$1</tt></blockquote>

from within function "<tt>$2</tt>".
MySQL returned error "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'データベースクエリの文法エラーが発生しました。
----
A database query syntax error has occurred.
The last attempted database query was:
"$1"
from within function "$2".
MySQL returned error "$3: $4"',
'noconnect'            => '申し訳ありません。技術的な問題が発生しており、データベースサーバーに接続できません。<br />$1',
'nodb'                 => 'データベース $1 を選択できません。',
'cachederror'          => 'あなたがアクセスしたページのコピーを保存したものを表示しています。また、コピーは更新されません。',
'laggedslavemode'      => '警告: ページに最新の編集が反映されていない可能性があります。反映されるまでしばらくお待ちください。',
'readonly'             => 'データベースはロックされています',
'enterlockreason'      => 'ロックする理由を入力してください。ロックが解除されるのがいつになるかの見積もりについても述べてください。',
'readonlytext'         => 'データベースは現在、新しいページの追加や編集を受け付けない「ロック状態」になっています。これはおそらく定期的なメンテナンスのためで、メンテナンス終了後は正常な状態に復帰します。データベースをロックしたサーバー管理者は次のような説明をしています:

$1',
'missing-article'      => '「$1」 $2 というページのテキストをデータベース上に見つけることができませんでした。

削除された版のページへの古い差分表示や固定リンクをたどった時にこのようなことになります。

それ以外の操作でこのメッセージが表示された場合、ソフトウェアのバグの可能性があります。
[[Special:ListUsers/sysop|管理者]]までURL の情報を添えてお知らせください。',
'missingarticle-rev'   => '(版#: $1)',
'missingarticle-diff'  => '(差分: $1, $2)',
'readonly_lag'         => 'データベースはスレーブ・サーバーがマスタ・サーバーに同期するまで自動的にロックされています。しばらくお待ちください。',
'internalerror'        => '内部処理エラー',
'internalerror_info'   => '内部エラー: $1',
'filecopyerror'        => 'ファイル "$1" から "$2" へのコピーに失敗しました。',
'filerenameerror'      => 'ファイル名を "$1" から "$2" へ変更できませんでした。',
'filedeleteerror'      => 'ファイル "$1" の削除に失敗しました。',
'directorycreateerror' => 'ディレクトリ "$1" を作成できません。',
'filenotfound'         => 'ファイル "$1" が見つかりません。',
'fileexistserror'      => '$1 への書き込みができません: ファイルが存在します',
'unexpected'           => '値が異常です: $1 = "$2"',
'formerror'            => 'エラー: フォームの送信に失敗しました。',
'badarticleerror'      => 'このページでは要求された処理を行えません。',
'cannotdelete'         => '指定されたページ、または画像の削除に失敗しました。',
'badtitle'             => 'ページタイトルの間違い',
'badtitletext'         => '要求されたページは無効か、何もないか、他言語または他ウィキ内のタイトルを誤って指定しています。タイトルに利用できない文字が一つ以上含まれている場合もあります。',
'perfdisabled'         => 'この機能はデータベースの負荷を軽くするために現在使えなくなっています。',
'perfcached'           => '以下のデータはキャッシュであり、しばらく更新されていません。',
'perfcachedts'         => '以下のデータは $1 に最終更新されたキャッシュです。',
'querypage-no-updates' => 'ページの更新は無効になっています。以下のデータの更新は現在行われていません。',
'wrong_wfQuery_params' => 'wfQuery()へ誤った引数が渡されました。<br />
関数: $1<br />
クエリ: $2',
'viewsource'           => 'ソースを表示',
'viewsourcefor'        => '$1 のソース',
'actionthrottled'      => '操作規制',
'actionthrottledtext'  => 'あなたが大量の操作を通常考えられないほど短い間隔で行ったため、スパム防止の観点から実行を制限しました。少し時間をおいてからもう一度操作してください。',
'protectedpagetext'    => 'このページは編集できないように保護されています。',
'viewsourcetext'       => '以下にソースを表示しています:',
'protectedinterface'   => 'このページはソフトウェアのインターフェースに使用されるテキストが保存されており、問題回避のために保護されています。',
'editinginterface'     => "'''警告:''' あなたはソフトウェアのインターフェースに使用されているテキストを編集しています。このページの変更はすべての利用者に影響します。翻訳をする場合、MediaWikiの多言語対応プロジェクトである[http://translatewiki.net/wiki/Main_Page?setlang=ja Betawiki]の利用を検討してください。",
'sqlhidden'            => '（SQLクエリ非表示）',
'cascadeprotected'     => 'このページはカスケード保護されている以下のページから呼び出されているため、編集できないように保護されています。
$2',
'namespaceprotected'   => "'''$1''' 名前空間に属するページを編集する権限がありません。",
'customcssjsprotected' => 'このページは利用者の環境設定を記録するページです。利用者本人以外は編集できません。',
'ns-specialprotected'  => '{{ns:special}}名前空間内にあるページは編集できません。',
'titleprotected'       => "このページは [[User:$1|$1]] によりページの作成が保護されています。
保護の理由は次の通りです。: ''$2''",

# Virus scanner
'virus-badscanner'     => '環境設定が不適合です: 不明なウイルス検知ソフト: <i>$1</i>',
'virus-scanfailed'     => 'スキャンに失敗しました (コード $1)',
'virus-unknownscanner' => '不明なウイルス駆除プログラム:',

# Login and logout pages
'logouttitle'                => 'ログアウト',
'logouttext'                 => '<p><strong>ログアウトしました。</strong>このまま{{SITENAME}}を匿名で使い続けることができます。もう一度ログインして元の、あるいは別の利用者として使うこともできます。</p>
<p>※いくつかのページはブラウザのキャッシュをクリアするまでログインしているかのように表示されることがあります。</p>',
'welcomecreation'            => '== $1 さん、ようこそ！ ==
あなたのアカウントができました。
お好みに合わせて[[Special:Preferences|オプション（{{SITENAME}}）]]を変更することができます。',
'loginpagetitle'             => 'ログイン',
'yourname'                   => '利用者名',
'yourpassword'               => 'パスワード',
'yourpasswordagain'          => 'パスワード再入力',
'remembermypassword'         => 'セッションを越えてパスワードを記憶する',
'yourdomainname'             => 'あなたのドメイン',
'externaldberror'            => '外部の認証データベースでエラーが発生たか、または外部アカウント情報の更新が許可されていません。',
'loginproblem'               => '<b>ログインでエラーが発生しました。</b><br />再度実行してください。',
'login'                      => 'ログイン',
'nav-login-createaccount'    => 'ログインまたはアカウント作成',
'loginprompt'                => '{{SITENAME}}にログインするにはクッキーを有効にする必要があります。',
'userlogin'                  => 'ログインまたはアカウント作成',
'logout'                     => 'ログアウト',
'userlogout'                 => 'ログアウト',
'notloggedin'                => 'ログインしていません',
'nologin'                    => 'アカウントをお持ちではありませんか? $1',
'nologinlink'                => 'アカウントを作成',
'createaccount'              => 'アカウント作成',
'gotaccount'                 => 'すでにアカウントをお持ちの場合: $1',
'gotaccountlink'             => 'ログイン',
'createaccountmail'          => 'メールで送信',
'badretype'                  => '両方のパスワードが一致しません。',
'userexists'                 => '入力された利用者名はすでに使われています。ほかの名前をお選びください。',
'youremail'                  => 'メールアドレス*:',
'username'                   => '利用者名:',
'uid'                        => '利用者ID:',
'prefs-memberingroups'       => 'グループ:',
'yourrealname'               => '本名*:',
'yourlanguage'               => '使用言語:',
'yourvariant'                => '字体変換',
'yournick'                   => 'ニックネーム（署名用）:',
'badsig'                     => '署名が正しくありません。HTMLタグを見直してください。',
'badsiglength'               => '署名が長すぎます。$1文字以下である必要があります。',
'email'                      => 'メールアドレス',
'prefs-help-realname'        => '* 本名 (任意): 本名を入力すると、ページ・クレジットに利用者名（アカウント名）の代わりに本名が表示されます。',
'loginerror'                 => 'ログイン失敗',
'prefs-help-email'           => 'メールアドレスの設定は任意ですが、設定しておけばパスワードを忘れた際に新しいパスワードを電子メールで受け取ることができます。
また、他の利用者からのウィキメールを受け取ることができるようになります。この時点ではあなたのメールアドレスはその利用者に知られることはありません。ただし、あなたから送信すれば、あなたのメールアドレスは先方に通知されます。',
'prefs-help-email-required'  => 'メールアドレスが必要です。',
'nocookiesnew'               => '利用者のアカウントは作成されましたが、ログインしていません。{{SITENAME}}ではログインにクッキーを使います。あなたはクッキーを無効な設定にしているようです。クッキーを有効にしてから作成した利用者名とパスワードでログインしてください。',
'nocookieslogin'             => '{{SITENAME}}ではログインにクッキーを使います。あなたはクッキーを無効な設定にしているようです。クッキーを有効にして、もう一度試してください。',
'noname'                     => '利用者名を正しく指定していません。',
'loginsuccesstitle'          => 'ログイン成功',
'loginsuccess'               => "'''{{SITENAME}} に \"\$1\" としてログインしました。'''",
'nosuchuser'                 => '"$1"という利用者は見当たりません。綴りが正しいことを再度確認するか、[[Special:Userlogin/signup|アカウントを作成してください]]。',
'nosuchusershort'            => '"<nowiki>$1</nowiki>" という利用者は見当たりません。綴りが正しいことを再度確認してください。',
'nouserspecified'            => '利用者名を指定してください。',
'wrongpassword'              => 'パスワードが間違っています。再度入力してください。',
'wrongpasswordempty'         => 'パスワードを空にすることはできません。再度入力してください。',
'passwordtooshort'           => 'パスワードが短すぎます。$1文字以上の文字列にしてください。',
'mailmypassword'             => '新しいパスワードをメールで送る',
'passwordremindertitle'      => '{{SITENAME}} パスワード再送通知',
'passwordremindertext'       => 'どなたか（$1のIPアドレスの使用者）が{{SITENAME}} ($4) のログイン用パスワードの再発行を依頼しました。利用者"$2"の仮パスワードは "$3" です。もしあなたがパスワードの発行を依頼したのであれば、ログインして別のパスワードに変更してください。

パスワード再発行の依頼に覚えがない、またはログイン用パスワードを思い出されパスワード変更の必要がないのであるならば、このメッセージは無視してください。引き続き以前のパスワードを使用し続けることができます。',
'noemail'                    => '利用者 "$1" のメールアドレスは登録されていません。',
'passwordsent'               => '新しいパスワードを "$1" さんの登録済みメールアドレスに送信しました。メールを受け取ったら、再度ログインしてください。',
'blocked-mailpassword'       => 'あなたの使用しているIPアドレスからの編集はブロックされています。悪用防止のため、パスワードの再発行は無効化されています。',
'eauthentsent'               => '指定されたメールアドレスにアドレス確認のためのメールを送信しました。このアカウントが本当にあなたのものであるか確認するため、あなたがメールの内容に従わない限り、その他のメールはこのアカウント宛には送信されません。',
'throttled-mailpassword'     => '新しいパスワードは $1 時間以内に送信済みです。悪用防止のため、パスワードは $1 時間間隔で再発行可能となります。',
'mailerror'                  => 'メールの送信中にエラーが発生しました: $1',
'acct_creation_throttle_hit' => 'あなたは既に $1 アカウントを作成しています。これ以上作成できません。',
'emailauthenticated'         => 'あなたのメールアドレスは $1 に確認されています。',
'emailnotauthenticated'      => 'あなたのメールアドレスは<strong>確認されていません</strong>。確認されるまで以下のいかなるメールも送られません。',
'noemailprefs'               => '<strong>これらの機能を有効にするにはメールアドレスを登録してください。</strong>',
'emailconfirmlink'           => 'メールアドレスを確認する',
'invalidemailaddress'        => '入力されたメールアドレスが正しい形式に従っていないため、受け付けられません。正しい形式で入力し直すか、メールアドレス欄を空にしてください。',
'accountcreated'             => 'アカウントを作成しました',
'accountcreatedtext'         => '利用者 $1 が作成されました。',
'createaccount-title'        => '{{SITENAME}}のアカウント作成',
'createaccount-text'         => 'この電子メールアドレスを連絡先として、{{SITENAME}} ($4) にアカウントが作成されました。

　利用者名： "$2"

パスワード： "$3"


今すぐログインし、パスワードを変更してください。

何かの手違いでアカウントが作成されたと思う場合、このメッセージは無視してください。',
'loginlanguagelabel'         => '言語: $1',

# Password reset dialog
'resetpass'               => 'パスワードの再設定',
'resetpass_announce'      => 'メールで送信した臨時パスワードでログインしています。ログインを完了するには、新しいパスワードを設定しなおす必要があります。',
'resetpass_text'          => '<!-- ここにテキストを挿入 -->',
'resetpass_header'        => 'パスワードを設定しなおす',
'resetpass_submit'        => '再設定してログイン',
'resetpass_success'       => 'あなたのパスワードは変更されました。ログインしています...',
'resetpass_bad_temporary' => '無効な臨時パスワードです。パスワードは既に再設定されているか、再びパスワード通知メールが送信されています。',
'resetpass_forbidden'     => 'このウィキでは、パスワードの変更は許可されていません。',
'resetpass_missing'       => 'データがセットされていません。',

# Edit page toolbar
'bold_sample'     => '強い強調（太字）',
'bold_tip'        => '強い強調（太字）',
'italic_sample'   => '弱い強調（斜体）',
'italic_tip'      => '弱い強調（斜体）',
'link_sample'     => '項目名',
'link_tip'        => '内部リンク',
'extlink_sample'  => 'http://www.example.com リンクのタイトル',
'extlink_tip'     => '外部リンク（http:// を忘れずにつけてください）',
'headline_sample' => '見出し',
'headline_tip'    => '標準の見出し',
'math_sample'     => '\\int f(x)dx',
'math_tip'        => '数式 (LaTeX)',
'nowiki_sample'   => 'そのまま表示させたい文字を入力',
'nowiki_tip'      => '入力文字をそのまま表示',
'image_tip'       => '画像の埋め込み',
'media_sample'    => 'Example.mp3',
'media_tip'       => 'メディアファイル（音声）へのリンク',
'sig_tip'         => '時刻つきの署名',
'hr_tip'          => '水平線（利用は控えめに）',

# Edit pages
'summary'                          => '編集内容の要約',
'subject'                          => '題名・見出し',
'minoredit'                        => 'これは細部の編集です',
'watchthis'                        => 'ウォッチリストに追加',
'savearticle'                      => '保存する',
'preview'                          => 'プレビュー',
'showpreview'                      => 'プレビューを実行',
'showlivepreview'                  => 'ライブプレビュー',
'showdiff'                         => '差分を表示',
'anoneditwarning'                  => 'あなたはログインしていません。このまま投稿を行った場合あなたのIPアドレスはこの項目の履歴に記録されます。',
'missingsummary'                   => "'''注意:''' 要約欄が空欄です。投稿ボタンをもう一度押すと、要約なしのまま投稿されます。",
'missingcommenttext'               => '以下にコメントを入力してください。',
'missingcommentheader'             => "'''注意:''' 題名・見出しが空欄です。投稿ボタンをもう一度押すと、要約なしのまま投稿されます。",
'summary-preview'                  => '要約のプレビュー',
'subject-preview'                  => '題名・見出しのプレビュー',
'blockedtitle'                     => '投稿ブロック',
'blockedtext'                      => 'ご使用の利用者名またはIPアドレスは $1 によって投稿をブロックされています。ブロックの理由は $2 です。

*ブロック開始時期: $8
*ブロック解除予定: $6
*ブロック対象: $7

$1 または他の[[{{MediaWiki:Grouppage-sysop}}|管理者]]にこの件についてメールで問い合わせることができます。ただし、[[Special:Preferences|オプション]]に正しいメールアドレスが登録されていない場合、メール送信機能が使えないことに注意してください。

あなたのIPアドレスは「$3」、ブロックIDは &#x23;$5 です。問い合わせを行う際には、この情報を必ず書いてください。',
'autoblockedtext'                  => 'ご利用のIPアドレスは $1 によって投稿をブロックされた利用者によって使用されたために自動的にブロックされています。理由は次の通りです。

:$2

* ブロックの開始: $8
* ブロック解除予定: $6
* 意図されているブロック対象者: $7

$1 または他の[[{{MediaWiki:Grouppage-sysop}}|管理者]]にこの件について問い合わせることができます。

ただし、[[Special:Preferences|オプション]]に正しいメールアドレスが登録されていない場合、またはメール送信がブロックされている場合、メール送信機能が使えないことに注意してください。

あなたの現在のIPアドレスは $3 、ブロックIDは &#x23;$5 です。問い合わせを行う際には、この情報を必ず書いてください。',
'blockednoreason'                  => '理由が設定されていません',
'blockedoriginalsource'            => "以下に '''$1''' のソースを示します:",
'blockededitsource'                => "'''$1''' への '''あなたの編集''' を以下に示します:",
'whitelistedittitle'               => '編集にはログインが必要',
'whitelistedittext'                => 'このページを編集するには $1 する必要があります。',
'confirmedittitle'                 => '編集にはメールアドレスの確認が必要です。',
'confirmedittext'                  => 'ページの編集を始める前にメールアドレスの確認をする必要があります。[[Special:Preferences|オプション]]でメールアドレスを設定し、確認を行ってください。',
'nosuchsectiontitle'               => 'セクションが存在しません',
'nosuchsectiontext'                => '指定されたセクションはありません。セクション $1 はありませんでしたので、セクション編集は無効となります。編集内容は保存されません。',
'loginreqtitle'                    => 'ログインが必要',
'loginreqlink'                     => 'ログイン',
'loginreqpagetext'                 => '他のページを閲覧するには$1する必要があります。',
'accmailtitle'                     => 'パスワードを送信しました',
'accmailtext'                      => '"$1" のパスワードを $2 に送信しました。',
'newarticle'                       => '（新規）',
'newarticletext'                   => 'ページを新規に作成するには新しい内容を書き込んでください。',
'anontalkpagetext'                 => "----
''これはアカウントをまだ作成していないか、あるいは使っていない匿名利用者のための会話ページです。匿名利用者の識別は利用者名のかわりにIPアドレスを用います。IPアドレスは何人かで共有されることがあります。もしあなたが匿名利用者で無関係なコメントが寄せられているとお考えの場合は、[[Special:UserLogin/signup|アカウントを作成する]]か[[Special:UserLogin|ログインして]]他の匿名利用者と間違えられないようにしてくださるようお願いします。''",
'noarticletext'                    => '現在このページには内容がありません。他のページから[[Special:Search/{{PAGENAME}}|このページタイトルを検索する]]か、[{{fullurl:{{FULLPAGENAME}}|action=edit}} このページを編集]できます。',
'userpage-userdoesnotexist'        => '"$1" という名前のアカウントは登録されていません。このページを編集することが適切かどうか確認してください。',
'clearyourcache'                   => "'''お知らせ:''' 保存した後、ブラウザのキャッシュをクリアする必要があります。
* '''Mozilla / Firefox / Safari:''' [Shift] を押しながら [再読み込み] をクリック、または [Ctrl]-[F5] か [Ctrl]-[R] （Macintoshでは [Cmd]-[Shift]-[R]）
* '''IE:''' [Ctrl] を押しながら [更新] をクリック、または [Ctrl]-[F5]
* '''Konqueror:''' [再読み込み] をクリック、または [F5]
* '''Opera:''' 「ツール」→「設定」からキャッシュをクリア。",
'usercssjsyoucanpreview'           => '<strong>ヒント:</strong> 「{{int:showpreview}}」ボタンを使うと保存前に新しいスタイルシート・スクリプトをテストできます。',
'usercsspreview'                   => "'''あなたはユーザースタイルシートをプレビューしています。まだ保存されていないので注意してください。'''",
'userjspreview'                    => "'''あなたはユーザースクリプトをテスト・プレビューしています。まだ保存されていないので注意してください。'''",
'userinvalidcssjstitle'            => "'''警告:''' \"\$1\" という外装はありません。.css と .js ページを編集する際にはタイトルを小文字にすることを忘れないでください。例えば {{ns:user}}:Hoge/Monobook.css ではなく {{ns:user}}:Hoge/monobook.css となります。",
'updated'                          => '（更新）',
'note'                             => '<strong>お知らせ:</strong>',
'previewnote'                      => '<strong>これはプレビューです。
まだ保存されていません!</strong>',
'previewconflict'                  => 'このプレビューは、上の文章編集エリアの文章を保存した場合にどう見えるようになるかを示すものです。',
'session_fail_preview'             => '<strong>セッションが切断されたため編集を保存できません。もう一度やりなおしてください。それでも失敗する場合、ログアウトしてからログインし直してください。</strong>',
'session_fail_preview_html'        => "<strong>申し訳ありません。セッションが切断されたため編集を保存することができませんでした。</strong>

''{{SITENAME}}ではHTMLタグの使用に制限を設けておらず、JavaScript でのアタックを予防するためにプレビューを隠しています。''

<strong>この編集が問題ないものであるならば再度保存してください。それでもうまくいかない際には一度[[Special:UserLogout|ログアウト]]して、もう一度ログインしてみてください。</strong>",
'token_suffix_mismatch'            => '<strong>あなたの使用しているクライアントが、エディット・トークン内の句読点を正しく処理していないことを確認しました。
このページの文章が破損するのを防ぐため、あなたの編集は反映されません。
問題のある匿名プロキシサービスを利用していると、この問題が起こることがあります。</strong>',
'editing'                          => '$1 を編集中',
'editingsection'                   => '$1 を編集中（節単位編集）',
'editingcomment'                   => '$1 を編集中（新規コメント）',
'editconflict'                     => '編集競合: $1',
'explainconflict'                  => 'あなたがこのページを編集し始めた後に、他の誰かがこのページを変更しました。上側のテキストエリアは現在の最新の状態です。あなたの編集していた文章は下側のテキストエリアに示されています。編集していた文章を、上側のテキストエリアの文章に組み込んでください。<strong>上側のテキストエリアの内容だけ</strong>が、「{{int:Savearticle}}」をクリックした時に実際に保存されます。',
'yourtext'                         => 'あなたの文章',
'storedversion'                    => '保存された版',
'nonunicodebrowser'                => '<strong>警告: あなたの使用しているブラウザはUnicode互換ではありません。項目を編集する前にブラウザを変更してください。</strong>',
'editingold'                       => '<strong>警告: あなたはこのページの古い版を編集しています。もしこの文章を保存すると、この版以降に追加された全ての変更が無効になってしまいます。</strong>',
'yourdiff'                         => 'あなたの更新内容',
'copyrightwarning'                 => "'''■投稿する前に以下を確認してください■'''
* {{SITENAME}}に投稿された文書は、すべて$2（詳細は$1を参照）によって公開されることに同意してください。
* あなたの文章が他人によって自由に編集、配布されることを望まない場合は、投稿を控えてください。
* あなたの投稿する文章はあなた自身によって書かれたものであるか、パブリック・ドメインかそれに類する自由なリソースからの複製であることを約束してください。'''あなたが著作権を保持していない作品を許諾なしに投稿してはいけません!'''",
'copyrightwarning2'                => "'''■投稿する前に以下を確認してください■'''
* あなたの文章が他人によって自由に編集、配布されることを望まない場合は、投稿を控えてください。
* あなたの投稿する文章はあなた自身によって書かれたものであるか、パブリック・ドメインかそれに類する自由なリソースからの複製であることを約束してください（詳細は$1を参照）。'''あなたが著作権を保持していない作品を許諾なしに投稿してはいけません!'''",
'longpagewarning'                  => "'''警告:''' このページのサイズは $1 キロバイトです。一部の古いブラウザでは 32 キロバイト以上のページを編集すると問題が起きるものがあります。ページを節に分けることを検討してください。",
'longpageerror'                    => '<strong>エラー: あなたが投稿したテキストは $1 キロバイトの長さがあります。これは投稿できる最大の長さである $2 キロバイトを超えています。この編集を保存することはできません。</strong>',
'readonlywarning'                  => '<strong>警告: データベースがメンテナンスのためにロックされています。現在は編集結果を保存できません。文章をカットアンドペーストしてローカルファイルとして保存し、後ほど保存をやり直してください。</strong>',
'protectedpagewarning'             => "'''警告:''' このページは保護されています。{{int:group-sysop}}しか編集できません。",
'semiprotectedpagewarning'         => "'''お知らせ:''' このページは登録利用者のみが編集できるよう保護されています。",
'cascadeprotectedwarning'          => "'''警告:''' このページはカスケード保護されている以下のページから呼び出されているため、{{int:group-sysop}}しか編集できません。",
'titleprotectedwarning'            => '<strong>警告:  このページはロックされているため、特定の利用者以外は作成することができません。</strong>',
'templatesused'                    => 'このページで使われているテンプレート:',
'templatesusedpreview'             => 'このプレビューで使われているテンプレート:',
'templatesusedsection'             => 'この節で使われているテンプレート:',
'template-protected'               => '（保護）',
'template-semiprotected'           => '（半保護）',
'hiddencategories'                 => 'このページは$1個の隠しカテゴリに属しています:',
'edittools'                        => '<!-- ここに書いたテキストは編集及びアップロードのフォームの下に表示されます。 -->',
'nocreatetitle'                    => 'ページを作成できません',
'nocreatetext'                     => '{{SITENAME}} ではページの新規作成を制限しています。元のページに戻って既存のページを編集するか、[[Special:UserLogin|ログインまたはアカウントを作成]]してください。',
'nocreate-loggedin'                => '{{SITENAME}}で新しいページを作成する権限がありません。',
'permissionserrors'                => '認証エラー',
'permissionserrorstext'            => 'あなたにはこのページの編集権限がありません。理由は以下の通りです:',
'permissionserrorstext-withaction' => '以下に示された理由により $2 を行うことができません:',
'recreate-deleted-warn'            => "'''警告:あなたは以前に削除されたページを再作成しようとしています。'''

このページの編集が適切であるかどうか確認してください。参考として以下にこのページの削除記録を表示しています:",

# Parser/template warnings
'expensive-parserfunction-warning'        => '警告: このページは条件文関数の呼び出し負荷が高過ぎます。

現在は $1 です。$2 より低い必要があります。',
'expensive-parserfunction-category'       => '条件文関数の呼び出しが高負荷となっているページ',
'post-expand-template-inclusion-warning'  => '警告: テンプレートのデータサイズが大き過ぎます。いくつかのテンプレートが含まれません。',
'post-expand-template-inclusion-category' => 'テンプレート読み込みサイズが制限値を越えているページ',
'post-expand-template-argument-warning'   => '警告：このページには展開後のサイズが大きすぎるテンプレートが1つ以上含まれています。これらのテンプレートは展開されませんでした。',
'post-expand-template-argument-category'  => '展開されなかったテンプレートを含むページ',

# "Undo" feature
'undo-success' => '編集の取り消しに成功しました。保存ボタンを押すと変更が確定されます。',
'undo-failure' => '中間の版での編集と競合したため、自動取り消しできませんでした。',
'undo-norev'   => '取り消そうとした編集は存在しないかすでに削除されたために取り消せませんでした。',
'undo-summary' => '[[Special:Contributions/$2|$2]] ([[User talk:$2|会話]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]]) による $1 版の編集を取り消し',

# Account creation failure
'cantcreateaccounttitle' => 'アカウントを作成できません',
'cantcreateaccount-text' => "以下の理由により、このIPアドレス ('''$1''') からのアカウント作成が [[User:$3|$3]] によってブロックされています。

ブロック理由: ''$2''",

# History pages
'viewpagelogs'        => 'このページに関するログを見る',
'nohistory'           => 'このページには変更履歴がありません。',
'revnotfound'         => '要求された版が見つかりません。',
'revnotfoundtext'     => '要求されたこのページの旧版は見つかりませんでした。このページにアクセスしたURLをもう一度確認してください。',
'currentrev'          => '最新版',
'revisionasof'        => '$1の版',
'revision-info'       => '$1; $2 による版',
'previousrevision'    => '←前の版',
'nextrevision'        => '次の版→',
'currentrevisionlink' => '最新版を表示',
'cur'                 => '最新版',
'next'                => '次の版',
'last'                => '前の版',
'page_first'          => '先頭',
'page_last'           => '末尾',
'histlegend'          => '凡例:（最新版）= 最新版との比較、（前の版）= 直前の版との比較、<strong>{{int:minoreditletter}}</strong> = 細部の編集',
'deletedrev'          => '[削除済み]',
'histfirst'           => '最古',
'histlast'            => '最新',
'historysize'         => '（$1 バイト）',
'historyempty'        => '（空です）',

# Revision feed
'history-feed-title'          => '変更履歴',
'history-feed-description'    => 'このウィキのこのページに関する変更履歴',
'history-feed-item-nocomment' => '$2 における $1 による編集', # user at time
'history-feed-empty'          => '要求したページは存在しません。既に削除されたか移動された可能性があります。 [[Special:Search|このウィキの検索]]で関連する新しいページを探してみてください。',

# Revision deletion
'rev-deleted-comment'         => '（要約は削除されています）',
'rev-deleted-user'            => '（投稿者名は削除されています）',
'rev-deleted-event'           => '（記事は削除されています）',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
この版は公の履歴から削除されました。[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 削除記録]におそらくログがあります。</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
この版は公の履歴から削除されており、{{SITENAME}}の{{int:group-sysop}}だけが内容を見ることができます。削除の詳細は[{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} 削除記録]を参照してください。
</div>',
'rev-delundel'                => '復帰/削除',
'revisiondelete'              => '版の削除と復帰',
'revdelete-nooldid-title'     => '対象版がありません',
'revdelete-nooldid-text'      => '操作の完了に必要な版が指定されていません。',
'revdelete-selected'          => "'''$1'''の、以下の選択された版に対する操作:",
'logdelete-selected'          => "'''$1'''の選択されたログに対する操作:",
'revdelete-text'              => '版の削除ではページの履歴にその版は表示されます。しかしその版に含まれるテキストにはアクセスできなくなります。

サーバー管理者にこれ以上の制限をかけられない限り、他の{{int:group-sysop}}は隠れた版を読んだり、元に戻したりできます。',
'revdelete-legend'            => '版の削除の適用',
'revdelete-hide-text'         => '版のテキストを隠す',
'revdelete-hide-name'         => '操作および対象を隠す',
'revdelete-hide-comment'      => '編集の要約を隠す',
'revdelete-hide-user'         => '版の投稿者を隠す',
'revdelete-hide-restricted'   => 'これらの制限を管理者にも適用する',
'revdelete-suppress'          => 'データを{{int:group-sysop}}からも隠す',
'revdelete-hide-image'        => 'ファイル内容を隠す',
'revdelete-unsuppress'        => '復帰版に対する制限を外す',
'revdelete-log'               => '要約:',
'revdelete-submit'            => '隠蔽の設定を適用',
'revdelete-logentry'          => '[[$1]]の版の削除情報を操作しました',
'logdelete-logentry'          => '[[$1]]の版の操作情報を変更しました',
'revdelete-success'           => '版の隠蔽状態を変更しました',
'logdelete-success'           => '操作情報の隠蔽状態を変更しました',
'revdel-restore'              => '表示内容の変更',
'pagehist'                    => 'ページの履歴',
'deletedhist'                 => '削除された履歴',
'revdelete-content'           => '本文',
'revdelete-summary'           => '編集内容の要約',
'revdelete-uname'             => '利用者名',
'revdelete-restricted'        => '管理者に対して制限を実施',
'revdelete-unrestricted'      => '管理者に対する制限を解除',
'revdelete-hid'               => '$1 を隠す',
'revdelete-unhid'             => '$1 を表示する',
'revdelete-log-message'       => '$2版に対して$1',
'logdelete-log-message'       => '$2の操作に対して$1',

# Suppression log
'suppressionlog'     => '秘匿記録',
'suppressionlogtext' => '以下は管理者から秘匿された内容を含む削除およびブロック記録です。
現在操作できるブロックについては[[Special:IPBlockList|投稿ブロック中の利用者やIPアドレス]]を参照してください。',

# History merging
'mergehistory'                     => 'ページの統合履歴',
'mergehistory-header'              => 'ページの履歴を他のページの履歴と統合しようとしています。
この変更によりページの履歴は連続したひとつの履歴とものになることを確認してください。',
'mergehistory-box'                 => '統合する2ページの過去の版:',
'mergehistory-from'                => '統合元となるページ:',
'mergehistory-into'                => '統合先のページ:',
'mergehistory-list'                => '編集履歴を統合できます',
'mergehistory-merge'               => '[[:$2]] へ統合可能な [[:$1]] の履歴を以下に表示しています。ラジオボタンで版を選択してから統合操作を行うと指定した時間までの版が自動的に統合されます。また、ナビゲーションリンクを使うことでフォームをリセットできます。',
'mergehistory-go'                  => '統合可能な版の表示',
'mergehistory-submit'              => '統合する版',
'mergehistory-empty'               => '統合できる過去の版がありません.',
'mergehistory-success'             => '[[:$1]] の過去版 $3 を [[:$2]] へ統合する作業に成功しました。',
'mergehistory-fail'                => '履歴の統合を行うことが出来ません。統合を行うページと過去の版を再確認してください。',
'mergehistory-no-source'           => '統合元となるページ $1 が存在しません。',
'mergehistory-no-destination'      => '統合先のページ $1 が存在しません。',
'mergehistory-invalid-source'      => '統合元となるページの正確なタイトルを指定してください。',
'mergehistory-invalid-destination' => '統合先のページの正確なタイトルを指定してください。',
'mergehistory-autocomment'         => '[[:$1]]を[[:$2]]に統合',
'mergehistory-comment'             => '[[:$1]]を[[:$2]]に統合: $3',

# Merge log
'mergelog'           => '統合記録',
'pagemerge-logentry' => '[[$1]]を[[$2]]へ統合（$3版へ）',
'revertmerge'        => '統合取り消し',
'mergelogpagetext'   => '以下に履歴統合の記録を示しています。',

# Diffs
'history-title'           => '$1 の変更履歴',
'difference'              => '版間での差分',
'lineno'                  => '$1 行',
'compareselectedversions' => '選択した版同士を比較',
'editundo'                => '取り消し',
'diff-multi'              => '（間の $1 版分が非表示です）',

# Search results
'searchresults'             => '検索結果',
'searchresulttext'          => '{{SITENAME}}の検索に関する詳しい情報は、[[{{MediaWiki:Helppage}}|{{int:help}}]]をご覧ください。',
'searchsubtitle'            => '検索語: \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|"$1" から始まるページ]] | [[Special:WhatLinksHere/$1|"$1" のリンク元]])',
'searchsubtitleinvalid'     => "検索語: '''$1'''",
'noexactmatch'              => '"$1" というタイトルのページは存在しませんでした。[[:$1|新規作成する]]。',
'noexactmatch-nocreate'     => "''\"\$1\"'' というタイトルのページは存在しませんでした。",
'toomanymatches'            => '一致したページが多すぎます、他の検索語を指定してください',
'titlematches'              => 'ページタイトルと一致',
'notitlematches'            => 'ページタイトルとは一致しませんでした',
'textmatches'               => 'ページ内本文と一致',
'notextmatches'             => 'ページ内本文とは一致しませんでした',
'prevn'                     => '前 $1',
'nextn'                     => '次 $1',
'viewprevnext'              => '（$1）（$2）（$3）を見る',
'search-result-size'        => '$1 ({{PLURAL:$2|1語|$2語}})',
'search-result-score'       => '関連度: $1%',
'search-redirect'           => '($1 のリダイレクト)',
'search-section'            => '(節 $1)',
'search-suggest'            => 'もしかして: $1',
'search-interwiki-caption'  => '姉妹プロジェクト',
'search-interwiki-default'  => '$1の結果:',
'search-interwiki-more'     => '(つづき)',
'search-mwsuggest-enabled'  => '検索候補を表示',
'search-mwsuggest-disabled' => '検索候補を表示しない',
'search-relatedarticle'     => '関連',
'mwsuggest-disable'         => 'AJAX によるサジェストを無効にする',
'searchrelated'             => '関連',
'searchall'                 => 'all',
'showingresults'            => '<b>$2</b> 件目から <b>$1</b> 件を表示しています。',
'showingresultsnum'         => '<b>$2</b> 件目から <b>$3</b> 件を表示しています。',
'showingresultstotal'       => "'''$3''' 件中 {{PLURAL:$3|'''$1''|'''$1 - $2'''}} 件目の検索結果を表示",
'nonefound'                 => "'''※'''検索がうまくいかないのは、「ある」や「から」のような一般的な語で索引付けがされていないか、複数の検索語を指定している（全ての検索語を含むページだけが結果に示されます）などのためかもしれません。",
'powersearch'               => '検索',
'powersearch-legend'        => '高度な検索',
'powersearch-ns'            => '名前空間を指定して検索:',
'powersearch-redir'         => 'リダイレクトの一覧',
'powersearch-field'         => '検索キーワード:',
'search-external'           => '外部検索',
'searchdisabled'            => '<p>全文検索はサーバー負荷の都合から、一時的に使用停止しています。元に戻るまでGoogleでの全文検索を利用してください。検索結果は少し古い内容となります。</p>',

# Preferences page
'preferences'              => 'オプション',
'mypreferences'            => 'オプション',
'prefs-edits'              => '編集回数:',
'prefsnologin'             => 'ログインしていません',
'prefsnologintext'         => 'オプションを変更するためには<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} ログイン]</span>している必要があります。',
'prefsreset'               => 'オプションは初期化されました。',
'qbsettings'               => 'クイックバー設定',
'qbsettings-none'          => 'なし',
'qbsettings-fixedleft'     => '左端',
'qbsettings-fixedright'    => '右端',
'qbsettings-floatingleft'  => 'ウィンドウの左上に固定',
'qbsettings-floatingright' => 'ウィンドウの右上に固定',
'changepassword'           => 'パスワード変更',
'skin'                     => '外装',
'math'                     => '数式',
'dateformat'               => '日付の書式',
'datedefault'              => '選択なし',
'datetime'                 => '日付と時刻',
'math_failure'             => '構文解析失敗',
'math_unknown_error'       => '不明なエラー',
'math_unknown_function'    => '不明な関数',
'math_lexing_error'        => '字句解析エラー',
'math_syntax_error'        => '構文エラー',
'math_image_error'         => 'PNGへの変換に失敗しました。latex, dvips, gs, convertが正しくインストールされているか確認してください。',
'math_bad_tmpdir'          => 'TeX一時ディレクトリを作成または書き込みできません',
'math_bad_output'          => 'TeX出力用ディレクトリを作成または書き込みできません',
'math_notexvc'             => 'texvcプログラムが見つかりません。math/READMEを読んで正しく設定してください。',
'prefs-personal'           => '利用者情報',
'prefs-rc'                 => '最近更新したページ',
'prefs-watchlist'          => 'ウォッチリスト',
'prefs-watchlist-days'     => 'ウォッチリストに表示する日数:',
'prefs-watchlist-edits'    => '拡張したウォッチリストに表示する件数:',
'prefs-misc'               => 'その他',
'saveprefs'                => '設定の保存',
'resetprefs'               => '設定の初期化',
'oldpassword'              => '古いパスワード:',
'newpassword'              => '新しいパスワード:',
'retypenew'                => '新しいパスワードを再入力:',
'textboxsize'              => '編集画面',
'rows'                     => '縦:',
'columns'                  => '横:',
'searchresultshead'        => '検索',
'resultsperpage'           => '1ページあたりの表示件数:',
'contextlines'             => '1件あたりの行数:',
'contextchars'             => '1行あたりの文字数:',
'stub-threshold'           => '<a href="#" class="stub">スタブリンク</a>として表示するしきい値:',
'recentchangesdays'        => '最近更新したページの表示日数:',
'recentchangescount'       => '最近更新したページの表示件数:',
'savedprefs'               => 'オプションを保存しました',
'timezonelegend'           => 'タイムゾーン',
'timezonetext'             => 'UTCとあなたの地域の標準時間との差を入力してください（日本国内は9:00）。',
'localtime'                => 'あなたの現在時刻',
'timezoneoffset'           => '時差¹',
'servertime'               => 'サーバーの現在時刻',
'guesstimezone'            => '自動設定',
'allowemail'               => '他の利用者からのメールの受け取りを許可する',
'prefs-searchoptions'      => '検索のオプション',
'prefs-namespaces'         => '名前空間',
'defaultns'                => '標準で検索する名前空間:',
'default'                  => 'デフォルト',
'files'                    => '画像等',

# User rights
'userrights'                  => '利用者権限の管理', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => '利用者の所属グループの管理',
'userrights-user-editname'    => '利用者名:',
'editusergroup'               => '編集',
'editinguser'                 => "利用者 [[User:$1|$1]]'''（[[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]）の権限を編集中",
'userrights-editusergroup'    => '利用者の所属グループ',
'saveusergroups'              => '利用者の所属グループを保存',
'userrights-groupsmember'     => '所属グループ:',
'userrights-groups-help'      => 'この利用者が属するグループを変更することができます。
* ボックスのチェックはこの利用者がそのグループに属していることを意味します。
* チェックが入っていないボックスはこの利用者がそのグループに属していないことを意味します。
* * は一旦グループへ登録または登録解除した場合、その決定を変更できないことを意味します。',
'userrights-reason'           => '変更理由:',
'userrights-no-interwiki'     => '他ウィキ上における利用者情報の編集権限はありません。',
'userrights-nodatabase'       => 'データベース $1は存在しないか、ローカル上にありません。',
'userrights-nologin'          => '利用者情報を変更するには[[Special:UserLogin|ログイン]]する必要があります。',
'userrights-notallowed'       => '利用者情報を変更する権限がありません。',
'userrights-changeable-col'   => '変更可能なグループ',
'userrights-unchangeable-col' => '変更できないグループ',

# Groups
'group'               => 'グループ:',
'group-user'          => 'ログイン利用者',
'group-autoconfirmed' => '自動承認された利用者',
'group-bot'           => 'ボット',
'group-sysop'         => '管理者',
'group-bureaucrat'    => 'ビューロクラット',
'group-suppress'      => 'Oversights',
'group-all'           => '（すべて）',

'group-user-member'          => '利用者',
'group-autoconfirmed-member' => '{{int:group-autoconfirmed}}',
'group-bot-member'           => '{{int:group-bot}}',
'group-sysop-member'         => '{{int:group-sysop}}',
'group-bureaucrat-member'    => '{{int:group-bureaucrat}}',
'group-suppress-member'      => 'Oversight',

'grouppage-user'          => '{{ns:project}}:利用者',
'grouppage-autoconfirmed' => '{{ns:project}}:自動承認された利用者',
'grouppage-bot'           => '{{ns:project}}:ボット',
'grouppage-sysop'         => '{{ns:project}}:管理者',
'grouppage-bureaucrat'    => '{{ns:project}}:ビューロクラット',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'ページの閲覧',
'right-edit'                 => 'ページの編集',
'right-createpage'           => 'ページの作成（ノートページ以外）',
'right-createtalk'           => 'ノートページの作成',
'right-createaccount'        => '新しい利用者アカウントの作成',
'right-minoredit'            => '編集を細部の編集としてマークする',
'right-move'                 => 'ページの移動',
'right-move-subpages'        => 'サブページを含めたページの移動',
'right-suppressredirect'     => 'ページの移動の際にもとのページ名からのリダイレクトを作成しない',
'right-upload'               => 'ファイルのアップロード',
'right-reupload'             => '存在するファイルの上書き',
'right-reupload-own'         => '自らがアップロードした存在するファイルの上書き',
'right-reupload-shared'      => 'メディア共有レポジトリのファイルをローカルで上書きする',
'right-upload_by_url'        => 'URL アドレスからファイルをアップロードする',
'right-purge'                => '確認画面を表示せずにキャッシュの破棄を行う',
'right-autoconfirmed'        => '半保護されたページの編集',
'right-bot'                  => '自動的に行われた作業として認識させる',
'right-nominornewtalk'       => 'ノートページへ細部の編集が行われた際に新しいメッセージのお知らせを表示しない',
'right-apihighlimits'        => 'APIクエリの制限値を上げる',
'right-writeapi'             => '編集APIの使用',
'right-delete'               => 'ページの削除',
'right-bigdelete'            => '履歴の大きなページを削除する',
'right-deleterevision'       => 'ページの特定の版を削除・復帰',
'right-deletedhistory'       => '削除された版の履歴を本文なしで閲覧する',
'right-browsearchive'        => '削除されたページを検索',
'right-undelete'             => 'ページの復帰',
'right-suppressrevision'     => '管理者から秘匿された版を閲覧・復帰',
'right-suppressionlog'       => 'プライベートログを閲覧',
'right-block'                => '他利用者の投稿ブロック',
'right-blockemail'           => '電子メール送信のブロック',
'right-hideuser'             => '利用者名をブロックし、一般の閲覧から秘匿する',
'right-ipblock-exempt'       => 'IPブロック、自動ブロック、広域ブロックをバイパスする',
'right-proxyunbannable'      => 'プロキシの自動ブロックをバイパスする',
'right-protect'              => '保護レベルの変更と保護されたページの編集',
'right-editprotected'        => '保護ページの編集 (カスケード保護を除く)',
'right-editinterface'        => 'ユーザーインターフェースの編集',
'right-editusercssjs'        => '他利用者のCSS・JSファイルの編集',
'right-rollback'             => '特定のページを最後に編集した利用者の編集を即時にロールバック',
'right-markbotedits'         => 'ロールバックの版をボットの編集として印する',
'right-noratelimit'          => 'レート制限を受けない',
'right-import'               => '他のWikiからのページのインポート',
'right-importupload'         => 'ファイルのアップロードからページをインポートする',
'right-patrol'               => '他人の編集をパトロール済みとして印する',
'right-autopatrol'           => '自分の編集をパトロール済みとして自動的に印する',
'right-patrolmarks'          => '最近更新したページのパトロールマークを見る',
'right-unwatchedpages'       => 'ウォッチされていないページのリストを見る',
'right-trackback'            => 'トラックバックを投稿',
'right-mergehistory'         => 'ページの履歴を統合',
'right-userrights'           => '全利用者権限を編集',
'right-userrights-interwiki' => '他のウィキの利用者の利用者権限を編集',
'right-siteadmin'            => 'データベースのロック・ロック解除',

# User rights log
'rightslog'      => '利用者権限変更記録',
'rightslogtext'  => '以下は利用者の権限変更の一覧です。',
'rightslogentry' => '$1 の権限を $2 から $3 へ変更しました。',
'rightsnone'     => '（権限なし）',

# Recent changes
'nchanges'                          => '$1 回の更新',
'recentchanges'                     => '最近更新したページ',
'recentchangestext'                 => '最近の更新はこのページから確認できます。',
'recentchanges-feed-description'    => '最近付け加えられた変更はこのフィードで確認できます。',
'rcnote'                            => '以下は $3 までの <strong>$2</strong> 日間に編集された <strong>$1</strong> ページです（<strong>{{int:newpageletter}}</strong>=新規項目、<strong>{{int:minoreditletter}}</strong>=細部の編集、<strong>{{int:boteditletter}}</strong>=ボットの編集、日時はオプションで未設定ならUTC）',
'rcnotefrom'                        => '以下は <b>$2</b> までの更新です。（最大 <b>$1</b> 件）',
'rclistfrom'                        => '$1以後現在までの更新を表示',
'rcshowhideminor'                   => '細部の編集を$1',
'rcshowhidebots'                    => 'ボットの編集を$1',
'rcshowhideliu'                     => '登録利用者の編集を$1',
'rcshowhideanons'                   => '匿名利用者の編集を$1',
'rcshowhidepatr'                    => 'パトロールされた編集を$1',
'rcshowhidemine'                    => '自分の編集を$1',
'rclinks'                           => '最近 $2 日間の $1 件分を表示する<br />$3',
'diff'                              => '差分',
'hist'                              => '履歴',
'hide'                              => '隠す',
'show'                              => '表示',
'minoreditletter'                   => 'M',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1人がウォッチしています]',
'rc_categories'                     => 'カテゴリを制限（"|" で区切る）',
'rc_categories_any'                 => 'すべて',
'newsectionsummary'                 => '/* $1 */ 新しい節',

# Recent changes linked
'recentchangeslinked'          => 'リンク先の更新状況',
'recentchangeslinked-title'    => '"$1" からリンクされているページの更新状況',
'recentchangeslinked-noresult' => '指定期間中に指定ページのリンク先に更新はありませんでした。',
'recentchangeslinked-summary'  => "この特別ページはリンク先の更新状況です。あなたのウォッチリストにあるページは'''太字'''で表示されます。",
'recentchangeslinked-page'     => 'ページ名:',
'recentchangeslinked-to'       => 'リンク元の更新状況に切り替える',

# Upload
'upload'                      => 'アップロード',
'uploadbtn'                   => 'アップロード',
'reupload'                    => '再アップロード',
'reuploaddesc'                => 'アップロードのフォームへ戻る',
'uploadnologin'               => 'ログインしていません',
'uploadnologintext'           => 'ファイルをアップロードするには[[Special:UserLogin|ログイン]]する必要があります。',
'upload_directory_missing'    => 'アップロードディレクトリ ($1) が見つからないためウェブサーバによって作成できませんでした',
'upload_directory_read_only'  => 'アップロード先のディレクトリ ($1) にウェブサーバーが書き込めません。',
'uploaderror'                 => 'アップロード エラー',
'uploadtext'                  => "ファイルを新しくアップロードする場合には、以下のフォームを利用してください。過去にアップロードされたファイルの閲覧・検索には[[Special:ImageList|{{int:imagelist}}]]をご利用ください。アップロードの記録は[[Special:Log/upload|アップロード記録]]、削除の記録は[[Special:Log/delete|削除記録]]にも記録されます。

ページにファイルを挿入するには以下の書式のリンクを使います。
* '''<tt><nowiki>[[</nowiki>{{ns:image}}:<nowiki>File.jpg]]</nowiki></tt>''' とするとファイルをもとのサイズのまま表示します
* '''<tt><nowiki>[[</nowiki>{{ns:image}}:<nowiki>File.png|200px|thumb|left|代替テキスト]]</nowiki></tt>''' とすると左寄せの枠内に200px幅に縮小した画像を説明文（代替テキスト）を添えて表示します
* '''<tt><nowiki>[[</nowiki>{{ns:media}}:<nowiki>File.ogg]]</nowiki></tt>''' とするとファイルを表示せずに直接ファイルへリンクします",
'upload-permitted'            => '許可されているファイル形式： $1',
'upload-preferred'            => '推奨されているファイル形式： $1',
'upload-prohibited'           => '禁止されているファイル形式： $1',
'uploadlog'                   => 'アップロード記録',
'uploadlogpage'               => 'アップロード記録',
'uploadlogpagetext'           => '以下はファイルアップロードの最近の記録です。画像付きで見るには[[Special:NewImages|新規画像ギャラリー]]をご覧ください。',
'filename'                    => 'ファイル名',
'filedesc'                    => 'ファイルの概要',
'fileuploadsummary'           => 'ファイルの概要:',
'filestatus'                  => '著作権情報:',
'filesource'                  => 'ファイルの出典:',
'uploadedfiles'               => 'アップロードされたファイル',
'ignorewarning'               => '警告を無視し、保存してしまう',
'ignorewarnings'              => '警告を無視',
'minlength1'                  => 'ファイル名は1文字以上である必要があります。',
'illegalfilename'             => 'ファイル名 "$1" にページ・タイトルとして使えない文字が含まれています。ファイル名を変更してからもう一度アップロードしてください。',
'badfilename'                 => 'ファイル名は "$1" へ変更されました。',
'filetype-badmime'            => 'MIME タイプ "$1" のファイルのアップロードは許可されていません。',
'filetype-unwanted-type'      => "'''\".\$1\"''' は好ましくないファイル形式です。次の{{PLURAL:\$3|ファイル形式|ファイル形式}}を推奨します： \$2",
'filetype-banned-type'        => "'''\".\$1\"''' は許可されていないファイル形式です。次の{{PLURAL:\$3|ファイル形式|ファイル形式}}を利用してください： \$2",
'filetype-missing'            => 'ファイルに拡張子 (".jpg" など)がありません。',
'large-file'                  => 'ファイルサイズは $1 バイト以下に抑えることが推奨されています。このファイルは $2 バイトです。',
'largefileserver'             => 'ファイルが大きすぎます。サーバー設定で許されている最大値を超過しました。',
'emptyfile'                   => 'あなたがアップロードしようとしているファイルは内容が空であるか、もしくはファイル名の指定が間違っています。もう一度、ファイル名が正しいか、あるいはアップロードしようとしたファイルであるかどうかを確認してください。',
'fileexists'                  => 'この名前のファイルは既に存在しています。<strong><tt>$1</tt></strong>と置き換えるかどうかお確かめください。',
'filepageexists'              => 'このファイル <strong><tt>$1</tt></strong> に関する説明ページは既に存在しますが、ファイル自体が存在していない現状です。よって、この名前でファイルをアップロードすることは可能ですが、説明ページには入力された概要が反映されません。新しい概要へ更新するためには、説明ページを手動で編集する必要があります。',
'fileexists-extension'        => '類似した名前のファイルが既に存在しています:<br />
アップロード中のファイル: <strong><tt>$1</tt></strong><br />
既存のファイル: <strong><tt>$2</tt></strong><br />
ファイルが本当に違うものであるか、確認してください。',
'fileexists-thumb'            => "<center>'''既存のファイル'''</center>",
'fileexists-thumbnail-yes'    => 'このファイルはサムネイル（縮小版画像）である可能性があります。以下のファイルを確認してください: <strong><tt>$1</tt></strong><br />
確認したファイルが通常サイズの元画像である場合、追加でサムネイルを登録する必要はありません。',
'file-thumbnail-no'           => 'ファイル名が <strong><tt>$1</tt></strong> から始まっており、サムネイル（縮小版画像）である可能性が高いです。
より高精細な画像をお持ちの場合は、フルサイズ版をアップロードしてください。そうでない場合はファイル名を変更してください。',
'fileexists-forbidden'        => 'この名前のファイルは既に存在しています。前のページに戻り、別のファイル名でアップロードし直してください。
[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'この名前のファイルは共有ファイルリポジトリに既に存在しています。アップロードを継続したい場合は、前のページに戻り、別のファイル名を選択してください。[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'このファイルは以下のファイルと重複しています',
'successfulupload'            => 'アップロード成功',
'uploadwarning'               => 'アップロード 警告',
'savefile'                    => 'ファイルを保存',
'uploadedimage'               => '"$1" をアップロードしました。',
'overwroteimage'              => '"[[$1]]"の新しい版をアップロードしました',
'uploaddisabled'              => '申し訳ありませんが、アップロードは現在使用できません。',
'uploaddisabledtext'          => '{{SITENAME}}ではファイルのアップロードが禁止されています。',
'uploadscripted'              => 'このファイルはウェブブラウザが誤って解釈してしまうおそれのあるHTMLまたはスクリプトコードを含んでいます。',
'uploadcorrupt'               => '指定したファイルは壊れているか拡張子が正しくありません。ファイルを確認の上再度アップロードをしてください。',
'uploadvirus'                 => 'このファイルにはウイルスが含まれています!! &nbsp;詳細: $1',
'sourcefilename'              => 'ファイル名:',
'destfilename'                => '掲載するファイル名:',
'upload-maxfilesize'          => '最大ファイルサイズ: $1',
'watchthisupload'             => 'このページをウォッチ',
'filewasdeleted'              => 'この名前のファイルは一度アップロードされその後削除されています。アップロードの前に$1を確認してみてください。',
'upload-wasdeleted'           => "'''警告:あなたは過去に削除されたファイルをアップロードしようとしています。'''

このままアップロードを行うことが適切かどうか確認してください。参考として以下にこのファイルの削除記録を表示しています:",
'filename-bad-prefix'         => 'アップロードしようとしている <strong>"$1"</strong> のファイル名が、デジタルカメラによって自動的に付与されるような名称となっています。どのようなファイルであるのか、ファイル名を見ただけでも分かるような名称にしてください。',

'upload-proto-error'      => '不正なプロトコル',
'upload-proto-error-text' => 'アップロード元のURLは <code>http://</code> か <code>ftp://</code> で始まっている必要があります。',
'upload-file-error'       => '内部エラー',
'upload-file-error-text'  => 'サーバーの内部エラーのため、一時ファイルの作成に失敗しました。システムの管理者に連絡してください。',
'upload-misc-error'       => '不明なエラー',
'upload-misc-error-text'  => 'アップロード時に不明なエラーが検出されました。指定したURLがアクセス可能で有効なものであるかを再度確認してください。それでもこのエラーが発生する場合は、システムの管理者に連絡してください。',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URLに到達不能',
'upload-curl-error6-text'  => '指定したURLに到達できませんでした。URLが正しいものであるか、指定したサイトが現在使用可能かを再度確認してください。',
'upload-curl-error28'      => 'タイムアウト',
'upload-curl-error28-text' => '相手サイトからの応答がありませんでした。指定したサイトが現在使用可能かを確認した上で、しばらく待ってもう一度お試しください。また、インターネットが混雑していない時間帯に実行することを推奨します。',

'license'            => 'ライセンス:',
'nolicense'          => 'ライセンス情報を選択してください:',
'license-nopreview'  => '（プレビューはありません）',
'upload_source_url'  => '（インターネット上のURL）',
'upload_source_file' => '（あなたのコンピューター上のファイル）',

# Special:ImageList
'imagelist-summary'     => 'この特別ページではすべてのアップロードされたファイルの一覧を表示します。

ソートのデフォルトでは新しい順です。ヘッダのクリックでソート順と種類を変更できます。',
'imagelist_search_for'  => 'メディア名で検索:',
'imgfile'               => 'ファイル',
'imagelist'             => 'ファイルリスト',
'imagelist_date'        => '日時',
'imagelist_name'        => '名前',
'imagelist_user'        => '利用者',
'imagelist_size'        => 'サイズ（バイト）',
'imagelist_description' => '概要',

# Image description page
'filehist'                       => 'ファイルの履歴',
'filehist-help'                  => '過去の版のファイルを参照するには、日付/時刻の列にあるリンクをクリックしてください。',
'filehist-deleteall'             => '全て削除',
'filehist-deleteone'             => '削除する',
'filehist-revert'                => '差し戻す',
'filehist-current'               => '現在の版',
'filehist-datetime'              => '日付/時刻',
'filehist-user'                  => '利用者',
'filehist-dimensions'            => '解像度',
'filehist-filesize'              => 'ファイルサイズ',
'filehist-comment'               => 'コメント',
'imagelinks'                     => 'リンク',
'linkstoimage'                   => 'このファイルを使用している{{PLURAL:$1|ページ|&nbsp;$1 ページ}}の一覧:',
'nolinkstoimage'                 => 'このファイルを使用しているページはありません。',
'morelinkstoimage'               => 'このファイルの[[Special:WhatLinksHere/$1|リンク元]]を見る。',
'redirectstofile'                => 'このファイルへ{{PLURAL:$1|リダイレクトされているファイル|リダイレクトされている $1 ファイル}}の一覧:',
'duplicatesoffile'               => '以下にこのファイルと同一のファイル $1 件を表示しています:',
'sharedupload'                   => 'このファイルは共有されており、他のプロジェクトで使用されている可能性があります。',
'shareduploadwiki'               => '詳しい情報は$1を参照してください。',
'shareduploadwiki-desc'          => '共有リポジトリ内の$1にあるこのファイルの説明は以下の通りです。',
'shareduploadwiki-linktext'      => 'ファイルの詳細ページ',
'shareduploadduplicate'          => 'このファイルは共有レポジトリの $1 と重複しています。',
'shareduploadduplicate-linktext' => '別のファイル',
'shareduploadconflict'           => 'このファイルは共有レポジトリの $1 とファイル名が同じです。',
'shareduploadconflict-linktext'  => '別のファイル',
'noimage'                        => '同名のファイルは存在しません。「$1」リンクをクリックしてください。',
'noimage-linktext'               => 'このファイル名でアップロードする',
'uploadnewversion-linktext'      => 'このファイルの新しいバージョンをアップロードする',
'imagepage-searchdupe'           => '重複ファイルを検索する',

# File reversion
'filerevert'                => '$1 を差し戻す',
'filerevert-legend'         => 'ファイルを差し戻す',
'filerevert-intro'          => "あなたは'''[[Media:$1|$1]]'''の[$4 $3, $2]の版を差し戻そうとしています。",
'filerevert-comment'        => 'コメント:',
'filerevert-defaultcomment' => '$1 $2 の版へ差し戻し',
'filerevert-submit'         => '差し戻す',
'filerevert-success'        => "'''[[Media:$1|$1]]'''は[$4 $3, $2]の版に差し戻されました。",
'filerevert-badversion'     => '指定されたタイムスタンプより前の版のファイルはありません。',

# File deletion
'filedelete'                  => '$1の削除',
'filedelete-legend'           => 'ファイルの削除',
'filedelete-intro'            => "あなたは'''[[Media:$1|$1]]'''を削除しようとしています。",
'filedelete-intro-old'        => "あなたは'''[[Media:$1|$1]]'''の[$4 $3, $2]の版を削除しようとしています。",
'filedelete-comment'          => 'コメント:',
'filedelete-submit'           => '削除する',
'filedelete-success'          => "'''$1''' は削除されました。",
'filedelete-success-old'      => "'''[[Media:$1|$1]]''' の $2 $3 版は削除されています。",
'filedelete-nofile'           => "'''$1''' は{{SITENAME}}上に存在しません。",
'filedelete-nofile-old'       => "指定された属性を持つ'''$1'''の古い版は存在しません。",
'filedelete-iscurrent'        => 'このファイルの最新版を削除しようとしています。直前の版に差し戻してください。',
'filedelete-otherreason'      => 'その他/理由の付記:',
'filedelete-reason-otherlist' => 'その他の理由',
'filedelete-reason-dropdown'  => '*よくある削除理由
** 著作権侵害
** ファイルの重複',
'filedelete-edit-reasonlist'  => '削除理由を編集する',

# MIME search
'mimesearch'         => 'MIMEタイプ検索',
'mimesearch-summary' => '指定したMIMEタイプに合致するファイルを検索します。contenttype/subtype の形式で指定してください（例: <tt>image/jpeg</tt>）。',
'mimetype'           => 'MIMEタイプ:',
'download'           => 'ダウンロード',

# Unwatched pages
'unwatchedpages' => 'ウォッチされていないページ',

# List redirects
'listredirects' => 'リダイレクトの一覧',

# Unused templates
'unusedtemplates'     => '使われていないテンプレート',
'unusedtemplatestext' => 'このページでは {{ns:template}} 名前空間にあって他のページから使用されていないものを一覧にしています。削除する前にリンク元を確認してください。',
'unusedtemplateswlh'  => 'リンク元',

# Random page
'randompage'         => 'おまかせ表示',
'randompage-nopages' => 'この名前空間にはページはありません。',

# Random redirect
'randomredirect'         => 'おまかせリダイレクト',
'randomredirect-nopages' => 'この名前空間にはリダイレクトはありません。',

# Statistics
'statistics'             => 'サイトの統計',
'sitestats'              => 'サイト全体の統計',
'userstats'              => '利用者登録統計',
'sitestatstext'          => "データベース内には'''$1'''ページのデータがあります。この数字には「ノートページ」や「{{SITENAME}}関連のページ」、「書きかけのページ」、「リダイレクト」など、記事とはみなせないページが含まれています。これらを除いた、記事とみなされるページ数は約'''$2'''ページになります。

'''$8'''個のファイルがアップロードされました。

ページの総閲覧回数は'''$3'''回です。また、'''$4'''回の編集が行われました。平均すると、1ページあたり'''$5'''回の編集が行われ、1編集あたり'''$6'''回閲覧されています。

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue] の長さは '''$7''' です。",
'userstatstext'          => "登録済みの利用者は'''$1'''人で、内'''$2'''人 ('''$4%''') が$5権限を持っています。($3を参照)",
'statistics-mostpopular' => '最も閲覧されているページ',

'disambiguations'      => '曖昧さ回避ページ',
'disambiguationspage'  => 'Template:Aimai',
'disambiguations-text' => "以下のページは'''曖昧さ回避ページ'''へリンクしています。これらのページはより適した主題のページへリンクされるべきです。<br />
[[MediaWiki:Disambiguationspage]] からリンクされたテンプレートを使用しているページは曖昧さ回避ページと見なされます。",

'doubleredirects'            => '二重リダイレクト',
'doubleredirectstext'        => '各列は最初及び2つ目のリダイレクトへのリンクが記されています。2つ目のそれ同様、最初のものを本来のページへリダイレクトしなおしてください。',
'double-redirect-fixed-move' => '[[$1]] が移動されているため、リダイレクト先を移動先の [[$2]] へ変更しました。',
'double-redirect-fixer'      => 'リダイレクトフィクサー',

'brokenredirects'        => '迷子のリダイレクト',
'brokenredirectstext'    => '以下は存在しないページにリンクしているリダイレクトです。',
'brokenredirects-edit'   => '(編集)',
'brokenredirects-delete' => '(削除)',

'withoutinterwiki'         => '言語間リンクを持たないページ',
'withoutinterwiki-summary' => '以下のページには多言語版へのリンクがありません:',
'withoutinterwiki-legend'  => 'プレフィックス',
'withoutinterwiki-submit'  => '表示',

'fewestrevisions' => '編集履歴の少ないページ',

# Miscellaneous special pages
'nbytes'                  => '$1 バイト',
'ncategories'             => '$1 のカテゴリ',
'nlinks'                  => '$1 個のリンク',
'nmembers'                => '$1 項目',
'nrevisions'              => '$1 の版',
'nviews'                  => '$1 回表示',
'specialpage-empty'       => '合致するものがありません。',
'lonelypages'             => '孤立しているページ',
'lonelypagestext'         => '以下のページは、どこからもリンクされていない孤立したページです。',
'uncategorizedpages'      => 'カテゴリ未導入のページ',
'uncategorizedcategories' => 'カテゴリ未導入のカテゴリ',
'uncategorizedimages'     => 'カテゴリ未分類のファイル',
'uncategorizedtemplates'  => 'カテゴリ未導入のテンプレート',
'unusedcategories'        => '使われていないカテゴリ',
'unusedimages'            => '使われていない画像',
'popularpages'            => '人気のページ',
'wantedcategories'        => 'カテゴリページが存在しないカテゴリ',
'wantedpages'             => '投稿が望まれているページ',
'missingfiles'            => '見つからないファイル',
'mostlinked'              => '被リンクの多いページ',
'mostlinkedcategories'    => '項目の多いカテゴリ',
'mostlinkedtemplates'     => '使用箇所の多いテンプレート',
'mostcategories'          => 'カテゴリの多い項目',
'mostimages'              => 'リンクの多い画像',
'mostrevisions'           => '版の多い項目',
'prefixindex'             => '全ページ (ページ指定)',
'shortpages'              => '短いページ',
'longpages'               => '長いページ',
'deadendpages'            => '有効なページへのリンクがないページ',
'deadendpagestext'        => '以下のページは、このウィキの他のページにリンクしていないページです。',
'protectedpages'          => '保護されているページ',
'protectedpages-indef'    => '無期限保護のみ',
'protectedpagestext'      => '以下のページは移動や編集が禁止されています。',
'protectedpagesempty'     => '現在保護中のページがありません。',
'protectedtitles'         => '作成保護されているページ名',
'protectedtitlestext'     => '以下のページは新規作成が禁止されています',
'protectedtitlesempty'    => '現在作成保護されているページはありません。',
'listusers'               => '登録利用者の一覧',
'newpages'                => '新しいページ',
'newpages-username'       => '利用者名:',
'ancientpages'            => '更新されていないページ',
'move'                    => '移動',
'movethispage'            => 'このページを移動',
'unusedimagestext'        => '<p>他のウェブサイトがURLを直接用いて画像にリンクしている場合もあります。以下の画像一覧には、そのような形で利用されている画像が含まれている可能性があります。</p>',
'unusedcategoriestext'    => '以下のカテゴリページはどの項目・カテゴリからも使われていません。',
'notargettitle'           => '対象となるページが存在しません',
'notargettext'            => '対象となるページ又は利用者が指定されていません',
'nopagetitle'             => 'そのようなページはありません',
'nopagetext'              => 'あなたの指定したページは存在しません。',
'pager-newer-n'           => '{{PLURAL:$1|前 1|前 $1}}',
'pager-older-n'           => '{{PLURAL:$1|次 1|次 $1}}',
'suppress'                => 'Oversight',

# Book sources
'booksources'               => '文献資料',
'booksources-search-legend' => '文献資料を検索',
'booksources-go'            => '検索',
'booksources-text'          => '以下のリストは、新本、古本などを販売している外部サイトへのリンクです。あなたがお探しの本について、更に詳しい情報が提供されている場合もあります。',

# Special:Log
'specialloguserlabel'  => '利用者名:',
'speciallogtitlelabel' => 'タイトル:',
'log'                  => 'ログ',
'all-logs-page'        => '全ての記録',
'log-search-legend'    => 'ログの検索',
'log-search-submit'    => '検索',
'alllogstext'          => '{{SITENAME}}の取得可能なログがまとめて表示されています。ログの種類、実行した利用者、影響を受けたページ（利用者）による絞り込みができます。',
'logempty'             => '条件にマッチする記録はありません。',
'log-title-wildcard'   => 'この文字列で始まるタイトルを検索する',

# Special:AllPages
'allpages'          => '全ページ',
'alphaindexline'    => '$1―$2',
'nextpage'          => '次のページ（$1）',
'prevpage'          => '前のページ（$1）',
'allpagesfrom'      => '表示開始ページ:',
'allarticles'       => '全ページ',
'allinnamespace'    => '全ページ ($1 名前空間)',
'allnotinnamespace' => '全ページ ($1 名前空間を除く)',
'allpagesprev'      => '前へ',
'allpagesnext'      => '次へ',
'allpagessubmit'    => '表示',
'allpagesprefix'    => '次の文字列から始まるページを表示:',
'allpagesbadtitle'  => '指定したタイトルは無効か、他言語版または他ウィキ内のタイトルです。ページタイトルに使用できない文字が含まれている可能性があります。',
'allpages-bad-ns'   => '{{SITENAME}}に "$1" という名前空間はありません。',

# Special:Categories
'categories'                    => 'カテゴリ',
'categoriespagetext'            => '以下のカテゴリにはページまたはメディアが存在します。[[Special:UnusedCategories|未使用のカテゴリ]]はここには表示されていません。 [[Special:WantedCategories|カテゴリページが存在しないカテゴリ]]も参照してください。',
'categoriesfrom'                => '表示開始カテゴリ:',
'special-categories-sort-count' => '項目数順',
'special-categories-sort-abc'   => 'アルファベット順',

# Special:ListUsers
'listusersfrom'      => 'この文字から表示:',
'listusers-submit'   => '表示',
'listusers-noresult' => '利用者が見つかりませんでした。大文字・小文字の区別を確認してください。',

# Special:ListGroupRights
'listgrouprights'          => '利用者グループの権限',
'listgrouprights-summary'  => '以下はこのウィキに登録されている利用者グループとそれぞれに割り当てられている権限の一覧です。
個々の権限に関する更なる情報は[[{{MediaWiki:Listgrouprights-helppage}}]]を見てください。',
'listgrouprights-group'    => 'グループ',
'listgrouprights-rights'   => '権限',
'listgrouprights-helppage' => 'Help:グループと権限',
'listgrouprights-members'  => '(該当者一覧)',

# E-mail user
'mailnologin'     => 'メールアドレスの記載がありません。',
'mailnologintext' => '他の利用者宛てにメールを送信するためには、[[Special:UserLogin|ログイン]]し、あなたのメールアドレスを[[Special:Preferences|オプション]]に設定する必要があります。',
'emailuser'       => 'この利用者にメールを送信',
'emailpage'       => 'メール送信ページ',
'emailpagetext'   => 'メールを送る先の利用者が有効なメールアドレスを{{int:preferences}}で登録していれば、下のフォームを通じてメールを送ることができます。
あなたが [[Special:Preferences|{{int:preferences}}]]で登録したご自分のメールアドレスが送るメールのFrom:の欄に自動的に組み込まれ、受け取った相手が直接あなたに返事を出せるようになっています。',
'usermailererror' => 'メール送信時に以下のエラーが発生しました:',
'defemailsubject' => '{{SITENAME}} 電子メール',
'noemailtitle'    => '送り先のメールアドレスがありません。',
'noemailtext'     => 'この利用者は有効なメールアドレスを登録していないか、メールを受け取りたくないというオプションを選択しています。',
'emailfrom'       => 'あなたのアドレス',
'emailto'         => 'あて先',
'emailsubject'    => '題名',
'emailmessage'    => '本文',
'emailsend'       => 'メール送信',
'emailccme'       => '自分宛に控えを送信する',
'emailccsubject'  => '$1宛てウィキメールの控え: $2',
'emailsent'       => 'メールを送りました',
'emailsenttext'   => 'メールは無事送信されました。',
'emailuserfooter' => 'この電子メールは $1 から $2 へ、{{SITENAME}}の「利用者へメールを送信」機能を使って送られました。',

# Watchlist
'watchlist'            => 'ウォッチリスト',
'mywatchlist'          => 'ウォッチリスト',
'watchlistfor'         => "'''$1'''",
'nowatchlist'          => 'あなたのウォッチリストは空です。',
'watchlistanontext'    => 'ウォッチリストを確認あるいは編集するには $1 してください。',
'watchnologin'         => 'ログインしていません',
'watchnologintext'     => 'ウォッチリストを変更するためには、[[Special:UserLogin|ログイン]]している必要があります。',
'addedwatch'           => 'ウォッチリストに追加しました',
'addedwatchtext'       => "ページ \"\$1\" をあなたの[[Special:Watchlist|ウォッチリスト]]に追加しました。

このページと、付属のノートのページに変更があった際にはそれをウォッチリストで知ることができます。また、[[Special:RecentChanges|最近更新したページ]]ではウォッチリストに含まれているページは'''ボールド体'''で表示され、見つけやすくなります。

ウォッチリストから特定のページを削除したい場合には、サイドバーかタブにある \"{{int:unwatch}}\" のリンクをクリックしてください。",
'removedwatch'         => 'ウォッチリストから削除しました',
'removedwatchtext'     => 'ページ "<nowiki>$1</nowiki>" をウォッチリストから削除しました。',
'watch'                => 'ウォッチリストに追加',
'watchthispage'        => 'ウォッチリストに追加',
'unwatch'              => 'ウォッチリストから削除',
'unwatchthispage'      => 'ウォッチリストから削除',
'notanarticle'         => 'これは記事ではありません。',
'notvisiblerev'        => 'この版は削除されました',
'watchnochange'        => 'その期間内にウォッチリストにあるページはどれも編集されていません。',
'watchlist-details'    => 'あなたのウォッチリストには $1 のページが入っています（ノートページは数えません）。',
'wlheader-enotif'      => '* メール通知が有効になっています',
'wlheader-showupdated' => "* あなたが最後に訪問したあとに変更されたページは'''ボールド体'''で表示されます",
'watchmethod-recent'   => 'ウォッチリストの中から最近編集されたものを抽出',
'watchmethod-list'     => '最近編集された中からウォッチしているページを抽出',
'watchlistcontains'    => 'あなたのウォッチリストには $1 ページ登録されています。',
'iteminvalidname'      => '"$1" をウォッチリストから削除できません。ページ名が不正です。',
'wlnote'               => '以下は最近 <strong>$2</strong> 時間に編集された <strong>$1</strong> ページです。',
'wlshowlast'           => '最近の [$1時間] [$2日間] [$3] のものを表示する',
'watchlist-show-bots'  => 'ボットの編集を表示',
'watchlist-hide-bots'  => 'ボットの編集を隠す',
'watchlist-show-own'   => '自分の編集を表示',
'watchlist-hide-own'   => '自分の編集を隠す',
'watchlist-show-minor' => '細部の編集を表示',
'watchlist-hide-minor' => '細部の編集を隠す',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'ウォッチリストに追加しています...',
'unwatching' => 'ウォッチリストから削除しています...',

'enotif_mailer'                => '{{SITENAME}} 通知メール',
'enotif_reset'                 => 'すべてのページを訪問済みにする',
'enotif_newpagetext'           => '(新規ページ)',
'enotif_impersonal_salutation' => '{{SITENAME}} 利用者',
'changed'                      => '変更',
'created'                      => '作成',
'enotif_subject'               => '{{SITENAME}} のページ "$PAGETITLE" が $PAGEEDITOR によって$CHANGEDORCREATEDされました',
'enotif_lastvisited'           => '
あなたが最後に閲覧してからの差分を見るには以下のURLにアクセスしてください:
$1',
'enotif_lastdiff'              => '
変更内容を見るには以下のURLにアクセスしてください:
$1',
'enotif_anon_editor'           => '匿名利用者 $1',
'enotif_body'                  => '$WATCHINGUSERNAMEさん、

{{SITENAME}}のページ $PAGETITLE が $PAGEEDITDATE に
$PAGEEDITOR によって$CHANGEDORCREATEDされました。
現在の版を見るには次のURLにアクセスしてください:
$PAGETITLE_URL
$NEWPAGE

編集内容の要約: $PAGESUMMARY ($PAGEMINOREDIT)

投稿者:
メール: $PAGEEDITOR_EMAIL
ウィキ: $PAGEEDITOR_WIKI

あなたがこのページを訪れない限り、これ以上の通知は送信されません。
ウォッチリストからすべての通知フラグをリセットすることもできます。

                         {{SITENAME}} 通知システム

--
ウォッチリストの設定を変更する:
{{fullurl:Special:Watchlist/edit}}

助けが必要ですか:
{{fullurl:Help:Contents}}',

# Delete/protect/revert
'deletepage'                  => 'ページを削除',
'confirm'                     => '確認',
'excontent'                   => "内容: '$1'",
'excontentauthor'             => "内容: '$1' (投稿者 [[Special:Contributions/$2|$2]] のみ)",
'exbeforeblank'               => "白紙化前の内容: '$1'",
'exblank'                     => '白紙ページ',
'delete-confirm'              => '"$1" の削除',
'delete-legend'               => '削除',
'historywarning'              => '警告: 削除しようとしているページには履歴があります:',
'confirmdeletetext'           => '指定されたページまたは画像は、その変更履歴と共にデータベースから永久に削除されようとしています。
あなたが削除を望んでおり、それがもたらす帰結を理解しており、かつあなたのしようとしていることが[[{{MediaWiki:Policy-url}}|方針]]に即したものであることを確認してください。',
'actioncomplete'              => '完了しました',
'deletedtext'                 => '"<nowiki>$1</nowiki>" は削除されました。最近の削除に関しては $2 を参照してください。',
'deletedarticle'              => '"$1" を削除しました。',
'suppressedarticle'           => '"[[$1]]"の秘匿を実施',
'dellogpage'                  => '削除記録',
'dellogpagetext'              => '以下は最近の削除と復帰の記録です。',
'deletionlog'                 => '削除記録',
'reverted'                    => '以前のバージョンへの差し戻し',
'deletecomment'               => '削除の理由',
'deleteotherreason'           => 'その他 / 理由の付記:',
'deletereasonotherlist'       => 'その他の理由',
'deletereason-dropdown'       => '*よくある削除理由
** 投稿者依頼
** 著作権侵害
** 荒らし',
'delete-edit-reasonlist'      => '削除理由を編集する',
'delete-toobig'               => 'このページには、$1 {{PLURAL:$1|版|版}}を超える多くの編集履歴があります。処理負荷増大によって{{SITENAME}}に偶発的なトラブルが起こることを防ぐため、このようなページの削除は制限されています。',
'delete-warning-toobig'       => 'このページには、 $1 {{PLURAL:$1|版|版}}を超える多くの編集履歴があります。削除の際、{{SITENAME}}のデータベース処理に大きな負荷がかかりますので、十分に注意してください。',
'rollback'                    => '編集の差し戻し',
'rollback_short'              => '差し戻し',
'rollbacklink'                => '差し戻し',
'rollbackfailed'              => '差し戻しに失敗しました',
'cantrollback'                => '投稿者がただ一人であるため、編集を差し戻せません。',
'alreadyrolled'               => 'ページ [[:$1]] の [[User:$2|$2]] ([[User talk:$2|会話]] | [[Special:Contributions/$2|{{int:contribslink}}]])による編集のロールバックに失敗しました。他の利用者がすでに編集を行ったかロールバックしたためです。

このページの最後の編集は [[User:$3|$3]] ([[User talk:$3|会話]] | [[Special:Contributions/$3|{{int:contribslink}}]]) によるものです。',
'editcomment'                 => '編集内容の要約: <i>$1</i>', # only shown if there is an edit comment
'revertpage'                  => '[[Special:Contributions/$2|$2]] ([[User talk:$2|会話]]) による編集を [[User:$1|$1]] による版へ差し戻し', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 による編集を取り消して $2 による直前の版へ差し戻しました。',
'sessionfailure'              => 'あなたのログイン・セッションに問題が発生しました。この動作はセッションハイジャックを防ぐために取り消されました。ブラウザの「戻る」を押してからページを再読込し、もう一度送信してください。',
'protectlogpage'              => '保護記録',
'protectlogtext'              => '以下はページの保護・保護解除の一覧です。',
'protectedarticle'            => '"[[$1]]" を保護しました。',
'modifiedarticleprotection'   => '"[[$1]]" の保護レベルを変更しました。',
'unprotectedarticle'          => '"[[$1]]" の保護を解除しました。',
'protect-title'               => '"$1" の保護',
'protect-legend'              => '保護の確認',
'protectcomment'              => '保護・保護解除の理由',
'protectexpiry'               => '期間',
'protect_expiry_invalid'      => '期間の指定が無効です。',
'protect_expiry_old'          => '保護期限が過去の時刻です。',
'protect-unchain'             => '移動権限を操作',
'protect-text'                => "ページ \"'''<nowiki>\$1</nowiki>'''\" の保護レベルを表示・操作できます。",
'protect-locked-blocked'      => 'あなたはブロックされているため、保護レベルを変更できません。
現在のページ<strong>$1</strong>の状態は以下の通りです:',
'protect-locked-dblock'       => '現在データベースがロックされているため保護レベルを変更できません
現在のページ<strong>$1</strong>の状態は以下の通りです:',
'protect-locked-access'       => 'あなたのアカウントはページの保護レベルを変更する権限を持っていません。
現在のページ<strong>$1</strong>の状態は以下の通りです:',
'protect-cascadeon'           => 'このページはカスケード保護されている以下のページから呼び出されているため、編集できないように保護されています。保護レベルを変更することは可能ですが、このカスケード保護には影響しません。',
'protect-default'             => '（解除）',
'protect-fallback'            => '"$1" 権限が必要です',
'protect-level-autoconfirmed' => '新規利用者と匿名利用者を禁止',
'protect-level-sysop'         => '{{int:group-sysop}}のみ',
'protect-summary-cascade'     => 'カスケード',
'protect-expiring'            => '$1 に解除',
'protect-cascade'             => 'カスケード保護 - このページで取り込んでいる全ての他ページも保護されます。',
'protect-cantedit'            => 'あなたにはこのページの編集権限がないため保護レベルを変更できません。',
'restriction-type'            => '制限:',
'restriction-level'           => '保護レベル:',
'minimum-size'                => '最小サイズ',
'maximum-size'                => '最大サイズ',
'pagesize'                    => '（バイト）',

# Restrictions (nouns)
'restriction-edit'   => '編集',
'restriction-move'   => '移動',
'restriction-create' => '作成',
'restriction-upload' => 'アップロード',

# Restriction levels
'restriction-level-sysop'         => '全保護',
'restriction-level-autoconfirmed' => '半保護',
'restriction-level-all'           => 'すべて',

# Undelete
'undelete'                     => '削除されたページを参照する',
'undeletepage'                 => '削除された編集の参照と復帰',
'undeletepagetitle'            => "'''以下に表示されているのは [[:$1]] の削除された版です'''。",
'viewdeletedpage'              => '削除されたページの削除記録と履歴',
'undeletepagetext'             => '以下のページは削除されていますが、アーカイブに残っているため、復帰できます。アーカイブは定期的に消去されます。',
'undelete-fieldset-title'      => '削除された版の復帰',
'undeleteextrahelp'            => '全ての版を復帰する場合は、全ての版のチェックボックスを選択していない状態で「{{int:undeletebtn}}」ボタンをクリックしてください。
特定の版を復帰する場合は、復帰する版のチェックボックスを選択した状態で「{{int:undeletebtn}}」ボタンをクリックしてください。
「{{int:undeletereset}}」ボタンををクリックするとコメント欄と全てのチェックボックスがクリアされます。',
'undeleterevisions'            => '$1版保管',
'undeletehistory'              => 'ページの復帰を行うと、全ての特定版が履歴に復帰します。ページが削除された後に、同じ名前で新しいページが作成されていた場合、復帰した特定版は、その前の履歴として出現します。',
'undeleterevdel'               => 'トップページの版が一時的に削除されているものに対しては、復帰処理を実行できません。このような場合、まずは最新の削除履歴に対するチェックまたは隠蔽を解除する必要があります。ファイル履歴を表示する権限がない場合も、復帰させることはできません。',
'undeletehistorynoadmin'       => '過去にこのページの全てもしくは一部が削除されています。以下に示すのは削除記録と削除された版の履歴です。削除された各版の内容は{{int:group-sysop}}のみが閲覧できます。',
'undelete-revision'            => '$1 の削除された $2 の版 投稿者 $3 :',
'undeleterevision-missing'     => '無効、あるいは誤った版です。当該版は既に復帰されたか、アーカイブから削除された可能性があります。',
'undelete-nodiff'              => 'これより前の版はありません。',
'undeletebtn'                  => '復帰',
'undeletelink'                 => '復帰',
'undeletereset'                => 'リセット',
'undeletecomment'              => 'コメント:',
'undeletedarticle'             => '"$1" を復帰しました。',
'undeletedrevisions'           => '$1 版を復帰しました。',
'undeletedrevisions-files'     => '$1 版のページと $2 ファイルを復帰しました',
'undeletedfiles'               => '$1 ファイルを復帰しました',
'cannotundelete'               => '復帰に失敗しました。誰かがすでにこのページを復帰しています。',
'undeletedpage'                => "<big>'''$1 を復帰しました。'''</big>

最近の削除と復帰については[[Special:Log/delete|削除記録]]を参照してください。",
'undelete-header'              => '最近の削除されたページは[[Special:Log/delete|削除記録]]で確認できます。',
'undelete-search-box'          => '削除されたページを検索',
'undelete-search-prefix'       => '表示するページ名の先頭:',
'undelete-search-submit'       => '検索',
'undelete-no-results'          => '一致する削除済みページのアーカイブが見つかりませんでした。',
'undelete-filename-mismatch'   => '$1 版のファイルを復帰できません: ファイル名が一致しません',
'undelete-bad-store-key'       => '$1 版のファイルを復帰できません: 削除前にファイルが失われています。',
'undelete-cleanup-error'       => '使用されていないログファイル "$1" の削除中にエラーが発生しました。',
'undelete-missing-filearchive' => 'ID $1 の記録がデータベースに存在しないため復帰できません。既に復帰されている可能性があります。',
'undelete-error-short'         => 'ファイル復帰エラー: $1',
'undelete-error-long'          => '$1 の復帰中にエラーが発生しました',

# Namespace form on various pages
'namespace'      => '名前空間:',
'invert'         => '選択した名前空間を隠す',
'blanknamespace' => '（標準）',

# Contributions
'contributions' => '利用者の投稿記録',
'mycontris'     => '自分の投稿記録',
'contribsub2'   => '利用者名: $1 ($2)',
'nocontribs'    => '利用者の投稿記録は見つかりませんでした。',
'uctop'         => '（最新）',
'month'         => '月:',
'year'          => '年:',

'sp-contributions-newbies'     => '新規利用者の投稿のみ表示',
'sp-contributions-newbies-sub' => '新規利用者',
'sp-contributions-blocklog'    => '投稿ブロック記録',
'sp-contributions-search'      => '投稿履歴の検索',
'sp-contributions-username'    => '利用者名または IPアドレス:',
'sp-contributions-submit'      => '検索',

# What links here
'whatlinkshere'            => 'リンク元',
'whatlinkshere-title'      => '$1 へリンクしているページ',
'whatlinkshere-page'       => 'ページ:',
'linklistsub'              => 'リンクの一覧',
'linkshere'                => '[[:$1]] は以下のページからリンクされています',
'nolinkshere'              => '[[:$1]] にリンクしているページはありません。',
'nolinkshere-ns'           => "指定された名前空間中で、'''[[:$1]]''' にリンクしているページはありません。",
'isredirect'               => 'リダイレクトページ',
'istemplate'               => 'テンプレート呼出',
'isimage'                  => '画像リンク',
'whatlinkshere-prev'       => '前 $1',
'whatlinkshere-next'       => '次 $1',
'whatlinkshere-links'      => '← リンク',
'whatlinkshere-hideredirs' => 'リダイレクトを$1',
'whatlinkshere-hidetrans'  => 'テンプレート呼出を$1',
'whatlinkshere-hidelinks'  => 'リンクを$1',
'whatlinkshere-hideimages' => 'ファイル呼び出しを$1',
'whatlinkshere-filters'    => 'フィルタ',

# Block/unblock
'blockip'                         => '投稿ブロック',
'blockip-legend'                  => '利用者のブロック',
'blockiptext'                     => '指定した利用者やIPアドレスからの投稿をブロックすることができます。',
'ipaddress'                       => 'IPアドレス',
'ipadressorusername'              => '利用者名 / IPアドレス',
'ipbexpiry'                       => '期間',
'ipbreason'                       => '理由',
'ipbreasonotherlist'              => 'その他',
'ipbreason-dropdown'              => '*よくあるブロック理由
** 虚偽情報の掲載
** ページ内容の除去
** スパム外部リンクの追加
** いたずら
** 嫌がらせ
** 複数アカウントの不正利用
** 不適切な利用者名',
'ipbanononly'                     => '匿名利用者のみブロック',
'ipbcreateaccount'                => 'アカウント作成をブロック',
'ipbemailban'                     => 'メール送信をブロック',
'ipbenableautoblock'              => 'この利用者が最後に使用したIPアドレスを自動的にブロック（ブロック後に使用したIPアドレスも含む）',
'ipbsubmit'                       => '投稿ブロックする',
'ipbother'                        => '期間 (その他のとき)',
'ipboptions'                      => '15分:15 minutes,30分:30 minutes,2時間:2 hours,1日:1 day,3日:3 days,1週間:1 week,2週間:2 weeks,1ヶ月:1 month,3ヶ月:3 months,6ヶ月:6 months,1年:1 year,無期限:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'その他',
'ipbotherreason'                  => '理由（その他/追加）',
'ipbhidename'                     => '利用者名/IPを{{int:blocklogpage}}、{{int:ipblocklist}}、{{int:listusers}}などに載せない',
'ipbwatchuser'                    => 'この利用者の利用者ページと利用者‐会話ページをウォッチリストに追加',
'badipaddress'                    => 'IPアドレスが異常です。',
'blockipsuccesssub'               => 'ブロックに成功しました。',
'blockipsuccesstext'              => '利用者またはIPアドレス [[Special:Contributions/$1|$1]] の投稿をブロックしました。<br />
[[Special:IPBlockList|投稿ブロック中の利用者やIPアドレス]]で確認できます。',
'ipb-edit-dropdown'               => 'ブロック理由を編集する',
'ipb-unblock-addr'                => '$1 のブロックを解除',
'ipb-unblock'                     => '利用者またはIPアドレスのブロックを解除する',
'ipb-blocklist-addr'              => '$1 の現在有効なブロックを表示',
'ipb-blocklist'                   => '現在有効なブロックを表示',
'unblockip'                       => '投稿ブロックを解除する',
'unblockiptext'                   => '以下のフォームで利用者またはIPアドレスの投稿ブロックを解除できます。',
'ipusubmit'                       => '投稿ブロックを解除する',
'unblocked'                       => '[[User:$1|$1]] の投稿ブロックを解除しました',
'unblocked-id'                    => 'ブロック $1 は解除されました',
'ipblocklist'                     => '投稿ブロック中の利用者やIPアドレス',
'ipblocklist-legend'              => 'ブロック中の利用者を検索',
'ipblocklist-username'            => '利用者名またはIPアドレス:',
'ipblocklist-submit'              => '検索',
'blocklistline'                   => '$1, $2 は $3 をブロック （$4）',
'infiniteblock'                   => '無期限',
'expiringblock'                   => '$1 に解除',
'anononlyblock'                   => '匿名のみ',
'noautoblockblock'                => '自動ブロックなし',
'createaccountblock'              => 'アカウント作成のブロック',
'emailblock'                      => 'メール送信のブロック',
'ipblocklist-empty'               => '{{int:ipblocklist}}はありません。',
'ipblocklist-no-results'          => '指定されたIPアドレスまたは利用者名はブロックされていません。',
'blocklink'                       => 'ブロック',
'unblocklink'                     => 'ブロック解除',
'contribslink'                    => '投稿記録',
'autoblocker'                     => '投稿ブロックされている利用者 "$1" と同じIPアドレスのため、自動的にブロックされています。ブロックの理由は "$2" です。',
'blocklogpage'                    => '投稿ブロック記録',
'blocklogentry'                   => '"$1" を $2 ブロックしました $3',
'blocklogtext'                    => 'このページは投稿ブロックと解除の記録です。自動的に投稿ブロックされたIPアドレスは記録されていません。現時点で有効な投稿ブロックは[[Special:IPBlockList|ブロック中の利用者一覧]]をご覧ください。',
'unblocklogentry'                 => '"$1" をブロック解除しました',
'block-log-flags-anononly'        => '匿名のみ',
'block-log-flags-nocreate'        => 'アカウント作成のブロック',
'block-log-flags-noautoblock'     => '自動ブロック無効',
'block-log-flags-noemail'         => 'メール送信のブロック',
'block-log-flags-angry-autoblock' => '拡張自動ブロック有効',
'range_block_disabled'            => '広域ブロックは無効に設定されています。',
'ipb_expiry_invalid'              => '不正な期間です。',
'ipb_expiry_temp'                 => '利用者名を秘匿したブロックは無期限でなければなりません。',
'ipb_already_blocked'             => '"$1" は既にブロックされています。',
'ipb_cant_unblock'                => 'エラー: ブロックされた ID $1 が見つかりません。おそらく既にブロック解除されています。',
'ipb_blocked_as_range'            => 'エラー: IPアドレス $1 は直接的なブロック対象となっていませんが、ブロックを解除できませんでした。これは恐らく、ブロック解除できないIPアドレス空間 $2 の範囲に含まれているためです。',
'ip_range_invalid'                => '不正なIPアドレス範囲です。',
'blockme'                         => 'ブロックする',
'proxyblocker'                    => 'プロキシブロッカー',
'proxyblocker-disabled'           => 'この機能は無効になっています。',
'proxyblockreason'                => 'Your IP address has been blocked because it is an open proxy. Please contact your Internet service provider or tech support and inform them of this serious security problem.

:あなたの使用しているIPアドレスは公開プロキシであるため投稿ブロックされています。あなたのインターネット・サービス・プロバイダ、もしくは技術担当者に連絡を取り、これが深刻なセキュリティ問題であることを伝えてください。',
'proxyblocksuccess'               => '終了しました。',
'sorbsreason'                     => 'あなたのIPアドレスは公開プロキシであると、{{SITENAME}}のブラックリストに掲載されています。',
'sorbs_create_account_reason'     => 'あなたのIPアドレスが公開プロキシであると{{SITENAME}}のブラックリストに掲載されているため、アカウントを作成できません。',

# Developer tools
'lockdb'              => 'データベースのロック',
'unlockdb'            => 'データベースのロック解除',
'lockdbtext'          => 'データベースをロックすると全ての利用者はページを編集できなくなり、オプションを変更できなくなり、ウォッチリストを編集できなくなるなど、データベースに書き込む全ての作業ができなくなります。本当にデータベースをロックして良いかどうか確認してください。メンテナンスが終了したらロックを解除してください。',
'unlockdbtext'        => 'データベースのロックを解除することで利用者はページを編集できるようになり、オプションを変更できるようになり、ウォッチリストを編集できるようになるなど、データベースに書き込む全ての作業ができるようになります。本当にデータベースのロックを解除していいかどうか確認してください。',
'lockconfirm'         => '本当にデータベースをロックする',
'unlockconfirm'       => 'ロックを解除する',
'lockbtn'             => 'ロック',
'unlockbtn'           => 'ロック解除',
'locknoconfirm'       => 'チェックボックスにチェックされていません。',
'lockdbsuccesssub'    => 'データベースはロックされました。',
'unlockdbsuccesssub'  => 'データベースのロックは解除されました',
'lockdbsuccesstext'   => 'データベースをロックしました。<br />
メンテナンスが終了したら忘れずに[[Special:UnlockDB|ロックを解除]]してください。',
'unlockdbsuccesstext' => 'データベースのロックは解除されました。',
'lockfilenotwritable' => 'データベースのロックファイルに書き込めません。データベースのロック・解除をするには、サーバー上のロックファイルに書き込める必要があります。',
'databasenotlocked'   => 'データベースはロックされていません。',

# Move page
'move-page'               => '$1 の移動',
'move-page-legend'        => 'ページの移動',
'movepagetext'            => "下のフォームを利用すると、ページ名が変更され、その履歴も変更先へ移動します。
古いページは変更先へのリダイレクトページとなります。
変更前のページへのリダイレクトは自動的に修正することができます。
自動的な修正を選択しない場合は、[[Special:DoubleRedirects|二重リダイレクト]]や[[Special:BrokenRedirects|迷子のリダイレクト]]を確認する必要があります。リンクを正しく維持するのはあなたの責任です。

移動先がすでに存在する場合には、履歴が移動元ページへのリダイレクトただ一つである場合を除いて移動できません。
つまり、間違えてページ名を変更した場合には元に戻せます。また移動によって既存のページを上書きしてしまうことはありません。

'''注意！'''
よく閲覧されるページや、他の多くのページからリンクされているページを移動すると予期せぬ結果が起こるかもしれません。
ページの移動に伴う影響をよく考えてから踏み切るようにしてください。",
'movepagetalktext'        => '付随するノートのページがある場合には、基本的には、一緒に移動されることになります。

但し、以下の場合については別です。
*名前空間をまたがる移動の場合
*移動先に既に履歴のあるノートページが存在する場合
*下のチェックボックスのチェックマークを消した場合

これらの場合、ノートページを移動する場合には、別に作業する必要があります。',
'movearticle'             => '移動するページ',
'movenotallowed'          => '{{SITENAME}}でページを移動する権限がありません。',
'newtitle'                => '新しいページ名',
'move-watch'              => '移動するページをウォッチ',
'movepagebtn'             => 'ページを移動',
'pagemovedsub'            => '無事移動しました。',
'movepage-moved'          => '<big>\'\'\'"$1"は"$2"へ移動されました。\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => '指定された移動先には既にページが存在するか、名前が不適切です。',
'cantmove-titleprotected' => '移動先ページが作成保護対象となっているため、ページを移動できません。',
'talkexists'              => 'ページ自身は移動されましたが、付随のノートページは移動先のページが存在したため移動できませんでした。手動で内容を統合してください。',
'movedto'                 => '移動先:',
'movetalk'                => 'ノートページが付随する場合には、それも一緒に移動する',
'move-subpages'           => 'サブページもすべて移動する',
'move-talk-subpages'      => 'ノートページのサブページもすべて移動する',
'movepage-page-exists'    => '$1 という名前のページは既に存在するため自動的な上書きは行われませんでした。',
'movepage-page-moved'     => '$1 は $2 へ移動されました。',
'movepage-page-unmoved'   => '$1 を $2 へ移動できませんでした。',
'movepage-max-pages'      => '自動的に移動できるのは $1ページ までで、それ以上は移動されません。',
'1movedto2'               => 'ページ [[$1]] を [[$2]] へ移動',
'1movedto2_redir'         => 'ページ [[$1]] をこのページあてのリダイレクト [[$2]] へ移動',
'movelogpage'             => '移動記録',
'movelogpagetext'         => '以下はページ移動の記録です。',
'movereason'              => '理由',
'revertmove'              => '差し戻し',
'delete_and_move'         => '削除して移動する',
'delete_and_move_text'    => '== 削除が必要です ==
移動先 "[[:$1]]" は既に存在しています。このページを移動のために削除しますか?',
'delete_and_move_confirm' => 'ページ削除の確認',
'delete_and_move_reason'  => '移動のための削除',
'selfmove'                => '移動元と移動先のページ名が同じです。自分自身へは移動できません。',
'immobile_namespace'      => '移動先のページ名は特別なページです。その名前空間にページを移動することはできません。',
'imagenocrossnamespace'   => 'ファイル用の名前空間以外にはファイルを移動することはできません。',
'imagetypemismatch'       => '新しいファイルの拡張子がファイルのタイプと一致していません。',
'imageinvalidfilename'    => '指定したファイル名が無効です',
'fix-double-redirects'    => 'このページへのリダイレクトがあればそのリダイレクトを修正する',

# Export
'export'            => 'ページデータの書き出し',
'exporttext'        => 'ここでは単独のまたは複数のページのテキストと編集履歴をXMLの形で書き出すことができます。書き出されたXML文書は他のMediaWikiで動いているウィキに[[Special:Import|インポートページ]]を使って取り込めます。

ページデータを書き出すには下のテキストボックスに書き出したいページのタイトルを一行に一ページずつ記入してください。また編集履歴とともに全ての古い版を含んで書き出すのか、最新版のみを書き出すのか選択してください。

後者のケースではリンクの形で使うこともできます。例: "[[{{MediaWiki:Mainpage}}]]"の最新版を取得するには[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]]を使用します。',
'exportcuronly'     => 'すべての履歴を含ませずに、最新版のみを書き出す',
'exportnohistory'   => "'''お知らせ:''' パフォーマンス上の理由により、このフォームによるページの完全な履歴の書き出しは行えません。",
'export-submit'     => '書き出し',
'export-addcattext' => 'カテゴリ内のページを対象に加える。 Category:',
'export-addcat'     => '追加',
'export-download'   => '書き出した結果をファイルに保存する',
'export-templates'  => 'テンプレートも含める',

# Namespace 8 related
'allmessages'               => '表示メッセージの一覧',
'allmessagesname'           => 'メッセージ名',
'allmessagesdefault'        => '既定の文章',
'allmessagescurrent'        => '現在の文章',
'allmessagestext'           => 'これは{{ns:mediawiki}}名前空間にある全てのシステムメッセージの一覧です。
もし、MediaWikiデフォルトメッセージの翻訳を行いたい場合は、[http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation]や[http://translatewiki.net?setlang=ja Betawiki]に行ってみてください。',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages''' が無効なので、このページを使うことはできません。",
'allmessagesfilter'         => 'メッセージ名フィルタ:',
'allmessagesmodified'       => '変更されたもののみを表示',

# Thumbnails
'thumbnail-more'           => '拡大',
'filemissing'              => '<i>ファイルがありません</i>',
'thumbnail_error'          => 'サムネイルの作成中にエラーが発生しました: $1',
'djvu_page_error'          => '指定ページ数はDjVuページ範囲を越えています',
'djvu_no_xml'              => 'DjVuファイルのXMLデータを取得できません',
'thumbnail_invalid_params' => 'サムネイルの指定パラメータが不正です',
'thumbnail_dest_directory' => '出力ディレクトリを作成できません',

# Special:Import
'import'                     => 'ページデータの取り込み',
'importinterwiki'            => 'Transwikiインポート',
'import-interwiki-text'      => 'インポートするウィキとページ名を選択してください。変更履歴の日付と編集者が保存されます。すべてのtranswikiは[[Special:Log/import|インポート記録]]に記録されます。',
'import-interwiki-history'   => 'このページの全ての版を複製する',
'import-interwiki-submit'    => '取り込み',
'import-interwiki-namespace' => '次の名前空間に取り込む:',
'importtext'                 => '書き出し元となるウィキから [[Special:Export|書き出し用ユーティリティ]]を使ってXMLファイルを書き出してください。あなたのコンピューターに保存した後、ここにアップロードしてください。',
'importstart'                => 'ページを取り込んでいます...',
'import-revision-count'      => '$1 版',
'importnopages'              => 'インポートするページがありません',
'importfailed'               => '取り込みに失敗しました: $1',
'importunknownsource'        => 'インポートするソースのファイルタイプが不明です',
'importcantopen'             => 'インポートするファイルを開けません',
'importbadinterwiki'         => '他ウィキへのリンクが正しくありません',
'importnotext'               => '内容が空か、テキストがありません。',
'importsuccess'              => '取り込みに成功しました。',
'importhistoryconflict'      => '取り込み時にいくつかの版が競合しました（以前に同じページを取り込んでいませんか）。',
'importnosources'            => 'Transwikiの読み込み元が定義されていないため、履歴の直接アップロードは無効になっています。',
'importnofile'               => 'ファイルがアップロードされませんでした',
'importuploaderrorsize'      => 'インポートファイルのアップロードに失敗しました。アップロード可能なファイルサイズ上限を超えています。',
'importuploaderrorpartial'   => 'インポートファイルのアップロードに失敗しました。このファイルは一部しかアップロードされていません。',
'importuploaderrortemp'      => 'インポートファイルのアップロードに失敗しました。テンポラリフォルダが見つかりません。',
'import-parse-failure'       => 'XMLの構文解析に失敗しました',
'import-noarticle'           => 'インポートするページがありません！',
'import-nonewrevisions'      => '含まれていた履歴は全て既にインポート済みです。',
'xml-error-string'           => '"$1" $2行 $3文字目（$4バイト目）: $5',
'import-upload'              => 'XMLデータをアップロード',

# Import log
'importlogpage'                    => 'インポート記録',
'importlogpagetext'                => '以下は管理者による他ウィキからのページデータの取り込み記録です。',
'import-logentry-upload'           => 'ファイルのアップロードにより [[$1]] をインポートしました',
'import-logentry-upload-detail'    => '$1 版',
'import-logentry-interwiki'        => '$1 をtranswikiしました',
'import-logentry-interwiki-detail' => '$2 の $1 版',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '自分の利用者ページ',
'tooltip-pt-anonuserpage'         => 'あなたのIPアドレス用の利用者ページ',
'tooltip-pt-mytalk'               => '自分の会話ページ',
'tooltip-pt-anontalk'             => 'あなたのIPアドレスからなされた編集の会話ページ',
'tooltip-pt-preferences'          => 'オプションの変更',
'tooltip-pt-watchlist'            => '変更を監視しているページの一覧',
'tooltip-pt-mycontris'            => '自分の投稿記録',
'tooltip-pt-login'                => 'ログインすることが推奨されますが、しなくても構いません。',
'tooltip-pt-anonlogin'            => 'ログインすることが推奨されますが、しなくても構いません。',
'tooltip-pt-logout'               => 'ログアウト',
'tooltip-ca-talk'                 => '項目のノート',
'tooltip-ca-edit'                 => 'このページを編集できます。投稿の前に「{{int:showpreview}}」ボタンを使ってください。',
'tooltip-ca-addsection'           => 'このページにコメントを加える',
'tooltip-ca-viewsource'           => 'このページは保護されています。ページのソースを閲覧できます。',
'tooltip-ca-history'              => 'このページの過去の版',
'tooltip-ca-protect'              => 'このページを保護',
'tooltip-ca-delete'               => 'このページを削除',
'tooltip-ca-undelete'             => '削除されたページを復帰する',
'tooltip-ca-move'                 => 'このページを移動',
'tooltip-ca-watch'                => 'このページをウォッチリストに追加',
'tooltip-ca-unwatch'              => 'このページをウォッチリストから外す',
'tooltip-search'                  => 'ウィキ内を検索',
'tooltip-search-go'               => '入力された文字列と一致する項目がある場合そのページへ移動します',
'tooltip-search-fulltext'         => '入力された文字列が含まれるページを検索します',
'tooltip-p-logo'                  => 'メインページ',
'tooltip-n-mainpage'              => 'メインページに移動',
'tooltip-n-portal'                => 'このプロジェクトについて、あなたのできることを探す場所です',
'tooltip-n-currentevents'         => '最近の出来事',
'tooltip-n-recentchanges'         => '最近更新が行われたページの一覧',
'tooltip-n-randompage'            => 'ランダムに記事を選んで表示',
'tooltip-n-help'                  => 'ヘルプ・使い方',
'tooltip-t-whatlinkshere'         => 'このページにリンクしているページの一覧',
'tooltip-t-recentchangeslinked'   => '最近更新が行われたこのページのリンク先',
'tooltip-feed-rss'                => 'このページのRSSフィード',
'tooltip-feed-atom'               => 'このページのAtomフィード',
'tooltip-t-contributions'         => '利用者の投稿記録',
'tooltip-t-emailuser'             => '{{int:emailuser}}',
'tooltip-t-upload'                => '画像やメディアファイルをアップロード',
'tooltip-t-specialpages'          => '特別ページの一覧',
'tooltip-t-print'                 => 'このページの印刷用バージョン',
'tooltip-t-permalink'             => 'この版への固定リンク',
'tooltip-ca-nstab-main'           => '本文を表示',
'tooltip-ca-nstab-user'           => '利用者ページを表示',
'tooltip-ca-nstab-media'          => 'メディアページを表示',
'tooltip-ca-nstab-special'        => 'これは特別ページです。編集することはできません。',
'tooltip-ca-nstab-project'        => 'プロジェクトページを表示',
'tooltip-ca-nstab-image'          => '画像ページを表示',
'tooltip-ca-nstab-mediawiki'      => 'システムメッセージを表示',
'tooltip-ca-nstab-template'       => 'テンプレートを表示',
'tooltip-ca-nstab-help'           => 'ヘルプページを表示',
'tooltip-ca-nstab-category'       => 'カテゴリページを表示',
'tooltip-minoredit'               => 'この編集を細部の変更とマーク',
'tooltip-save'                    => '編集を保存します。',
'tooltip-preview'                 => '編集結果を確認します。保存前に是非使用してください。',
'tooltip-diff'                    => 'あなたが編集した版の変更点を表示します。',
'tooltip-compareselectedversions' => '選択された二つの版の差分を表示します。',
'tooltip-watch'                   => 'このページをウォッチリストへ追加します。',
'tooltip-recreate'                => 'このままこのページを新規作成する',
'tooltip-upload'                  => 'アップロードを開始',

# Stylesheets
'common.css'   => '/* ここに書いた CSS は全ての外装に反映されます */',
'monobook.css' => '/* ここに書いた CSS は Monobook 外装に反映されます */',

# Scripts
'common.js'   => '/* ここに書いた JavaScript は全てのページ上で実行されます */',
'monobook.js' => '/* こちらは廃止されました; [[MediaWiki:Common.js]]をお使いください */',

# Metadata
'nodublincore'      => 'このサーバーでは Dublin Core RDF メタデータが許可されていません。',
'nocreativecommons' => 'このサーバーではクリエイティブ・コモンズの RDF メタデータが許可されていません。',
'notacceptable'     => 'ウィキサーバーはあなたの使用しているクライアントが読める形式で情報を提供できません。',

# Attribution
'anonymous'        => '{{SITENAME}}の匿名利用者',
'siteuser'         => '{{SITENAME}}の利用者$1',
'lastmodifiedatby' => '最終更新は $3 による $2, $1 の編集です。', # $1 date, $2 time, $3 user
'othercontribs'    => '$1の版に基づきます。',
'others'           => 'その他の利用者',
'siteusers'        => '{{SITENAME}}の利用者$1',
'creditspage'      => 'ページ・クレジット',
'nocredits'        => 'このページには有効なクレジット情報がありません。',

# Spam protection
'spamprotectiontitle' => 'スパム防御フィルター',
'spamprotectiontext'  => 'あなたが保存しようとしたページはスパム・フィルターによって保存をブロックされました。これは主に外部サイトへのリンクが原因です。',
'spamprotectionmatch' => '以下はスパム・フィルターによって検出されたテキストです: $1',
'spambot_username'    => 'MediaWiki スパム除去',
'spam_reverting'      => '$1 へのリンクを含まない以前の版に差し戻し',
'spam_blanking'       => 'すべての版から $1 へのリンクを削除',

# Info page
'infosubtitle'   => 'ページ情報',
'numedits'       => '編集数（項目）: $1',
'numtalkedits'   => '編集数（ノート）: $1',
'numwatchers'    => 'ウォッチしている利用者数: $1',
'numauthors'     => '投稿者数（項目）: $1',
'numtalkauthors' => '投稿者数（ノート）: $1',

# Math options
'mw_math_png'    => '常にPNG',
'mw_math_simple' => 'シンプルな数式はHTML、それ以外はPNG',
'mw_math_html'   => 'できる限りHTML、さもなければPNG',
'mw_math_source' => 'TeXのままにする (テキストブラウザ向け)',
'mw_math_modern' => '最近のブラウザで推奨',
'mw_math_mathml' => '可能ならばMathMLを使う (実験中の機能)',

# Patrolling
'markaspatrolleddiff'                 => 'パトロール済みにする',
'markaspatrolledtext'                 => 'この項目をパトロール済みにする',
'markedaspatrolled'                   => 'パトロール済みにしました。',
'markedaspatrolledtext'               => '選択された編集をパトロール済みにしました。',
'rcpatroldisabled'                    => 'RCパトロールが無効です',
'rcpatroldisabledtext'                => '最近更新されたページのパトロール機能は現在無効になっています。',
'markedaspatrollederror'              => 'パトロール済みにできません。',
'markedaspatrollederrortext'          => 'パトロール済みにするためにはどの版かを指定する必要があります。',
'markedaspatrollederror-noautopatrol' => '自分自身による編集をパトロール済みにする権限がありません。',

# Patrol log
'patrol-log-page'   => 'パトロール記録',
'patrol-log-header' => '以下はパトロールされた版の記録です。',
'patrol-log-line'   => '$2 の $1 をパトロール済みにマーク$3',
'patrol-log-auto'   => '（自動）',
'patrol-log-diff'   => '第$1版',

# Image deletion
'deletedrevision'                 => '古い版 $1 を削除しました',
'filedeleteerror-short'           => 'ファイル削除エラー: $1',
'filedeleteerror-long'            => '$1 の削除中にエラーが発生しました',
'filedelete-missing'              => 'ファイル"$1"は存在しないため、削除することができません。',
'filedelete-old-unregistered'     => '指定されたファイルの "$1" 版はデータベースにありません。',
'filedelete-current-unregistered' => '指定されたファイル"$1"はデータベース内にはありません。',
'filedelete-archive-read-only'    => 'ログ用ディレクトリ "$1" は、ウェブサーバーにより書き込み不可となっています。',

# Browsing diffs
'previousdiff' => '←前の差分',
'nextdiff'     => '次の差分→',

# Media information
'mediawarning'         => "'''警告:''' このファイルは悪意のあるコードを含んでいる可能性があり、実行するとコンピューターが危害を被る場合があります。
----",
'imagemaxsize'         => 'ファイルページで表示する画像の最大サイズ:',
'thumbsize'            => 'サムネイルの大きさ:',
'widthheightpage'      => '$1×$2, $3 ページ',
'file-info'            => '(ファイルサイズ: $1, MIMEタイプ: $2)',
'file-info-size'       => '($1 × $2 ピクセル, ファイルサイズ: $3, MIMEタイプ: $4)',
'file-nohires'         => '<small>高精細度の画像はありません。</small>',
'svg-long-desc'        => '(SVGファイル, $1 × $2 ピクセル, ファイルサイズ: $3)',
'show-big-image'       => '高解像度での画像',
'show-big-image-thumb' => '<small>このプレビューのサイズ: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => '新規画像展示室',
'imagelisttext'         => "'''$1''' 個のファイルを $2 に表示しています",
'newimages-summary'     => 'この特別ページでは最近、アップロードされたファイルを表示します。',
'showhidebots'          => '（ボットを$1）',
'noimages'              => '画像がありません。',
'ilsubmit'              => '検索',
'bydate'                => '日付順',
'sp-newimages-showfrom' => '$1 $2 以後現在までの新着ファイルを表示',

# Bad image list
'bad_image_list' => '書式は以下の通りです：

リストアイテム（ * で始まる行）のみであることが考慮されています。各行最初のリンクは、好ましくないファイルへのリンクとしてください。
各行2番目以降のリンクはその限りではありません。要は、ファイルへのリンクを列挙することです。',

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => '簡体',
'variantname-zh-hant' => '正字',
'variantname-zh-cn'   => '簡体（中国）',
'variantname-zh-tw'   => '正字（台湾）',
'variantname-zh-hk'   => '正字（香港）',
'variantname-zh-sg'   => '簡体（シンガポール）',
'variantname-zh'      => '無変換',

# Metadata
'metadata'          => 'メタデータ',
'metadata-help'     => 'このファイルはデジタルカメラ・スキャナなどが付加した追加情報を含んでいます。このファイルがオリジナルの状態から変更されている場合、いくつかの項目は変更を完全に反映していないかもしれません。',
'metadata-expand'   => '拡張項目を表示',
'metadata-collapse' => '拡張項目を隠す',
'metadata-fields'   => 'ここに挙げたEXIF情報のフィールドのみが標準で表示されます。
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => '画像の幅',
'exif-imagelength'                 => '画像の高さ',
'exif-bitspersample'               => 'ビット深度',
'exif-compression'                 => '圧縮形式',
'exif-photometricinterpretation'   => '画素構成',
'exif-orientation'                 => '画像方向',
'exif-samplesperpixel'             => 'コンポーネント数',
'exif-planarconfiguration'         => 'データ格納形式',
'exif-ycbcrsubsampling'            => 'YCCの画素構成（Cの間引き率）',
'exif-ycbcrpositioning'            => 'YCCの画素構成（YとCの位置）',
'exif-xresolution'                 => '水平解像度',
'exif-yresolution'                 => '垂直解像度',
'exif-resolutionunit'              => '解像度の単位',
'exif-stripoffsets'                => '画像データの場所',
'exif-rowsperstrip'                => 'ストリップのライン数',
'exif-stripbytecounts'             => 'ストリップのデータ量',
'exif-jpeginterchangeformat'       => 'JPEGのSOIへのオフセット',
'exif-jpeginterchangeformatlength' => 'JPEGデータのバイト数',
'exif-transferfunction'            => '再生階調カーブ特性',
'exif-whitepoint'                  => '参照白色点の色度座標値',
'exif-primarychromaticities'       => '原色の色度座標値',
'exif-ycbcrcoefficients'           => '色変換マトリックス係数',
'exif-referenceblackwhite'         => '参照黒色点値・参照白色点値',
'exif-datetime'                    => 'ファイル変更日時',
'exif-imagedescription'            => '画像の説明',
'exif-make'                        => '画像入力機器のメーカー',
'exif-model'                       => '画像入力機器の機種',
'exif-software'                    => 'ファームウェアのバージョン',
'exif-artist'                      => '作成者',
'exif-copyright'                   => '著作権者',
'exif-exifversion'                 => 'Exifバージョン',
'exif-flashpixversion'             => '対応フラッシュピックスバージョン',
'exif-colorspace'                  => '色空間',
'exif-componentsconfiguration'     => '各コンポーネントの構成',
'exif-compressedbitsperpixel'      => '画像圧縮モード',
'exif-pixelydimension'             => '実効画像幅',
'exif-pixelxdimension'             => '実効画像高さ',
'exif-makernote'                   => 'メーカーノート',
'exif-usercomment'                 => '利用者のコメント',
'exif-relatedsoundfile'            => '関連音声ファイル',
'exif-datetimeoriginal'            => '画像データ生成日時',
'exif-datetimedigitized'           => 'デジタルデータ作成日時',
'exif-subsectime'                  => 'ファイル変更日時 (秒未満)',
'exif-subsectimeoriginal'          => '画像データ生成日時 (秒未満)',
'exif-subsectimedigitized'         => 'デジタルデータ作成日時 (秒未満)',
'exif-exposuretime'                => '露出時間',
'exif-exposuretime-format'         => '$1秒 ($2)',
'exif-fnumber'                     => 'F値',
'exif-exposureprogram'             => '露出プログラム',
'exif-spectralsensitivity'         => 'スペクトル感度',
'exif-isospeedratings'             => 'ISOスピードレート',
'exif-oecf'                        => '光電変換関数',
'exif-shutterspeedvalue'           => 'シャッタースピード',
'exif-aperturevalue'               => '絞り値',
'exif-brightnessvalue'             => '明るさ',
'exif-exposurebiasvalue'           => '露出補正値',
'exif-maxaperturevalue'            => 'レンズ最小F値',
'exif-subjectdistance'             => '被写体距離',
'exif-meteringmode'                => '測光方式',
'exif-lightsource'                 => '光源',
'exif-flash'                       => 'フラッシュ',
'exif-focallength'                 => 'レンズの焦点距離',
'exif-subjectarea'                 => '主要被写体の位置',
'exif-flashenergy'                 => 'フラッシュ強度',
'exif-spatialfrequencyresponse'    => '空間周波数応答',
'exif-focalplanexresolution'       => '水平方向の焦点面解像度',
'exif-focalplaneyresolution'       => '垂直方向の焦点面解像度',
'exif-focalplaneresolutionunit'    => '焦点面解像度の単位',
'exif-subjectlocation'             => '被写体の場所',
'exif-exposureindex'               => '露出インデックス',
'exif-sensingmethod'               => 'センサー方式',
'exif-filesource'                  => 'ファイルソース',
'exif-scenetype'                   => 'シーンタイプ',
'exif-cfapattern'                  => 'CFAパターン',
'exif-customrendered'              => '画像処理',
'exif-exposuremode'                => '露出モード',
'exif-whitebalance'                => 'ホワイトバランス',
'exif-digitalzoomratio'            => 'デジタルズーム倍率',
'exif-focallengthin35mmfilm'       => 'レンズの焦点距離（35mmフィルム換算）',
'exif-scenecapturetype'            => '被写体の種別',
'exif-gaincontrol'                 => 'ゲインコントロール',
'exif-contrast'                    => 'コントラスト',
'exif-saturation'                  => '彩度',
'exif-sharpness'                   => 'シャープネス',
'exif-devicesettingdescription'    => '機器設定',
'exif-subjectdistancerange'        => '被写体距離の範囲',
'exif-imageuniqueid'               => 'ユニーク画像ID',
'exif-gpsversionid'                => 'GPSタグのバージョン',
'exif-gpslatituderef'              => '北緯/南緯',
'exif-gpslatitude'                 => '緯度',
'exif-gpslongituderef'             => '東経/西経',
'exif-gpslongitude'                => '経度',
'exif-gpsaltituderef'              => '高度の基準',
'exif-gpsaltitude'                 => '高度',
'exif-gpstimestamp'                => 'GPS時刻（原子時計）',
'exif-gpssatellites'               => '測位に用いた衛星信号',
'exif-gpsstatus'                   => 'GPS受信機の状態',
'exif-gpsmeasuremode'              => 'GPS測位方法',
'exif-gpsdop'                      => '測位精度',
'exif-gpsspeedref'                 => '速度の単位',
'exif-gpsspeed'                    => '速度',
'exif-gpstrackref'                 => '進行方向の基準',
'exif-gpstrack'                    => '進行方向',
'exif-gpsimgdirectionref'          => '撮影方向の基準',
'exif-gpsimgdirection'             => '撮影方向',
'exif-gpsmapdatum'                 => '測地系',
'exif-gpsdestlatituderef'          => '目的地の北緯/南緯',
'exif-gpsdestlatitude'             => '目的地の緯度',
'exif-gpsdestlongituderef'         => '目的地の東経/西経',
'exif-gpsdestlongitude'            => '目的地の経度',
'exif-gpsdestbearingref'           => '目的地の方角の基準',
'exif-gpsdestbearing'              => '目的地の方角',
'exif-gpsdestdistanceref'          => '目的地までの距離の単位',
'exif-gpsdestdistance'             => '目的地までの距離',
'exif-gpsprocessingmethod'         => 'GPS処理方法',
'exif-gpsareainformation'          => 'GPSエリア名',
'exif-gpsdatestamp'                => 'GPS測位日時',
'exif-gpsdifferential'             => 'ディファレンシャル補正',

# EXIF attributes
'exif-compression-1' => '非圧縮',
'exif-compression-6' => 'JPEG圧縮',

'exif-unknowndate' => '不明な日付',

'exif-orientation-1' => '通常', # 0th row: top; 0th column: left
'exif-orientation-2' => '左右反転', # 0th row: top; 0th column: right
'exif-orientation-3' => '180°回転', # 0th row: bottom; 0th column: right
'exif-orientation-4' => '上下反転', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '反時計回りに90°回転 上下反転', # 0th row: left; 0th column: top
'exif-orientation-6' => '時計回りに90°回転', # 0th row: right; 0th column: top
'exif-orientation-7' => '時計回りに90°回転 上下反転', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '反時計回りに90°回転', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => '点順次フォーマット',
'exif-planarconfiguration-2' => '面順次フォーマット',

'exif-colorspace-ffff.h' => 'その他',

'exif-componentsconfiguration-0' => 'なし',

'exif-exposureprogram-0' => '未定義',
'exif-exposureprogram-1' => 'マニュアル',
'exif-exposureprogram-2' => 'ノーマルプログラム',
'exif-exposureprogram-3' => '露出優先',
'exif-exposureprogram-4' => 'シャッター速度優先',
'exif-exposureprogram-5' => 'クリエイティブ・プログラム',
'exif-exposureprogram-6' => 'アクション・プログラム',
'exif-exposureprogram-7' => 'ポートレイトモード（近景）',
'exif-exposureprogram-8' => 'ランドスケープモード（遠景）',

'exif-subjectdistance-value' => '$1 メートル',

'exif-meteringmode-0'   => '不明',
'exif-meteringmode-1'   => '平均',
'exif-meteringmode-2'   => '中央重点',
'exif-meteringmode-3'   => 'スポット',
'exif-meteringmode-4'   => 'マルチスポット',
'exif-meteringmode-5'   => '分割測光',
'exif-meteringmode-6'   => '部分測光',
'exif-meteringmode-255' => 'その他',

'exif-lightsource-0'   => '不明',
'exif-lightsource-1'   => '昼光',
'exif-lightsource-2'   => '蛍光灯',
'exif-lightsource-3'   => 'タングステン（白熱灯）',
'exif-lightsource-4'   => 'フラッシュ',
'exif-lightsource-9'   => '晴天',
'exif-lightsource-10'  => '曇天',
'exif-lightsource-11'  => '日陰',
'exif-lightsource-12'  => '昼光色蛍光灯 (D 5700 - 7100K)',
'exif-lightsource-13'  => '昼白色蛍光灯 (N 4600 - 5400K)',
'exif-lightsource-14'  => '白色蛍光灯 (W 3900 - 4500K)',
'exif-lightsource-15'  => '温白色蛍光灯 (WW 3200 - 3700K)',
'exif-lightsource-17'  => '標準光A',
'exif-lightsource-18'  => '標準光B',
'exif-lightsource-19'  => '標準光C',
'exif-lightsource-24'  => 'ISOスタジオタングステン',
'exif-lightsource-255' => 'その他',

'exif-focalplaneresolutionunit-2' => 'インチ',

'exif-sensingmethod-1' => '未定義',
'exif-sensingmethod-2' => '単板カラーセンサー',
'exif-sensingmethod-3' => '2板カラーセンサー',
'exif-sensingmethod-4' => '3板カラーセンサー',
'exif-sensingmethod-5' => '色順次カラーセンサー',
'exif-sensingmethod-7' => '3線リニアセンサー',
'exif-sensingmethod-8' => '色順次リニアセンサー',

'exif-filesource-3' => 'デジタルスチルカメラ',

'exif-scenetype-1' => '直接撮影された画像',

'exif-customrendered-0' => '通常',
'exif-customrendered-1' => 'カスタム',

'exif-exposuremode-0' => '自動',
'exif-exposuremode-1' => 'マニュアル',
'exif-exposuremode-2' => 'オートブラケット',

'exif-whitebalance-0' => '自動',
'exif-whitebalance-1' => 'マニュアル',

'exif-scenecapturetype-0' => '標準',
'exif-scenecapturetype-1' => '風景',
'exif-scenecapturetype-2' => '人物',
'exif-scenecapturetype-3' => '夜景',

'exif-gaincontrol-0' => 'なし',
'exif-gaincontrol-1' => '弱増感',
'exif-gaincontrol-2' => '強増感',
'exif-gaincontrol-3' => '弱減感',
'exif-gaincontrol-4' => '強減感',

'exif-contrast-0' => '標準',
'exif-contrast-1' => '軟調',
'exif-contrast-2' => '硬調',

'exif-saturation-0' => '標準',
'exif-saturation-1' => '低彩度',
'exif-saturation-2' => '高彩度',

'exif-sharpness-0' => '標準',
'exif-sharpness-1' => '弱',
'exif-sharpness-2' => '強',

'exif-subjectdistancerange-0' => '不明',
'exif-subjectdistancerange-1' => 'マクロ',
'exif-subjectdistancerange-2' => '近景',
'exif-subjectdistancerange-3' => '遠景',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => '北緯',
'exif-gpslatitude-s' => '南緯',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => '東経',
'exif-gpslongitude-w' => '西経',

'exif-gpsstatus-a' => '測位中',
'exif-gpsstatus-v' => '未測位',

'exif-gpsmeasuremode-2' => '2次元測位',
'exif-gpsmeasuremode-3' => '3次元測位',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'キロメートル毎時',
'exif-gpsspeed-m' => 'マイル毎時',
'exif-gpsspeed-n' => 'ノット',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => '真方位',
'exif-gpsdirection-m' => '磁方位',

# External editor support
'edit-externally'      => '外部アプリケーションを使ってこのファイルを編集する',
'edit-externally-help' => '詳しい情報は[http://www.mediawiki.org/wiki/Manual:External_editors 外部エディタに関する説明（英語）]をご覧ください。',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'すべて',
'imagelistall'     => 'すべて',
'watchlistall2'    => 'すべて',
'namespacesall'    => 'すべて',
'monthsall'        => 'すべて',

# E-mail address confirmation
'confirmemail'             => 'メールアドレスの確認',
'confirmemail_noemail'     => '[[Special:Preferences|オプション設定]]で有効なメールアドレスが指定されていません。',
'confirmemail_text'        => 'このウィキではメール通知を受け取る前にメールアドレスの確認が必要です。以下のボタンを押すと「{{int:Confirmemail_subject}}」という件名の確認メールがあなたのメールアドレスに送られます。メールには確認用コードを含むリンクが書かれています。そのリンクを開くことによってメールアドレスの正当性が確認されます。',
'confirmemail_pending'     => '<div class="error">
確認メールは既に送信されています。あなたがこのアカウントを作成したばかりであれば、数分待って既にメールが送信されていないかを確かめてください。
</div>',
'confirmemail_send'        => '確認用コードを送信する',
'confirmemail_sent'        => '確認メールを送信しました。',
'confirmemail_oncreate'    => 'メールアドレスの正当性を確認するためのコードを含んだメールを送信しました。この確認を行わなくてもログインはできますが、確認するまでメール通知の機能は無効化されます。',
'confirmemail_sendfailed'  => '{{SITENAME}}からの確認メールを送信できませんでした。メールアドレスに不正な文字が含まれていないかどうか確認してください。

メールサーバーからの返答: $1',
'confirmemail_invalid'     => '確認用コードが正しくありません。このコードは期限切れです。',
'confirmemail_needlogin'   => 'メールアドレスを確認するために$1が必要です。',
'confirmemail_success'     => 'あなたのメールアドレスは確認されました。ログインしてウィキを使用できます。',
'confirmemail_loggedin'    => 'あなたのメールアドレスは確認されました。',
'confirmemail_error'       => 'あなたの確認を保存する際に内部エラーが発生しました。',
'confirmemail_subject'     => '{{SITENAME}} メールアドレスの確認',
'confirmemail_body'        => 'どなたか（IPアドレス $1 の使用者）がこのメールアドレスを
{{SITENAME}} のアカウント "$2" に登録しました。

このアカウントがあなたのものであるか確認してください。
あなたの登録したアカウントであるならば、{{SITENAME}}
のメール通知機能を有効にするために、以下のURLにアクセスしてください:

$3

もし {{SITENAME}} について身に覚えがない場合は、リンクを開かず
次のURLにアクセスしてメール登録を解除ください:

$5

確認用コードは $4 に期限切れになります。
-- 
{{SITENAME}}
{{SERVER}}/',
'confirmemail_invalidated' => 'メールアドレスの認証がキャンセルされました',
'invalidateemail'          => 'メールアドレスの認証をキャンセルする',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki transcluding は無効になっています]',
'scarytranscludefailed'   => '[テンプレート $1 の取得に失敗しました]',
'scarytranscludetoolong'  => '[URLが長すぎます]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
この項目へのトラックバック:
$1
</div>',
'trackbackremove'   => ' ([$1 削除])',
'trackbacklink'     => 'トラックバック',
'trackbackdeleteok' => 'トラックバックを削除しました。',

# Delete conflict
'deletedwhileediting' => "'''警告:''' このページはあなたが編集し始めた後、削除されました!",
'confirmrecreate'     => "あなたがこのページを編集し始めた後に、このページは[[User:$1|$1]] ([[User talk:$1|会話]]) によって削除されました。その理由は次の通りです:
: ''$2''
このままこのページを新規作成して良いか確認してください。",
'recreate'            => '新規作成する',

# HTML dump
'redirectingto' => '[[:$1]]へ転送しています...',

# action=purge
'confirm_purge'        => 'ページのキャッシュを破棄します。よろしいですか?

$1',
'confirm_purge_button' => 'はい',

# AJAX search
'searchcontaining' => "'''$1''' を含むページの検索。",
'searchnamed'      => "ページ名が '''$1''' の項目の検索。",
'articletitles'    => "''$1'' からはじまる項目",
'hideresults'      => '結果を隠す',
'useajaxsearch'    => 'Ajax による検索を利用する',

# Multipage image navigation
'imgmultipageprev' => '&larr; 前ページ',
'imgmultipagenext' => '次ページ &rarr;',
'imgmultigo'       => '表示',
'imgmultigoto'     => '$1へ行く',

# Table pager
'ascending_abbrev'         => '昇順',
'descending_abbrev'        => '降順',
'table_pager_next'         => '次のページ',
'table_pager_prev'         => '前のページ',
'table_pager_first'        => '最初のページ',
'table_pager_last'         => '最後のページ',
'table_pager_limit'        => '1ページに $1 個表示',
'table_pager_limit_submit' => '実行',
'table_pager_empty'        => '結果なし',

# Auto-summaries
'autosumm-blank'   => 'ページの白紙化',
'autosumm-replace' => "ページの置換: '$1'",
'autoredircomment' => '[[$1]]へのリダイレクト',
'autosumm-new'     => "新しいページ: '$1'",

# Size units
'size-bytes'     => '$1 バイト',
'size-kilobytes' => '$1 キロバイト',
'size-megabytes' => '$1 メガバイト',
'size-gigabytes' => '$1 ギガバイト',

# Live preview
'livepreview-loading' => '読み込み中…',
'livepreview-ready'   => '読み込み中… 完了',
'livepreview-failed'  => 'ライブプレビューが失敗しました。
通常のプレビューを試みてください。',
'livepreview-error'   => '接続に失敗しました: $1 "$2"
通常のプレビューを試みてください。',

# Friendlier slave lag warnings
'lag-warn-normal' => 'この一覧には$1秒前までの編集が反映されていない可能性があります。',
'lag-warn-high'   => 'データベースサーバの負荷のため同期が遅れています。この一覧には$1秒前までの編集が反映されていない可能性があります。',

# Watchlist editor
'watchlistedit-numitems'       => 'あなたのウォッチリストには $1タイトルが登録されています（ノートページもあわせて登録されています）。',
'watchlistedit-noitems'        => 'あなたのウォッチリストには、現在タイトルがありません。',
'watchlistedit-normal-title'   => 'ウォッチリストの編集',
'watchlistedit-normal-legend'  => 'ウォッチリストからタイトルを削除',
'watchlistedit-normal-explain' => 'あなたのウォッチリストにあるタイトルが以下に表示されています。タイトルの横にあるチェックボックスにチェックを入れ、「{{int:watchlistedit-normal-submit}}」を選べば削除できます。また、[[Special:Watchlist/raw|一覧をテキストで編集]]することもできます。',
'watchlistedit-normal-submit'  => 'タイトルの削除',
'watchlistedit-normal-done'    => 'あなたのウォッチリストから $1 タイトルを削除しました:',
'watchlistedit-raw-title'      => 'ウォッチリストをテキストで編集',
'watchlistedit-raw-legend'     => 'ウォッチリストをテキストで編集',
'watchlistedit-raw-explain'    => 'あなたのウォッチリストにあるタイトルが以下に表示されています。1行につき1つのタイトルを表し、タイトルを追加・削除することにより編集できます。編集を反映させる時は "{{int:Watchlistedit-raw-submit}}" を選びます。この編集方法の他に、[[Special:Watchlist/edit|標準的なエディタ]]も利用できます。',
'watchlistedit-raw-titles'     => 'タイトル:',
'watchlistedit-raw-submit'     => 'ウォッチリストを更新',
'watchlistedit-raw-done'       => 'あなたのウォッチリストを更新しました。',
'watchlistedit-raw-added'      => '$1 タイトルを追加しました:',
'watchlistedit-raw-removed'    => '$1 タイトルを削除しました:',

# Watchlist editing tools
'watchlisttools-view' => 'ウォッチリストの確認',
'watchlisttools-edit' => 'ウォッチリストの編集',
'watchlisttools-raw'  => 'ウォッチリストをテキストで編集',

# Iranian month names
'iranian-calendar-m1'  => 'イラン歴第1月',
'iranian-calendar-m2'  => 'イラン歴第2月',
'iranian-calendar-m3'  => 'イラン歴第3月',
'iranian-calendar-m4'  => 'イラン歴第4月',
'iranian-calendar-m5'  => 'イラン歴第5月',
'iranian-calendar-m6'  => 'イラン歴第6月',
'iranian-calendar-m7'  => 'イラン歴第7月',
'iranian-calendar-m8'  => 'イラン歴第8月',
'iranian-calendar-m9'  => 'イラン歴第9月',
'iranian-calendar-m10' => 'イラン歴第10月',
'iranian-calendar-m11' => 'イラン歴第11月',
'iranian-calendar-m12' => 'イラン歴第12月',

# Core parser functions
'unknown_extension_tag' => '拡張機能タグ "$1" は登録されていません',

# Special:Version
'version'                          => 'バージョン情報', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'インストール済み拡張機能',
'version-specialpages'             => '特別ページ',
'version-parserhooks'              => 'パーサーフック',
'version-variables'                => '変数',
'version-other'                    => 'その他',
'version-mediahandlers'            => 'メディアハンドラ',
'version-hooks'                    => 'フック',
'version-extension-functions'      => '拡張機能関数',
'version-parser-extensiontags'     => 'パーサー拡張機能タグ',
'version-parser-function-hooks'    => 'パーサー関数フック',
'version-skin-extension-functions' => 'スキン拡張機能関数',
'version-hook-name'                => 'フック名',
'version-hook-subscribedby'        => 'フックが記述されている場所',
'version-version'                  => 'バージョン',
'version-license'                  => 'ライセンス',
'version-software'                 => 'インストール済みソフトウェア',
'version-software-product'         => 'ソフトウェア名',
'version-software-version'         => 'バージョン',

# Special:FilePath
'filepath'         => 'パスの取得',
'filepath-page'    => 'ファイル名:',
'filepath-submit'  => 'パスを取得',
'filepath-summary' => 'この特別ページは、ファイルへの完全なパスを返します。
画像は最大解像度で表示され、他のファイルタイプでは関連付けられたプログラムが直接起動します。

ファイル名は"{{ns:image}}:"を付けずに入力してください。',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => '重複ファイルの検索',
'fileduplicatesearch-summary'  => '重複ファイルを、ファイルのハッシュ値に基づいて検索します。

先頭の「{{ns:image}}:」を付けないでファイル名を入力してください。',
'fileduplicatesearch-legend'   => '重複の検索',
'fileduplicatesearch-filename' => 'ファイル名:',
'fileduplicatesearch-submit'   => '検索',
'fileduplicatesearch-info'     => '$1 × $2 ピクセル<br />ファイルサイズ: $3<br />MIMEタイプ: $4',
'fileduplicatesearch-result-1' => '「$1」と重複するファイルはありません。',
'fileduplicatesearch-result-n' => '「$1」と重複するファイルが $2 個あります。',

# Special:SpecialPages
'specialpages'                   => '特別ページ',
'specialpages-note'              => '----
* このように示したものは誰でも利用できる通常の特別ページです。
* <span class="mw-specialpagerestricted">このように示したものは一定の権限がなければ利用できない特別ページです。</span>',
'specialpages-group-maintenance' => 'ページのメンテナンス情報',
'specialpages-group-other'       => 'その他特別ページ',
'specialpages-group-login'       => 'ログイン / 利用者登録',
'specialpages-group-changes'     => '最近更新したページとログ',
'specialpages-group-media'       => 'メディア情報とアップロード',
'specialpages-group-users'       => '利用者と権限',
'specialpages-group-highuse'     => 'よく利用されているページ',
'specialpages-group-pages'       => 'ページのリスト',
'specialpages-group-pagetools'   => 'ページツール',
'specialpages-group-wiki'        => 'このサイトのウィキに関する情報とツール',
'specialpages-group-redirects'   => 'リダイレクトになっている特別ページ',
'specialpages-group-spam'        => 'スパム対策ツール',

# Special:BlankPage
'blankpage'              => '白紙ページ',
'intentionallyblankpage' => 'このページは意図的に白紙にされています',

);
