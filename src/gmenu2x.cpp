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
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <sys/statvfs.h>
#include <errno.h>

// for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// for soundcard
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include <sys/mman.h>

#include <ctime>
#include <sys/time.h>   /* for settimeofday() */

#include "linkapp.h"
#include "menu.h"
#include "fonthelper.h"
#include "surface.h"
#include "browsedialog.h"
#include "powermanager.h"
#include "gmenu2x.h"
#include "filelister.h"
#include "iconbutton.h"
#include "messagebox.h"
#include "inputdialog.h"
#include "settingsdialog.h"
#include "wallpaperdialog.h"
#include "textdialog.h"
#include "terminaldialog.h"
#include "menusettingint.h"
#include "menusettingbool.h"
#include "menusettingrgba.h"
#include "menusettingstring.h"
#include "menusettingmultistring.h"
#include "menusettingfile.h"
#include "menusettingimage.h"
#include "menusettingdir.h"
#include "imageviewerdialog.h"
#include "menusettingdatetime.h"
#include "debug.h"

#define sync() sync(); system("sync &");

enum vol_mode_t {
	VOLUME_MODE_MUTE, VOLUME_MODE_PHONES, VOLUME_MODE_NORMAL
};
uint16_t mmcPrev = MMC_REMOVE, mmcStatus;
uint16_t udcPrev = UDC_REMOVE, udcStatus;
uint16_t tvOutPrev = TV_REMOVE, tvOutStatus;
uint16_t volumeModePrev, volumeMode;
uint16_t batteryIcon = 3;

#if defined(TARGET_RETROGAME)
	#include "hw/retrogame.h"
#elif defined(TARGET_BITTBOY)
	#include "hw/bittboy.h"
#elif defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	#include "hw/gp2x.h"
#else //if defined(TARGET_PC)
	#include "hw/pc.h"
#endif

const char *CARD_ROOT = getenv("HOME"); //Note: Add a trailing /!
const int CARD_ROOT_LEN = 1;
int FB_SCREENPITCH = 1;
string fwType = "";

static GMenu2X *app;

using std::ifstream;
using std::ofstream;
using std::stringstream;
using namespace fastdelegate;

// Note: Keep this in sync with the enum!
static const char *colorNames[NUM_COLORS] = {
	"topBarBg",
	"listBg",
	"bottomBarBg",
	"selectionBg",
	"messageBoxBg",
	"messageBoxBorder",
	"messageBoxSelection",
	"font",
	"fontOutline",
	"fontAlt",
	"fontAltOutline"
};

static enum color stringToColor(const string &name) {
	for (uint32_t i = 0; i < NUM_COLORS; i++) {
		if (strcmp(colorNames[i], name.c_str()) == 0) {
			return (enum color)i;
		}
	}
	return (enum color)-1;
}

static const char *colorToString(enum color c) {
	return colorNames[c];
}

char *ms2hms(uint32_t t, bool mm = true, bool ss = true) {
	static char buf[10];

	t = t / 1000;
	int s = (t % 60);
	int m = (t % 3600) / 60;
	int h = (t % 86400) / 3600;
	// int d = (t % (86400 * 30)) / 86400;

	if (!ss) sprintf(buf, "%02d:%02d", h, m);
	else if (!mm) sprintf(buf, "%02d", h);
	else sprintf(buf, "%02d:%02d:%02d", h, m, s);
	return buf;
};

static void quit_all(int err) {
	delete app;
	exit(err);
}

GMenu2X::~GMenu2X() {
	confStr["datetime"] = getDateTime();

	writeConfig();

	sc.clear();
	s->free();

	quit();
}

void GMenu2X::quit() {
	s->flip(); s->flip(); s->flip(); // flush buffers

	delete menu;
	delete s;
	delete font;
	delete titlefont;

	fflush(NULL);
	SDL_Quit();
	hwDeinit();
}

int main(int /*argc*/, char * /*argv*/[]) {
	INFO("GMenu2X starting: If you read this message in the logs, check http://mtorromeo.github.com/gmenu2x/troubleshooting.html for a solution");

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);

	int fd = open("/dev/tty0", O_RDONLY);
	if (fd > 0) {
		ioctl(fd, VT_UNLOCKSWITCH, 1);
		ioctl(fd, KDSETMODE, KD_TEXT);
		ioctl(fd, KDSKBMODE, K_XLATE);
	}
	close(fd);

	usleep(1000);

	system("if [ -d sections/systems ]; then mkdir -p sections/emulators.systems; cp -r sections/systems/* sections/emulators.systems/; rm -rf sections/systems; fi");
#if !defined(TARGET_PC)
	system("mount -o remount,rw,async,noatime,iocharset=utf8 /home/retrofw");
#endif

	app = new GMenuNX();
	DEBUG("Starting GMenuNX");
	app->main();

	return 0;
}

GMenu2X *GMenu2X::instance = NULL;
void GMenu2X::main() {
	instance = this;
	// load config data
	hwInit();

	path = "";
	getExePath();

	setenv("SDL_FBCON_DONT_CLEAR", "1", 0);

	//Screen
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0 ) {
		ERROR("Could not initialize SDL: %s", SDL_GetError());
		quit();
		return;
	}

	input.init(path + "input.conf");
	setInputSpeed();

	readConfig();
	setDateTime();

	s = new Surface();

	SDL_ShowCursor(0);
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	//I'm forced to use SW surfaces since with HW there are issuse with changing the clock frequency
	SDL_Surface *dbl = SDL_SetVideoMode(resX, resY, 16, SDL_SWSURFACE);
	s->enableVirtualDoubleBuffer(dbl);
#else
	s->ScreenSurface = SDL_SetVideoMode(resX, resY * FB_SCREENPITCH, 16, SDL_HWSURFACE
	#ifdef SDL_TRIPLEBUF
		| SDL_TRIPLEBUF
	#endif
	);
	s->raw = SDL_CreateRGBSurface(SDL_SWSURFACE, resX, resY, 16, 0, 0, 0, 0);
