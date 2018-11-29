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

// #include "messagebox.h"
#include "linkapp.h"
#include "selector.h"
#include "debug.h"

using namespace std;

Selector::Selector(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon, LinkApp *link)
: BrowseDialog(gmenu2x, title, description, icon), link(link) {
	loadAliases();
	directoryEnter(getPath());
}


// string Selector::getScreenshot(const string &path, const string &fname, const string &screendir)
const std::string Selector::getPreview(uint32_t i) {
	string fname = getFilePath(i);
	string screendir = link->getSelectorScreens();
	string noext, realdir;
	int pos = fname.rfind(".");
	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);

	WARNING("noext: %s", noext.c_str());
	if (noext == "") return "";

	if (screendir != "") {
		if (screendir[0] == '.') realdir = real_path(getPath() + "/" + screendir) + "/"; // allow "." as "current directory", therefore, relative paths
		else realdir = real_path(screendir) + "/";
		// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
		if (fileExists(realdir + noext + ".png")) {
			return realdir + noext + ".png";
		} else if (fileExists(realdir + noext + ".jpg")){
			return realdir + noext + ".jpg";
		}
	}
	// fallback - always search for filename.png
	if (fileExists(noext + ".png"))
		return noext + ".png";
	else if (fileExists(noext + ".jpg"))
		return noext + ".jpg";
	else
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
const std::string Selector::getFile(uint32_t i) {
	string noext, fname = at(i);
	int pos = fname.rfind(".");
	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);
	unordered_map<string, string>::iterator it = aliases.find(noext);
	if (it == aliases.end())
		return fname;
	else
		return it->second;
}



// using namespace std;

// Selector::Selector(GMenu2X *gmenu2x, LinkApp *link, const string &selectorDir) :
// Dialog(gmenu2x)
// {
// 	this->link = link;
// 	loadAliases();

// 	if (selectorDir == "")
// 		dir = link->getSelectorDir();
// 	else
// 		dir = selectorDir;
// 	if (dir[dir.length() - 1] != '/') dir += "/";

// 	FileLister fl(dir, link->getSelectorBrowser());
// 	fl.setFilter(link->getSelectorFilter());
// }

// int Selector::exec(int startSelection) {
// 	if (gmenu2x->sc[link->getBackdrop()] != NULL) gmenu2x->sc[link->getBackdrop()]->blit(this->bg,0,0);

// 	bool close = false, result = true, inputAction = false;

// 	uint32_t i, firstElement = 0, iY, animation = 0, padding = 6;
// 	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
// 	uint32_t numRows = (gmenu2x->listRect.h - 2)/rowHeight - 1;

// 	drawTopBar(this->bg, link->getTitle(), link->getDescription(), link->getIconPath());
// 	drawBottomBar(this->bg);
// 	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

// 	gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"],
// 	gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Exit"], 5));

// 	// moved surfaces out to prevent reloading on loop
// 	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
// 	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
// 	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");
// 	// Surface *iconPreview = gmenu2x->sc.skinRes("imgs/preview.png");

// 	gmenu2x->sc.defaultAlpha = false;
// 	// gmenu2x->input.setWakeUpInterval(1); // refresh on load

// 	this->bg->blit(gmenu2x->s, 0, 0);
// 	gmenu2x->s->flip();

// 	if (dir.empty() || !dirExists(dir))
// 		dir = CARD_ROOT;

// 	setPath(dir);

// 	int selected = constrain(startSelection, 0, fl.size() - 1);

// 	uint32_t tickStart = SDL_GetTicks();

// 	while (!close) {
// 		this->bg->blit(gmenu2x->s, 0, 0);

// 		if (!fl.size()) {
// 			MessageBox mb(gmenu2x, gmenu2x->tr["This directory is empty"]);
// 			mb.setAutoHide(-1);
// 			mb.setBgAlpha(0);
// 			mb.exec();
// 		} else {
// 			//Selection
// 			if (selected >= firstElement + numRows) firstElement = selected - numRows;
// 			if (selected < firstElement) firstElement = selected;

// 			//Files & Directories
// 			iY = gmenu2x->listRect.y + 1;
// 			for (i = firstElement; i < fl.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
// 				if (i == selected) gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
// 				if (fl.isDirectory(i)) {
// 					if (fl[i] == "..")
// 						iconGoUp->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
// 					else
// 						iconFolder->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
// 				} else {
// 					iconFile->blit(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
// 				}
// 				gmenu2x->s->write(gmenu2x->font, getAlias(fl[i]), gmenu2x->listRect.x + 21, iY + rowHeight/2, VAlignMiddle);
// 			}

// 			//Screenshot
// 			string screenshot = getScreenshot(dir, fl[selected], link->getSelectorScreens());

// 			if (screenshot != "") {
// 				gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);

