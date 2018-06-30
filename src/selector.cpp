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

 #include <SDL.h>
 #include <SDL_gfxPrimitives.h>

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>

#include "messagebox.h"
#include "menu.h"
#include "linkapp.h"
#include "selector.h"
#include "filelister.h"
#include "debug.h"

using namespace std;

string screendir;

Selector::Selector(GMenu2X *gmenu2x, LinkApp *link, const string &selectorDir) :
Dialog(gmenu2x)
{
	this->link = link;
	loadAliases();
	selRow = 0;
	if (selectorDir == "")
		dir = link->getSelectorDir();
	else
		dir = selectorDir;
	if (dir[dir.length() - 1] != '/') dir += "/";
}

int Selector::exec(int startSelection) {
	if (gmenu2x->sc[link->getBackdrop()] != NULL) gmenu2x->sc[link->getBackdrop()]->blit(this->bg,0,0);

	bool close = false, result = true;
	vector<string> screens, titles;

	uint i, firstElement = 0, iY, animation = 0;

	FileLister fl(dir, link->getSelectorBrowser());
	fl.setFilter(link->getSelectorFilter());
	fl.browse();

	screendir = link->getSelectorScreens();

	// SDL_Rect rect;

	// if (screendir == "") {
		// drawTopBar(this->bg);
		// rect = gmenu2x->listRect; //{0, gmenu2x->skinConfInt["topBarHeight"], gmenu2x->resX, gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"] - gmenu2x->skinConfInt["topBarHeight"]};
	// } else {
		// this->bg->box(0, 0, gmenu2x->skinConfInt["selectorX"], gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"], gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
		// this->bg->setClipRect(0, 0, gmenu2x->skinConfInt["selectorX"] - 4, gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"]);
		// rect = (SDL_Rect){gmenu2x->skinConfInt["selectorX"], 0, gmenu2x->resX - gmenu2x->skinConfInt["selectorX"], gmenu2x->resY - gmenu2x->skinConfInt["bottomBarHeight"]};
	// }

	// dc: adjust rowHeight with font
	uint rowHeight = gmenu2x->font->getHeight() + 1; // gp2x=15+1 / pandora=19+1
	uint numRows = gmenu2x->listRect.h / rowHeight - 1;

	// drawTitleIcon(link->getIconPath(), this->bg);
	// writeTitle(link->getTitle(), this->bg);
	// writeSubTitle(link->getDescription(), this->bg);

	// this->bg->clearClipRect();

	// this->bg->box(rect, gmenu2x->skinConfColors[COLOR_LIST_BG]);
	// drawBottomBar(this->bg);


	drawTopBar(this->bg, link->getTitle(), link->getDescription(), link->getIconPath());
	drawBottomBar(this->bg);
	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	if (link->getSelectorBrowser()) {
		gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"],
			gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Folder up"],
				gmenu2x->drawButton(this->bg, "start", gmenu2x->tr["Exit"], 5)));
	} else {
		gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Exit"],
			gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"], 5));
	}

	prepare(&fl, &screens, &titles);
	int selected = constrain(startSelection, 0, fl.size() - 1);

	// moved surfaces out to prevent reloading on loop
	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");
	Surface *iconPreview = gmenu2x->sc.skinRes("imgs/preview.png");

	gmenu2x->sc.defaultAlpha = false;
	// gmenu2x->input.setWakeUpInterval(1); // refresh on load
	Uint32 tickStart = SDL_GetTicks();

	while (!close) {
		this->bg->blit(gmenu2x->s, 0, 0);

		if (selected > firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		iY = selected - firstElement;
		iY = gmenu2x->listRect.y + (iY * rowHeight);

		// if (selected < fl.size()) {
			gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
		// }


		//Files & Dirs
		iY = gmenu2x->listRect.y + 1;

		for (i = firstElement; i < fl.size() && i <= firstElement + numRows; i++) {
			if (fl.isDirectory(i)) {
				if (fl[i] == "..")
					iconGoUp->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2);
				else
					iconFolder->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2);
			} else {
				iconFile->blitCenter(gmenu2x->s, gmenu2x->listRect.x + 10, iY + rowHeight/2);
			}
			gmenu2x->s->write(gmenu2x->font, fl[i], gmenu2x->listRect.x + 21, iY + 4, HAlignLeft, VAlignMiddle);

			iY += rowHeight;
		}

		//Screenshot
		if (selected - fl.dirCount() < screens.size() && screens[selected - fl.dirCount()] != "") {
			gmenu2x->s->box(320 - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["selectorX"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
			gmenu2x->sc[screens[selected - fl.dirCount()]]->blitCenter(gmenu2x->s, 320 - animation + (gmenu2x->skinConfInt["selectorPreviewX"] + gmenu2x->skinConfInt["selectorPreviewWidth"]/2), gmenu2x->skinConfInt["selectorPreviewY"] + gmenu2x->skinConfInt["selectorPreviewHeight"]/2, gmenu2x->skinConfInt["selectorPreviewWidth"], gmenu2x->skinConfInt["selectorPreviewHeight"]);

			if (animation < gmenu2x->skinConfInt["selectorX"]) {
				animation = intTransition(0, gmenu2x->skinConfInt["selectorX"], tickStart, 100);
				gmenu2x->s->flip();
				gmenu2x->input.setWakeUpInterval(10);
				continue;
			}
		} else {
			if (animation > 0) {
				gmenu2x->s->box(320 - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["selectorX"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
				animation = gmenu2x->skinConfInt["selectorX"] - intTransition(0, gmenu2x->skinConfInt["selectorX"], tickStart, 100);
				gmenu2x->s->flip();
				gmenu2x->input.setWakeUpInterval(10);
				continue;
			}
			// animation = 0;
		}
		gmenu2x->input.setWakeUpInterval(1000);

		// if (screendir != "") {
		// 	if (selected - fl.dirCount() < screens.size() && screens[selected - fl.dirCount()] != "") {
		// 		gmenu2x->sc[screens[selected - fl.dirCount()]]->blitCenter(gmenu2x->s, gmenu2x->skinConfInt["selectorPreviewX"] + gmenu2x->skinConfInt["selectorPreviewWidth"]/2, gmenu2x->skinConfInt["selectorPreviewY"] + gmenu2x->skinConfInt["selectorPreviewHeight"]/2, gmenu2x->skinConfInt["selectorPreviewWidth"], gmenu2x->skinConfInt["selectorPreviewHeight"]);
		// 	} else {
		// 		if (gmenu2x->sc.skinRes("imgs/preview.png") != NULL)
		// 			iconPreview->blitCenter(gmenu2x->s, gmenu2x->skinConfInt["selectorPreviewX"] + gmenu2x->skinConfInt["selectorPreviewWidth"]/2, gmenu2x->skinConfInt["selectorPreviewY"] + gmenu2x->skinConfInt["selectorPreviewHeight"]/2, gmenu2x->skinConfInt["selectorPreviewWidth"], gmenu2x->skinConfInt["selectorPreviewHeight"]);
		// 	}
		// }


		gmenu2x->s->clearClipRect();
		gmenu2x->drawScrollBar(numRows, fl.size(), firstElement, gmenu2x->listRect);
		gmenu2x->s->flip();
		
		bool inputAction = gmenu2x->input.update();
		if (gmenu2x->inputCommonActions(inputAction)) continue;

		if ( gmenu2x->input[UP] ) {
			tickStart = SDL_GetTicks();
			selected -= 1;
			if (selected < 0) selected = fl.size()-1;
		} else if ( gmenu2x->input[DOWN] ) {
			tickStart = SDL_GetTicks();
			selected += 1;
			if (selected >= fl.size()) selected = 0;
		} else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT] ) {
			tickStart = SDL_GetTicks();
			selected -= numRows;
			if (selected < 0) selected = 0;
		} else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT] ) {
			tickStart = SDL_GetTicks();
			selected += numRows;
			if (selected > fl.size()) selected = fl.size() - 1;
		} else if ( gmenu2x->input[SETTINGS] ) {
			close = true; result = false;
		} else if ( gmenu2x->input[MENU] ) {
			gmenu2x->editLink();
		} else if ( gmenu2x->input[CANCEL] ) {
			if (link->getSelectorBrowser()) {
				string::size_type p = dir.rfind("/", dir.size() - 2);
				if (p == string::npos || dir.compare(0, CARD_ROOT_LEN, CARD_ROOT) != 0 || p < 4) {
					close = true;
					result = false;
				} else {
					dir = dir.substr(0, p + 1);
					// INFO("%s", dir.c_str());
					selected = 0;
					firstElement = 0;
					prepare(&fl, &screens, &titles);
				}
			} else {
				close = true;
				result = false;
			}
		} else if ( gmenu2x->input[CONFIRM] ) {
			if (fl.isFile(selected)) {
				file = fl[selected];
				close = true;
			} else {
				dir = real_path(dir + "/" + fl[selected]);//+"/";
				selected = 0;
				firstElement = 0;
				prepare(&fl, &screens, &titles);
			}
		}
	}

	gmenu2x->sc.defaultAlpha = true;
	freeScreenshots(&screens);
	// gmenu2x->input.setWakeUpInterval(0);

	return result ? (int)selected : -1;
}

