<?php # $Id: lang_es.inc.php,v 1.0 2005/08/20 11:37:42 garvinhicking Exp $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by  Rodrigo Lazo <rlazo.paz@gmail.com>
/* vim: set sts=4 ts=4 expandtab : */


@define('PLUGIN_EVENT_WEBLOGPING_PING', 'Anunciar entradas (via XML-RPC ping) a:');
@define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', 'Enviando XML-RPC ping al host %s');
@define('PLUGIN_EVENT_WEBLOGPING_TITLE', 'Anuncio de entradas');
@define('PLUGIN_EVENT_WEBLOGPING_DESC', 'Envía notificaciones de nuevas entradas a servicios en linea');
@define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(supersedes %s)');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', 'Servicios  ping personalizados');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', 'Uno o más servicios especializados ping, separados por ",". Las entradas necesitan tener el siguiente formato: "host.domain/path". Si un "*" es ingresado al inicio del nombre del host, las opciones XML-RPC extendidas serán enviadas a ese host (sólo si este las soporta).');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_FAILURE', 'Error(Razón: %s)');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_SUCCESS', '!!Éxito!!');

?>