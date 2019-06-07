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

#ifndef GMENU2X_H
#define GMENU2X_H

class PowerManager;

#include <iostream>
#include <string>
#include <vector>
#include <tr1/unordered_map>

#include "surfacecollection.h"
#include "iconbutton.h"
#include "translator.h"
#include "FastDelegate.h"
#include "utilities.h"
#include "touchscreen.h"
#include "inputmanager.h"
#include "surface.h"
#include "fonthelper.h"


const int MAX_VOLUME_SCALE_FACTOR = 200;
// Default values - going to add settings adjustment, saving, loading and such
const int VOLUME_SCALER_MUTE = 0;
const int VOLUME_SCALER_PHONES = 65;
const int VOLUME_SCALER_NORMAL = 100;
const int BATTERY_READS = 10;

extern const char *CARD_ROOT;
extern const int CARD_ROOT_LEN;

// Note: Keep this in sync with colorNames!
enum color {
	COLOR_TOP_BAR_BG,
	COLOR_LIST_BG,
	COLOR_BOTTOM_BAR_BG,
	COLOR_SELECTION_BG,
	COLOR_PREVIEW_BG,
	COLOR_MESSAGE_BOX_BG,
	COLOR_MESSAGE_BOX_BORDER,
	COLOR_MESSAGE_BOX_SELECTION,
	COLOR_FONT,
	COLOR_FONT_OUTLINE,
	COLOR_FONT_ALT,
	COLOR_FONT_ALT_OUTLINE,

	NUM_COLORS,
};

enum sb {
	SB_OFF,
	SB_LEFT,
	SB_BOTTOM,
	SB_RIGHT,
	SB_TOP,
	SB_CLASSIC,
};

enum bd {
	BD_OFF,
	BD_MENU,
	BD_DIALOG,
};


enum tvout {
	TV_OFF  = CANCEL,
	TV_PAL  = MANUAL,
	TV_NTSC = CONFIRM,
};

using std::string;
using std::vector;
using fastdelegate::FastDelegate0;

typedef FastDelegate0<> MenuAction;
typedef unordered_map<string, string, hash<string> > ConfStrHash;
typedef unordered_map<string, int, hash<string> > ConfIntHash;

struct MenuOption {
	string text;
	MenuAction action;
};

char *ms2hms(uint32_t t, bool mm, bool ss);

class Menu;

class GMenu2X {
private:
	static GMenu2X *instance;
	int getVolume();

	string path; //!< Contains the working directory of GMenu2X
	/*!
	Retrieves the free disk space on the sd
	@return String containing a human readable representation of the free disk space
	*/
	string getDiskFree(const char *path);

	/*!
	Starts the scanning of the nand and sd filesystems, searching for gpe and gpu files and creating the links in 2 dedicated sections.
	*/
	// void linkScanner();
	/*!
	Performs the actual scan in the given path and populates the files vector with the results. The creation of the links is not performed here.
	@see scanner
	*/
	// void scanPath(string path, vector<string> *files);

	/*!
	Displays a selector and launches the specified executable file
	*/
	void explorer();

	string lastSelectorDir, currBackdrop;
	int lastSelectorElement;
	void readConfig();
	void readTmp();

	void initFont();
	void initMenu();

	void showManual();
	void umountSdDialog();

	virtual void udcDialog(int udcStatus = -1) { };
	virtual void tvOutDialog(int TVOut = -1) { };
	virtual void hwInit() { };
	virtual void hwDeinit() {};
	virtual int getBacklight() { return -1; };

public:

	/*
	 * Variables needed for elements disposition
	 */
	uint32_t resX = 320, resY = 240;
	uint32_t linkCols, linkRows, linkWidth, linkHeight, linkSpacing = 4;
	SDL_Rect listRect, linksRect, sectionBarRect, bottomBarRect;
	/*!
	Retrieves the parent directory of GMenu2X.
	This functions is used to initialize the "path" variable.
	@see path
	@return String containing the parent directory
	*/
	const string &getExePath();

	//Configuration hashes
	ConfStrHash confStr, skinConfStr;
	ConfIntHash confInt, skinConfInt;

	RGBAColor skinConfColors[NUM_COLORS];
	SurfaceCollection sc;
	Surface *s, *bg, *iconInet = NULL;
	Translator tr;
	FontHelper *font = NULL, *titlefont = NULL;
	PowerManager *powerManager;
	InputManager input;
	Touchscreen ts;
	Menu *menu;
	bool f200 = true; //gp2x type // touchscreen
	uint32_t section_changed = 0, icon_changed = 0;

	~GMenu2X();
	void quit();
	void main();
	void settings();
	void restartDialog(bool showDialog = false);
	void poweroffDialog();
	void resetSettings();
	void cpuSettings();

	void setSkin(const string &skin, bool clearSC = true);
	void skinMenu();
	void skinColors();
	uint32_t onChangeSkin();
	void initLayout();

	bool inputCommonActions(bool &inputAction);

	unsigned int TVOut = 0;
	void about();
	void viewLog();
	// void batteryLogger();
	void contextMenu();
	void changeWallpaper();

	const string getDateTime();
	void setDateTime(const char* _datetime);

	bool saveScreenshot();
	void drawSlider(int val, int min, int max, Surface &icon, Surface &bg);

	void setInputSpeed();

	void writeConfig();
	void writeSkinConfig();
	void writeTmp(int selelem = -1, const string &selectordir = "");

	void addLink();
	void editLink();
	void deleteLink();
	void addSection();
	void renameSection();
	void deleteSection();

	void setBackground(Surface *_bg, const string &_wallpaper);

	int drawButton(Button *btn, int x = 5, int y = -8);
	int drawButton(Surface *s, const string &btn, const string &text = "", int x = 5, int y = -8);
	int drawButtonRight(Surface *s, const string &btn, const string &text = "", int x = 5, int y = -8);
	void drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect);

	/*!
	Reads the current battery state and returns a number representing it's level of charge
	@return A number representing battery charge. 0 means fully discharged. 5 means fully charged. 6 represents a gp2x using AC power.
	*/
	virtual uint16_t getBatteryLevel() { return 6; };
	virtual void setScaleMode(unsigned int mode) { };
	virtual void setTVOut(unsigned int _TVOut) { };
	virtual void setCPU(uint32_t mhz) {};
	virtual void ledOn() { };
	virtual void ledOff() { };
	virtual int setVolume(int val, bool popup = false);
	virtual int setBacklight(int val, bool popup = false);

};

#endif
