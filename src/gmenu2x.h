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

#include "surfacecollection.h"
#include "iconbutton.h"
#include "translator.h"
#include "FastDelegate.h"
#include "utilities.h"
#include "touchscreen.h"
#include "inputmanager.h"
#include "surface.h"
#include "fonthelper.h"

#include <iostream>
#include <string>
#include <vector>
#include <tr1/unordered_map>

const int MAX_VOLUME_SCALE_FACTOR = 200;
// Default values - going to add settings adjustment, saving, loading and such
const int VOLUME_SCALER_MUTE = 0;
const int VOLUME_SCALER_PHONES = 65;
const int VOLUME_SCALER_NORMAL = 100;
const int VOLUME_MODE_MUTE = 0;
const int VOLUME_MODE_PHONES = 1;
const int VOLUME_MODE_NORMAL = 2;
const int BATTERY_READS = 10;

// const int LOOP_DELAY = 50000;

#if defined(TARGET_GP2X)
	#define DEFAULT_CPU_CLK 200
#else
	#define DEFAULT_CPU_CLK 528
#endif

extern const char *CARD_ROOT;
extern const int CARD_ROOT_LEN;

// Note: Keep this in sync with colorNames!
enum color {
	COLOR_TOP_BAR_BG,
	COLOR_LIST_BG,
	COLOR_BOTTOM_BAR_BG,
	COLOR_SELECTION_BG,
	COLOR_MESSAGE_BOX_BG,
	COLOR_MESSAGE_BOX_BORDER,
	COLOR_MESSAGE_BOX_SELECTION,
	COLOR_FONT,
	COLOR_FONT_OUTLINE,
	COLOR_FONT_ALT,
	COLOR_FONT_ALT_OUTLINE,

	NUM_COLORS,
};

using std::string;
using std::vector;
using fastdelegate::FastDelegate0;

typedef FastDelegate0<> MenuAction;
typedef unordered_map<string, string, hash<string> > ConfStrHash;
typedef unordered_map<string, int, hash<string> > ConfIntHash;
// typedef unordered_map<string, RGBAColor, hash<string> > ConfColorHash;

typedef struct {
	unsigned short batt;
	unsigned short remocon;
} MMSP2ADC;

struct MenuOption {
	string text;
	MenuAction action;
};

char *ms2hms(unsigned long t, bool mm, bool ss);

class Menu;

class GMenu2X {
private:
	int getBacklight();

	void setSuspend(bool suspend);
	bool suspendActive = false;

	string path; //!< Contains the working directory of GMenu2X
	/*!
	Retrieves the free disk space on the sd
	@return String containing a human readable representation of the free disk space
	*/
	string getDiskFree(const char *path);
	// unsigned short cpuX; //!< Offset for displaying cpu clock information
	unsigned short volumeX; //!< Offset for displaying volume level
	// unsigned short manualX; //!< Offset for displaying the manual indicator in the taskbar


	int batteryHandle;
	void browsePath(const string &path, vector<string>* directories, vector<string>* files);
	/*!
	Starts the scanning of the nand and sd filesystems, searching for gpe and gpu files and creating the links in 2 dedicated sections.
	*/
	void linkScanner();
	/*!
	Performs the actual scan in the given path and populates the files vector with the results. The creation of the links is not performed here.
	@see scanner
	*/
	void scanPath(string path, vector<string> *files);

	/*!
	Displays a selector and launches the specified executable file
	*/
	void explorer();

	string lastSelectorDir;
	int lastSelectorElement;
	void readConfig();

	void readTmp();
	// void writeCommonIni();

	void initFont();
	void initMenu();

	IconButton *btnContextMenu;

	unsigned int memdev;
#ifdef TARGET_RS97
	volatile unsigned long *memregs;
#else
	volatile unsigned short *memregs;
#endif

#ifdef TARGET_GP2X
	string ip, defaultgw;
	
	bool inet, //!< Represents the configuration of the basic network services. @see readCommonIni @see usbnet @see samba @see web
		usbnet,
		samba,
		web;
	volatile unsigned short *MEM_REG;
	int cx25874; //tv-out


