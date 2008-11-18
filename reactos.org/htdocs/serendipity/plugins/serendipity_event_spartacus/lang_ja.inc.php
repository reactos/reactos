<?php # $Id: lang_ja.inc.php 511 2005-09-30 03:51:47Z elf2000 $

/**
 *  @version $Revision: 1.3 $
 *  @author Tadashi Jokagi <elf2000@users.sourceforge.net>
 *  EN-Revision: 394
 */

@define('PLUGIN_EVENT_SPARTACUS_NAME', 'Spartacus');
@define('PLUGIN_EVENT_SPARTACUS_DESC', '[S]erendipity [P]lugin [A]ccess [R]epository [T]ool [A]nd [C]ustomization/[U]nification [S]ystem - オンラインリポジトリからプラグインのダウンロードを可能にします。');
@define('PLUGIN_EVENT_SPARTACUS_FETCH', '新しい%sを Serendipity オンラインリポジトリから取得する');
@define('PLUGIN_EVENT_SPARTACUS_FETCHERROR', 'URL %s にアクセスできませんでした。おそらく Serendipity か SourceForge.net のサーバーがダウンしています - すみませんが、あとで再度試してください。');
@define('PLUGIN_EVENT_SPARTACUS_FETCHING', 'URL %s にアクセスを試みます...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL', 'Fetched %s bytes from the URL above. Saving file as %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE', 'Fetched %s bytes from already existing file on your server. Saving file as %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_DONE', 'データの取得に成功しました。');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_XML', 'ファイル/ミラーの場所 (XML メタデータ)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_FILES', 'ファイル/ミラーの場所 (ファイル)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_DESC', 'Choose a download location. Do NOT change this value unless you know what you are doing and if servers get oudated. This option is available mainly for forward compatibility.');

?>
