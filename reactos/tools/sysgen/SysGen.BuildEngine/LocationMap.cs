using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.XPath;
using System.Collections;

namespace SysGen.BuildEngine 
{
    /// <summary>
    /// Maps XML nodes to the text positions from their original source.
    /// </summary>
    public class LocationMap {

        struct TextPosition {
            public static readonly TextPosition InvalidPosition = new TextPosition(-1,-1);

            public TextPosition(int line, int column) {
                Line = line;
                Column = column;
            }

            public int Line;
            public int Column;
        }

        // The LocationMap uses a hash table to map filenames to resolve specific maps.
        Hashtable _fileMap = new Hashtable();

        public LocationMap() {
        }

        /// <summary>Add a XmlDocument to the map.</summary>
        /// <remarks>
        ///   <para>A document can only be added to the map once.</para>
        /// </remarks>
        public void Add(XmlDocument doc) {
            // prevent duplicate mapping
            // NOTE: if this becomes a liability then just return when a duplicate map has happened
            string fileName = doc.BaseURI;
            
            //check for non-backed documents
            if(fileName == "")
                return;

            if (_fileMap.ContainsKey(fileName)) {
                throw new ArgumentException(String.Format("XmlDocument '{0}' already mapped.", fileName), "doc");
            }

            Hashtable map = new Hashtable();

            string parentXPath = "/"; // default to root
            string previousXPath = "";
            int previousDepth = 0;

            // Load text reader.
            XmlTextReader reader = new XmlTextReader(fileName);

            reader.XmlResolver = null;

            try {
                map.Add((object) "/", (object) new TextPosition(1, 1));

                ArrayList indexAtDepth = new ArrayList();

                // loop thru all nodes in the document
                while (reader.Read()) {
                    // Ignore nodes we aren't interested in
                    if ((reader.NodeType != XmlNodeType.Whitespace) &&
                        (reader.NodeType != XmlNodeType.EndElement) &&
                        (reader.NodeType != XmlNodeType.ProcessingInstruction) &&
                        (reader.NodeType != XmlNodeType.XmlDeclaration)) {

                        int level = reader.Depth;
                        string currentXPath = "";

                        // If we are higher than before
                        if (reader.Depth < previousDepth) {
                            // Clear vars for new depth
                            string[] list = parentXPath.Split('/');
                            string newXPath = ""; // once appended to / will be root node ...

                            for (int j = 1; j < level+1; j++) {
                                newXPath += "/" + list[j];
                            }

                            // higher than before so trim xpath\
                            parentXPath = newXPath; // one up from before

                            // clear indexes for depth greater than ours
                            indexAtDepth.RemoveRange(level+1, indexAtDepth.Count - (level+1));

                        } else if (reader.Depth > previousDepth) {
                            // we are lower
                            parentXPath = previousXPath;
                        }

                        // End depth setup
                        // Setup up index array
                        // add any needed extra items ( usually only 1 )
                        // would have used array but not sure what maximum depth will be beforehand
                        for (int index = indexAtDepth.Count; index < level+1; index++) {
                            indexAtDepth.Add(0);
                        }
                        // Set child index
                        if ((int) indexAtDepth[level] == 0) {
                            // first time thru
                            indexAtDepth[level] = 1;
                        } else {
                            indexAtDepth[level] = (int) indexAtDepth[level] + 1; // lower so append to xpath
                        }

                        // Do actual XPath generation
                        if (parentXPath.EndsWith("/")) {
                            currentXPath = parentXPath;
                        } else {
                            currentXPath = parentXPath + "/"; // add seperator
                        }

                        // Set the final XPath
                        currentXPath += "child::node()[" + indexAtDepth[level] + "]";

                        // Add to our hash structures
                        map.Add((object) currentXPath, (object) new TextPosition(reader.LineNumber, reader.LinePosition));

                        // setup up loop vars for next iteration
                        previousXPath = currentXPath;
                        previousDepth = reader.Depth;
                    }
                }
            } finally {
                reader.Close();
            }

            // add map at the end to prevent adding maps that had errors
            _fileMap.Add(fileName, map);

        }

        /// <summary>Return the <see cref="Location"/> in the xml file for the given node.</summary>
        /// <remarks>
        ///   <para>The <c>node</c> passed in must be from a XmlDocument that has been added to the map.</para>
        /// </remarks>
        public Location GetLocation(XmlNode node) {
            // find hashtable this node's file is mapped under
            string fileName = node.BaseURI;
            if (fileName == "" ) {
                return new Location(null, 0, 0 ); // return null location because we have a fileless node.
            } 
            if (!_fileMap.ContainsKey(fileName)) {
                //throw new ArgumentException("Xml node has not been mapped.");
                return new Location(null, 0, 0);
            }

            // find xpath for node
            Hashtable map = (Hashtable) _fileMap[fileName];
            string xpath = GetXPathFromNode(node);                                                                
            if (!map.ContainsKey(xpath)) {
                //throw new ArgumentException("Xml node has not been mapped.");
                return new Location(null, 0, 0);
            }

            TextPosition pos = (TextPosition) map[xpath];
            Location location = new Location(fileName, pos.Line, pos.Column);
            return location;
        }

        private string GetXPathFromNode(XmlNode node) {
            // IM TODO review this algorithm - tidy up
            XPathNavigator nav = node.CreateNavigator();

            string xpath = "";
            int index = 0;

            while (nav != null && nav.NodeType.ToString() != "Root")
            {
                // loop thru children until we find ourselves
                XPathNavigator navParent = nav.Clone();
                navParent.MoveToParent();
                int parentIndex = 0;
                navParent.MoveToFirstChild();
                if (navParent.IsSamePosition(nav)) {
                    index = parentIndex;
                }
                while (navParent.MoveToNext()) {
                    parentIndex++;
                    if (navParent.IsSamePosition(nav)) {
                        index = parentIndex;
                    }
                }

                nav.MoveToParent(); // do loop condition here               
                index++; // Convert to 1 based index

                string thisNode = "child::node()[" + index  + "]";

                if (xpath == "") {
                    xpath = thisNode;
                } else {
                    // build xpath string
                    xpath = thisNode + "/" + xpath;
                }
            }

            // prepend slash to ...
            xpath = "/" + xpath;

            return xpath;
        }
    }
}