	void gp2x_tvout_on(bool pal);
	void gp2x_tvout_off();
	void readCommonIni();
	void initServices();
#endif
	void gp2x_deinit();
	void gp2x_init();
	// void toggleTvOut();
	void setTvOut();

public:
	GMenu2X();
	~GMenu2X();
	void quit();

	/*
	 * Variables needed for elements disposition
	 */
	uint resX, resY, halfX, halfY;
	// uint bottomBarIconY, bottomBarTextY
	uint linkColumns, linkRows;
	SDL_Rect listRect, linksRect, sectionBarRect;
	/*!
	Retrieves the parent directory of GMenu2X.
	This functions is used to initialize the "path" variable.
	@see path
	@return String containing the parent directory
	*/
	const string &getExePath();

	InputManager input;
	Touchscreen ts;

	unsigned long tickSuspend; //, tickPowerOff;

	//Configuration hashes
	ConfStrHash confStr, skinConfStr;
	ConfIntHash confInt, skinConfInt;
	// ConfColorHash skinConfColor;

	RGBAColor skinConfColors[NUM_COLORS];

	//Configuration settings
	bool useSelectionPng;
	void setSkin(const string &skin, bool setWallpaper = true, bool clearSC = true);
	//firmware type and version
	string fwType, fwVersion;
	//gp2x type
	bool f200;
	int volumeMode;

	SurfaceCollection sc;
	Translator tr;
	Surface *s, *bg;
	FontHelper *font, *titlefont; //, *bottombarfont;

	//Status functions
	void main();
	void options();
	void restartDialog();
	void poweroffDialog();

	/*!
	Reads the current battery state and returns a number representing it's level of charge
	@return A number representing battery charge. 0 means fully discharged. 5 means fully charged. 6 represents a gp2x using AC power.
	*/
	unsigned short getBatteryLevel();
	long getBatteryStatus();

	// void toggleSpeaker();
	// void reboot();
	void skinMenu();
	uint onChangeSkin();

	bool inputCommonActions();
	bool powerManager(bool &inputAction);

#if defined(TARGET_GP2X)
	void writeConfigOpen2x();
	void readConfigOpen2x();
	void settingsOpen2x();
	// Open2x settings ---------------------------------------------------------
	bool o2x_usb_net_on_boot, o2x_ftp_on_boot, o2x_telnet_on_boot, o2x_gp2xjoy_on_boot, o2x_usb_host_on_boot, o2x_usb_hid_on_boot, o2x_usb_storage_on_boot;
	string o2x_usb_net_ip;
	int savedVolumeMode;		//	just use the const int scale values at top of source

	//  Volume scaling values to store from config files
	int volumeScalerPhones;
	int volumeScalerNormal;
	//--------------------------------------------------------------------------
	void activateSdUsb();
	void activateNandUsb();
	void activateRootUsb();
	void applyRamTimings();
	void applyDefaultTimings();
	void setGamma(int gamma);
	void setVolumeScaler(int scaler);
	int getVolumeScaler();
#elif defined(TARGET_RS97)
	void umountSd();
	void formatSd();
#endif

	void about();
	void viewLog();
	void batteryLogger();
	void contextMenu();
	void changeWallpaper();
	bool saveScreenshot();

	void setClock(unsigned mhz);

	void setVolume(int vol);
	int getVolume();

	void setInputSpeed();
	int setBacklight(int val, bool popup = false);

	void writeConfig();
	void writeSkinConfig();
	void writeTmp(int selelem=-1, const string &selectordir="");

	void ledOn();
	void ledOff();

	void addLink();
	void editLink();
	void deleteLink();
	void addSection();
	void renameSection();
	void deleteSection();

	void initBG(const string &imagePath="");
	int drawButton(Button *btn, int x=5, int y=-10);
	int drawButton(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
	int drawButtonRight(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
	void drawScrollBar(uint pagesize, uint totalsize, uint pagepos, SDL_Rect scrollRect);

	// void drawSectionBar(Surface *s=NULL);
	// void drawTopBar(Surface *s = NULL, const string &title = NULL, const string &description = NULL, const string &icon = NULL);
	// void drawBottomBar(Surface *s=NULL);
	// void redrawBottomBar();

	Menu* menu;
};

#endif
