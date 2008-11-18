<?php # $Id: serendipity_event_weblogping.php 609 2005-10-26 18:40:13Z jmatos $

require_once S9Y_PEAR_PATH . 'HTTP/Request.php';

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_WEBLOGPING_PING', 'Announce entries (via XML-RPC ping) to:');
@define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', 'Sending XML-RPC ping to host %s');
@define('PLUGIN_EVENT_WEBLOGPING_TITLE', 'Announce entries');
@define('PLUGIN_EVENT_WEBLOGPING_DESC', 'Send notification of new entries to online services');
@define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(supersedes %s)');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', 'Custom ping-services');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', 'One or more special ping services, separated by ",". The entries need to be formatted like: "host.domain/path". If a "*" is entered at the beginning of the hostname, the extended XML-RPC options will be sent to that host (only if supported by the host).');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_FAILURE', 'Failure(Reason: %s)');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_SUCCESS', 'Success!!');

class serendipity_event_weblogping extends serendipity_event
{
    var $services;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_WEBLOGPING_TITLE);
        $propbag->add('description',   PLUGIN_EVENT_WEBLOGPING_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.02');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',    array(
            'backend_display' => true,
            'frontend_display' => true,
            'backend_insert' => true,
            'backend_update' => true,
            'backend_publish' => true,
            'backend_draft' => true
        ));
        $propbag->add('groups', array('BACKEND_EDITOR'));

        $servicesdb = array();
        $servicesdb_file = dirname(__FILE__) . '/servicesdb_' . $serendipity['lang'] . '.inc.php';
        if (!file_exists($servicesdb_file)) {
            $servicesdb_file = dirname(__FILE__) . '/servicesdb_en.inc.php';
        }
        include $servicesdb_file;
        $this->services =& $servicesdb;


        $manual_services = explode(',', $this->get_config('manual_services'));
        if (is_array($manual_services)) {
            foreach($manual_services as $ms_index => $ms_name) {
                if (!empty($ms_name)) {
                    $is_extended = ($ms_name{0} == '*' ? true : false);
                    $ms_name = trim($ms_name, '*');
                    $ms_parts = explode('/', $ms_name);
                    $ms_host = $ms_parts[0];
                    unset($ms_party[0]);

                    array_shift( $ms_parts);  //  remove hostname.
                    $this->services[] = array(
                                          'name'     => $ms_name,
                                          'host'     => $ms_host,
                                          'path'     => '/'.implode('/', $ms_parts),
                                          'extended' => $is_extended
                    );
                }
            }
        }

        $conf_array = array();
        foreach($this->services AS $key => $service) {
            $conf_array[] = $service['name'];
        }

        $conf_array[] = 'manual_services';

        $propbag->add('configuration', $conf_array);
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'manual_services':
                $propbag->add('type',        'string');
                $propbag->add('name',        PLUGIN_EVENT_WEBLOGPING_CUSTOM);
                $propbag->add('description', PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA);
                $propbag->add('default', '');
                break;

            default:
                $propbag->add('type',        'boolean');
                $propbag->add('name',        $name);
                $propbag->add('description', sprintf(PLUGIN_EVENT_WEBLOGPING_PING, $name));
                $propbag->add('default', 'false');
        }
        return true;
    }

    function generate_content(&$title) {
        $title = PLUGIN_EVENT_WEBLOGPING_TITLE;
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');
        if (isset($hooks[$event])) {
            switch($event) {
                case 'backend_display':
?>
                    <fieldset style="margin: 5px">
                        <legend><?php echo PLUGIN_EVENT_WEBLOGPING_PING; ?></legend>
<?php
                    foreach($this->services AS $index => $service) {
                        // Detect if the current checkbox needs to be saved. We use the field chk_timestamp to see,
                        // if the form has already been submitted and individual changes shall be preserved
                        $selected = (($serendipity['POST']['chk_timestamp'] && $serendipity['POST']['announce_entries_' . $service['name']]) || (!isset($serendipity['POST']['chk_timestamp']) && $this->get_config($service['name']) == 'true') ? 'checked="checked"' : '');

                        $onclick = '';
                        if (!empty($service['supersedes'])) {
                            $onclick    = 'onclick="';
                            $supersedes = explode(', ', $service['supersedes']);
                            foreach($supersedes AS $sid => $servicename) {
                                $onclick .= 'document.getElementById(\'serendipity[announce_entries_' . $servicename . ']\').checked = false; ';
                            }
                            $onclick    .= '"';
                        }

                        $title    = sprintf(PLUGIN_EVENT_WEBLOGPING_SENDINGPING, $service['name'])
                                  . (!empty($service['supersedes']) ?  ' ' . sprintf(PLUGIN_EVENT_WEBLOGPING_SUPERSEDES, $service['supersedes']) : '');
?>
                            <input <?php echo $onclick; ?> style="margin: 0px; padding: 0px; vertical-align: bottom;" type="checkbox" name="serendipity[announce_entries_<?php echo $service['name']; ?>]" id="serendipity[announce_entries_<?php echo $service['name']; ?>]" value="true" <?php echo $selected; ?> />
                                <label title="<?php echo $title; ?>" style="vertical-align: bottom; margin: 0px; padding: 0px;" for="serendipity[announce_entries_<?php echo $service['name']; ?>]">&nbsp;<?php echo $service['name']; ?>&nbsp;&nbsp;</label><br />
<?php
    }
?>
                    </fieldset>
<?php
                    return true;
                    break;

                case 'backend_publish':
                    include_once(S9Y_PEAR_PATH . "XML/RPC.php");

                    // First cycle through list of services to remove superseding services which may have been checked
                    foreach ($this->services as $index => $service) {
                        if (!empty($service['supersedes']) && isset($serendipity['POST']['announce_entries_' . $service['name']])) {
                            $supersedes = explode(', ', $service['supersedes']);
                            foreach($supersedes AS $sid => $servicename) {
                                // A service has been checked that is superseded by another checked meta-service. Remove that service from the list of services to be ping'd
                                unset($serendipity['POST']['announce_entries_' . $servicename]);
                            }
                        }
                    }
                    foreach ($this->services as $index => $service) {
                        if (isset($serendipity['POST']['announce_entries_' . $service['name']])) {
                            printf(PLUGIN_EVENT_WEBLOGPING_SENDINGPING . '...', $service['host']);
                            flush();

                            # XXX append $serendipity['indexFile'] to baseURL?
                            $args = array(
                              new XML_RPC_Value(
                                $serendipity['blogTitle'],
                                'string'
                              ),
                              new XML_RPC_Value(
                                $serendipity['baseURL'],
                                'string'
                              )
                            );

                            if ($service['extended']) {
                                # the checkUrl: for when the main page is not really the main page
                                $args[] = new XML_RPC_Value(
                                  '',
                                  'string'
                                );

                                # the rssUrl
                                $args[] = new XML_RPC_Value(
                                  $serendipity['baseURL'] . 'rss.php?version=2.0',
                                  'string'
                                );
                            }

                            $message = new XML_RPC_Message(
                              $service['extended'] ? 'weblogUpdates.extendedPing' : 'weblogUpdates.ping',
                              $args
                            );

                            $client = new XML_RPC_Client(
                              trim($service['path']),
                              trim($service['host'])
                            );

                            # 15 second timeout may not be long enough for weblogs.com
                            $message->createPayload();
                            $req = new HTTP_Request("http://".$service['host'].$service['path']);
                            $req->setMethod(HTTP_REQUEST_METHOD_POST);
                            $req->addHeader("Content-Type", "text/xml");
                            if (strtoupper(LANG_CHARSET) != 'UTF-8') {
                                $payload = utf8_encode($message->payload);
                            } else {
                                $payload = $message->payload;
                            }
                            $req->addRawPostData($payload);
                            $http_result   = $req->sendRequest();
                            $http_response = $req->getResponseBody();
                            $xmlrpc_result = $message->parseResponse($http_response);
                            if ($xmlrpc_result->faultCode()) {
                                echo sprintf(PLUGIN_EVENT_WEBLOGPING_SEND_FAILURE . "<br />", htmlspecialchars($xmlrpc_result->faultString()));
                            } else {
                                echo PLUGIN_EVENT_WEBLOGPING_SEND_SUCCESS . "<br />";
                            }
                        }
                    }

                    return true;
                    break;

                case 'frontend_display':
                case 'backend_insert':
                case 'backend_update':
                case 'backend_draft':
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
