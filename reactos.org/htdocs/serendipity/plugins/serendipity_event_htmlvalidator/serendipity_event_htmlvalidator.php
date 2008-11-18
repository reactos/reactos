<?php # $Id: serendipity_event_htmlvalidator.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_HTMLVALIDATOR_NAME', 'HTML Validator');
@define('PLUGIN_EVENT_HTMLVALIDATOR_DESC', 'Validates entries on their XML-conformity');
@define('PLUGIN_EVENT_HTMLVALIDATOR_CHARSET', 'Charset');
@define('PLUGIN_EVENT_HTMLVALIDATOR_CHARSETDESC', 'The usual charset of your articles');
@define('PLUGIN_EVENT_HTMLVALIDATOR_DOCTYPE', 'Doctype');
@define('PLUGIN_EVENT_HTMLVALIDATOR_DOCTYPEDESC', 'The usual document type of your articles');
@define('PLUGIN_EVENT_HTMLVALIDATOR_VALIDATE', 'Validate on each preview');
@define('PLUGIN_EVENT_HTMLVALIDATOR_GOVALIDATE', 'Show HTML-Validator on preview');

class serendipity_event_htmlvalidator extends serendipity_event
{
    var $title = PLUGIN_EVENT_HTMLVALIDATOR_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_HTMLVALIDATOR_NAME);
        $propbag->add('description',   PLUGIN_EVENT_HTMLVALIDATOR_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',    array(
            'backend_preview' => true,
            'backend_display' => true,
        ));

        $propbag->add('configuration', array('charset', 'doctype', 'default_validate'));
        $propbag->add('groups', array('BACKEND_EDITOR'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'default_validate':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_HTMLVALIDATOR_VALIDATE);
                $propbag->add('description', PLUGIN_EVENT_HTMLVALIDATOR_VALIDATE);
                $propbag->add('default',     'false');
                break;

            case 'charset':
                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_HTMLVALIDATOR_CHARSET);
                $propbag->add('description', PLUGIN_EVENT_HTMLVALIDATOR_CHARSETDESC);
                $propbag->add('default',     '(detect automatically)');
                $propbag->add('select_values', array(
                    '(detect automatically)' => '(detect automatically)',
                    'utf-8 (Unicode, worldwide)' => 'utf-8 (Unicode, worldwide)',
                    'utf-16 (Unicode, worldwide)' => 'utf-16 (Unicode, worldwide)',
                    'iso-8859-1 (Western Europe)' => 'iso-8859-1 (Western Europe)',
                    'iso-8859-2 (Central Europe)' => 'iso-8859-2 (Central Europe)',
                    'iso-8859-3 (Southern Europe)' => 'iso-8859-3 (Southern Europe)',
                    'iso-8859-4 (Baltic Rim)' => 'iso-8859-4 (Baltic Rim)',
                    'iso-8859-5 (Cyrillic)' => 'iso-8859-5 (Cyrillic)',
                    'iso-8859-6-i (Arabic)' => 'iso-8859-6-i (Arabic)',
                    'iso-8859-7 (Greek)' => 'iso-8859-7 (Greek)',
                    'iso-8859-8-i (Hebrew)' => 'so-8859-8-i (Hebrew)',
                    'iso-8859-9 (Turkish)' => 'iso-8859-9 (Turkish)',
                    'iso-8859-10 (Latin 6)' => 'iso-8859-10 (Latin 6)',
                    'iso-8859-13 (Latin 7)' => 'iso-8859-13 (Latin 7)',
                    'iso-8859-14 (Celtic)' => 'iso-8859-14 (Celtic)',
                    'iso-8859-15 (Latin 9)' => 'iso-8859-15 (Latin 9)',
                    'us-ascii (basic English)' => 'us-ascii (basic English)',
                    'euc-jp (Japanese, Unix)' => 'euc-jp (Japanese, Unix)',
                    'shift_jis (Japanese, Win/Mac)' => 'shift_jis (Japanese, Win/Mac)',
                    'iso-2022-jp (Japanese, email)' => 'iso-2022-jp (Japanese, email)',
                    'euc-kr (Korean)' => 'euc-kr (Korean)',
                    'gb2312 (Chinese, simplified)' => 'gb2312 (Chinese, simplified)',
                    'gb18030 (Chinese, simplified)' => 'gb18030 (Chinese, simplified)',
                    'big5 (Chinese, traditional)' => 'big5 (Chinese, traditional)',
                    'tis-620 (Thai)' => 'tis-620 (Thai)',
                    'koi8-r (Russian)' => 'koi8-r (Russian)',
                    'koi8-u (Ukrainian)' => 'koi8-u (Ukrainian)',
                    'macintosh (MacRoman)' => 'macintosh (MacRoman)',
                    'windows-1250 (Central Europe)' => 'windows-1250 (Central Europe)',
                    'windows-1251 (Cyrillic)' => 'windows-1251 (Cyrillic)',
                    'windows-1252 (Western Europe)' => 'windows-1252 (Western Europe)',
                    'windows-1253 (Greek)' => 'windows-1253 (Greek)',
                    'windows-1254 (Turkish)' => 'windows-1254 (Turkish)',
                    'windows-1255 (Hebrew)' => 'windows-1255 (Hebrew)',
                    'windows-1256 (Arabic)' => 'windows-1256 (Arabic)',
                    'windows-1257 (Baltic Rim)' => 'windows-1257 (Baltic Rim)'
                ));
                break;

            case 'doctype':
                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_HTMLVALIDATOR_DOCTYPE);
                $propbag->add('description', PLUGIN_EVENT_HTMLVALIDATOR_DOCTYPEDESC);
                $propbag->add('default',     'Inline');
                $propbag->add('select_values', array(
                    'Inline'                 => '(detect automatically)',
                    'XHTML 1.1'              => 'XHTML 1.1',
                    'XHTML Basic 1.0'        => 'XHTML Basic 1.0',
                    'XHTML 1.0 Strict'       => 'XHTML 1.0 Strict',
                    'XHTML 1.0 Transitional' => 'XHTML 1.0 Transitional',
                    'XHTML 1.0 Frameset'     => 'XHTML 1.0 Frameset',
                    'HTML 4.01 Strict'       => 'HTML 4.01 Strict',
                    'HTML 4.01 Transitional' => 'HTML 4.01 Transitional',
                    'HTML 4.01 Frameset'     => 'HTML 4.01 Frameset',
                    'HTML 3.2'               => 'HTML 3.2',
                    'HTML 2.0'               => 'HTML 2.0'
                ));
                break;

            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'backend_display':
