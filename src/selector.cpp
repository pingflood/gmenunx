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
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <dirent.h>
#include <fstream>
#include "messagebox.h"
#include "linkapp.h"
#include "selector.h"
#include "debug.h"
#include "menu.h"

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

	INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
	if (dir_exists(realdir)) {
		if (file_exists(realdir + noext + ".png"))
			return realdir + noext + ".png";
		else if (file_exists(realdir + noext + ".jpg"))
			return realdir + noext + ".jpg";
	}
	else if (file_exists(getPath() + "/" + noext + ".png")) // fallback - always search for ./filename.png
		return getPath() + "/" + noext + ".png";
	else if (file_exists(getPath() + "/" + noext + ".jpg"))
		return getPath() + "/" + noext + ".jpg";

	return "";
}

void Selector::loadAliases() {
	aliases.clear();
	params.clear();
	if (file_exists(link->getAliasFile())) {
		string line;
		ifstream infile (link->getAliasFile().c_str(), ios_base::in);
		while (getline(infile, line, '\n')) {
			string name, value;
			int d1 = line.find("=");
			int d2 = line.find(";");
			if (d2 > d1) d2 -= d1 + 1;

			name = lowercase(trim(line.substr(0, d1)));
			aliases[name] = trim(line.substr(d1 + 1, d2));


			if (d2 > 0)
				params[name] = trim(line.substr(d1 + d2 + 2));
		}
		infile.close();
	}
}

const std::string Selector::getFileName(uint32_t i) {
	string fname = at(i);
	string noext = lowercase(fname);
	int d1 = fname.rfind(".");
	if (d1 != string::npos && d1 > 0)
		noext = lowercase(fname.substr(0, d1));

	unordered_map<string, string>::iterator it = aliases.find(noext);
	if (it == aliases.end() || it->second.empty()) return fname;
	return it->second;
}

const std::string Selector::getParams(uint32_t i) {
	string fname = at(i);
	string noext = fname;
	int d1 = fname.rfind(".");
	if (d1 != string::npos && d1 > 0)
		noext = lowercase(fname.substr(0, d1));

	unordered_map<string, string>::iterator it = params.find(noext);
	if (it == params.end()) return "";
	return it->second;
}

string favicon;
bool Selector::customAction(bool &inputAction) {
	return false;
	if ( gmenu2x->input[MENU] ) {
		favicon = getPreview(selected);
		WARNING("favicon1: %s", favicon.c_str());
		if (favicon.empty()) favicon = this->icon;
		WARNING("favicon2: %s", favicon.c_str());
		contextMenu();
		return true;
	}
	return false;
}

void Selector::addFavourite() {
	WARNING("favicon3: %s", favicon.c_str());
	gmenu2x->menu->addLink(link->getExec(), "favourites", getFileName(selected), description, favicon, link->getParams() + " " + cmdclean(getFilePath(selected)));
}

void Selector::contextMenu() {
	ERROR("%s:%d", __func__, __LINE__);
	vector<MenuOption> options;
	// if (menu->selLinkApp() != NULL) {
	// 	options.push_back((MenuOption){tr.translate("Edit $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::editLink)});
	// 	options.push_back((MenuOption){tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::deleteLink)});
	// }
	ERROR("%s:%d", __func__, __LINE__);
	options.push_back((MenuOption){gmenu2x->tr["Add to home screen"], 		MakeDelegate(this, &Selector::addFavourite)});
	// options.push_back((MenuOption){tr["Add section"],	MakeDelegate(this, &GMenu2X::addSection)});
	// options.push_back((MenuOption){tr["Rename section"],	MakeDelegate(this, &GMenu2X::renameSection)});
	// options.push_back((MenuOption){tr["Delete section"],	MakeDelegate(this, &GMenu2X::deleteSection)});
	// options.push_back((MenuOption){tr["Link scanner"],	MakeDelegate(this, &GMenu2X::linkScanner)});
	ERROR("%s:%d", __func__, __LINE__);

	MessageBox mb(gmenu2x, options);
}
