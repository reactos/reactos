using System;
using System.Net;
using System.IO;
using System.Text;
using System.Net.Sockets;
using System.Diagnostics;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;

/*
 * FTP Client library in C#
 * Author: Jaimon Mathew
 * mailto:jaimonmathew@rediffmail.com
 * http://www.csharphelp.com/archives/archive9.html
 * 
 * Addapted for use by Dan Glass 07/03/03
 */


namespace ReactOS.CustomRevisionAction
{

	public class FtpClient
	{

		public class FtpException : Exception
		{
			public FtpException(string message) : base(message){}
			public FtpException(string message, Exception innerException) : base(message,innerException){}
		}

		private static int BUFFER_SIZE = 512;
		private static Encoding ASCII = Encoding.ASCII;

		private bool verboseDebugging = false;

		// defaults
		private string server = "localhost";
		private string remotePath = ".";
		private string username = "anonymous";
		private string password = "anonymous@anonymous.net";
		private string message = null;
		private string result = null;

		private int port = 21;
		private int bytes = 0;
		private int resultCode = 0;

		private bool loggedin = false;
		private bool binMode = false;

		private Byte[] buffer = new Byte[BUFFER_SIZE];
		private Socket clientSocket = null;

		private int timeoutSeconds = 10;

		/// <summary>
		/// Default contructor
		/// </summary>
		public FtpClient()
		{
		}
		/// <summary>
		/// 
		/// </summary>
		/// <param name="server"></param>
		/// <param name="username"></param>
		/// <param name="password"></param>
		public FtpClient(string server, string username, string password)
		{
			this.server = server;
			this.username = username;
			this.password = password;
		}
		/// <summary>
		/// 
		/// </summary>
		/// <param name="server"></param>
		/// <param name="username"></param>
		/// <param name="password"></param>
		/// <param name="timeoutSeconds"></param>
		/// <param name="port"></param>
		public FtpClient(string server, string username, string password, int timeoutSeconds, int port)
		{
			this.server = server;
			this.username = username;
			this.password = password;
			this.timeoutSeconds = timeoutSeconds;
			this.port = port;
		}

		/// <summary>
		/// Display all communications to the debug log
		/// </summary>
		public bool VerboseDebugging
		{
			get
			{
				return this.verboseDebugging;
			}
			set
			{
				this.verboseDebugging = value;
			}
		}
		/// <summary>
		/// Remote server port. Typically TCP 21
		/// </summary>
		public int Port
		{
			get
			{
				return this.port;
			}
			set
			{
				this.port = value;
			}
		}
		/// <summary>
		/// Timeout waiting for a response from server, in seconds.
		/// </summary>
		public int Timeout
		{
			get
			{
				return this.timeoutSeconds;
			}
			set
			{
				this.timeoutSeconds = value;
			}
		}
		/// <summary>
		/// Gets and Sets the name of the FTP server.
		/// </summary>
		/// <returns></returns>
		public string Server
		{
			get
			{
				return this.server;
			}
			set
			{
				this.server = value;
			}
		}
		/// <summary>
		/// Gets and Sets the port number.
		/// </summary>
		/// <returns></returns>
		public int RemotePort
		{
			get
			{
				return this.port;
			}
			set
			{
				this.port = value;
			}
		}
		/// <summary>
		/// GetS and Sets the remote directory.
		/// </summary>
		public string RemotePath
		{
			get
			{
				return this.remotePath;
			}
			set
			{
				this.remotePath = value;
			}

		}
		/// <summary>
		/// Gets and Sets the username.
		/// </summary>
		public string Username
		{
			get
			{
				return this.username;
			}
			set
			{
				this.username = value;
			}
		}
		/// <summary>
		/// Gets and Set the password.
		/// </summary>
		public string Password
		{
			get
			{
				return this.password;
			}
			set
			{
				this.password = value;
			}
		}

