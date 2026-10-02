// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


namespace Utilities
{
using System;
using System.IO;
using System.Text;

/// <summary>
/// Stream which writes at different indents.
/// </summary>
public class IndentedWriter : StreamWriter
{
    /// <summary>
    /// Create a new Indented Writer.
    /// </summary>
    /// <param name="stream"></param>
    public IndentedWriter(Stream stream)
        : base(stream)
    {
        SetNewLine();
    }

    /// <summary>
    /// The current number of spaces to indent.
    /// </summary>
    public int Indent
    {
        get { return indent; }
        set
        {
            indent = value;
            SetNewLine();
        }
    }

    private void SetNewLine()
    {
        StringBuilder s = new StringBuilder();
        s.Append(Utility.NewLine);
        s.Append(' ', indent);
        NewLine = s.ToString();
    }

    private int indent;
}

/// <summary>
/// 
/// </summary>
public struct Indent : IDisposable
{
    /// <summary>
    /// 
    /// </summary>
    /// <param name="writer"></param>
    /// <param name="indent"></param>
    public Indent(IndentedWriter writer, int indent)
    {
        this.writer = writer;
        this.indent = indent;
        
        writer.Indent += indent;
    }
    
    /// <summary>
    /// 
    /// </summary>
    public void Dispose()
    {
        writer.Indent -= indent;
        writer = null;
    }

    private IndentedWriter writer;
    private int indent;
}
}