?>
                    <fieldset style="margin: 5px">
                        <legend><?php echo PLUGIN_EVENT_HTMLVALIDATOR_NAME; ?></legend>
<?php
                        $selected = (($serendipity['POST']['chk_timestamp'] && $serendipity['POST']['default_validate']) || (!isset($serendipity['POST']['chk_timestamp']) && $this->get_config('default_validate') == 'true') ? 'checked="checked"' : '');
?>
                            <input style="margin: 0px; padding: 0px; vertical-align: bottom;" type="checkbox" name="serendipity[default_validate]" id="serendipity[default_validate]" value="true" <?php echo $selected; ?> />
                                <label style="vertical-align: bottom; margin: 0px; padding: 0px;" for="serendipity[default_validate]">&nbsp;<?php echo PLUGIN_EVENT_HTMLVALIDATOR_GOVALIDATE; ?>&nbsp;&nbsp;</label>
                    </fieldset>
<?php
                    break;

                case 'backend_preview':
                    if (!$serendipity['POST']['default_validate']) {
                        return true;
                    }

                    $url = 'validator.w3.org';
                    $path = '/check';
                    $fp = fsockopen($url, 80, $errno, $errstr, 30);

                    $doctype = $this->get_config('doctype');
                    $charset = $this->get_config('charset');

                    if (empty($doctype)) {
                        $doctype = 'XHTML 1.1';
                    }

                    if (empty($charset)) {
                        $charset = 'iso-8859-1 (Western Europe)';
                    }

                    $data = '<html><head><title>s9y</title></head><body><div>'
                          . $eventData
                          . '</div></body></html>';
                    $request_data .= '-----------------------------24464570528145
Content-Disposition: form-data; name="uploaded_file"; filename="s9y.htm"
Content-Type: text/html

' . $data . '
-----------------------------24464570528145
Content-Disposition: form-data; name="charset"

' . $charset . '
-----------------------------24464570528145
Content-Disposition: form-data; name="doctype"

' . $doctype . '
-----------------------------24464570528145
Content-Disposition: form-data; name="verbose"

1
-----------------------------24464570528145--';

                    $request_length = strlen($request_data);
                    $REQUEST = array();
                    $REQUEST[] = 'POST ' . $path . ' HTTP/1.0';
                    $REQUEST[] = 'Host: ' . $url;
                    $REQUEST[] = 'User-Agent: serendipity/' . $serendipity['version'];
                    $REQUEST[] = 'Referer: http://validator.w3.org/';
                    $REQUEST[] = 'Content-Type: multipart/form-data; boundary=---------------------------24464570528145';
                    $REQUEST[] = 'Content-Length: ' . $request_length;
                    $REQUEST[] = 'Connection: close' . "\r\n";
                    $REQUEST[] = $request_data;

                    $REQUEST_STRING = implode("\r\n", $REQUEST);
                    fputs($fp, $REQUEST_STRING);

                    $line = fgets($fp, 1024);
                    if (preg_match('@^HTTP/1\.. (2|3)0(2|0)@', $line)) {
                        $out = '';
                        $inheader = 1;
                        while(!feof($fp) && strlen($out) < 200000) {
                            $line = fgets($fp,1024);
                            if ($inheader && ($line == "\n" || $line == "\r\n")) {
                                $inheader = 0;
                            } elseif (!$inheader) {
                                $out .= $line;
                            }
                        }
                    }
                    fclose($fp);

                    preg_match('@<table class="header">.+</table>.+</div>.+(<h2 .+)</body>@ms', $out, $html);

                    // Cut the waste
                    $html[1] = preg_replace(
                                 array(
                                   '@<address>.+</address>@ms',
                                   '@<dl class="tip">.+</dl>@ms',
                                   '@<div id="source".+>.+</div>@msU'
                                 ),

                                 array(
                                   '',
                                   '',
                                   ''
                                 ),

                                 $html[1]
                    );

                    echo '<div style="border: 1px solid red; margin: 10px; padding: 5px; "><div>' . $html[1] . '</div>';
                    return true;
                    break;

                default:
                    return false;
                    break;
            }
        } else {
            return false;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