		/// <summary>
		/// If the value of mode is true, set binary mode for downloads, else, Ascii mode.
		/// </summary>
		public bool BinaryMode
		{
			get
			{
				return this.binMode;
			}
			set
			{
				if ( this.binMode == value ) return;

				if ( value )
					sendCommand("TYPE I");

				else
					sendCommand("TYPE A");

				if ( this.resultCode != 200 ) throw new FtpException(result.Substring(4));
			}
		}
		/// <summary>
		/// Login to the remote server.
		/// </summary>
		public void Login()
		{
			if ( this.loggedin ) this.Close();

			Debug.WriteLine("Opening connection to " + this.server, "FtpClient" );

			IPAddress addr = null;
			IPEndPoint ep = null;

			try
			{
				this.clientSocket = new Socket( AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp );
				addr = Dns.Resolve(this.server).AddressList[0];
				ep = new IPEndPoint( addr, this.port );
				this.clientSocket.Connect(ep);
			}
			catch(Exception ex)
			{
				// doubtfull
				if ( this.clientSocket != null && this.clientSocket.Connected ) this.clientSocket.Close();

				throw new FtpException("Couldn't connect to remote server",ex);
			}

			this.readResponse();

			if(this.resultCode != 220)
			{
				this.Close();
				throw new FtpException(this.result.Substring(4));
			}

			this.sendCommand( "USER " + username );

			if( !(this.resultCode == 331 || this.resultCode == 230) )
			{
				this.cleanup();
				throw new FtpException(this.result.Substring(4));
			}

			if( this.resultCode != 230 )
			{
				this.sendCommand( "PASS " + password );

				if( !(this.resultCode == 230 || this.resultCode == 202) )
				{
					this.cleanup();
					throw new FtpException(this.result.Substring(4));
				}
			}

			this.loggedin = true;

			Debug.WriteLine( "Connected to " + this.server, "FtpClient" );

			this.ChangeDir(this.remotePath);
		}
		
		/// <summary>
		/// Close the FTP connection.
		/// </summary>
		public void Close()
		{
			Debug.WriteLine("Closing connection to " + this.server, "FtpClient" );

			if( this.clientSocket != null )
			{
				this.sendCommand("QUIT");
			}

			this.cleanup();
		}

		/// <summary>
		/// Return a string array containing the remote directory's file list.
		/// </summary>
		/// <returns></returns>
		public string[] GetFileList()
		{
			return this.GetFileList("*.*");
		}

		/// <summary>
		/// Return a string array containing the remote directory's file list.
		/// </summary>
		/// <param name="mask"></param>
		/// <returns></returns>
		public string[] GetFileList(string mask)
		{
			if ( !this.loggedin ) this.Login();

			Socket cSocket = createDataSocket();

			this.sendCommand("NLST " + mask);

			if(!(this.resultCode == 150 || this.resultCode == 125)) throw new FtpException(this.result.Substring(4));

			this.message = "";

			DateTime timeout = DateTime.Now.AddSeconds(this.timeoutSeconds);

			while( timeout > DateTime.Now )
			{
				int bytes = cSocket.Receive(buffer, buffer.Length, 0);
				this.message += ASCII.GetString(buffer, 0, bytes);

				if ( bytes < this.buffer.Length ) break;
			}

			string[] msg = this.message.Replace("\r","").Split('\n');

			cSocket.Close();

			if ( this.message.IndexOf( "No such file or directory" ) != -1 )
				msg = new string[]{};

			this.readResponse();

			if ( this.resultCode != 226 )
				msg = new string[]{};
			//	throw new FtpException(result.Substring(4));

			return msg;
		}

		public bool DirectoryExists(string directory)
		{
			try
			{
				ChangeDir(directory);
				ChangeDir("..");
				return true;
			}
			catch (FtpException)
			{
				return false;
			}
		}
	
		/// <summary>
		/// Return the size of a file.
		/// </summary>
		/// <param name="fileName"></param>
		/// <returns></returns>
		public long GetFileSize(string fileName)
		{
			if ( !this.loggedin ) this.Login();

			this.sendCommand("SIZE " + fileName);
			long size=0;

			if ( this.resultCode == 213 )
				size = long.Parse(this.result.Substring(4));

			else
				throw new FtpException(this.result.Substring(4));

			return size;
		}
	
		
		/// <summary>
		/// Download a file to the Assembly's local directory,
		/// keeping the same file name.
		/// </summary>
		/// <param name="remFileName"></param>
		public void Download(string remFileName)
		{
			this.Download(remFileName,"",false);
		}

