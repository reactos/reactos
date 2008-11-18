<?php # $Id: lang_ja.inc.php 511 2005-09-30 03:51:47Z elf2000 $

/**
 *  @version $Revision$
 *  @author Tadashi Jokagi <elf2000@users.sourceforge.net>
 *  EN-Revision: r446
 */

@define('PLUGIN_EVENT_SPAMBLOCK_TITLE', 'スパムプロテクター');
@define('PLUGIN_EVENT_SPAMBLOCK_DESC', 'コメントスパムを防止するさまざまな方法です。');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', 'スパム防止: 無効なメッセージです。');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', 'スパム防止: コメントした直後にコメントすることはできません。');

@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', 'このブログは「緊急コメント遮断モード」です。別の機会に来てください。');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', '複製コメントを許可しない');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', 'ユーザーが既に送信したコメントと同じ内容のコメントを送信することを許可しません。');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', '緊急時のコメント遮断する');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', '一時的にすべてのエントリに対してコメントを無効にします。ウェブログがスパム攻撃の下にある場合、役立ちます。');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'IP ブロックの間隔');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', 'Only allow an IP to submit a comment every n minutes. コメントの氾濫を防ぐのに有用です。');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Captcha を有効にする');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', 'Will force the user to input a random string displayed in a specially crafted image. This will disallow automated submits to your blog. Please remember that people with decreased vision may find it hard to read those captchas.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', '自動化ロボットからのコメントスパムを防ぐために、画像の下の入力ボックスに適切な文字列を入力してください。文字列が一致する場合のみ、コメントが送信されるでしょう。ブラウザーがクッキーをサポートし受け入れることを確認してください。さもなければ、コメントを正確に確認することができません。');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', '入力ボックスにここに見える文字列を入力してください!');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', '上のスパム予防画像から文字列を入力してください: ');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', 'スパム防止画像ボックスで表示された文字列を正確に入力ていませんでした。画像を見て、そこに表示された値を入力してください。');

@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', 'Captcha はサーバーで無効化されています。PHP 向けに GD ライブラリと freetype ライブラリがコンパイルされていることと、TTF ファイルがディレクトリに存在している必要があります。');

@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', '何日経過したら captcha を強制しますか?');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', 'Captchas can be enforced depending on the age of your articles. Enter the amount of days after which entering a correct captcha is necessary. もし 0 に設定すると、captcha は常に使用されるでしょう。');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', '何日経過したらモデレートを強制しますか?');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', 'エントリの全てのコメントにモデレートを自動設定できます。Enter the age of an entry in days, after which it should be auto-moderated. 0 は自動モデレーションをしないことを意味します。');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', 'How many links before a comment gets moderated');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', 'When a comment reaches a certain amount of links, that comment can be set to be moderated. 0 means that no link-checking is done.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', 'How many links before a comment gets rejected');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', 'When a comment reaches a certain amount of links, that comment can be set to be rejected. 0 means that no link-checking is done.');

@define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', 'いくつかの条件のために、コメントはこのウェブログの所有者によってモデレートを要求するように記録されました。');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'captcha の背景色');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'RGB 値を入力します: 0,255,255');

@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', 'ログファイルの位置');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', '拒絶/モデレートした投稿に関する情報をログファイルに記録することができます。ログ記録を無効にしたい場合、空の文字列に設定してください。');

@define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', 'コメント緊急バリケード');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', '複製コメント');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'IP ブロック');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_RBL', 'RBL ブロック');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_SURBL', 'SURBL ブロック');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', '無効な captcha (入力値: %s, 期待値: %s)');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'X 日後は自動モデレートする');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', 'コメントしたユーザーの電子メールアドレスを隠す');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', 'コメントユーザーの電子メールアドレスを表示しなくなるでしょう。');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', '電子メールアドレスは表示せず、電子メールの通告にのみ使用します。');

@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', 'ログ記録方法を選択する');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', '拒絶したコメントのログ記録はデータベース、あるいは平文テキストファイルへ行うことができます。');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', 'ファイル (下の「ログファイル」オプションを参照)');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', 'データベース');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', 'ログ記録しない');

@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'API によってコメントを作成する方法');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'これは、API コールにより作成されたコメントのモデレーションに影響します (Trackback、WFW:commentAPI コメント). 「モデレート」に設定した場合、すべてのこれらのコメントは常にまず承認される必要があります。もし「拒否」に設定した場合、完全に不許可です。もし「なし」に設定した場合、コメントは通常のコメントとして扱われるでしょう。');
@define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', 'モデレート');
@define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', '拒否');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', '(トラックバックに似た)API でのコメント作成を許可しません');

@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', '単語フィルターを有効にする');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', 'ある文字列のコメントを検索し、スパムとして記録します。');

@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', 'URL の単語フィルター');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', '正規表現が許可されており、セミコロン(;)で文字列を区切ります。');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', '著者名の単語フィルター');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', '正規表現が許可されており、セミコロン(;)で文字列を区切ります。');

@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL', '正しくない電子メールアドレスです。');
@define('PLUGIN_EVENT_SPAMBLOCK_CHECKMAIL', '電子メールアドレスの確認をしますか?');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS', 'どのコメント項目を要求しますか?');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS_DESC', 'ユーザーコメントで埋める必要がある必須項目の一覧を入力します。
複数の項目は「,(半角カンマ)」で区切ります。有効なキーは次のとおり(カッコは意味): name(名前), email(電子メール), url(URL), replyTo(返信元), comment(コメント)');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD', '項目「%s」を指定していません!');

/* vim: set sts=4 ts=4 expandtab : */
?>
