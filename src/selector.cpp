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

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>

#include "linkapp.h"
#include "selector.h"
#include "debug.h"

using namespace std;

Selector::Selector(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, LinkApp *link)
: BrowseDialog(gmenu2x, title, description, icon), link(link) {
	loadAliases();
	setFilter(link->getSelectorFilter());
	directoryEnter(getPath());
}

const std::string Selector::getPreview(uint32_t i) {
	string fname = getFile(i);
	string screendir = link->getSelectorScreens();
	string noext, realdir;
	int pos = fname.rfind(".");
	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);
	if (noext.empty() || noext == ".") return "";
	if (screendir.empty()) screendir = "./.images";
	if (screendir[0] == '.') realdir = real_path(getPath() + "/" + screendir) + "/"; // allow "." as "current directory", therefore, relative paths
	else realdir = real_path(screendir) + "/";

	// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
	if (dirExists(realdir)) {
		if (fileExists(realdir + noext + ".png"))
			return realdir + noext + ".png";
		else if (fileExists(realdir + noext + ".jpg"))
			return realdir + noext + ".jpg";
	}
	else if (fileExists(noext + ".png")) // fallback - always search for ./filename.png
		return noext + ".png";
	else if (fileExists(noext + ".jpg"))
		return noext + ".jpg";

	return "";
}

void Selector::loadAliases() {
	aliases.clear();
	if (fileExists(link->getAliasFile())) {
		string line;
		ifstream infile (link->getAliasFile().c_str(), ios_base::in);
		while (getline(infile, line, '\n')) {
			string::size_type position = line.find("=");
			string name = trim(line.substr(0,position));
			string value = trim(line.substr(position+1));
			aliases[name] = value;
		}
		infile.close();
	}
}

const std::string Selector::getFileName(uint32_t i) {
	string noext, fname = at(i);
	int pos = fname.rfind(".");
	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);
	unordered_map<string, string>::iterator it = aliases.find(noext);
	if (it == aliases.end())
		return fname;
	else
		return it->second;
}