#endif

	bg = new Surface(s);

	setSkin(confStr["skin"], false, true);
	powerManager = new PowerManager(this, confInt["backlightTimeout"], confInt["powerTimeout"]);

	MessageBox mb(this,tr["Loading"]);
	mb.setAutoHide(1);
	mb.exec();

	initMenu();

	currBackdrop = confStr["wallpaper"];
	setBacklight(confInt["backlight"]);

	tvOutStatus = getTVOutStatus();
	mmcStatus = getMMCStatus();
	udcStatus = getUDCStatus();
	volumeModePrev = volumeMode = getVolumeMode(confInt["globalVolume"]);
	
	readTmp();
	setCPU(confInt["cpuMenu"]);

	SDL_TimerID hwCheckTimer = SDL_AddTimer(1000, hwCheck, NULL);

	section_changed = SDL_GetTicks();
	SDL_TimerID sectionChangedTimer = SDL_AddTimer(2000, input.wakeUp, (void*)false);

	// recover last session
	if (lastSelectorElement >- 1 && menu->selLinkApp() != NULL && (!menu->selLinkApp()->getSelectorDir().empty() || !lastSelectorDir.empty())) {
		if (confInt["skinBackdrops"] & BD_DIALOG)
			setBackground(bg, currBackdrop);
		else
			setBackground(bg, confStr["wallpaper"]);
		menu->selLinkApp()->selector(lastSelectorElement, lastSelectorDir);
	}

	bool quit = false;
	int i = 0, x = 0, y = 0, ix = 0, iy = 0, sx = 0, sy = 0;
	const int iconPadding = 4;

	int8_t brightnessIcon = 5;
	Surface *iconBrightness[6] = {
		sc.skinRes("imgs/brightness/0.png"),
		sc.skinRes("imgs/brightness/1.png"),
		sc.skinRes("imgs/brightness/2.png"),
		sc.skinRes("imgs/brightness/3.png"),
		sc.skinRes("imgs/brightness/4.png"),
		sc.skinRes("imgs/brightness.png"),
	};

	batteryIcon = getBatteryLevel();

	Surface *iconBattery[7] = {
		sc.skinRes("imgs/battery/0.png"),
		sc.skinRes("imgs/battery/1.png"),
		sc.skinRes("imgs/battery/2.png"),
		sc.skinRes("imgs/battery/3.png"),
		sc.skinRes("imgs/battery/4.png"),
		sc.skinRes("imgs/battery/5.png"),
		sc.skinRes("imgs/battery/ac.png"),
	};

	Surface *iconVolume[3] = {
		sc.skinRes("imgs/mute.png"),
		sc.skinRes("imgs/phones.png"),
		sc.skinRes("imgs/volume.png"),
	};

	Surface *iconSD = sc.skinRes("imgs/sd.png"),
			*iconManual = sc.skinRes("imgs/manual.png"),
			*iconCPU = sc.skinRes("imgs/cpu.png"),
			*iconMenu = sc.skinRes("imgs/menu.png"),
			*iconL = sc.skinRes("imgs/section-l.png"),
			*iconR = sc.skinRes("imgs/section-r.png"),
			*iconBGoff = sc.skinRes("imgs/iconbg_off.png"),
			*iconBGon = sc.skinRes("imgs/iconbg_on.png")
	;

	while (!quit) {
		// // //Background
		currBackdrop = confStr["wallpaper"];
		if (menu->selLink() != NULL && menu->selLinkApp() != NULL && !menu->selLinkApp()->getBackdropPath().empty()) {
			currBackdrop = menu->selLinkApp()->getBackdropPath();
		}

		if (confInt["skinBackdrops"] & BD_MENU)
			setBackground(s, currBackdrop);
		else
			setBackground(s, confStr["wallpaper"]);

		// SECTIONS
		if (skinConfInt["sectionBar"]) {
			s->box(sectionBarRect, skinConfColors[COLOR_TOP_BAR_BG]);

			x = sectionBarRect.x; y = sectionBarRect.y; ix = 0;
			sx = (menu->selSectionIndex() - menu->firstDispSection()) * skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				ix = (resX - skinConfInt["sectionBarSize"] * min(menu->sectionNumItems(), menu->getSections().size())) / 2;
				sx += ix; sy = y;
				s->box(sx, sy, skinConfInt["sectionBarSize"], skinConfInt["sectionBarSize"], skinConfColors[COLOR_SELECTION_BG]);
			}
			
			for (i = menu->firstDispSection(); i < menu->getSections().size() && i < menu->firstDispSection() + menu->sectionNumItems(); i++) {
				if (skinConfInt["sectionBar"] == SB_LEFT || skinConfInt["sectionBar"] == SB_RIGHT) {
					y = (i - menu->firstDispSection()) * skinConfInt["sectionBarSize"];
				} else {
					x = (i - menu->firstDispSection()) * skinConfInt["sectionBarSize"] + ix;
				}

				if (skinConfInt["sectionBar"] != SB_CLASSIC && menu->selSectionIndex() == (int)i) {
					sx = x; sy = y;
					s->box(sx, sy, skinConfInt["sectionBarSize"], skinConfInt["sectionBarSize"], skinConfColors[COLOR_SELECTION_BG]);
				}

				sc[menu->getSectionIcon(i)]->blit(s, {x, y, skinConfInt["sectionBarSize"], skinConfInt["sectionBarSize"]}, HAlignCenter | VAlignMiddle);
			}

			if (skinConfInt["sectionLabel"] && SDL_GetTicks() - section_changed < 1400) {
				s->write(font, menu->selSectionName(), sx + skinConfInt["sectionBarSize"] / 2 , sy + skinConfInt["sectionBarSize"], HAlignCenter | VAlignBottom);
			} else if (sectionChangedTimer != NULL) {
				SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;
			}

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				iconL->blit(s, 0, 0, HAlignLeft | VAlignTop);
				iconR->blit(s, resX, 0, HAlignRight | VAlignTop);
			}
		}

		// LINKS
		s->setClipRect(linksRect);
		s->box(linksRect, skinConfColors[COLOR_LIST_BG]);

		i = menu->firstDispRow() * linkCols;

		if (linkCols == 1) {
			// LIST
			ix = linksRect.x;
			for (y = 0; y < linkRows && i < menu->sectionLinks()->size(); y++, i++) {
				iy = linksRect.y + y * linkHeight;

				if (i == (uint32_t)menu->selLinkIndex())
					s->box(ix, iy, linksRect.w, linkHeight, skinConfColors[COLOR_SELECTION_BG]);

				sc[menu->sectionLinks()->at(i)->getIconPath()]->blit(s, {ix, iy, 36, linkHeight}, HAlignCenter | VAlignMiddle);
				s->write(titlefont, tr.translate(menu->sectionLinks()->at(i)->getTitle()), ix + linkSpacing + 36, iy + titlefont->getHeight()/2, VAlignMiddle);
				s->write(font, tr.translate(menu->sectionLinks()->at(i)->getDescription()), ix + linkSpacing + 36, iy + linkHeight - linkSpacing/2, VAlignBottom);
			}
		} else {
			for (y = 0; y < linkRows; y++) {
				for (x = 0; x < linkCols && i < menu->sectionLinks()->size(); x++, i++) {
					ix = linksRect.x + x * linkWidth  + (x + 1) * linkSpacing;
					iy = linksRect.y + y * linkHeight + (y + 1) * linkSpacing;

					Surface *icon = sc[menu->sectionLinks()->at(i)->getIconPath()];

					// s->setClipRect({ix, iy, linkWidth, linkHeight});
					if (i == (uint32_t)menu->selLinkIndex()) {
						if (iconBGon != NULL) iconBGon->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle);
						else s->box(ix + (linkWidth - icon->width()) / 2 - 4, iy + (linkHeight - icon->height()) / 2 - 4, icon->width() + 8, icon->height() + 8, skinConfColors[COLOR_SELECTION_BG]);
						// s->box(ix, iy, linkWidth, linkHeight, skinConfColors[COLOR_SELECTION_BG]);
					} else if (iconBGoff != NULL) iconBGoff->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle);

					icon->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle);
					// s->clearClipRect();

					if (skinConfInt["linkLabel"]) s->write(font, tr.translate(menu->sectionLinks()->at(i)->getTitle()), ix + 2 + linkWidth/2, iy + (linkHeight + icon->height())/2 - 6 , HAlignCenter);
				}
			}
		}
		s->clearClipRect();

		drawScrollBar(linkRows, menu->sectionLinks()->size()/linkCols + ((menu->sectionLinks()->size()%linkCols==0) ? 0 : 1), menu->firstDispRow(), linksRect);

		// TRAY DEBUG
		// s->box(sectionBarRect.x + sectionBarRect.w - 38 + 0 * 20, sectionBarRect.y + sectionBarRect.h - 18,16,16, strtorgba("ffff00ff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 38 + 1 * 20, sectionBarRect.y + sectionBarRect.h - 18,16,16, strtorgba("00ff00ff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38,16,16, strtorgba("0000ffff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 18, sectionBarRect.y + sectionBarRect.h - 38,16,16, strtorgba("ff00ffff"));
	// drawBottomBar(this->bg);
		if (skinConfInt["sectionBar"]) {
			int iconTrayShift = 0;

			brightnessIcon = confInt["backlight"] / 20;
			if (brightnessIcon > 4 || iconBrightness[brightnessIcon] == NULL) brightnessIcon = 5;

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				const int iconWidth = 16, pctWidth = font->getTextWidth("100");
				char buf[32]; int x = 0;

				s->box(bottomBarRect, skinConfColors[COLOR_BOTTOM_BAR_BG]);

				// Volume indicator
				// TODO: use drawButton(s, iconVolume[volumeMode], confInt["globalVolume"], x);
				{ stringstream ss; ss << confInt["globalVolume"] /*<< "%"*/; ss.get(&buf[0], sizeof(buf)); }
				x = iconPadding; //1 * (iconWidth + 2 * iconPadding) + iconPadding + 1 * pctWidth;
				iconVolume[volumeMode]->blit(s, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
				x += iconWidth + iconPadding;
				s->write(font, buf, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);

				// Brightness indicator
				{ stringstream ss; ss << confInt["backlight"] /*<< "%"*/; ss.get(&buf[0], sizeof(buf)); }
				x += iconPadding + pctWidth;
				iconBrightness[brightnessIcon]->blit(s, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
				x += iconWidth + iconPadding;
				s->write(font, buf, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);

				// // Menu indicator
				// iconMenu->blit(s, iconPadding, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
				// sc.skinRes("imgs/debug.png")->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);

				iconTrayShift = 0;

				// Battery indicator
				iconBattery[batteryIcon]->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
				iconTrayShift++;

				if (iconInet != NULL) {
					// Network indicator
					iconInet->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
					iconTrayShift++;
				}

				// SD Card indicator
				if (mmcStatus == MMC_INSERT) {
					iconSD->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
					iconTrayShift++;
				}

				if (menu->selLink() != NULL) {
					if (menu->selLinkApp() != NULL) {
						if (!menu->selLinkApp()->getManualPath().empty()) {
							// Manual indicator
							iconManual->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
						}

						// // CPU indicator
						{ stringstream ss; ss << menu->selLinkApp()->getCPU() << "MHz"; ss.get(&buf[0], sizeof(buf)); }
						x += iconPadding + pctWidth;
						iconCPU->blit(s, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
						x += iconWidth + iconPadding;
						s->write(font, buf, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
					}
				}
		
			} else {
				// TRAY 0,0
				iconVolume[volumeMode]->blit(s, sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38);

				// TRAY 1,0
				iconBattery[batteryIcon]->blit(s, sectionBarRect.x + sectionBarRect.w - 18, sectionBarRect.y + sectionBarRect.h - 38);

				// TRAY iconTrayShift,1
				if (mmcStatus == MMC_INSERT) {
					iconSD->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
					iconTrayShift++;
				}

				if (menu->selLink() != NULL) {
					if (menu->selLinkApp() != NULL) {
						if (!menu->selLinkApp()->getManualPath().empty() && iconTrayShift < 2) {
							// Manual indicator
							iconManual->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
							iconTrayShift++;
						}

						if (menu->selLinkApp()->getCPU() != confInt["cpuMenu"] && iconTrayShift < 2) {
							// CPU indicator
							iconCPU->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
							iconTrayShift++;
						}
					}
				}

				if (iconTrayShift < 2) {
					brightnessIcon = confInt["backlight"]/20;
					if (brightnessIcon > 4 || iconBrightness[brightnessIcon] == NULL) brightnessIcon = 5;
					iconBrightness[brightnessIcon]->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
					iconTrayShift++;
				}
				if (iconTrayShift < 2) {
					// Menu indicator
					iconMenu->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
					iconTrayShift++;
				}
			}
		}

		s->flip();

		bool inputAction = input.update();
		if (input.combo()) {
			skinConfInt["sectionBar"] = ((skinConfInt["sectionBar"] + 1) % 5);
			if (!skinConfInt["sectionBar"]) skinConfInt["sectionBar"]++;
			initMenu();
			MessageBox mb(this,tr["CHEATER! ;)"]);
			mb.setBgAlpha(0);
			mb.setAutoHide(200);
			mb.exec();
			continue;
		}

		if (inputCommonActions(inputAction)) continue;

		if (skinConfInt["sectionLabel"] && (input[SECTION_PREV] || input[SECTION_NEXT]) ) {
			section_changed = SDL_GetTicks();
			SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;
			sectionChangedTimer = SDL_AddTimer(2000, input.wakeUp, (void*)false);
		}

		if ( input[CONFIRM] && menu->selLink() != NULL ) {
			SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;

			if (confInt["skinBackdrops"] & BD_DIALOG)
				setBackground(bg, currBackdrop);
			else
				setBackground(bg, confStr["wallpaper"]);

			menu->selLink()->run();
		}
		else if ( input[SETTINGS] ) settings();
		else if ( input[MENU]     ) contextMenu();
		// LINK NAVIGATION
		else if ( input[LEFT ] && linkCols == 1) menu->pageUp();
		else if ( input[RIGHT] && linkCols == 1) menu->pageDown();
		else if ( input[LEFT ] ) menu->linkLeft();
		else if ( input[RIGHT] ) menu->linkRight();
		else if ( input[UP   ] ) menu->linkUp();
		else if ( input[DOWN ] ) menu->linkDown();
		// SECTION
		else if ( input[SECTION_PREV] ) menu->decSectionIndex();
		else if ( input[SECTION_NEXT] ) menu->incSectionIndex();

		// SELLINKAPP SELECTED
		else if (input[MANUAL] && menu->selLinkApp() != NULL) showManual(); // menu->selLinkApp()->showManual();

		// On Screen Help
		// else if (input[MODIFIER]) {
		// 	s->box(10,50,300,162, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		// 	s->rectangle( 12,52,296,158, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );
		// 	int line = 60; s->write( font, tr["CONTROLS"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["A: Confirm action"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["B: Cancel action"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["X: Show manual"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["L, R: Change section"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["Select: Modifier"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["Start: Contextual menu"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["Select+Start: Options menu"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["Backlight: Adjust backlight level"], 20, line);
		// 	line += font->getHeight() + 5; s->write( font, tr["Power: Toggle speaker on/off"], 20, line);
		// 	s->flip();
		// 	bool close = false;
		// 	while (!close) {
		// 		input.update();
		// 		if (input[MODIFIER] || input[CONFIRM] || input[CANCEL]) close = true;
		// 	}
		// }
	}
}

bool GMenu2X::inputCommonActions(bool &inputAction) {
	if (powerManager->suspendActive) {
		// SUSPEND ACTIVE
		while (!(input[POWER] || input[SETTINGS] || input[UDC_CONNECT])) {
			input.update();
		}
		powerManager->doSuspend(0);
		input[DO_NOTHING] = true;
		return true;
	}

	if (inputAction) powerManager->resetSuspendTimer();

	int wasActive = 0;
	while (input[POWER] || input[SETTINGS]) {
		if (input[POWER]) wasActive = POWER;
		else if (input[SETTINGS]) wasActive = SETTINGS;

		input.update();
		if (input[POWER] || input[SETTINGS]) {
			// HOLD POWER BUTTON
			poweroffDialog();
			return true;
		} else if (wasActive == POWER) {
			powerManager->doSuspend(1);
			return true;
		}
	}

	while (input[MENU]) {
		wasActive = MENU;
		input.update();
		if (input[SETTINGS]) {
			// SCREENSHOT
			if (!saveScreenshot()) { ERROR("Can't save screenshot"); return true; }
			MessageBox mb(this, tr["Screenshot saved"]);
			mb.setAutoHide(1000);
			mb.exec();
			return true;
		} else if (input[SECTION_NEXT]) {
			setBacklight(confInt["backlight"], true);
			return true;
		} else if (input[SECTION_PREV]) {
			// VOLUME / MUTE
			setVolume(confInt["globalVolume"], true);
			return true;
		} else if (input[POWER]) {
			udcDialog(UDC_CONNECT);
			return true;
		}
	}

	if ( input[BACKLIGHT] ) {
		setBacklight(confInt["backlight"], true);
		return true;
	} else if ( input[UDC_CONNECT] ) {
		powerManager->setPowerTimeout(0);
		batteryIcon = 6;
		udcDialog(UDC_CONNECT);
		return true;
	} else if ( input[UDC_REMOVE] ) {
		udcDialog(UDC_REMOVE);
		iconInet = NULL;
		batteryIcon = getBatteryLevel();
		powerManager->setPowerTimeout(confInt["powerTimeout"]);
		return true;
	} else if ( input[TV_CONNECT] ) {
		tvOutDialog();
		return true;
	} else if ( input[TV_REMOVE] ) {
		tvOutDialog(TV_OFF);
		return true;
	// } else if ( input[PHONES_CONNECT] ) {
	// 	// tvOutDialog(TV_OFF);
	// 	WARNING("volume mode changed");
	// 	return true;
	}

	input[wasActive] = true;
	return false;
}

void GMenu2X::setBackground(Surface *_bg, const string &_wallpaper) {
	string wallpaper = _wallpaper;

	if (sc[wallpaper] == NULL) { // search and scale background
		if (wallpaper.empty() || sc[wallpaper] == NULL) {
			DEBUG("Searching wallpaper");
			FileLister fl("skins/Default/wallpapers", false, true);
			fl.setFilter(".png,.jpg,.jpeg,.bmp");
			fl.browse();
			wallpaper = "skins/Default/wallpapers/" + fl.getFiles()[0];
		}
		if (sc[wallpaper] == NULL) return;
		if (confStr["bgscale"] == "Stretch") sc[wallpaper]->softStretch(resX, resY, false, true);
		else if (confStr["bgscale"] == "Aspect") sc[wallpaper]->softStretch(resX, resY, true, true);
		else if (confStr["bgscale"] == "Crop") sc[wallpaper]->softStretch(resX, resY, true, false);
	}

	_bg->box((SDL_Rect){0, 0, resX, resY}, (RGBAColor){0, 0, 0, 255});
	sc[wallpaper]->blit(_bg,0,0);
}

void GMenu2X::initLayout() {
	// LINKS rect
	linksRect = (SDL_Rect){0, 0, resX, resY};
	sectionBarRect = (SDL_Rect){0, 0, resX, resY};

	if (skinConfInt["sectionBar"]) {
		// x = 0; y = 0;
		if (skinConfInt["sectionBar"] == SB_LEFT || skinConfInt["sectionBar"] == SB_RIGHT) {
			sectionBarRect.x = (skinConfInt["sectionBar"] == SB_RIGHT)*(resX - skinConfInt["sectionBarSize"]);
			sectionBarRect.w = skinConfInt["sectionBarSize"];
			linksRect.w = resX - skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_LEFT) {
				linksRect.x = skinConfInt["sectionBarSize"];
			}
		} else {
			sectionBarRect.y = (skinConfInt["sectionBar"] == SB_BOTTOM)*(resY - skinConfInt["sectionBarSize"]);
			sectionBarRect.h = skinConfInt["sectionBarSize"];
			linksRect.h = resY - skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_TOP || skinConfInt["sectionBar"] == SB_CLASSIC) {
				linksRect.y = skinConfInt["sectionBarSize"];
			}
			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				linksRect.h -= skinConfInt["bottomBarHeight"];
			}
		}
	}

	listRect = (SDL_Rect){0, skinConfInt["sectionBarSize"], resX, resY - skinConfInt["bottomBarHeight"] - skinConfInt["sectionBarSize"]};
	bottomBarRect = (SDL_Rect){0, resY - skinConfInt["bottomBarHeight"], resX, skinConfInt["bottomBarHeight"]};

	// WIP
	linkCols = skinConfInt["linkCols"];
	linkRows = skinConfInt["linkRows"];

	linkWidth  = (linksRect.w - (linkCols + 1 ) * linkSpacing) / linkCols;
	linkHeight = (linksRect.h - (linkCols > 1) * (linkRows + 1 ) * linkSpacing) / linkRows;
}

