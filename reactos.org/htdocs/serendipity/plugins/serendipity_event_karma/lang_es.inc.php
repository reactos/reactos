<?php # $Id: lang_es.inc.php,v 1.0 2005/08/20 11:37:42 garvinhicking Exp $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by  Rodrigo Lazo <rlazo.paz@gmail.com>
/* vim: set sts=4 ts=4 expandtab : */

@define('PLUGIN_KARMA_VERSION', '1.3');
@define('PLUGIN_KARMA_NAME', 'Karma');
@define('PLUGIN_KARMA_BLAHBLAH', 'Permite a los visitantes calificar la calidad de tus entradas');
@define('PLUGIN_KARMA_VOTETEXT', 'Karma de este artículo: ');
@define('PLUGIN_KARMA_RATE', 'Califica este artículo: %s');
@define('PLUGIN_KARMA_VOTEPOINT_1', '¡Muy bueno!');
@define('PLUGIN_KARMA_VOTEPOINT_2', 'Bueno');
@define('PLUGIN_KARMA_VOTEPOINT_3', 'Regular');
@define('PLUGIN_KARMA_VOTEPOINT_4', 'No es interesante');
@define('PLUGIN_KARMA_VOTEPOINT_5', 'Malo');
@define('PLUGIN_KARMA_VOTED', 'Tu calificación de "%s" ha sido guardada.');
@define('PLUGIN_KARMA_INVALID', 'Tu voto fue inválido.');
@define('PLUGIN_KARMA_ALREADYVOTED', 'Tu calificación ya habia sido almacenada.');
@define('PLUGIN_KARMA_NOCOOKIE', 'Tu navegador debe aceptar cookies para que puedas botar.');
@define('PLUGIN_KARMA_CLOSED', 'Vote for articles fresher than %s days!'); //translate
@define('PLUGIN_KARMA_ENTRYTIME', 'Tiempo de votación luego de la publicación');
@define('PLUGIN_KARMA_VOTINGTIME', 'Tiempo de votación');
@define('PLUGIN_KARMA_ENTRYTIME_BLAHBLAH', '¿Cuánto tiempo (en minutos) luego de que tu artículo ha sido publicado permitirás votar sin restricciones? Por defecto: 1440 (un día)');
@define('PLUGIN_KARMA_VOTINGTIME_BLAHBLAH', 'Cantidad de tiempo (en minutos) que necesitan transcurrir entre un voto y otro. Sólo se aplica luego de que el tiempo indica expira. Por defecto: 5');
@define('PLUGIN_KARMA_TIMEOUT', 'Protección contra sobrecarga: Otro visitante ha votado hace poco. Por favor espera %s minutos.');
@define('PLUGIN_KARMA_CURRENT', 'Karma actual: %2$s, %3$s voto(s)');
@define('PLUGIN_KARMA_EXTENDEDONLY', 'Sólo artículos extendidos');
@define('PLUGIN_KARMA_EXTENDEDONLY_BLAHBLAH', 'Mostrar votación karma sólo para la vista de artículos extendida');
@define('PLUGIN_KARMA_MAXKARMA', 'Periodo de votación karma');
@define('PLUGIN_KARMA_MAXKARMA_BLAHBLAH', 'Permitir solamente votación karma luego de que el artículo tenga una antigüedad de X días (Por defecto: 7)');
@define('PLUGIN_KARMA_LOGGING', '¿Log votes?');//translate
@define('PLUGIN_KARMA_LOGGING_BLAHBLAH', 'Should karma votes be logged?');//translate
@define('PLUGIN_KARMA_ACTIVE', '¿Activar votación karma?');
@define('PLUGIN_KARMA_ACTIVE_BLAHBLAH', '¿Está la votación karma disponible?');
@define('PLUGIN_KARMA_VISITS', '¿Activar registro de visitas?');
@define('PLUGIN_KARMA_VISITS_BLAHBLAH', '¿Debe ser contado y mostrado cada click hacia la vista extendida?');
@define('PLUGIN_KARMA_VISITSCOUNT', ' %4$s hits');
@define('PLUGIN_KARMA_STATISTICS_VISITS_TOP', 'Artículos más visitados');
@define('PLUGIN_KARMA_STATISTICS_VISITS_BOTTOM', 'Artículos menos visitados');
@define('PLUGIN_KARMA_STATISTICS_VOTES_TOP', 'Artículos con karma más alto');
@define('PLUGIN_KARMA_STATISTICS_VOTES_BOTTOM', 'Artículos con karma más bajo');
@define('PLUGIN_KARMA_STATISTICS_POINTS_TOP', 'Artículos con mejor votación karma');
@define('PLUGIN_KARMA_STATISTICS_POINTS_BOTTOM', 'Artículos con peor votación karma');
@define('PLUGIN_KARMA_STATISTICS_VISITS_NO', 'visitas');
@define('PLUGIN_KARMA_STATISTICS_VOTES_NO', 'votos');
@define('PLUGIN_KARMA_STATISTICS_POINTS_NO', 'puntos');

?>