		/// <summary>
		/// Download a remote file to the Assembly's local directory,
		/// keeping the same file name, and set the resume flag.
		/// </summary>
		/// <param name="remFileName"></param>
		/// <param name="resume"></param>
		public void Download(string remFileName,Boolean resume)
		{
			this.Download(remFileName,"",resume);
		}
		
		/// <summary>
		/// Download a remote file to a local file name which can include
		/// a path. The local file name will be created or overwritten,
		/// but the path must exist.
		/// </summary>
		/// <param name="remFileName"></param>
		/// <param name="locFileName"></param>
		public void Download(string remFileName,string locFileName)
		{
			this.Download(remFileName,locFileName,false);
		}

		/// <summary>
		/// Download a remote file to a local file name which can include
		/// a path, and set the resume flag. The local file name will be
		/// created or overwritten, but the path must exist.
		/// </summary>
		/// <param name="remFileName"></param>
		/// <param name="locFileName"></param>
		/// <param name="resume"></param>
		public void Download(string remFileName,string locFileName,Boolean resume)
		{
			if ( !this.loggedin ) this.Login();

			this.BinaryMode = true;

			Debug.WriteLine("Downloading file " + remFileName + " from " + server + "/" + remotePath, "FtpClient" );

			if (locFileName.Equals(""))
			{
				locFileName = remFileName;
			}

			FileStream output = null;

			if ( !File.Exists(locFileName) )
				output = File.Create(locFileName);

			else
				output = new FileStream(locFileName,FileMode.Open);

			Socket cSocket = createDataSocket();

			long offset = 0;

			if ( resume )
			{
				offset = output.Length;

				if ( offset > 0 )
				{
					this.sendCommand( "REST " + offset );
					if ( this.resultCode != 350 )
					{
						//Server dosnt support resuming
						offset = 0;
						Debug.WriteLine("Resuming not supported:" + result.Substring(4), "FtpClient" );
					}
					else
					{
						Debug.WriteLine("Resuming at offset " + offset, "FtpClient" );
						output.Seek( offset, SeekOrigin.Begin );
					}
				}
			}

			this.sendCommand("RETR " + remFileName);

			if ( this.resultCode != 150 && this.resultCode != 125 )
			{
				throw new FtpException(this.result.Substring(4));
			}

			DateTime timeout = DateTime.Now.AddSeconds(this.timeoutSeconds);

			while ( timeout > DateTime.Now )
			{
				this.bytes = cSocket.Receive(buffer, buffer.Length, 0);
				output.Write(this.buffer,0,this.bytes);

				if ( this.bytes <= 0)
				{
					break;
				}
			}

			output.Close();

			if ( cSocket.Connected ) cSocket.Close();

			this.readResponse();

			if( this.resultCode != 226 && this.resultCode != 250 )
				throw new FtpException(this.result.Substring(4));
		}

		
		/// <summary>
		/// Upload a file.
		/// </summary>
		/// <param name="fileName"></param>
		public void Upload(string fileName)
		{
			this.Upload(fileName,false);
		}

		
		/// <summary>
		/// Upload a file and set the resume flag.
		/// </summary>
		/// <param name="fileName"></param>
		/// <param name="resume"></param>
		public void Upload(string fileName, bool resume)
		{
			if ( !this.loggedin ) this.Login();

			Socket cSocket = null ;
			long offset = 0;

			if ( resume )
			{
				try
				{
					this.BinaryMode = true;

					offset = GetFileSize( Path.GetFileName(fileName) );
				}
				catch(Exception)
				{
					// file not exist
					offset = 0;
				}
			}

			// open stream to read file
			FileStream input = new FileStream(fileName,FileMode.Open);

			if ( resume && input.Length < offset )
			{
				// different file size
				Debug.WriteLine("Overwriting " + fileName, "FtpClient");
				offset = 0;
			}
			else if ( resume && input.Length == offset )
			{
				// file done
				input.Close();
				Debug.WriteLine("Skipping completed " + fileName + " - turn resume off to not detect.", "FtpClient");
				return;
			}

			// dont create untill we know that we need it
			cSocket = this.createDataSocket();

			if ( offset > 0 )
			{
				this.sendCommand( "REST " + offset );
				if ( this.resultCode != 350 )
				{
					Debug.WriteLine("Resuming not supported", "FtpClient");
					offset = 0;
				}
			}

			this.sendCommand( "STOR " + Path.GetFileName(fileName) );

			if ( this.resultCode != 125 && this.resultCode != 150 ) throw new FtpException(result.Substring(4));

			if ( offset != 0 )
			{
				Debug.WriteLine("Resuming at offset " + offset, "FtpClient" );

				input.Seek(offset,SeekOrigin.Begin);
			}

			Debug.WriteLine( "Uploading file " + fileName + " to " + remotePath, "FtpClient" );

			while ((bytes = input.Read(buffer,0,buffer.Length)) > 0)
			{
				cSocket.Send(buffer, bytes, 0);
			}
			
			input.Close();

			if (cSocket.Connected)
			{
				cSocket.Close();
			}

			this.readResponse();

			if( this.resultCode != 226 && this.resultCode != 250 ) throw new FtpException(this.result.Substring(4));
		}
		