void GMenu2X::initFont() {
	if (font != NULL) delete font;
	if (titlefont != NULL) delete titlefont;

	string skinFont = confStr["skinFont"] == "Default" ? "skins/Default/font.ttf" : sc.getSkinFilePath("font.ttf");
	font = new FontHelper(skinFont, skinConfInt["fontSize"], skinConfColors[COLOR_FONT], skinConfColors[COLOR_FONT_OUTLINE]);
	titlefont = new FontHelper(skinFont, skinConfInt["fontSizeTitle"], skinConfColors[COLOR_FONT], skinConfColors[COLOR_FONT_OUTLINE]);
}

void GMenu2X::initMenu() {
	if (menu != NULL) delete menu;
	initLayout();

	// Menu structure handler
	menu = new Menu(this);

	for (uint32_t i = 0; i < menu->getSections().size(); i++) {
		//Add virtual links in the applications section
		if (menu->getSections()[i] == "applications") {
			menu->addActionLink(i, tr["Explorer"], MakeDelegate(this, &GMenu2X::explorer), tr["Browse files and launch apps"], "skin:icons/explorer.png");
			menu->addActionLink(i, tr["Umount"], MakeDelegate(this, &GMenu2X::umountSdDialog), tr["Umount external media device"], "skin:icons/eject.png");

// #if !defined(TARGET_PC)
// 			if (getBatteryLevel() > 5) // show only if charging
// #endif
// 				menu->addActionLink(i, tr["Battery Logger"], MakeDelegate(this, &GMenu2X::batteryLogger), tr["Log battery power to battery.csv"], "skin:icons/ebook.png");
		}

		//Add virtual links in the setting section
		else if (menu->getSections()[i] == "settings") {
			menu->addActionLink(i, tr["Settings"], MakeDelegate(this, &GMenu2X::settings), tr["Configure system"], "skin:icons/configure.png");
			menu->addActionLink(i, tr["Skin"], MakeDelegate(this, &GMenu2X::skinMenu), tr["Appearance & skin settings"], "skin:icons/skin.png");
#if defined(TARGET_GP2X)
			if (fwType == "open2x")
				menu->addActionLink(i, "Open2x", MakeDelegate(this, &GMenu2X::settingsOpen2x), tr["Configure Open2x system settings"], "skin:icons/o2xconfigure.png");
			menu->addActionLink(i, "USB SD", MakeDelegate(this, &GMenu2X::activateSdUsb), tr["Activate USB on SD"], "skin:icons/usb.png");
			if (fwType == "gph" && !f200)
				menu->addActionLink(i, "USB Nand", MakeDelegate(this, &GMenu2X::activateNandUsb), tr["Activate USB on NAND"], "skin:icons/usb.png");
#endif
			if (fileExists(path + "log.txt"))
				menu->addActionLink(i, tr["Log Viewer"], MakeDelegate(this, &GMenu2X::viewLog), tr["Displays last launched program's output"], "skin:icons/ebook.png");

			menu->addActionLink(i, tr["About"], MakeDelegate(this, &GMenu2X::about), tr["Info about system"], "skin:icons/about.png");
			menu->addActionLink(i, tr["Power"], MakeDelegate(this, &GMenu2X::poweroffDialog), tr["Power menu"], "skin:icons/exit.png");
		}
	}
	menu->setSectionIndex(confInt["section"]);
	menu->setLinkIndex(confInt["link"]);
	// menu->loadIcons();
}

void GMenu2X::settings() {
//G
	// int prevgamma = confInt["gamma"];
	// bool showRootFolder = fileExists("/mnt/root");

	FileLister fl_tr("translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	string lang = tr.lang();
	if (lang == "") lang = "English";

	vector<string> opFactory;
	opFactory.push_back(">>");
	string tmp = ">>";


	SettingsDialog sd(this, ts, tr["Settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenu2X"], &lang, &fl_tr.getFiles()));

	string prevDateTime = confStr["datetime"] = getDateTime();
	sd.addSetting(new MenuSettingDateTime(this, tr["Date & Time"], tr["Set system's date & time"], &confStr["datetime"]));

	sd.addSetting(new MenuSettingBool(this, tr["Remember selection"], tr["Remember the last selected section, link and file"], &confInt["saveSelection"]));
	sd.addSetting(new MenuSettingBool(this, tr["Output logs"], tr["Logs the link's output to read with Log Viewer"], &confInt["outputLogs"]));
	sd.addSetting(new MenuSettingInt(this, tr["Screen timeout"], tr["Seconds to turn display off if inactive"], &confInt["backlightTimeout"], 30, 10, 300));
	sd.addSetting(new MenuSettingInt(this, tr["Power timeout"], tr["Minutes to poweroff system if inactive"], &confInt["powerTimeout"], 10, 1, 300));
	sd.addSetting(new MenuSettingInt(this, tr["Backlight"], tr["Set LCD backlight"], &confInt["backlight"], 70, 1, 100));
	sd.addSetting(new MenuSettingInt(this, tr["Audio volume"], tr["Set the default audio volume"], &confInt["globalVolume"], 60, 0, 100));
	sd.addSetting(new MenuSettingDir(this, tr["Default path"],	tr["Default directory to start the file browser"], &confStr["defaultDir"]));

	vector<string> usbMode;
	usbMode.push_back("Ask");
	usbMode.push_back("Storage");
	usbMode.push_back("Charger");
	sd.addSetting(new MenuSettingMultiString(this, tr["USB mode"], tr["Define default USB mode"], &confStr["usbMode"], &usbMode));

#if defined(TARGET_RETROGAME)
	if (fwType == "RETROGAME") {
		vector<string> batteryType;
		batteryType.push_back("BL-5B");
		batteryType.push_back("Linear");
		sd.addSetting(new MenuSettingMultiString(this, tr["Battery profile"], tr["Set the battery discharge profile"], &confStr["batteryType"], &batteryType));
	}
	sd.addSetting(new MenuSettingMultiString(this, tr["CPU settings"], tr["Define CPU and overclock settings"], &tmp, &opFactory, 0, MakeDelegate(this, &GMenu2X::cpuSettings)));
#endif
	sd.addSetting(new MenuSettingMultiString(this, tr["Reset settings"], tr["Choose settings to reset back to defaults"], &tmp, &opFactory, 0, MakeDelegate(this, &GMenu2X::resetSettings)));

	if (sd.exec() && sd.edited() && sd.save) {
		if (confStr["usbMode"] == "Ask") confStr["usbMode"] = "";
		if (lang == "English") lang = "";
		if (confStr["lang"] != lang) {
			confStr["lang"] = lang;
			tr.setLang(lang);
		}

		setBacklight(confInt["backlight"], false);
		writeConfig();

		powerManager->resetSuspendTimer();
		powerManager->setSuspendTimeout(confInt["backlightTimeout"]);
		powerManager->setPowerTimeout(confInt["powerTimeout"]);

#if defined(TARGET_GP2X)
		if (prevgamma != confInt["gamma"]) setGamma(confInt["gamma"]);
#endif

		if (prevDateTime != confStr["datetime"]) restartDialog();
	}
}

