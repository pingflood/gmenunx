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
// #include <iostream>

#include "link.h"

using std::string;

class GMenu2X;
class InputManager;

/**
	Parses links files.
	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class LinkApp : public Link {
private:
	InputManager &inputMgr;
	int iclock = 0, selectorelement = 0, scalemode, _scalemode = 0; //, ivolume = 0;
	bool selectorbrowser;
	string params, workdir, manual, manualPath, selectordir, selectorfilter, selectorscreens, aliasfile, file; //, backdrop, backdropPath, resolution;

public:
	LinkApp(GMenu2X *gmenu2x, InputManager &inputMgr, const char* file);
	const string &getExec();
	void setExec(const string &exec);
	const string &getParams();
	void setParams(const string &params);
	const string &getWorkdir();
	const string getRealWorkdir();
	void setWorkdir(const string &workdir);
	const string &getManual();
	const string &getManualPath() { return manualPath; }
	void setManual(const string &manual);
	const string &getSelectorDir();
	void setSelectorDir(const string &selectordir);
	bool getSelectorBrowser();
	void setSelectorBrowser(bool value);
	int getScaleMode();
	void setScaleMode(int value);
	const string &getSelectorScreens();
	void setSelectorScreens(const string &selectorscreens);
	const string &getSelectorFilter();
	void setSelectorFilter(const string &selectorfilter);
	int getSelectorElement();
	void setSelectorElement(int i);
	const string &getAliasFile();
	void setAliasFile(const string &aliasfile);
	int getCPU();
	void setCPU(int mhz = 0);
	bool save();
	void run();
	void selector(int startSelection=0, const string &selectorDir = "");
	void launch(const string &selectedFile="", string selectedDir = "");
	bool targetExists();
	void renameFile(const string &name);
	const string &getFile() { return file; }

#if defined(TARGET_GP2X)
	// int volume();
	// const string &volumeStr();
	// void setVolume(int vol);
	// bool getUseRamTimings();
	// void setUseRamTimings(bool value);
	// bool getUseGinge();
	// void setUseGinge(bool value);
	// const string &clockStr(int maxClock);
	// string sgamma;
	int igamma;
	int gamma();
	// const string &gammaStr();
	void setGamma(int gamma);
	// bool &needsWrapperRef() { return wrapper; }
	// bool &runsInBackgroundRef() { return dontleave; }
#endif

	const string &searchIcon();
	const string &searchBackdrop();
	const string &searchManual();
};

#endif
