/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // searchprogram.h
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 02.10.2003
 //


struct ShellPathWithFolder
{
	ShellPathWithFolder(const ShellFolder& folder, const ShellPath& path)
	 :	_folder(folder), _path(path) {}

	ShellFolder	_folder;
	ShellPath	_path;
};


typedef void (*COLLECT_CALLBACK)(ShellFolder& folder, const ShellEntry* entry, void* param);

struct CollectProgramsThread : public Thread
{
	CollectProgramsThread(COLLECT_CALLBACK callback, HWND hwnd, void* para)
	 :	_callback(callback),
		_hwnd(hwnd),
		_para(para)
	{
	}

	int Run();

protected:
	COLLECT_CALLBACK _callback;
	HWND	_hwnd;
	void*	_para;

	void CollectProgramsThread::collect_programs(const ShellPath& path);
};


struct FindProgramTopicDlg : public ResizeController<Dialog>
{
	typedef ResizeController<Dialog> super;

	FindProgramTopicDlg(HWND hwnd);
	~FindProgramTopicDlg();

protected:
	CommonControlInit _usingCmnCtrl;
	HWND	_list_ctrl;
	HACCEL	_haccel;
	HIMAGELIST _himl;

	CollectProgramsThread _thread;

	virtual LRESULT WndProc(UINT message, WPARAM wparam, LPARAM lparam);
	virtual int		Command(int id, int code);
	virtual int	Notify(int id, NMHDR* pnmh);

	void	Refresh();

	static void collect_programs_callback(ShellFolder& folder, const ShellEntry* entry, void* param);
};