void GMenu2X::resetSettings() {
	bool	reset_gmenu = true,
			reset_skin = true,
			reset_icon = false,
			reset_manual = false,
			reset_parameter = false,
			reset_backdrop = false,
			reset_filter = false,
			reset_directory = false,
			reset_preview = false,
			reset_cpu = false;

	string tmppath = "";

	SettingsDialog sd(this, ts, tr["Reset settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingBool(this, tr["GMenuNX"], tr["Reset GMenuNX settings"], &reset_gmenu));
	sd.addSetting(new MenuSettingBool(this, tr["Default skin"], tr["Reset Default skin settings back to default"], &reset_skin));
	sd.addSetting(new MenuSettingBool(this, tr["Icons"], tr["Reset link's icon back to default"], &reset_icon));
	sd.addSetting(new MenuSettingBool(this, tr["Manuals"], tr["Unset link's manual"], &reset_manual));
	sd.addSetting(new MenuSettingBool(this, tr["Parameters"], tr["Unset link's additional parameters"], &reset_parameter));
	sd.addSetting(new MenuSettingBool(this, tr["Backdrops"], tr["Unset link's backdrops"], &reset_backdrop));
	sd.addSetting(new MenuSettingBool(this, tr["Filters"], tr["Unset link's selector file filters"], &reset_filter));
	sd.addSetting(new MenuSettingBool(this, tr["Directories"], tr["Unset link's selector directory"], &reset_directory));
	sd.addSetting(new MenuSettingBool(this, tr["Previews"], tr["Unset link's selector previews path"], &reset_preview));
	sd.addSetting(new MenuSettingBool(this, tr["CPU speed"], tr["Reset link's custom CPU speed back to default"], &reset_cpu));

	if (sd.exec() && sd.edited() && sd.save) {
		MessageBox mb(this, tr["Changes will be applied to ALL\napps and GMenuNX. Are you sure?"], "skin:icons/exit.png");
		mb.setButton(CONFIRM, tr["Cancel"]);
		mb.setButton(SECTION_NEXT,  tr["Confirm"]);
		if (mb.exec() != SECTION_NEXT) return;

		for (uint32_t s = 0; s < menu->getSections().size(); s++) {
			for (uint32_t l = 0; l < menu->sectionLinks(s)->size(); l++) {
				menu->setSectionIndex(s);
				menu->setLinkIndex(l);
				bool islink = menu->selLinkApp() != NULL;
				// WARNING("APP: %d %d %d %s", s, l, islink, menu->sectionLinks(s)->at(l)->getTitle().c_str());
				if (!islink) continue;
				if (reset_cpu)			menu->selLinkApp()->setCPU();
				if (reset_icon)			menu->selLinkApp()->setIcon("");
				if (reset_manual)		menu->selLinkApp()->setManual("");
				if (reset_parameter) 	menu->selLinkApp()->setParams("");
				if (reset_filter) 		menu->selLinkApp()->setSelectorFilter("");
				if (reset_directory) 	menu->selLinkApp()->setSelectorDir("");
				if (reset_preview) 		menu->selLinkApp()->setSelectorScreens("");
				if (reset_backdrop) 	menu->selLinkApp()->setBackdrop("");
				if (reset_icon || reset_manual || reset_parameter || reset_backdrop || reset_filter || reset_directory || reset_preview )
					menu->selLinkApp()->save();
			}
		}
		if (reset_skin) {
			tmppath = path + "skins/Default/skin.conf";
			unlink(tmppath.c_str());
		}
		if (reset_gmenu) {
			tmppath = path + "gmenu2x.conf";
			unlink(tmppath.c_str());
		}
		restartDialog();
	}
}

void GMenu2X::cpuSettings() {
	SettingsDialog sd(this, ts, tr["CPU settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingInt(this, tr["Default CPU clock"], tr["Set the default working CPU frequency"], &confInt["cpuMenu"], 528, 528, 600, 6));
	sd.addSetting(new MenuSettingInt(this, tr["Maximum CPU clock"], tr["Maximum overclock for launching links"], &confInt["cpuMax"], 624, 600, 1200, 6));
	sd.addSetting(new MenuSettingInt(this, tr["Minimum CPU clock"], tr["Minimum underclock used in Suspend mode"], &confInt["cpuMin"], 342, 200, 528, 6));

	if (sd.exec() && sd.edited() && sd.save) {
		setCPU(confInt["cpuMenu"]);
		writeConfig();
	}
}

void GMenu2X::readTmp() {
	lastSelectorElement = -1;
	if (!fileExists("/tmp/gmenu2x.tmp")) return;
	ifstream inf("/tmp/gmenu2x.tmp", ios_base::in);
	if (!inf.is_open()) return;
	string line, name, value;

	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		name = trim(line.substr(0,pos));
		value = trim(line.substr(pos+1,line.length()));
		if (name == "section") menu->setSectionIndex(atoi(value.c_str()));
		else if (name == "link") menu->setLinkIndex(atoi(value.c_str()));
		else if (name == "selectorelem") lastSelectorElement = atoi(value.c_str());
		else if (name == "selectordir") lastSelectorDir = value;
		else if (name == "TVOut") TVOut = atoi(value.c_str());
		else if (name == "tvOutPrev") tvOutPrev = atoi(value.c_str());
		else if (name == "udcPrev") udcPrev = atoi(value.c_str());
		else if (name == "currBackdrop") currBackdrop = value.c_str();
	}
	if (TVOut > 2) TVOut = 0;
	inf.close();
	unlink("/tmp/gmenu2x.tmp");
}

void GMenu2X::writeTmp(int selelem, const string &selectordir) {
	string conffile = "/tmp/gmenu2x.tmp";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "section=" << menu->selSectionIndex() << endl;
		inf << "link=" << menu->selLinkIndex() << endl;
		if (selelem >- 1) inf << "selectorelem=" << selelem << endl;
		if (selectordir != "") inf << "selectordir=" << selectordir << endl;
		inf << "udcPrev=" << udcPrev << endl;
		inf << "tvOutPrev=" << tvOutPrev << endl;
		inf << "TVOut=" << TVOut << endl;
		inf << "currBackdrop=" << currBackdrop << endl;
		inf.close();
	}
}

void GMenu2X::readConfig() {
	string conffile = path + "gmenu2x.conf";

	// Defaults *** Sync with default values in writeConfig
	confStr["datetime"] = __BUILDTIME__;
	if (fwType != "RETROARCADE") confStr["batteryType"] = "BL-5B";
	else confStr["batteryType"] = "Linear";
	confInt["saveSelection"] = 1;
	confInt["skinBackdrops"] = 0;
	confStr["defaultDir"] = CARD_ROOT;
	confInt["globalVolume"] = 60;

	input.update(false);

	if (!input[SETTINGS] && fileExists(conffile)) {
		ifstream inf(conffile.c_str(), ios_base::in);
		if (inf.is_open()) {
			string line;
			while (getline(inf, line, '\n')) {
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0, pos));
				string value = trim(line.substr(pos + 1, line.length()));

				if (value.length() > 1 && value.at(0) == '"' && value.at(value.length() - 1) == '"')
					confStr[name] = value.substr(1,value.length()-2);
				else
					confInt[name] = atoi(value.c_str());
			}
			inf.close();
		}
	}

	if (!confStr["lang"].empty()) tr.setLang(confStr["lang"]);
	if (!confStr["wallpaper"].empty() && !fileExists(confStr["wallpaper"])) confStr["wallpaper"] = "";
	if (confStr["skin"].empty() || !dirExists("skins/" + confStr["skin"])) confStr["skin"] = "Default";

	evalIntConf( &confInt["backlightTimeout"], 30, 10, 300);
	evalIntConf( &confInt["powerTimeout"], 10, 1, 300);
	evalIntConf( &confInt["outputLogs"], 0, 0, 1 );
	evalIntConf( &confInt["cpuMax"], 642, 200, 1200 );
	evalIntConf( &confInt["cpuMin"], 342, 200, 1200 );
	evalIntConf( &confInt["cpuMenu"], 528, 200, 1200 );
	evalIntConf( &confInt["globalVolume"], 60, 0, 100 );
	evalIntConf( &confInt["backlight"], 70, 1, 100);
	evalIntConf( &confInt["minBattery"], 3550, 1, 10000);
	evalIntConf( &confInt["maxBattery"], 3720, 1, 10000);

	if (!confInt["saveSelection"]) {
		confInt["section"] = 0;
		confInt["link"] = 0;
	}
}

void GMenu2X::writeConfig() {
	ledOn();
	if (confInt["saveSelection"] && menu != NULL) {
		confInt["section"] = menu->selSectionIndex();
		confInt["link"] = menu->selLinkIndex();
	}

	string conffile = path + "gmenu2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		for (ConfStrHash::iterator curr = confStr.begin(); curr != confStr.end(); curr++) {
			if (
				// deprecated
				curr->first == "sectionBarPosition" ||
				curr->first == "tvoutEncoding" ||

				// defaults
				(curr->first == "defaultDir" && curr->second == CARD_ROOT) ||
				(curr->first == "skin" && curr->second == "Default") ||
				(curr->first == "usbMode" && curr->second.empty()) ||
				(curr->first == "lang" && curr->second.empty()) ||
				(curr->first == "lang" && curr->second.empty()) ||
				(curr->first == "bgscale" && curr->second.empty()) ||
				(curr->first == "bgscale" && curr->second == "Original") ||

				curr->first.empty() || curr->second.empty()
			) continue;

			inf << curr->first << "=\"" << curr->second << "\"" << endl;
		}

		for (ConfIntHash::iterator curr = confInt.begin(); curr != confInt.end(); curr++) {
			if (
				// deprecated
				curr->first == "batteryLog" ||
				curr->first == "maxClock" ||
				curr->first == "minClock" ||
				curr->first == "menuClock" ||
				curr->first == "TVOut" ||

				// moved to skin conf
				curr->first == "linkCols" ||
				curr->first == "linkRows" ||
				curr->first == "sectionBar" ||
				curr->first == "sectionLabel" ||
				curr->first == "linkLabel" ||

				// defaults
				(curr->first == "skinBackdrops" && curr->second == 0) ||
				(curr->first == "backlightTimeout" && curr->second == 30) ||
				(curr->first == "powerTimeout" && curr->second == 10) ||
				(curr->first == "outputLogs" && curr->second == 0) ||
				(curr->first == "cpuMax" && curr->second == 642) ||
				(curr->first == "cpuMin" && curr->second == 342) ||
				(curr->first == "cpuMenu" && curr->second == 528) ||
				(curr->first == "globalVolume" && curr->second == 60) ||
				(curr->first == "backlight" && curr->second == 70) ||
				(curr->first == "minBattery" && curr->second == 3550) ||
				(curr->first == "maxBattery" && curr->second == 3720) ||
				(curr->first == "saveSelection" && curr->second == 1) ||
				(curr->first == "section" && curr->second == 0) ||
				(curr->first == "link" && curr->second == 0) ||

				curr->first.empty()
			) continue;

			inf << curr->first << "=" << curr->second << endl;
		}
		inf.close();
	}
	sync();

