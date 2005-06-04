
#ifndef __DEVCPP_H__
#define __DEVCPP_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

class FileUnit
{
	public:
		std::string filename;
		std::string folder;
};

class DevCppBackend : public Backend
{
	public:

		DevCppBackend(Project &project,
		              Configuration& configuration);
		virtual ~DevCppBackend() {}

		virtual void Process();

	private:

		void ProcessModules();
		void ProcessFile(std::string &filename);
		
		bool CheckFolderAdded(std::string &folder);
		void AddFolders(std::string &folder);

		void OutputFolders();
		void OutputFileUnits();
		
		std::vector<FileUnit> m_fileUnits;
		std::vector<std::string> m_folders;

		int m_unitCount;

		std::ofstream m_devFile;
};

#endif // __DEVCPP_H__

