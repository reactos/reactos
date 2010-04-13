using System;
using System.IO;
using System.Windows.Forms;
using System.ComponentModel;
using System.Collections;
using System.Drawing;

namespace C2C.FileSystem
{
	/// <summary>
	/// Summary description for DirectoryTreeView.
	/// </summary> 
	///    

	public class FileSystemTreeView : TreeView
	{            
      private bool _showFiles = true;
      private ImageList _imageList = new ImageList();
      private Hashtable _systemIcons = new Hashtable();

      public static readonly int Folder = 0;      

		public FileSystemTreeView()
		{
         this.ImageList = _imageList;         
         this.MouseDown += new MouseEventHandler(FileSystemTreeView_MouseDown);         
         this.BeforeExpand += new TreeViewCancelEventHandler(FileSystemTreeView_BeforeExpand);
		}

      void FileSystemTreeView_MouseDown(object sender, MouseEventArgs e)
      {
         TreeNode node = this.GetNodeAt(e.X, e.Y);

         if (node == null)
            return;

         this.SelectedNode = node; //select the node under the mouse         
      }

      void FileSystemTreeView_BeforeExpand(object sender, TreeViewCancelEventArgs e)
      {
         if( e.Node is FileNode ) return;
         
         DirectoryNode node = (DirectoryNode)e.Node;

         if (!node.Loaded)
         {
            node.Nodes[0].Remove(); //remove the fake child node used for virtualization
            node.LoadDirectory();
            if( this._showFiles == true )
               node.LoadFiles();
         }
      }      

      public void Load( string directoryPath )
      {
         if( Directory.Exists( directoryPath ) == false )
            throw new DirectoryNotFoundException( "Directory Not Found" );

         _systemIcons.Clear();
         _imageList.Images.Clear();
         Nodes.Clear();
                         
         Icon folderIcon = new Icon( typeof( FileSystemTreeView ), "icons.folder.ico"); 
         
         _imageList.Images.Add( folderIcon );
         _systemIcons.Add( FileSystemTreeView.Folder, 0 );
        
         DirectoryNode node = new DirectoryNode( this, new DirectoryInfo( directoryPath ) );                  
         node.Expand();
      }     

      public int GetIconImageIndex( string path )
      {  
         string extension = Path.GetExtension( path );

         if( _systemIcons.ContainsKey( extension ) == false )
         {
            Icon icon = ShellIcon.GetSmallIcon( path );            
            _imageList.Images.Add( icon );
            _systemIcons.Add( extension, _imageList.Images.Count-1 );
         }
            
         return (int)_systemIcons[ Path.GetExtension( path )];         
      }

      public bool ShowFiles
      {
         get{ return this._showFiles; }
         set{ this._showFiles = value; }
      }
	}

   public class DirectoryNode : TreeNode
   {
      private DirectoryInfo _directoryInfo;      

      public DirectoryNode( DirectoryNode parent, DirectoryInfo directoryInfo ) : base( directoryInfo.Name )
      {         
         this._directoryInfo = directoryInfo;         
         
         this.ImageIndex = FileSystemTreeView.Folder;
         this.SelectedImageIndex = this.ImageIndex;   

         parent.Nodes.Add( this );
         
         Virtualize();
      }

      public DirectoryNode( FileSystemTreeView treeView, DirectoryInfo directoryInfo ) : base( directoryInfo.Name )
      {
         this._directoryInfo = directoryInfo;         
         
         this.ImageIndex = FileSystemTreeView.Folder;
         this.SelectedImageIndex = this.ImageIndex;   

         treeView.Nodes.Add( this );

         Virtualize();
         
      }

      void Virtualize()
      {
         int fileCount = 0;
         
         try
         {
            if( this.TreeView.ShowFiles == true )
               fileCount = this._directoryInfo.GetFiles().Length;         

            if( (fileCount + this._directoryInfo.GetDirectories().Length) > 0 )
               new FakeChildNode( this );            
         }
         catch
         {
         }
      }

      public void LoadDirectory()
      {         
         foreach( DirectoryInfo directoryInfo in _directoryInfo.GetDirectories() )
         {
            new DirectoryNode( this, directoryInfo );            
         }
      }           

      public void LoadFiles()
      {
         foreach( FileInfo file in _directoryInfo.GetFiles() )
         {
            new FileNode( this, file );            
         }
      }

      public bool Loaded
      {
         get
         {
            if( this.Nodes.Count != 0 )
            {
               if( this.Nodes[0] is FakeChildNode )
                  return false;               
            }

            return true;
         }
      }

      public new FileSystemTreeView TreeView
      {
         get{ return (FileSystemTreeView)base.TreeView; }
      }
   }

   public class FileNode : TreeNode
   {
      private FileInfo _fileInfo;
      private DirectoryNode _directoryNode;

      public FileNode( DirectoryNode directoryNode, FileInfo fileInfo ) : base( fileInfo.Name )
      {         
         this._directoryNode = directoryNode;
         this._fileInfo = fileInfo;         

         this.ImageIndex = ((FileSystemTreeView)_directoryNode.TreeView).GetIconImageIndex( _fileInfo.FullName );
         this.SelectedImageIndex = this.ImageIndex;

         _directoryNode.Nodes.Add( this );        
      }
   }

   public class FakeChildNode : TreeNode
   {
      public FakeChildNode( TreeNode parent ) : base()
      {
         parent.Nodes.Add( this );
      }
   }

}