// 				gmenu2x->sc[screenshot]->softStretch(gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding, true, false);
// 				gmenu2x->sc[screenshot]->blit(gmenu2x->s, {gmenu2x->resX - animation + padding, gmenu2x->listRect.y + padding, gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding}, HAlignCenter | VAlignMiddle, 220);

// 				if (animation < gmenu2x->skinConfInt["previewWidth"]) {
// 					animation = intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 110);
// 					gmenu2x->s->flip();
// 					gmenu2x->input.setWakeUpInterval(45);
// 					continue;
// 				}
// 			} else {
// 				if (animation > 0) {
// 					gmenu2x->s->box(gmenu2x->resX - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
// 					animation = gmenu2x->skinConfInt["previewWidth"] - intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 80);
// 					gmenu2x->s->flip();
// 					gmenu2x->input.setWakeUpInterval(45);
// 					continue;
// 				}
// 			}
// 			gmenu2x->input.setWakeUpInterval(1000);
// 			// gmenu2x->s->clearClipRect();
// 			gmenu2x->drawScrollBar(numRows, fl.size(), firstElement, gmenu2x->listRect);
// 			gmenu2x->s->flip();
// 		}

// 		do {
// 			inputAction = gmenu2x->input.update();
// 			if (gmenu2x->inputCommonActions(inputAction)) continue;
// 			if (inputAction) tickStart = SDL_GetTicks();

// 			if ( gmenu2x->input[UP] ) {
// 				selected -= 1;
// 				if (selected < 0) selected = fl.size() - 1;
// 			} else if ( gmenu2x->input[DOWN] ) {
// 				selected += 1;
// 				if (selected >= fl.size()) selected = 0;
// 			} else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT] ) {
// 				selected -= numRows;
// 				if (selected < 0) selected = 0;
// 			} else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT] ) {
// 				selected += numRows;
// 				if (selected >= fl.size()) selected = fl.size() - 1;
// 			} else if ( gmenu2x->input[CANCEL] ) {
// 				close = true;
// 				result = false;
// 			// } else if ( gmenu2x->input[SETTINGS] ) {
// 			// 	close = true;
// 			// 	result = false;
// 			// } else if ( gmenu2x->input[MENU] ) {
// 				// gmenu2x->editLink();
// 			} else if ( gmenu2x->input[MODIFIER] && link->getSelectorBrowser()) { /*Directory Up */
// 				string::size_type p = dir.rfind("/", dir.size() - 2);
// 				dir = dir.substr(0, p + 1);
// 				setPath(dir);
// 			} else if ( gmenu2x->input[CONFIRM] ) {
// 				if (fl.isFile(selected)) {
// 					file = fl[selected];
// 					close = true;
// 				} else {
// 					dir = real_path(dir + "/" + fl[selected]);//+"/";
// 					setPath(dir);
// 					selected = 0;
// 					firstElement = 0;
// 				}
// 			}
// 		} while (!inputAction);
// 	}

// 	gmenu2x->sc.defaultAlpha = true;
// 	// freeScreenshots(&screens);

// 	return result ? (int)selected : -1;
// }

// void Selector::freeScreenshots(vector<string> *screens) {
// 	for (uint32_t i = 0; i < screens->size(); i++) {
// 		if (screens->at(i) != "")
// 			gmenu2x->sc.del(screens->at(i));
// 	}
// }


// string Selector::getScreenshot(const string &path, const string &fname, const string &screendir) {
// 	string noext, realdir;
// 	int pos = fname.rfind(".");
// 	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);

// 	if (screendir != "") {
// 		if (screendir[0] == '.') realdir = real_path(path + "/" + screendir) + "/"; // allow "." as "current directory", therefore, relative paths
// 		else realdir = real_path(screendir) + "/";
// 		// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
// 		if (fileExists(realdir + noext + ".png")) {
// 			return realdir + noext + ".png";
// 		} else if (fileExists(realdir + noext + ".jpg")){
// 			return realdir + noext + ".jpg";
// 		}
// 	}
// 	// fallback - always search for filename.png
// 	if (fileExists(real_path(path + "/" + noext + ".png")))
// 		return real_path(path + "/" + noext + ".png");
// 	else if (fileExists(real_path(path + "/" + noext + ".jpg")))
// 		return real_path(path + "/" + noext + ".jpg");
// 	else
// 		return "";
// }

// string Selector::getAlias(const string &fname) {
// 	string noext, realdir;
// 	int pos = fname.rfind(".");
// 	if (pos != string::npos && pos > 0) noext = fname.substr(0, pos);
// 	unordered_map<string, string>::iterator i = aliases.find(noext);
// 	if (i == aliases.end())
// 		return fname;
// 	else
// 		return i->second;
// }
// void Selector::setPath(const string &path) {
// 	MessageBox mb(gmenu2x, gmenu2x->tr["Loading"]);
// 	mb.setAutoHide(-1);
// 	mb.setBgAlpha(0);
// 	mb.exec(3e3);

// 	fl.setPath(path);

// 	mb.clearTimer();
// }