void Selector::prepare(FileLister *fl, vector<string> *screens, vector<string> *titles) {
	fl->setPath(dir);
	freeScreenshots(screens);
	screens->resize(fl->getFiles().size());
	titles->resize(fl->getFiles().size());

	string noext, realdir;
	string::size_type pos;
	for (uint i = 0; i < fl->getFiles().size(); i++) {
		noext = fl->getFiles()[i];
		pos = noext.rfind(".");
		if (pos != string::npos && pos>0)
			noext = noext.substr(0, pos);
		titles->at(i) = getAlias(noext);

		if (screendir != "") {
			if (screendir[0] == '.') realdir = real_path(fl->getPath() + "/" + screendir) + "/"; // allow "." as "current directory", therefore, relative paths
			else realdir = real_path(screendir) + "/";
			// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
			if (fileExists(realdir + noext + ".jpg"))
				screens->at(i) = realdir + noext + ".jpg";
			else if (fileExists(realdir + noext + ".png"))
				screens->at(i) = realdir + noext + ".png";
			else if (fileExists(real_path(fl->getPath() + "/" + noext + ".png")))
				screens->at(i) = real_path(fl->getPath() + "/" + noext + ".png");
			else if (fileExists(real_path(fl->getPath() + "/" + noext + ".jpg")))
				screens->at(i) = real_path(fl->getPath() + "/" + noext + ".jpg");
			else
				screens->at(i) = "";
		}
	}
}

void Selector::freeScreenshots(vector<string> *screens) {
	for (uint i = 0; i < screens->size(); i++) {
		if (screens->at(i) != "")
			gmenu2x->sc.del(screens->at(i));
	}
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

string Selector::getAlias(const string &key) {
	unordered_map<string, string>::iterator i = aliases.find(key);
	if (i == aliases.end())
		return key;
	else
		return i->second;
}