#if defined(TARGET_GP2X)
		if (fwType == "open2x" && savedVolumeMode != volumeMode)
			writeConfigOpen2x();
#endif
	ledOff();
}

void GMenu2X::writeSkinConfig() {
	ledOn();
	string conffile = path + "skins/" + confStr["skin"] + "/skin.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		for (ConfStrHash::iterator curr = skinConfStr.begin(); curr != skinConfStr.end(); curr++) {
			if (
				curr->first.empty() || curr->second.empty()
			) continue;

			inf << curr->first << "=\"" << curr->second << "\"" << endl;
		}
		for (ConfIntHash::iterator curr = skinConfInt.begin(); curr != skinConfInt.end(); curr++) {
			if (
				curr->first == "titleFontSize" ||
				curr->first == "sectionBarHeight" ||
				curr->first == "linkHeight" ||
				curr->first == "selectorPreviewX" ||
				curr->first == "selectorPreviewY" ||
				curr->first == "selectorPreviewWidth" ||
				curr->first == "selectorPreviewHeight" ||
				curr->first == "selectorX" ||
				curr->first == "linkItemHeight" ||
				curr->first == "topBarHeight" ||

				(curr->first == "linkCols" && curr->second == 4) ||
				(curr->first == "linkRows" && curr->second == 4) ||
				(curr->first == "sectionBar" && curr->second == SB_CLASSIC) ||
				(curr->first == "sectionLabel" && curr->second == 0) ||
				(curr->first == "linkLabel" && curr->second == 1) ||

				curr->first.empty()
			) continue;
			inf << curr->first << "=" << curr->second << endl;
		}

		for (int i = 0; i < NUM_COLORS; ++i) {
			inf << colorToString((enum color)i) << "=" << rgbatostr(skinConfColors[i]) << endl;
		}

		inf.close();
		sync();
	}
	ledOff();
}

void GMenu2X::setSkin(const string &skin, bool resetWallpaper, bool clearSC) {
	confStr["skin"] = skin;

	// Clear previous skin settings
	skinConfStr.clear();
	skinConfInt.clear();

	// Defaults *** Sync with default values in writeConfig
	skinConfInt["sectionBar"] = SB_CLASSIC;
	skinConfInt["sectionLabel"] = 1;
	skinConfInt["linkLabel"] = 1;

	// clear collection and change the skin path
	if (clearSC) {
		sc.clear();
	}

	sc.setSkin(skin);

	// reset colors to the default values
	skinConfColors[COLOR_TOP_BAR_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_LIST_BG] = (RGBAColor){255,255,255,0};
	skinConfColors[COLOR_BOTTOM_BAR_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_SELECTION_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_MESSAGE_BOX_BG] = (RGBAColor){255,255,255,255};
	skinConfColors[COLOR_MESSAGE_BOX_BORDER] = (RGBAColor){80,80,80,255};
	skinConfColors[COLOR_MESSAGE_BOX_SELECTION] = (RGBAColor){160,160,160,255};
	skinConfColors[COLOR_FONT] = (RGBAColor){255,255,255,255};
	skinConfColors[COLOR_FONT_OUTLINE] = (RGBAColor){0,0,0,200};
	skinConfColors[COLOR_FONT_ALT] = (RGBAColor){253,1,252,0};
	skinConfColors[COLOR_FONT_ALT_OUTLINE] = (RGBAColor){253,1,252,0};

	// load skin settings
	string skinconfname = "skins/" + skin + "/skin.conf";
	if (fileExists(skinconfname)) {
		ifstream skinconf(skinconfname.c_str(), ios_base::in);
		if (skinconf.is_open()) {
			string line;
			while (getline(skinconf, line, '\n')) {
				line = trim(line);
				// DEBUG("skinconf: '%s'", line.c_str());
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (value.length() > 0) {
					if (value.length() > 1 && value.at(0) == '"' && value.at(value.length() - 1) == '"') {
						skinConfStr[name] = value.substr(1, value.length() - 2);
					} else if (value.at(0) == '#') {
						// skinConfColor[name] = strtorgba(value.substr(1,value.length()) );
						skinConfColors[stringToColor(name)] = strtorgba(value);
					} else if (name.length() > 6 && name.substr(name.length() - 6, 5 ) == "Color") {
						value += name.substr(name.length() - 1);
						name = name.substr(0, name.length() - 6);
						if (name == "selection" || name == "topBar" || name == "bottomBar" || name == "messageBox") name += "Bg";
						if (value.substr(value.length() - 1) == "R") skinConfColors[stringToColor(name)].r = atoi(value.c_str());
						if (value.substr(value.length() - 1) == "G") skinConfColors[stringToColor(name)].g = atoi(value.c_str());
						if (value.substr(value.length() - 1) == "B") skinConfColors[stringToColor(name)].b = atoi(value.c_str());
						if (value.substr(value.length() - 1) == "A") skinConfColors[stringToColor(name)].a = atoi(value.c_str());
					} else {
						skinConfInt[name] = atoi(value.c_str());
					}
				}
			}
			skinconf.close();

			// if (resetWallpaper && !skinConfStr["wallpaper"].empty() && fileExists("skins/" + skin + "/wallpapers/" + skinConfStr["wallpaper"])) {
			// 	setWallpaper("skins/" + skin + "/wallpapers/" + skinConfStr["wallpaper"]);
			// 	// confStr["wallpaper"] = "skins/" + skin + "/wallpapers/" + skinConfStr["wallpaper"];
			// 	// sc[confStr["wallpaper"]]->blit(bg,0,0);
			// }
		}
	}

// (poor) HACK: ensure font alt colors have a default value
	if (skinConfColors[COLOR_FONT_ALT].r == 253 && skinConfColors[COLOR_FONT_ALT].g == 1 && skinConfColors[COLOR_FONT_ALT].b == 252 && skinConfColors[COLOR_FONT_ALT].a == 0) skinConfColors[COLOR_FONT_ALT] = skinConfColors[COLOR_FONT];
	if (skinConfColors[COLOR_FONT_ALT_OUTLINE].r == 253 && skinConfColors[COLOR_FONT_ALT_OUTLINE].g == 1 && skinConfColors[COLOR_FONT_ALT_OUTLINE].b == 252 && skinConfColors[COLOR_FONT_ALT_OUTLINE].a == 0) skinConfColors[COLOR_FONT_ALT_OUTLINE] = skinConfColors[COLOR_FONT_OUTLINE];

	// prevents breaking current skin until they are updated
	if (!skinConfInt["fontSizeTitle"] && skinConfInt["titleFontSize"] > 0) skinConfInt["fontSizeTitle"] = skinConfInt["titleFontSize"];

	evalIntConf( &skinConfInt["sectionBarSize"], 40, 1, resX);
	evalIntConf( &skinConfInt["bottomBarHeight"], 16, 1, resY);
	evalIntConf( &skinConfInt["previewWidth"], 142, 1, resX);
	evalIntConf( &skinConfInt["fontSize"], 12, 6, 60);
	evalIntConf( &skinConfInt["fontSizeTitle"], 20, 6, 60);
	evalIntConf( &skinConfInt["sectionBar"], SB_CLASSIC, 0, 5);
	evalIntConf( &skinConfInt["linkCols"], 4, 1, 8);
	evalIntConf( &skinConfInt["linkRows"], 4, 1, 8);

	// if (menu != NULL && clearSC) menu->loadIcons();
	initFont();
}

uint32_t GMenu2X::onChangeSkin() {
	return 1;
}