		/// <summary>
		/// Upload a directory and its file contents
		/// </summary>
		/// <param name="path"></param>
		/// <param name="recurse">Whether to recurse sub directories</param>
		public void UploadDirectory(string path, bool recurse)
		{
			this.UploadDirectory(path,recurse,"*.*");
		}
		
		/// <summary>
		/// Upload a directory and its file contents
		/// </summary>
		/// <param name="path"></param>
		/// <param name="recurse">Whether to recurse sub directories</param>
		/// <param name="mask">Only upload files of the given mask - everything is '*.*'</param>
		public void UploadDirectory(string path, bool recurse, string mask)
		{
			string[] dirs = path.Replace("/",@"\").Split('\\');
			string rootDir = dirs[ dirs.Length - 1 ];

			// make the root dir if it doed not exist
			if ( this.GetFileList(rootDir).Length < 1 ) this.MakeDir(rootDir);

			this.ChangeDir(rootDir);

			foreach ( string file in Directory.GetFiles(path,mask) )
			{
				this.Upload(file,true);
			}
			if ( recurse )
			{
				foreach ( string directory in Directory.GetDirectories(path) )
				{
					this.UploadDirectory(directory,recurse,mask);
				}
			}

			this.ChangeDir("..");
		}

		/// <summary>
		/// Delete a file from the remote FTP server.
		/// </summary>
		/// <param name="fileName"></param>
		public void DeleteFile(string fileName)
		{
			if ( !this.loggedin ) this.Login();

			this.sendCommand( "DELE " + fileName );

			if ( this.resultCode != 250 ) throw new FtpException(this.result.Substring(4));

			Debug.WriteLine( "Deleted file " + fileName, "FtpClient" );
		}

		/// <summary>
		/// Rename a file on the remote FTP server.
		/// </summary>
		/// <param name="oldFileName"></param>
		/// <param name="newFileName"></param>
		/// <param name="overwrite">setting to false will throw exception if it exists</param>
		public void RenameFile(string oldFileName,string newFileName, bool overwrite)
		{
			if ( !this.loggedin ) this.Login();

			this.sendCommand( "RNFR " + oldFileName );

			if ( this.resultCode != 350 ) throw new FtpException(this.result.Substring(4));

			if ( !overwrite && this.GetFileList(newFileName).Length > 0 ) throw new FtpException("File already exists");

			this.sendCommand( "RNTO " + newFileName );

			if ( this.resultCode != 250 ) throw new FtpException(this.result.Substring(4));

			Debug.WriteLine( "Renamed file " + oldFileName + " to " + newFileName, "FtpClient" );
		}
		
		/// <summary>
		/// Create a directory on the remote FTP server.
		/// </summary>
		/// <param name="dirName"></param>
		public void MakeDir(string dirName)
		{
			if ( !this.loggedin ) this.Login();

			this.sendCommand( "MKD " + dirName );

			if ( this.resultCode != 250 && this.resultCode != 257 ) throw new FtpException(this.result.Substring(4));

			Debug.WriteLine( "Created directory " + dirName, "FtpClient" );
		}

		/// <summary>
		/// Delete a directory on the remote FTP server.
		/// </summary>
		/// <param name="dirName"></param>
		public void RemoveDir(string dirName)
		{
			if ( !this.loggedin ) this.Login();

			this.sendCommand( "RMD " + dirName );

			if ( this.resultCode != 250 ) throw new FtpException(this.result.Substring(4));

			Debug.WriteLine( "Removed directory " + dirName, "FtpClient" );
		}

		/// <summary>
		/// Change the current working directory on the remote FTP server.
		/// </summary>
		/// <param name="dirName"></param>
		public void ChangeDir(string dirName)
		{
			if( dirName == null || dirName.Equals(".") || dirName.Length == 0 )
			{
				return;
			}

			if ( !this.loggedin ) this.Login();

			this.sendCommand( "CWD " + dirName );

			if ( this.resultCode != 250 )
			{
				if (this.resultCode == 550)
					throw new FtpException(String.Format("Access denied to {0}", this.remotePath + "/" + dirName));
				throw new FtpException(result.Substring(4));
			}

			this.sendCommand( "PWD" );

			if ( this.resultCode != 257 ) throw new FtpException(result.Substring(4));

			// gonna have to do better than this....
			this.remotePath = this.message.Split('"')[1];

			Debug.WriteLine( "Current directory is " + this.remotePath, "FtpClient" );
		}

		/// <summary>
		/// 
		/// </summary>
		private void readResponse()
		{
			this.message = "";
			this.result = this.readLine();

			if ( this.result.Length > 3 )
				this.resultCode = int.Parse( this.result.Substring(0,3) );
			else
				this.result = null;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <returns></returns>
		private string readLine()
		{
			while(true)
			{
				this.bytes = clientSocket.Receive( this.buffer, this.buffer.Length, 0 );
				this.message += ASCII.GetString( this.buffer, 0, this.bytes );

				if ( this.bytes < this.buffer.Length )
				{
					break;
				}
			}

			string[] msg = this.message.Split('\n');

			if ( this.message.Length > 2 )
				this.message = msg[ msg.Length - 2 ];

			else
				this.message = msg[0];


			if ( this.message.Length > 4 && !this.message.Substring(3,1).Equals(" ") ) return this.readLine();

			if ( this.verboseDebugging )
			{
				for(int i = 0; i < msg.Length - 1; i++)
				{
					Debug.Write( msg[i], "FtpClient" );
				}
			}

			return message;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="command"></param>
		private void sendCommand(String command)
		{
			if ( this.verboseDebugging ) Debug.WriteLine(command,"FtpClient");

			Byte[] cmdBytes = Encoding.ASCII.GetBytes( ( command + "\r\n" ).ToCharArray() );
			clientSocket.Send( cmdBytes, cmdBytes.Length, 0);
			this.readResponse();
		}

		/// <summary>
		/// when doing data transfers, we need to open another socket for it.
		/// </summary>
		/// <returns>Connected socket</returns>
		private Socket createDataSocket()
		{
			this.sendCommand("PASV");

			if ( this.resultCode != 227 ) throw new FtpException(this.result.Substring(4));

			int index1 = this.result.IndexOf('(');
			int index2 = this.result.IndexOf(')');

			string ipData = this.result.Substring(index1+1,index2-index1-1);

			int[] parts = new int[6];

			int len = ipData.Length;
			int partCount = 0;
			string buf="";

			for (int i = 0; i < len && partCount <= 6; i++)
			{
				char ch = char.Parse( ipData.Substring(i,1) );

				if ( char.IsDigit(ch) )
					buf+=ch;

				else if (ch != ',')
					throw new FtpException("Malformed PASV result: " + result);

				if ( ch == ',' || i+1 == len )
				{
					try
					{
						parts[partCount++] = int.Parse(buf);
						buf = "";
					}
					catch (Exception ex)
					{
						throw new FtpException("Malformed PASV result (not supported?): " + this.result, ex);
					}
				}
			}

			string ipAddress = parts[0] + "."+ parts[1]+ "." + parts[2] + "." + parts[3];

			int port = (parts[4] << 8) + parts[5];

			Socket socket = null;
			IPEndPoint ep = null;

			try
			{
				socket = new Socket(AddressFamily.InterNetwork,SocketType.Stream,ProtocolType.Tcp);
				ep = new IPEndPoint(Dns.Resolve(ipAddress).AddressList[0], port);
				socket.Connect(ep);
			}
			catch(Exception ex)
			{
				// doubtfull....
				if ( socket != null && socket.Connected ) socket.Close();

				throw new FtpException("Can't connect to remote server", ex);
			}

			return socket;
		}
		
		/// <summary>
		/// Always release those sockets.
		/// </summary>
		private void cleanup()
		{
			if ( this.clientSocket!=null )
			{
				this.clientSocket.Close();
				this.clientSocket = null;
			}
			this.loggedin = false;
		}

		/// <summary>
		/// Destuctor
		/// </summary>
		~FtpClient()
		{
			this.cleanup();
		}


		/**************************************************************************************************************/
		#region Async methods (auto generated)

/*
				WinInetApi.FtpClient ftp = new WinInetApi.FtpClient();

				MethodInfo[] methods = ftp.GetType().GetMethods(BindingFlags.DeclaredOnly|BindingFlags.Instance|BindingFlags.Public);

				foreach ( MethodInfo method in methods )
				{
					string param = "";
					string values = "";
					foreach ( ParameterInfo i in  method.GetParameters() )
					{
						param += i.ParameterType.Name + " " + i.Name + ",";
						values += i.Name + ",";
					}
					

					Debug.WriteLine("private delegate " + method.ReturnType.Name + " " + method.Name + "Callback(" + param.TrimEnd(',') + ");");

					Debug.WriteLine("public System.IAsyncResult Begin" + method.Name + "( " + param + " System.AsyncCallback callback )");
					Debug.WriteLine("{");
					Debug.WriteLine("" + method.Name + "Callback ftpCallback = new " + method.Name + "Callback(" + values + " this." + method.Name + ");");
					Debug.WriteLine("return ftpCallback.BeginInvoke(callback, null);");
					Debug.WriteLine("}");
					Debug.WriteLine("public void End" + method.Name + "(System.IAsyncResult asyncResult)");
					Debug.WriteLine("{");
					Debug.WriteLine(method.Name + "Callback fc = (" + method.Name + "Callback) ((AsyncResult)asyncResult).AsyncDelegate;");
					Debug.WriteLine("fc.EndInvoke(asyncResult);");
					Debug.WriteLine("}");
					//Debug.WriteLine(method);
				}
*/


		private delegate void LoginCallback();
		public System.IAsyncResult BeginLogin(  System.AsyncCallback callback )
		{
			LoginCallback ftpCallback = new LoginCallback( this.Login);
			return ftpCallback.BeginInvoke(callback, null);
		}
		private delegate void CloseCallback();
		public System.IAsyncResult BeginClose(  System.AsyncCallback callback )
		{
			CloseCallback ftpCallback = new CloseCallback( this.Close);
			return ftpCallback.BeginInvoke(callback, null);
		}
		private delegate String[] GetFileListCallback();
		public System.IAsyncResult BeginGetFileList(  System.AsyncCallback callback )
		{
			GetFileListCallback ftpCallback = new GetFileListCallback( this.GetFileList);
			return ftpCallback.BeginInvoke(callback, null);
		}
		private delegate String[] GetFileListMaskCallback(String mask);
		public System.IAsyncResult BeginGetFileList( String mask, System.AsyncCallback callback )
		{
			GetFileListMaskCallback ftpCallback = new GetFileListMaskCallback(this.GetFileList);
			return ftpCallback.BeginInvoke(mask, callback, null);
		}
		private delegate Int64 GetFileSizeCallback(String fileName);
		public System.IAsyncResult BeginGetFileSize( String fileName, System.AsyncCallback callback )
		{
			GetFileSizeCallback ftpCallback = new GetFileSizeCallback(this.GetFileSize);
			return ftpCallback.BeginInvoke(fileName, callback, null);
		}
		private delegate void DownloadCallback(String remFileName);
		public System.IAsyncResult BeginDownload( String remFileName, System.AsyncCallback callback )
		{
			DownloadCallback ftpCallback = new DownloadCallback(this.Download);
			return ftpCallback.BeginInvoke(remFileName, callback, null);
		}
		private delegate void DownloadFileNameResumeCallback(String remFileName,Boolean resume);
		public System.IAsyncResult BeginDownload( String remFileName,Boolean resume, System.AsyncCallback callback )
		{
			DownloadFileNameResumeCallback ftpCallback = new DownloadFileNameResumeCallback(this.Download);
			return ftpCallback.BeginInvoke(remFileName, resume, callback, null);
		}
		private delegate void DownloadFileNameFileNameCallback(String remFileName,String locFileName);
		public System.IAsyncResult BeginDownload( String remFileName,String locFileName, System.AsyncCallback callback )
		{
			DownloadFileNameFileNameCallback ftpCallback = new DownloadFileNameFileNameCallback(this.Download);
			return ftpCallback.BeginInvoke(remFileName, locFileName, callback, null);
		}
		private delegate void DownloadFileNameFileNameResumeCallback(String remFileName,String locFileName,Boolean resume);
		public System.IAsyncResult BeginDownload( String remFileName,String locFileName,Boolean resume, System.AsyncCallback callback )
		{
			DownloadFileNameFileNameResumeCallback ftpCallback = new DownloadFileNameFileNameResumeCallback(this.Download);
			return ftpCallback.BeginInvoke(remFileName, locFileName, resume, callback, null);
		}
		private delegate void UploadCallback(String fileName);
		public System.IAsyncResult BeginUpload( String fileName, System.AsyncCallback callback )
		{
			UploadCallback ftpCallback = new UploadCallback(this.Upload);
			return ftpCallback.BeginInvoke(fileName, callback, null);
		}
		private delegate void UploadFileNameResumeCallback(String fileName,Boolean resume);
		public System.IAsyncResult BeginUpload( String fileName,Boolean resume, System.AsyncCallback callback )
		{
			UploadFileNameResumeCallback ftpCallback = new UploadFileNameResumeCallback(this.Upload);
			return ftpCallback.BeginInvoke(fileName, resume, callback, null);
		}
		private delegate void UploadDirectoryCallback(String path,Boolean recurse);
		public System.IAsyncResult BeginUploadDirectory( String path,Boolean recurse, System.AsyncCallback callback )
		{
			UploadDirectoryCallback ftpCallback = new UploadDirectoryCallback(this.UploadDirectory);
			return ftpCallback.BeginInvoke(path, recurse, callback, null);
		}
		private delegate void UploadDirectoryPathRecurseMaskCallback(String path,Boolean recurse,String mask);
		public System.IAsyncResult BeginUploadDirectory( String path,Boolean recurse,String mask, System.AsyncCallback callback )
		{
			UploadDirectoryPathRecurseMaskCallback ftpCallback = new UploadDirectoryPathRecurseMaskCallback(this.UploadDirectory);
			return ftpCallback.BeginInvoke(path, recurse, mask, callback, null);
		}
		private delegate void DeleteFileCallback(String fileName);
		public System.IAsyncResult BeginDeleteFile( String fileName, System.AsyncCallback callback )
		{
			DeleteFileCallback ftpCallback = new DeleteFileCallback(this.DeleteFile);
			return ftpCallback.BeginInvoke(fileName, callback, null);
		}
		private delegate void RenameFileCallback(String oldFileName,String newFileName,Boolean overwrite);
		public System.IAsyncResult BeginRenameFile( String oldFileName,String newFileName,Boolean overwrite, System.AsyncCallback callback )
		{
			RenameFileCallback ftpCallback = new RenameFileCallback(this.RenameFile);
			return ftpCallback.BeginInvoke(oldFileName, newFileName, overwrite, callback, null);
		}
		private delegate void MakeDirCallback(String dirName);
		public System.IAsyncResult BeginMakeDir( String dirName, System.AsyncCallback callback )
		{
			MakeDirCallback ftpCallback = new MakeDirCallback(this.MakeDir);
			return ftpCallback.BeginInvoke(dirName, callback, null);
		}
		private delegate void RemoveDirCallback(String dirName);
		public System.IAsyncResult BeginRemoveDir( String dirName, System.AsyncCallback callback )
		{
			RemoveDirCallback ftpCallback = new RemoveDirCallback(this.RemoveDir);
			return ftpCallback.BeginInvoke(dirName, callback, null);
		}
		private delegate void ChangeDirCallback(String dirName);
		public System.IAsyncResult BeginChangeDir( String dirName, System.AsyncCallback callback )
		{
			ChangeDirCallback ftpCallback = new ChangeDirCallback(this.ChangeDir);
			return ftpCallback.BeginInvoke(dirName, callback, null);
		}

		#endregion
	}
}