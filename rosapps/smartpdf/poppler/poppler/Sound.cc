/* Sound.cc - an object that holds the sound structure
 * Copyright (C) 2006-2007, Pino Toscano <pino@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "GooString.h"
#include "Object.h"
#include "Sound.h"
#include "Stream.h"
#include "Link.h"

Sound *Sound::parseSound(Object *obj)
{
  // let's try to see if this Object is a Sound, according to the PDF specs
  // (section 9.2)
  Stream *str = NULL;
  // the Object must be a Stream
  if (obj->isStream()) {
    str = obj->getStream();
  } else {
    return NULL;
  }
  // the Stream must have a Dict
  Dict *dict = str->getDict();
  if (dict == NULL)
    return NULL;
  Object tmp;
  // the Dict must have the 'R' key of type num
  dict->lookup("R", &tmp);
  if (tmp.isNum()) {
    return new Sound(obj);
  } else {
    return NULL;
  }
}

Sound::Sound(Object *obj, bool readAttrs)
{
  streamObj = new Object();
  streamObj->initNull();
  obj->copy(streamObj);

  fileName = NULL;
  samplingRate = 0.0;
  channels = 1;
  bitsPerSample = 8;
  encoding = soundRaw;

  if (readAttrs)
  {
    Object tmp;
    Dict *dict = streamObj->getStream()->getDict();
    dict->lookup("F", &tmp);
    if (!tmp.isNull()) {
      // valid 'F' key -> external file
      kind = soundExternal;
      fileName = LinkAction::getFileSpecName(&tmp);
    } else {
      // no file specification, then the sound data have to be
      // extracted from the stream
      kind = soundEmbedded;
    }
    tmp.free();
    // sampling rate
    dict->lookup("R", &tmp);
    if (tmp.isNum()) {
      samplingRate = tmp.getNum();
    }
    tmp.free();
    // sound channels
    dict->lookup("C", &tmp);
    if (tmp.isInt()) {
      channels = tmp.getInt();
    }
    tmp.free();
    // bits per sample
    dict->lookup("B", &tmp);
    if (tmp.isInt()) {
      bitsPerSample = tmp.getInt();
    }
    tmp.free();
    // encoding format
    dict->lookup("E", &tmp);
    if (tmp.isName())
    {
      const char *enc = tmp.getNameC();
      if (strcmp("Raw", enc) == 0) {
        encoding = soundRaw;
      } else if (strcmp("Signed", enc) == 0) {
        encoding = soundSigned;
      } else if (strcmp("muLaw", enc) == 0) {
        encoding = soundMuLaw;
      } else if (strcmp("ALaw", enc) == 0) {
        encoding = soundALaw;
      }
    }
    tmp.free();
  }
}

Sound::~Sound()
{
  delete fileName;
  streamObj->free();
  delete streamObj;
}

Stream *Sound::getStream()
{
  return streamObj->getStream();
}

Sound *Sound::copy()
{
  Sound *newsound = new Sound(streamObj, false);

  newsound->kind = kind;
  if (fileName) {
    newsound->fileName = fileName->copy();
  }
  newsound->samplingRate = samplingRate;
  newsound->channels = channels;
  newsound->bitsPerSample = bitsPerSample;
  newsound->encoding = encoding;

  return newsound;
}
