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

#include <iostream>

#include "wallpaperdialog.h"
#include "filelister.h"
#include "debug.h"
#include "messagebox.h"

using namespace std;

WallpaperDialog::WallpaperDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
	: Dialog(gmenu2x) {
	this->title = title;
	this->description = description;
	this->icon = icon;
}

bool WallpaperDialog::exec()
{
	gmenu2x->initBG();

	bool close = false, result = true;

	// dc: adjust rowheight with font
	uint rowHeight = gmenu2x->font->getHeight()+1; // gp2x=15+1 / pandora=19+1
	uint numRows = gmenu2x->listRect.h/rowHeight - 1;

	FileLister fl("skins/"+gmenu2x->confStr["skin"]+"/wallpapers");
	fl.setFilter(".png,.jpg,.jpeg,.bmp");
	vector<string> wallpapers;
	if (dirExists("skins/"+gmenu2x->confStr["skin"]+"/wallpapers")) {
		fl.browse();
		wallpapers = fl.getFiles();
	}
	if (gmenu2x->confStr["skin"] != "Default") {
		fl.setPath("skins/Default/wallpapers",true);
		for (uint i=0; i<fl.getFiles().size(); i++)
			wallpapers.push_back(fl.getFiles()[i]);
	}

	DEBUG("Wallpapers: %i", wallpapers.size());

	uint i, selected = 0, firstElement = 0, iY;

	while (!close) {
		if (selected>firstElement+numRows) firstElement=selected-numRows;
		if (selected<firstElement) firstElement=selected;

		//Wallpaper
		if (selected<wallpapers.size()-fl.getFiles().size())
			gmenu2x->sc["skins/"+gmenu2x->confStr["skin"]+"/wallpapers/"+wallpapers[selected]]->blit(gmenu2x->s,0,0);
		else
			gmenu2x->sc["skins/Default/wallpapers/"+wallpapers[selected]]->blit(gmenu2x->s,0,0);

		gmenu2x->drawTopBar(gmenu2x->s);
		gmenu2x->drawBottomBar(gmenu2x->s);

		writeTitle(title);
		writeSubTitle(description);
		drawTitleIcon(icon, true);

		gmenu2x->s->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);
		gmenu2x->drawButton(gmenu2x->s, "a", gmenu2x->tr["Select"],
		gmenu2x->drawButton(gmenu2x->s, "b", gmenu2x->tr["Back"],5));

		//Selection
		iY = selected-firstElement;
		iY = gmenu2x->listRect.y+(iY*rowHeight);

		gmenu2x->s->box(0, iY + 3, gmenu2x->resX, rowHeight-1, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

		//Files & Directories
		for (i=firstElement; i<wallpapers.size() && i<=firstElement+numRows; i++) {
			iY = i-firstElement;
			iY = iY*rowHeight + 3 + gmenu2x->listRect.y + rowHeight/3;

			gmenu2x->s->write(gmenu2x->font, wallpapers[i], 5, iY, HAlignLeft, VAlignMiddle);
		}

		gmenu2x->drawScrollBar(numRows, wallpapers.size(), firstElement, gmenu2x->listRect);

		gmenu2x->s->flip();

		gmenu2x->input.update();

// COMMON ACTIONS
		if ( gmenu2x->input.isActive(MODIFIER) ) {
			if (gmenu2x->input.isActive(SECTION_NEXT)) {
				if (!gmenu2x->saveScreenshot()) { continue; }
				MessageBox mb(gmenu2x, gmenu2x->tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
				continue;
			} else if (gmenu2x->input.isActive(SECTION_PREV)) {
				int vol = gmenu2x->getVolume();
				if (vol) {
					vol = 0;
					gmenu2x->volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					gmenu2x->volumeMode = VOLUME_MODE_NORMAL;
				}
				gmenu2x->confInt["globalVolume"] = vol;
				gmenu2x->setVolume(vol);
				gmenu2x->writeConfig();
				continue;
			}
		}
		// BACKLIGHT
		else if ( gmenu2x->input[BACKLIGHT] ) gmenu2x->setBacklight(gmenu2x->confInt["backlight"], true);
// END OF COMMON ACTIONS

		else if ( gmenu2x->input[SETTINGS] ) {
			close = true;
			result = false;
		} else if ( gmenu2x->input[UP] ) {
			if (selected == 0)
				selected = wallpapers.size()-1;
			else
				selected -= 1;
		} else if ( gmenu2x->input[DOWN] ) {
			if (selected + 1 >= wallpapers.size())
				selected = 0;
			else
				selected += 1;
		} else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT] ) {
			if (selected < numRows)
				selected = 0;
			else
				selected -= numRows;
		} else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT] ) {
			if (selected + numRows >= wallpapers.size())
				selected = wallpapers.size()-1;
			else
				selected += numRows;
		} else if ( gmenu2x->input[CANCEL] ) {
			close = true;
			result = false;
		} else if ( gmenu2x->input[CONFIRM] ) {
			close = true;
			if (wallpapers.size()>0) {
				if (selected<wallpapers.size()-fl.getFiles().size())
					wallpaper = "skins/"+gmenu2x->confStr["skin"]+"/wallpapers/"+wallpapers[selected];
				else
					wallpaper = "skins/Default/wallpapers/"+wallpapers[selected];
			} else {
				result = false;
			}
		}
	}

	for (uint i=0; i<wallpapers.size(); i++)
		if (i<wallpapers.size()-fl.getFiles().size())
			gmenu2x->sc.del("skins/"+gmenu2x->confStr["skin"]+"/wallpapers/"+wallpapers[i]);
		else
			gmenu2x->sc.del("skins/Default/wallpapers/"+wallpapers[i]);

	return result;
}