void GMenu2X::skinMenu() {
	bool save = false;
	int selected = 0;
	string initSkin = confStr["skin"];
	string prevSkin = "/";

	FileLister fl_sk("skins", true, false);
	fl_sk.addExclude("..");
	fl_sk.browse();

	vector<string> wpLabel;
	wpLabel.push_back(">>");
	string tmp = ">>";

	vector<string> sbStr;
	sbStr.push_back("OFF");
	sbStr.push_back("Left");
	sbStr.push_back("Bottom");
	sbStr.push_back("Right");
	sbStr.push_back("Top");
	sbStr.push_back("Classic");
	int sbPrev = skinConfInt["sectionBar"];
	string sectionBar = sbStr[skinConfInt["sectionBar"]];

	vector<string> bgScale;
	bgScale.push_back("Original");
	bgScale.push_back("Crop");
	bgScale.push_back("Aspect");
	bgScale.push_back("Stretch");
	string bgScalePrev = confStr["bgscale"];

	vector<string> bdStr;
	bdStr.push_back("OFF");
	bdStr.push_back("Menu only");
	bdStr.push_back("Dialog only");
	bdStr.push_back("Menu & Dialog");
	int bdPrev = confInt["skinBackdrops"];
	string skinBackdrops = bdStr[confInt["skinBackdrops"]];

	vector<string> skinFont;
	skinFont.push_back("Custom");
	skinFont.push_back("Default");
	string skinFontPrev = confStr["skinFont"];

	vector<string> wallpapers;
	string wpPath = confStr["wallpaper"];
	confStr["tmp_wallpaper"] = "";

	do {
		if (prevSkin != confStr["skin"] || skinFontPrev != confStr["skinFont"]) {

			prevSkin = confStr["skin"];
			skinFontPrev = confStr["skinFont"];

			setSkin(confStr["skin"], false, false);
			sectionBar = sbStr[skinConfInt["sectionBar"]];
	
			confStr["tmp_wallpaper"] = (confStr["tmp_wallpaper"].empty() || skinConfStr["wallpaper"].empty()) ? base_name(confStr["wallpaper"]) : skinConfStr["wallpaper"];
			wallpapers.clear();

			FileLister fl_wp("skins/" + confStr["skin"] + "/wallpapers");
			fl_wp.setFilter(".png,.jpg,.jpeg,.bmp");
			fl_wp.browse();
			wallpapers = fl_wp.getFiles();

			if (confStr["skin"] != "Default") {
				fl_wp.setPath("skins/Default/wallpapers", true);
				wallpapers.insert( wallpapers.end(), fl_wp.getFiles().begin(), fl_wp.getFiles().end());
			}

			sc.del("skin:icons/skin.png");
			sc.del("skin:imgs/buttons/left.png");
			sc.del("skin:imgs/buttons/right.png");
			sc.del("skin:imgs/buttons/a.png");
		}

		wpPath = "skins/" + confStr["skin"] + "/wallpapers/" + confStr["tmp_wallpaper"];
		if (!fileExists(wpPath)) wpPath = "skins/Default/wallpapers/" + confStr["tmp_wallpaper"];
		if (!fileExists(wpPath)) wpPath = "skins/" + confStr["skin"] + "/wallpapers/" + wallpapers.at(0);
		if (!fileExists(wpPath)) wpPath = "skins/Default/wallpapers/" + wallpapers.at(0);

		setBackground(bg, wpPath);

		SettingsDialog sd(this, ts, tr["Skin"], "skin:icons/skin.png");
		sd.selected = selected;
		sd.allowCancel = false;
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenu2X"], &confStr["skin"], &fl_sk.getDirectories(), MakeDelegate(this, &GMenu2X::onChangeSkin)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Wallpaper"], tr["Select an image to use as a wallpaper"], &confStr["tmp_wallpaper"], &wallpapers, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::changeWallpaper)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Background scale"], tr["How to scale wallpaper and backdrops"], &confStr["bgscale"], &bgScale));
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin colors"], tr["Customize skin colors"], &tmp, &wpLabel, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::skinColors)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin backdrops"], tr["Automatic load backdrops from skin pack"], &skinBackdrops, &bdStr));
		sd.addSetting(new MenuSettingMultiString(this, tr["Font face"], tr["Override the skin font face"], &confStr["skinFont"], &skinFont, MakeDelegate(this, &GMenu2X::onChangeSkin)));
		sd.addSetting(new MenuSettingInt(this, tr["Font size"], tr["Size of text font"], &skinConfInt["fontSize"], 12, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Title font size"], tr["Size of title's text font"], &skinConfInt["fontSizeTitle"], 20, 6, 60));
		sd.addSetting(new MenuSettingMultiString(this, tr["Section bar layout"], tr["Set the layout and position of the Section Bar"], &sectionBar, &sbStr));
		sd.addSetting(new MenuSettingInt(this, tr["Section bar size"], tr["Size of section and top bar"], &skinConfInt["sectionBarSize"], 40, 1, resX));
		sd.addSetting(new MenuSettingInt(this, tr["Bottom bar height"], tr["Height of bottom bar"], &skinConfInt["bottomBarHeight"], 16, 1, resY));
		sd.addSetting(new MenuSettingInt(this, tr["Menu columns"], tr["Number of columns of links in main menu"], &skinConfInt["linkCols"], 4, 1, 8));
		sd.addSetting(new MenuSettingInt(this, tr["Menu rows"], tr["Number of rows of links in main menu"], &skinConfInt["linkRows"], 4, 1, 8));
		sd.addSetting(new MenuSettingBool(this, tr["Link label"], tr["Show link labels in main menu"], &skinConfInt["linkLabel"]));
		sd.addSetting(new MenuSettingBool(this, tr["Section label"], tr["Show the active section label in main menu"], &skinConfInt["sectionLabel"]));
		sd.exec();

		selected = sd.selected;
		save = sd.save;
	} while (!save);

	if (skinBackdrops == "OFF") confInt["skinBackdrops"] = BD_OFF;
	else if (skinBackdrops == "Menu & Dialog") confInt["skinBackdrops"] = BD_MENU | BD_DIALOG;
	else if (skinBackdrops == "Menu only") confInt["skinBackdrops"] = BD_MENU;
	else if (skinBackdrops == "Dialog only") confInt["skinBackdrops"] = BD_DIALOG;

	if (sectionBar == "OFF") skinConfInt["sectionBar"] = SB_OFF;
	else if (sectionBar == "Right") skinConfInt["sectionBar"] = SB_RIGHT;
	else if (sectionBar == "Top") skinConfInt["sectionBar"] = SB_TOP;
	else if (sectionBar == "Bottom") skinConfInt["sectionBar"] = SB_BOTTOM;
	else if (sectionBar == "Left") skinConfInt["sectionBar"] = SB_LEFT;
	else skinConfInt["sectionBar"] = SB_CLASSIC;

	confStr["tmp_wallpaper"] = "";
	confStr["wallpaper"] = wpPath;
	writeSkinConfig();
	writeConfig();

	if (bdPrev != confInt["skinBackdrops"] || initSkin != confStr["skin"] || bgScalePrev != confStr["bgscale"]) restartDialog();
	if (sbPrev != skinConfInt["sectionBar"]) initMenu();
	initLayout();
}

void GMenu2X::skinColors() {
	bool save = false;
	do {
		setSkin(confStr["skin"], false, false);

		SettingsDialog sd(this, ts, tr["Skin Colors"], "skin:icons/skin.png");
		sd.allowCancel = false;
		sd.addSetting(new MenuSettingRGBA(this, tr["Top/Section Bar"], tr["Color of the top and section bar"], &skinConfColors[COLOR_TOP_BAR_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["List Body"], tr["Color of the list body"], &skinConfColors[COLOR_LIST_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Bottom Bar"], tr["Color of the bottom bar"], &skinConfColors[COLOR_BOTTOM_BAR_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Selection"], tr["Color of the selection and other interface details"], &skinConfColors[COLOR_SELECTION_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Message Box"], tr["Background color of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Msg Box Border"], tr["Border color of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_BORDER]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Msg Box Selection"], tr["Color of the selection of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_SELECTION]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Font"], tr["Color of the font"], &skinConfColors[COLOR_FONT]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Font Outline"], tr["Color of the font's outline"], &skinConfColors[COLOR_FONT_OUTLINE]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font"], tr["Color of the alternative font"], &skinConfColors[COLOR_FONT_ALT]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font Outline"], tr["Color of the alternative font outline"], &skinConfColors[COLOR_FONT_ALT_OUTLINE]));
		sd.exec();
		save = sd.save;
	} while (!save);
	writeSkinConfig();
}

void GMenu2X::about() {
	vector<string> text;
	string temp = "", buf;

	// temp = tr["Build date: "] + __DATE__ + "\n";

	float battlevel = getBatteryStatus();
	{ stringstream ss; ss.precision(2); ss << battlevel/1000; ss >> buf; }
	temp += tr["Battery: "] + ((battlevel < 0 || battlevel > 10000) ? tr["Charging"] : buf + " V") + "\n";

	temp += tr["Uptime: "] + ms2hms(SDL_GetTicks()) + "\n";

	temp += "----\n";

	TextDialog td(this, "GMenuNX", tr["Info about GMenuNX"], "skin:icons/about.png");

	td.appendText(temp);
	td.appendFile("about.txt");
	td.exec();
}

void GMenu2X::viewLog() {
	string logfile = path + "log.txt";
	if (!fileExists(logfile)) return;

	TextDialog td(this, tr["Log Viewer"], tr["Last launched program's output"], "skin:icons/ebook.png");
	td.appendFile(path + "log.txt");
	td.exec();

	MessageBox mb(this, tr["Do you want to delete the log file?"], "skin:icons/ebook.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		ledOn();
		unlink(logfile.c_str());
		sync();
		menu->deleteSelectedLink();
		ledOff();
	}
}

void GMenu2X::changeWallpaper() {
	WallpaperDialog wp(this, tr["Wallpaper"], tr["Select an image to use as a wallpaper"], "skin:icons/wallpaper.png");
	if (wp.exec() && confStr["wallpaper"] != wp.wallpaper) {
		confStr["wallpaper"] = wp.wallpaper;
		confStr["tmp_wallpaper"] = base_name(confStr["wallpaper"]);
	}
}

void GMenu2X::showManual() {
	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManualPath();
	string linkBackdrop = confInt["skinBackdrops"] | BD_DIALOG ? menu->selLinkApp()->getBackdropPath() : "";

	if (linkManual == "" || !fileExists(linkManual)) return;

	string ext = linkManual.substr(linkManual.size() - 4, 4);
	if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == "jpeg") {
		ImageViewerDialog im(this, linkTitle, linkDescription, linkIcon, linkManual);
		im.exec();
		return;
	}

	TextDialog td(this, linkTitle, linkDescription, linkIcon); //, linkBackdrop);
	td.appendFile(linkManual);
	td.exec();
}

void GMenu2X::explorer() {
	BrowseDialog bd(this, tr["Explorer"], tr["Select a file or application"]);
	bd.showDirectories = true;
	bd.showFiles = true;

	while (bd.exec()) {
		string ext = bd.getExt(bd.selected);
		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp") {
			ImageViewerDialog im(this, tr["Image viewer"], bd.getFile(bd.selected), "icons/explorer.png", bd.getFilePath(bd.selected));
			im.exec();
		} else if (ext == ".txt" || ext == ".conf" || ext == ".me" || ext == ".md" || ext == ".xml" || ext == ".log" || ext == ".ini") {
			TextDialog td(this, tr["Text viewer"], bd.getFile(bd.selected), "skin:icons/ebook.png");
			td.appendFile(bd.getFilePath(bd.selected));
			td.exec();
		} else if (ext == ".ipk") {
			TerminalDialog td(this, tr["Package installer"], "opkg install " + bd.getFileName(bd.selected), "skin:icons/configure.png");
			// string cmd = "opkg install --force-reinstall " + bd.getFilePath(bd.selected);
			td.exec("opkg install --force-reinstall " + bd.getFilePath(bd.selected));
			initMenu();
		} else if (ext == ".sh") {
			TerminalDialog td(this, tr["Terminal"], "sh" + bd.getFilePath(bd.selected), "skin:icons/terminal.png");
			// string cmd = bd.getFilePath(bd.selected);
			td.exec(bd.getFilePath(bd.selected));
		} else {
			if (confInt["saveSelection"] && (confInt["section"] != menu->selSectionIndex() || confInt["link"] != menu->selLinkIndex()))
				writeConfig();

			string command = cmdclean(bd.getFilePath(bd.selected));
			chdir(bd.getPath().c_str());
			quit();
			setCPU(confInt["cpuMenu"]);

			if (confInt["outputLogs"]) {
				if (fileExists("/usr/bin/gdb")) command = "gdb -batch -ex \"run\" -ex \"bt\" --args " + command;
				command += " 2>&1 | tee " + cmdclean(getExePath()) + "/log.txt";
			}
			execlp("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
			// if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
			// try relaunching gmenu2x
			// WARNING("Error executing selected application, re-launching gmenu2x");
			chdir(getExePath().c_str());
			execlp("./gmenu2x", "./gmenu2x", NULL);
			return;
		}
	}
}

const string GMenu2X::getDateTime() {
	char buf[80];
	time_t now = time(0);
	struct tm  tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%F %R", &tstruct);
	return buf;
}

void GMenu2X::setDateTime() {
	int imonth, iday, iyear, ihour, iminute;
	string value = confStr["datetime"];

	sscanf(value.c_str(), "%d-%d-%d %d:%d", &iyear, &imonth, &iday, &ihour, &iminute);

	struct tm datetime = { 0 };

	datetime.tm_year = iyear - 1900;
	datetime.tm_mon  = imonth - 1;
	datetime.tm_mday = iday;
	datetime.tm_hour = ihour;
	datetime.tm_min  = iminute;
	datetime.tm_sec  = 0;

	if (datetime.tm_year < 0) datetime.tm_year = 0;

	time_t t = mktime(&datetime);

	struct timeval newtime = { t, 0 };

#if !defined(TARGET_PC)
	settimeofday(&newtime, NULL);
#endif
}

bool GMenu2X::saveScreenshot() {
	ledOn();
	uint32_t x = 0;
	string fname;

	mkdir("screenshots/", 0777);

	do {
		x++;
		stringstream ss;
		ss << x;
		ss >> fname;
		fname = "screenshots/screen" + fname + ".bmp";
	} while (fileExists(fname));
	x = SDL_SaveBMP(s->raw, fname.c_str());
	sync();
	ledOff();
	return x == 0;
}

