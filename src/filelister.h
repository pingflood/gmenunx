/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef FILELISTER_H_
#define FILELISTER_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

class FileLister {
private:
	vector<string> directories, files, excludes;

public:
	string path, filter;
	FileLister();
	bool showDirectories = true, showFiles = true, showFullPath = false, allowDirUp = true;
	void browse(string _path);

	uint32_t size();
	uint32_t dirCount();
	uint32_t fileCount();
	string operator[](uint32_t);
	string getFile(uint32_t);
	bool isFile(uint32_t);
	bool isDirectory(uint32_t);
	const string getExt(uint32_t i = 0);
	const string getPath(uint32_t i = 0);
	const string &getDir() { return path; }
	void setPath(const string &path);
	const string &getFilter() { return filter; }
	void setFilter(const string &filter);
	const vector<string> &getDirectories() { return directories; }
	const vector<string> &getFiles() { return files; }
	void insertFile(const string &file);
	void addExclude(const string &exclude);
};

#endif /*FILELISTER_H_*/
