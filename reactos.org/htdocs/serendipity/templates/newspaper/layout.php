<?php
##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
#          Kristian Köhntopp fucked it up                                #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
##########################################################################

$OPENSHADOW = '<table  cellspacing="0" width="100%" cellpadding="0" style="padding:0px">
                 <tr>
                   <td valign="top" align="left" style="padding:10px;border-top:1px solid #cdcdcd; border-left: 1px solid #cdcdcd;">
';

$CLOSESHADOW = '</td>
                <td style="width:8px; background-image:url('.$serendipity['baseURL'].'/templates/newspaper/img/shadowr.png);" valign="top"><img src="'.$serendipity['baseURL'].'/templates/newspaper/img/shadowt.png" border="0" hspace="0" vspace="0" alt="neu" /></td>
              </tr>
              <tr>
                <td style="height:10px; background-image:url('.$serendipity['baseURL'].'/templates/newspaper/img/shadowb.png);"><img src="'.$serendipity['baseURL'].'/templates/newspaper/img/shadowbl.png" border="0" hspace="0" vspace="0" alt="new" /></td>
                <td style="height:10px; width:8px;"><img src="'.$serendipity['baseURL'].'/templates/newspaper/img/shadowbr.png" border="0" hspace="0" vspace="0" alt="neu" /></td>
              </tr>
            </table>
';
?>

<table width="100%" cellspacing="4" border="0">
    <tr>
        <td colspan="3" width="100%" class="serendipityBanner">
            <div id="serendipity_banner">
                <h1><a class="homelink1" href="<?php echo $serendipity['baseURL']; ?>"><?php echo htmlspecialchars($serendipity['blogTitle']); ?></a></h1>
            <h2>
<?php
$sub = isset($serendipity['blogSubTitle']) ? $serendipity['blogSubTitle'] : $serendipity['blogDescription'];
if (strlen($sub)) {
?>
    <h2><a class="homelink2" href="<?php echo $serendipity['baseURL']; ?>"><?php echo $sub ?></a></h2>
<?php
}
?>

</h2>
            </div>
        </td>
    </tr>
    <tr>
        <td width="150" valign="top" align="left" class="serendipitySideBar">
            <?php
                serendipity_plugin_api::generate_plugins('left', 'span');
//              serendipity_plugin_api::generate_plugins('right', 'span');
            ?>

        </td>
        <td width="100%" valign="top" align="left" class="serendipityContent">
            <?php echo $OPENSHADOW; ?>
            <?php
                // The main area
                switch ($serendipity["GET"]["action"]) {

                    // User wants to read the diary
                    case "read":
                        if (isset($serendipity['GET']['id'])) {
                            serendipity_printEntries(array(serendipity_fetchEntry("id", $serendipity['GET']['id'])), 1);
                        } else {
                            serendipity_printEntries(serendipity_fetchEntries($serendipity['range'], true, $serendipity['fetchLimit']));
                        }
                    break;

                    // User searches
                    case "search":
                        $r = serendipity_searchEntries($serendipity["GET"]["searchTerm"]);
                        if ( strlen($serendipity["GET"]["searchTerm"]) <= 3 ) {
                                echo SEARCH_TOO_SHORT;
                                break;
                        }

                        if ($r === true) {
                            echo sprintf(NO_ENTRIES_BLAHBLAH, $serendipity['GET']['searchTerm']);
                            break;
                        }
                        echo sprintf(YOUR_SEARCH_RETURNED_BLAHBLAH, $serendipity["GET"]["searchTerm"], count($r));
                        serendipity_printEntries($r);
                    break;

                    // Show the archive
                    case "archives":
                       serendipity_printArchives();
                    break;


                    // Welcome screen or whatever
                    default:
                        serendipity_printEntries(serendipity_fetchEntries(null, true, $serendipity['fetchLimit']));
                }
            ?>
            <?php echo $CLOSESHADOW; ?>
        </td>
        <td valign="top" align="left" class="serendipitySideBar">
            <?php
//              serendipity_plugin_api::generate_plugins('left', 'span');
                serendipity_plugin_api::generate_plugins('right', 'span');
            ?>

        </td>
    </tr>
</table>