void GMenu2X::restartDialog(bool showDialog) {
	if (showDialog) {
		MessageBox mb(this, tr["GMenuNX will restart to apply\nthe settings. Continue?"], "skin:icons/exit.png");
		mb.setButton(CONFIRM, tr["Restart"]);
		mb.setButton(CANCEL,  tr["Cancel"]);
		if (mb.exec() == CANCEL) return;
	}

	quit();
	WARNING("Re-launching gmenu2x");
	chdir(getExePath().c_str());
	execlp("./gmenu2x", "./gmenu2x", NULL);
}

void GMenu2X::poweroffDialog() {
	MessageBox mb(this, tr["Poweroff or reboot the device?"], "skin:icons/exit.png");
	mb.setButton(SECTION_NEXT, tr["Reboot"]);
	mb.setButton(CONFIRM, tr["Poweroff"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	int response = mb.exec();
	writeConfig();
	if (response == CONFIRM) {
		MessageBox mb(this, tr["Poweroff"]);
		mb.setAutoHide(500);
		mb.exec();
		setBacklight(0);
#if !defined(TARGET_PC)
		system("sync; poweroff");
#endif
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(this, tr["Rebooting"]);
		mb.setAutoHide(500);
		mb.exec();
		setBacklight(0);
#if !defined(TARGET_PC)
		system("sync; reboot");
#endif
	}
}

void GMenu2X::umountSdDialog() {
	BrowseDialog bd(this, tr["Umount"], tr["Umount external media device"], "skin:icons/eject.png");
	bd.showDirectories = true;
	bd.showFiles = false;
	bd.allowDirUp = false;
	bd.allowEnterDirectory = false;
	bd.setPath("/media");
	bd.exec();
}

void GMenu2X::contextMenu() {
	vector<MenuOption> voices;
	if (menu->selLinkApp() != NULL) {
		voices.push_back((MenuOption){tr.translate("Edit $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::editLink)});
		voices.push_back((MenuOption){tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::deleteLink)});
	}
	voices.push_back((MenuOption){tr["Add link"], 		MakeDelegate(this, &GMenu2X::addLink)});
	voices.push_back((MenuOption){tr["Add section"],	MakeDelegate(this, &GMenu2X::addSection)});
	voices.push_back((MenuOption){tr["Rename section"],	MakeDelegate(this, &GMenu2X::renameSection)});
	voices.push_back((MenuOption){tr["Delete section"],	MakeDelegate(this, &GMenu2X::deleteSection)});
	// voices.push_back((MenuOption){tr["Link scanner"],	MakeDelegate(this, &GMenu2X::linkScanner)});

	Surface bg(s);
	bool close = false, inputAction = false;
	int sel = 0;
	uint32_t i, fadeAlpha = 0, h = font->getHeight(), h2 = font->getHalfHeight();

	SDL_Rect box;
	box.h = h * voices.size() + 8;
	box.w = 0;
	for (i = 0; i < voices.size(); i++) {
		int w = font->getTextWidth(voices[i].text);
		if (w > box.w) box.w = w;
	}
	box.w += 23;
	box.x = resX/2 - box.w/2;
	box.y = resY/2 - box.h/2;

	uint32_t tickStart = SDL_GetTicks();
	while (!close) {
		bg.blit(s, 0, 0);

		s->box(0, 0, resX, resY, 0,0,0, fadeAlpha);
		s->box(box.x, box.y, box.w, box.h, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		s->rectangle( box.x + 2, box.y + 2, box.w - 4, box.h - 4, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );

		//draw selection rect
		s->box( box.x + 4, box.y + 4 + h * sel, box.w - 8, h, skinConfColors[COLOR_MESSAGE_BOX_SELECTION] );
		for (i = 0; i < voices.size(); i++)
			s->write( font, voices[i].text, box.x + 12, box.y + h2 + 3 + h * i, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);

		s->flip();

		if (fadeAlpha < 200) {
			fadeAlpha = intTransition(0, 200, tickStart, 200);
			continue;
		}
		do {
			inputAction = input.update();

			if (inputCommonActions(inputAction)) continue;

			if ( input[MENU] || input[CANCEL]) close = true;
			else if ( input[UP] ) sel = (sel - 1 < 0) ? (int)voices.size() - 1 : sel - 1 ;
			else if ( input[DOWN] ) sel = (sel + 1 > (int)voices.size() - 1) ? 0 : sel + 1;
			else if ( input[LEFT] || input[PAGEUP] ) sel = 0;
			else if ( input[RIGHT] || input[PAGEDOWN] ) sel = (int)voices.size() - 1;
			else if ( input[SETTINGS] || input[CONFIRM] ) { voices[sel].action(); close = true; }
		} while (!inputAction);
	}
}

void GMenu2X::addLink() {
	BrowseDialog bd(this, tr["Add link"], tr["Select an application"]);
	bd.showDirectories = true;
	bd.showFiles = true;
	bd.setFilter(".dge,.gpu,.gpe,.sh,.bin,.elf,");
	if (bd.exec()) {
		ledOn();
		if (menu->addLink(bd.getPath(), bd.getFile(bd.selected))) {
			editLink();
		}
		sync();
		ledOff();
	}
}

void GMenu2X::editLink() {
	if (menu->selLinkApp() == NULL) return;

	vector<string> pathV;
	split(pathV, menu->selLinkApp()->getFile(), "/");
	string oldSection = "";
	if (pathV.size() > 1) oldSection = pathV[pathV.size()-2];
	string newSection = oldSection;

	string linkExec = menu->selLinkApp()->getExec();
	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManual();
	string linkParams = menu->selLinkApp()->getParams();
	string linkSelFilter = menu->selLinkApp()->getSelectorFilter();
	string linkSelDir = menu->selLinkApp()->getSelectorDir();
	bool useSelector = !linkSelDir.empty();
	bool linkSelBrowser = menu->selLinkApp()->getSelectorBrowser();
	string linkSelScreens = menu->selLinkApp()->getSelectorScreens();
	string linkSelAliases = menu->selLinkApp()->getAliasFile();
	int linkClock = menu->selLinkApp()->getCPU();
	string linkBackdrop = menu->selLinkApp()->getBackdrop();
	string dialogTitle = tr.translate("Edit $1", linkTitle.c_str(), NULL);
	string dialogIcon = menu->selLinkApp()->getIconPath();
	string linkDir = dir_name(linkExec);

	SettingsDialog sd(this, ts, dialogTitle, dialogIcon);
	sd.addSetting(new MenuSettingFile(			this, tr["Executable"],		tr["Application this link points to"], &linkExec, ".dge,.gpu,.gpe,.sh,.bin,.elf,", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Title"],			tr["Link title"], &linkTitle, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Description"],	tr["Link description"], &linkDescription, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingMultiString(	this, tr["Section"],		tr["The section this link belongs to"], &newSection, &menu->getSections()));
	sd.addSetting(new MenuSettingImage(			this, tr["Icon"],			tr["Select a custom icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg,.gif", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingInt(			this, tr["CPU Clock"],		tr["CPU clock frequency when launching this link"], &linkClock, confInt["cpuMenu"], confInt["cpuMin"], confInt["cpuMax"], 6));
	sd.addSetting(new MenuSettingString(		this, tr["Parameters"],		tr["Command line arguments to pass to the application"], &linkParams, dialogTitle, dialogIcon));

	if (confInt["saveSelection"]) {
		sd.addSetting(new MenuSettingBool(		this, tr["File Selector"],	tr["Use file browser selector"], &useSelector));
	} else {
		sd.addSetting(new MenuSettingDir(		this, tr["Selector Path"],	tr["Directory to start the selector"], &linkSelDir, linkDir, dialogTitle, dialogIcon));
	}

	sd.addSetting(new MenuSettingBool(			this, tr["Show Folders"],	tr["Allow the selector to change directory"], &linkSelBrowser));
	sd.addSetting(new MenuSettingString(		this, tr["File Filter"],	tr["Filter by file extension (separate with commas)"], &linkSelFilter, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingDir(			this, tr["Box Art"],		tr["Directory of the box art for the selector"], &linkSelScreens, linkDir, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingFile(			this, tr["Aliases"],		tr["File containing a list of aliases for the selector"], &linkSelAliases, ".txt,.dat", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingImage(			this, tr["Backdrop"],		tr["Select an image backdrop"], &linkBackdrop, ".png,.bmp,.jpg,.jpeg", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingFile(			this, tr["Manual"],			tr["Select a Manual or Readme file"], &linkManual, ".man.png,.txt,.me", linkExec, dialogTitle, dialogIcon));

#if defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	bool linkUseGinge = menu->selLinkApp()->getUseGinge();
	string ginge_prep = getExePath() + "/ginge/ginge_prep";
	if (fileExists(ginge_prep))
		sd.addSetting(new MenuSettingBool(		this, tr["Use Ginge"],			tr["Compatibility layer for running GP2X applications"], &linkUseGinge ));
#elif defined(TARGET_GP2X)
	int linkGamma = menu->selLinkApp()->gamma();
	sd.addSetting(new MenuSettingInt(			this, tr["Gamma (default: 0)"],	tr["Gamma value to set when launching this link"], &linkGamma, 0, 100 ));
#endif

	if (sd.exec() && sd.edited() && sd.save) {
		ledOn();

		menu->selLinkApp()->setExec(linkExec);
		menu->selLinkApp()->setTitle(linkTitle);
		menu->selLinkApp()->setDescription(linkDescription);
		menu->selLinkApp()->setIcon(linkIcon);
		menu->selLinkApp()->setManual(linkManual);
		menu->selLinkApp()->setParams(linkParams);
		menu->selLinkApp()->setSelectorFilter(linkSelFilter);

		if (useSelector) {
			if (linkSelDir.empty()) linkSelDir = confStr["defaultDir"];
		} else if (confInt["saveSelection"]) {
			linkSelDir = "";
		}

		menu->selLinkApp()->setSelectorDir(linkSelDir);
		menu->selLinkApp()->setSelectorBrowser(linkSelBrowser);
		menu->selLinkApp()->setSelectorScreens(linkSelScreens);
		menu->selLinkApp()->setAliasFile(linkSelAliases);
		menu->selLinkApp()->setBackdrop(linkBackdrop);
		menu->selLinkApp()->setCPU(linkClock);

#if defined(TARGET_GP2X)
		menu->selLinkApp()->setGamma(linkGamma);
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
		menu->selLinkApp()->setUseGinge(linkUseGinge);
#endif

		// if section changed move file and update link->file
		if (oldSection != newSection) {
			vector<string>::const_iterator newSectionIndex = find(menu->getSections().begin(), menu->getSections().end(), newSection);
			if (newSectionIndex == menu->getSections().end()) return;
			string newFileName = "sections/" + newSection + "/" + linkTitle;
			uint32_t x = 2;
			while (fileExists(newFileName)) {
				string id = "";
				stringstream ss; ss << x; ss >> id;
				newFileName = "sections/" + newSection + "/" + linkTitle + id;
				x++;
			}
			rename(menu->selLinkApp()->getFile().c_str(), newFileName.c_str());
			menu->selLinkApp()->renameFile(newFileName);

			INFO("New section: (%i) %s", newSectionIndex - menu->getSections().begin(), newSection.c_str());

			menu->linkChangeSection(menu->selLinkIndex(), menu->selSectionIndex(), newSectionIndex - menu->getSections().begin());
		}
		menu->selLinkApp()->save();
		sync();
		ledOff();
	}
	confInt["section"] = menu->selSectionIndex();
	confInt["link"] = menu->selLinkIndex();
	initMenu();
}

void GMenu2X::deleteLink() {
	if (menu->selLinkApp() != NULL) {
		MessageBox mb(this, tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL) + "\n" + tr["Are you sure?"], menu->selLink()->getIconPath());
		mb.setButton(CONFIRM, tr["Yes"]);
		mb.setButton(CANCEL,  tr["No"]);
		if (mb.exec() == CONFIRM) {
			ledOn();
			menu->deleteSelectedLink();
			sync();
			ledOff();
		}
	}
}

void GMenu2X::addSection() {
	InputDialog id(this, ts, tr["Insert a name for the new section"], "", tr["Add section"], "skin:icons/section.png");
	if (id.exec()) {
		// only if a section with the same name does not exist
		if (find(menu->getSections().begin(), menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			// section directory doesn't exists
			ledOn();
			if (menu->addSection(id.getInput())) {
				menu->setSectionIndex(menu->getSections().size() - 1); //switch to the new section
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::renameSection() {
	InputDialog id(this, ts, tr["Insert a new name for this section"], menu->selSection(), tr["Rename section"], menu->getSectionIcon(menu->selSectionIndex()));
	if (id.exec()) {
		// only if a section with the same name does not exist & !samename
		if (menu->selSection() != id.getInput() && find(menu->getSections().begin(),menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			// section directory doesn't exists
			string newsectiondir = "sections/" + id.getInput();
			string sectiondir = "sections/" + menu->selSection();
			ledOn();
			if (rename(sectiondir.c_str(), "tmpsection")==0 && rename("tmpsection", newsectiondir.c_str())==0) {
				string oldpng = sectiondir + ".png", newpng = newsectiondir+".png";
				string oldicon = sc.getSkinFilePath(oldpng), newicon = sc.getSkinFilePath(newpng);
				if (!oldicon.empty() && newicon.empty()) {
					newicon = oldicon;
					newicon.replace(newicon.find(oldpng), oldpng.length(), newpng);

					if (!fileExists(newicon)) {
						rename(oldicon.c_str(), "tmpsectionicon");
						rename("tmpsectionicon", newicon.c_str());
						sc.move("skin:" + oldpng, "skin:" + newpng);
					}
				}
				menu->renameSection(menu->selSectionIndex(), id.getInput());
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::deleteSection() {
	MessageBox mb(this, tr["Are you sure?"]);
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		ledOn();
		if (rmtree(path+"sections/"+menu->selSection())) {
			menu->deleteSelectedSection();
			sync();
		}
		ledOff();
	}
}

void GMenu2X::setInputSpeed() {
	input.setInterval(180);
	input.setInterval(1000, SETTINGS);
	input.setInterval(1000, MENU);
	input.setInterval(1000, CONFIRM);
	input.setInterval(1000, POWER);
	// input.setInterval(30,  VOLDOWN);
	// input.setInterval(30,  VOLUP);
	// input.setInterval(300, CANCEL);
	// input.setInterval(300, MANUAL);
	// input.setInterval(100, INC);
	// input.setInterval(100, DEC);
	input.setInterval(500, SECTION_PREV);
	input.setInterval(500, SECTION_NEXT);
	// input.setInterval(500, PAGEUP);
	// input.setInterval(500, PAGEDOWN);
	// input.setInterval(200, BACKLIGHT);
}

int GMenu2X::getVolume() {
	int vol = -1;
	uint32_t soundDev = open("/dev/mixer", O_RDONLY);

	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_READ, &vol);
		close(soundDev);
		if (vol != -1) {
			// just return one channel , not both channels, they're hopefully the same anyways
			return vol & 0xFF;
		}
	}
	return vol;
}

int GMenu2X::setVolume(int val, bool popup) {
	int volumeStep = 10;

	if (val < 0) val = 100;
	else if (val > 100) val = 0;

	if (popup) {
		bool close = false;

		Surface bg(s);

		Surface *iconVolume[3] = {
			sc.skinRes("imgs/mute.png"),
			sc.skinRes("imgs/phones.png"),
			sc.skinRes("imgs/volume.png"),
		};

		powerManager->clearTimer();
		SDL_TimerID wakeUpTimer = NULL;
		while (!close) {
			SDL_RemoveTimer(wakeUpTimer); wakeUpTimer = NULL;
			wakeUpTimer = SDL_AddTimer(3000, input.wakeUp, (void*)false);

			drawSlider(val, 0, 100, *iconVolume[getVolumeMode(val)], bg);

			close = !input.update();

			if (input[SETTINGS] || input[CONFIRM] || input[CANCEL]) close = true;
			if ( input[LEFT] || input[DEC] )		val = max(0, val - volumeStep);
			else if ( input[RIGHT] || input[INC] )	val = min(100, val + volumeStep);
			else if ( input[SECTION_PREV] )	{
													val += volumeStep;
													if (val > 100) val = 0;
			}
		}
		powerManager->resetSuspendTimer();
		confInt["globalVolume"] = val;
		writeConfig();
	}

	uint32_t soundDev = open("/dev/mixer", O_RDWR);
	if (soundDev) {
		int vol = (val << 8) | val;
		ioctl(soundDev, SOUND_MIXER_WRITE, &vol);
		close(soundDev);

	}
	volumeMode = getVolumeMode(val);
	return val;
}

int GMenu2X::setBacklight(int val, bool popup) {
	int backlightStep = 10;

	if (val <= 6 - backlightStep) val = 100;
	else if (val < 0) val = 5;
	else if (val > 100 + backlightStep - 1) val = backlightStep;
	else if (val > 100) val = 100;

	if (popup) {
		bool close = false;

		Surface bg(s);

		Surface *iconBrightness[6] = {
			sc.skinRes("imgs/brightness/0.png"),
			sc.skinRes("imgs/brightness/1.png"),
			sc.skinRes("imgs/brightness/2.png"),
			sc.skinRes("imgs/brightness/3.png"),
			sc.skinRes("imgs/brightness/4.png"),
			sc.skinRes("imgs/brightness.png")
		};

		SDL_TimerID wakeUpTimer = NULL;
		while (!close) {
			SDL_RemoveTimer(wakeUpTimer); wakeUpTimer = NULL;
			wakeUpTimer = SDL_AddTimer(3000, input.wakeUp, (void*)false);

			if ( input[LEFT] || input[DEC] )		val = setBacklight(max(5, val - backlightStep), false);
			else if ( input[RIGHT] || input[INC] )	val = setBacklight(min(100, val + backlightStep), false);
			else if ( input[SECTION_NEXT] )			val = setBacklight(val + backlightStep, false);
			else if ( input[BACKLIGHT] )			{ SDL_Delay(50); val = getBacklight(); }

			int brightnessIcon = val/20;
			if (brightnessIcon > 4 || iconBrightness[brightnessIcon] == NULL) brightnessIcon = 5;

			drawSlider(val, 0, 100, *iconBrightness[brightnessIcon], bg);

			if ( input[SETTINGS] || input[CONFIRM] || input[CANCEL] ) close = true;
			else close = !input.update();
		}
		powerManager->resetSuspendTimer();
		confInt["backlight"] = val;
		writeConfig();
	}

	return val;
}

const string &GMenu2X::getExePath() {
	if (path.empty()) {
		char buf[255];
		memset(buf, 0, 255);
		int l = readlink("/proc/self/exe", buf, 255);

		path = buf;
		path = path.substr(0, l);
		l = path.rfind("/");
		path = path.substr(0, l + 1);
	}
	return path;
}

string GMenu2X::getDiskFree(const char *path) {
	string df = "N/A";
	struct statvfs b;

	if (statvfs(path, &b) == 0) {
		// Make sure that the multiplication happens in 64 bits.
		uint32_t freeMiB = ((uint64_t)b.f_bfree * b.f_bsize) / (1024 * 1024);
		uint32_t totalMiB = ((uint64_t)b.f_blocks * b.f_frsize) / (1024 * 1024);
		stringstream ss;
		if (totalMiB >= 10000) {
			ss	<< (freeMiB / 1024) << "." << ((freeMiB % 1024) * 10) / 1024 << "/"
				<< (totalMiB / 1024) << "." << ((totalMiB % 1024) * 10) / 1024 << "GiB";
		} else {
			ss	<< freeMiB << "/" << totalMiB << "MiB";
		}
		ss >> df;
	} else WARNING("statvfs failed with error '%s'", strerror(errno));
	return df;
}

int GMenu2X::drawButton(Button *btn, int x, int y) {
	if (y < 0) y = resY + y;
	btn->setPosition(x, y);
	btn->paint();
}

int GMenu2X::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = resY + y;
	if (sc.skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		sc["imgs/buttons/" + btn + ".png"]->blit(s, x, y, HAlignLeft | VAlignMiddle);
		x += 19;
		if (!text.empty()) {
			s->write(font, text, x, y, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
			x += font->getTextWidth(text);
		}
	}
	return x + 6;
}

int GMenu2X::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = resY + y;
	if (sc.skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		if (!text.empty()) {
			x -= font->getTextWidth(text);
			s->write(font, text, x, y, HAlignLeft | VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
		}
		x -= 19;
		sc["imgs/buttons/" + btn + ".png"]->blit(s, x, y, HAlignLeft | VAlignMiddle);
	}
	return x - 6;
}

void GMenu2X::drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect) {
	if (totalsize <= pagesize) return;

	// internal bar total height = height-2
	// bar size
	uint32_t bs = (scrollRect.h - 3) * pagesize / totalsize;
	// bar y position
	uint32_t by = (scrollRect.h - 3) * pagepos / totalsize;
	by = scrollRect.y + 3 + by;
	if (by + bs > scrollRect.y + scrollRect.h - 4)
		by = scrollRect.y + scrollRect.h - 4 - bs;

	s->rectangle(scrollRect.x + scrollRect.w - 4, by, 4, bs, skinConfColors[COLOR_LIST_BG]);
	s->box(scrollRect.x + scrollRect.w - 3, by + 1, 2, bs - 2, skinConfColors[COLOR_SELECTION_BG]);
}

void GMenu2X::drawSlider(int val, int min, int max, Surface &icon, Surface &bg) {
	SDL_Rect progress = {52, 32, resX-84, 8};
	SDL_Rect box = {20, 20, resX-40, 32};

	val = constrain(val, min, max);

	bg.blit(s,0,0);
	s->box(box, skinConfColors[COLOR_MESSAGE_BOX_BG]);
	s->rectangle(box.x+2, box.y+2, box.w-4, box.h-4, skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

	icon.blit(s, 28, 28);

	s->box(progress, skinConfColors[COLOR_MESSAGE_BOX_BG]);
	s->box(progress.x + 1, progress.y + 1, val * (progress.w - 3) / max + 1, progress.h - 2, skinConfColors[COLOR_MESSAGE_BOX_SELECTION]);
	s->flip();
}
