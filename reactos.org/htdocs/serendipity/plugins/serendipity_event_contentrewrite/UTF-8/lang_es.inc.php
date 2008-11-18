<?php # $Id: lang_es.inc.php,v 1.0 2005/08/20 11:37:42 garvinhicking Exp $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by  Rodrigo Lazo <rlazo.paz@gmail.com>
/* vim: set sts=4 ts=4 expandtab : */

@define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'de');
@define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'a');
@define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'Reemplazar texto');
@define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', 'Reemplaza palabras con nuevas cadenas (útil para acrónimos)');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', 'Nuevo título');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'Ingresa el título-acrónimo para un nuevo ítem aquí ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Título #%d');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'Ingresa el acrónimo aquí ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'Título de la extensión');
@define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'El nombre de esta extensión');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', 'Nueva descripción');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'Ingresa la descripción para un nuevo ítem aquí ({a})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Descripción #%s');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'Ingresa la descripción aquí ({a})');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'Cadena de reemplazo');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'La cadena utilizada para reemplazar. Posiciona {de} y {a} donde desees para conseguir un reemplazo.' . "\n" . 'Por ejemplo: <acronym title="{a}">{de}</acronym>');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'Caracter de reemplazo');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'Si existe algún caracter que añadiste para forzar el reemplazo, ingrésalo aquí. Por ejemplo: si sólo deseabas reemplazar \'serendipity*\' con lo que ingresaste para esa palabra y quieres el \'*\' eliminado, ingrésalo aquí.');

?>