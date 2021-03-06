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
#ifndef LINKAPP_H
#define LINKAPP_H

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "link.h"

enum package_type {
	PKG_GENERIC,
	PKG_OPK,
	PKG_RETROARCH,
};

/**
	Parses links files.
	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class LinkApp : public Link {
private:
	// InputManager &input;
	int		gamma = 0,
			clock = 0,
			selectorelement = 0,
			scalemode,
			_scalemode = 0,
			package_type = PKG_GENERIC;

	bool	selectorbrowser = true,
			terminal = false,
			autorun = false;

	string	exec = "",
			params = "",
			homedir = "",
			manual = "",
			manualPath = "",
			selectordir = "",
			selectorfilter = "",
			selectorscreens = "",
			aliasfile = "",
			file = "",
			icon_opk = "";

	vector<string> favourites;

public:
	LinkApp(GMenu2X *gmenu2x, const char* file);
	const string &getExec() { return exec; }
	void setExec(const string &exec);
	const string &getParams() { return params; }
	void setParams(const string &params);
	const string &getHomeDir() { return homedir; }
	void setHomeDir(const string &homedir);
	const string &getManual() { return manual; }
	const string &getManualPath() { return manualPath; }
	void setManual(const string &manual);
	const string &getSelectorDir() { return selectordir; }
	void addFavourite(const string &fav);
	void delFavourite(const string &fav);
	vector<string> &getFavourites() { return favourites; }
	void setSelectorDir(const string &selectordir);
	bool getSelectorBrowser() { return selectorbrowser; }
	void setSelectorBrowser(bool value);
	bool getTerminal() { return terminal; }
	void setTerminal(bool value);
	int getScaleMode() { return scalemode; }
	void setScaleMode(int value);
	const string &getSelectorScreens() { return selectorscreens; }
	void setSelectorScreens(const string &selectorscreens);
	const string &getSelectorFilter() { return selectorfilter; }
	void setSelectorFilter(const string &selectorfilter);
	int getSelectorElement() { return selectorelement; }
	void setSelectorElement(int i);
	const string &getAliasFile() { return aliasfile; }
	void setAliasFile(const string &aliasfile);
	int getCPU() { return clock; }
	void setCPU(int mhz = 0);
	bool save();
	void run();
	void selector(int startSelection = 0, string selectorDir = "");
	void launch(const string &selectedFile = "", string selectedDir = "");
	bool targetExists();
	void renameFile(const string &name);
	const string &getFile() { return file; }
	int getGamma() { return gamma; }
	void setGamma(int gamma);
	const string searchIcon();
	const string searchBackdrop();
	const string searchManual();
};

#endif
