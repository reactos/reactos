package com.ms.xml.util;

//
//
// NativeXMLInputStream
//
//
public interface XMLStreamReader
{
  public int open(String url);

  public int read(int[] buf, int len);

  public void setEncoding(int encoding, int offset);

  public void close();
}

