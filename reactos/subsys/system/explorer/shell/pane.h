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
 // pane.h
 //
 // Martin Fuchs, 23.07.2003
 //


#define IDW_TREE_LEFT		3
#define IDW_TREE_RIGHT		6
#define IDW_HEADER_LEFT		2
#define IDW_HEADER_RIGHT	5


enum COLUMN_FLAGS {
	COL_TYPE		= 0x0001,
	COL_SIZE		= 0x0002,
	COL_DATE		= 0x0004,
	COL_TIME		= 0x0008,
	COL_ATTRIBUTES	= 0x0010,
	COL_DOSNAMES	= 0x0020,
	COL_INDEX		= 0x0040,
	COL_LINKS		= 0x0080,
	COL_CONTENT		= 0x0100,
	COL_ALL = COL_TYPE|COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES|COL_INDEX|COL_LINKS|COL_CONTENT
};


 /// Worker for drawing contents of file lists in child pane
struct OutputWorker
{
	OutputWorker();

	void	init_output(HWND hwnd);
	void	output_text(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str, DWORD flags);
	void	output_tabbed_text(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str);
	void	output_number(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str);

	SIZE	_spaceSize;
	TCHAR	_num_sep;
	HFONT	_hfont;
};


 /// child window pane for displaying file lists
struct Pane : public SubclassedWindow
{
	typedef SubclassedWindow super;

	Pane(HWND hparent, int id, int id_header, Entry* rool, bool treePane, int visible_cols);

#define COLUMNS 12
	int 	_widths[COLUMNS];
	int 	_positions[COLUMNS+1];
		
	WindowHandle _hwndHeader;

	Entry*	_root;
	Entry*	_cur;

	COLORREF _clrCompressed;

	int 	_visible_cols;
	bool	_treePane;

	void	init();
	void	set_header();
	bool	create_header(HWND parent, int id);

	bool	calc_widths(bool anyway);
	void	calc_single_width(int col);
	void	draw_item(LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol=-1);

	void	insert_entries(Entry* dir, int idx);
	BOOL	command(UINT cmd);
	int		Notify(int id, NMHDR* pnmh);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void	calc_width(LPDRAWITEMSTRUCT dis, int col, LPCTSTR str);
	void	calc_tabbed_width(LPDRAWITEMSTRUCT dis, int col, LPCTSTR str);
	MainFrame* get_frame();

protected:
	HIMAGELIST	_himl;
	OutputWorker _out_wrkr;
};

