/*
 * Copyright 2004 Martin Fuchs
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
 // Explorer and Desktop clone
 //
 // favorites.h
 //
 // Martin Fuchs, 04.04.2004
 //


struct Bookmark
{
	Bookmark() : _icon_idx(0) {}

	String	_name;
	String	_description;
	String	_url;
	String	_icon_path;
	int		_icon_idx;

	bool	read_url(LPCTSTR path);
	bool	read(const_XMLPos& pos);
	void	write(XMLPos& pos) const;
};

struct BookmarkFolder;

struct BookmarkNode
{
	BookmarkNode();
	BookmarkNode(const Bookmark& bm);
	BookmarkNode(const BookmarkFolder& bmf);
	BookmarkNode(const BookmarkNode& other);

	~BookmarkNode();

	BookmarkNode& operator=(const Bookmark& bm);
	BookmarkNode& operator=(const BookmarkFolder& bmf);
	BookmarkNode& operator=(const BookmarkNode& other);

	void	clear();

	enum BOOKMARKNODE_TYPE {
		BMNT_NONE, BMNT_BOOKMARK, BMNT_FOLDER
	};

	BOOKMARKNODE_TYPE	_type;

	union {
		Bookmark*		_pbookmark;
		BookmarkFolder* _pfolder;
	};
};

struct BookmarkList : public list<BookmarkNode>
{
	void	import_IE_favorites(struct ShellDirectory& dir, HWND hwnd);

	void	read(const_XMLPos& pos);
	void	write(XMLPos& pos) const;

	void	fill_tree(HWND hwnd, HTREEITEM parent, HIMAGELIST, HDC hdc_wnd) const;
};

struct BookmarkFolder
{
	String	_name;
	String	_description;
	BookmarkList _bookmarks;

	void	read(const_XMLPos& pos);
	void	write(XMLPos& pos) const;
};

struct Favorites : public BookmarkList
{
	typedef BookmarkList super;

	bool	read(LPCTSTR path);
	void	write(LPCTSTR path) const;

	bool	import_IE_favorites(HWND hwnd);
};
