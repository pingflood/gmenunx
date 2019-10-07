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

// #include <sys/reboot.h>

#include <sys/statvfs.h>
#include <errno.h>

// for browsing the filesystem
#include <sys/stat.h>
// #include <sys/types.h>
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

using std::ifstream;
using std::ofstream;
using std::stringstream;
using namespace fastdelegate;

#define sync() sync(); system("sync &");

enum vol_mode_t {
	VOLUME_MODE_MUTE, VOLUME_MODE_PHONES, VOLUME_MODE_NORMAL
};
uint16_t mmcPrev = MMC_REMOVE, mmcStatus;
uint16_t udcPrev = UDC_REMOVE, udcStatus;
uint16_t tvOutPrev = TV_REMOVE, tvOutStatus;
uint16_t volumeModePrev, volumeMode;
uint16_t batteryIcon = 3;
uint8_t numJoy = 0; // number of connected joysticks

#if defined(TARGET_RETROGAME)
	#include "hw/retrogame.cpp"
#elif defined(TARGET_BITTBOY)
	#include "hw/bittboy.cpp"
#elif defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	#include "hw/gp2x.cpp"
	string fwType = "";
#else //if defined(TARGET_PC)
	#include "hw/pc.cpp"
#endif

#if defined(OPK_SUPPORT)
	#include <libopk.h>
#endif

const char *CARD_ROOT = getenv("HOME"); // Note: Add a trailing /!
// const int CARD_ROOT_LEN = 1;

static GMenu2X *app;

// Note: Keep this in sync with the enum!
static const char *colorNames[NUM_COLORS] = {
	"topBarBg",
	"listBg",
	"bottomBarBg",
	"selectionBg",
	"previewBg",
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

// char *ms2hms(uint32_t t, bool mm = true, bool ss = true) {
// 	static char buf[10];

// 	t = t / 1000;
// 	int s = (t % 60);
// 	int m = (t % 3600) / 60;
// 	int h = (t % 86400) / 3600;
// 	// int d = (t % (86400 * 30)) / 86400;

// 	if (!ss) sprintf(buf, "%02d:%02d", h, m);
// 	else if (!mm) sprintf(buf, "%02d", h);
// 	else sprintf(buf, "%02d:%02d:%02d", h, m, s);
// 	return buf;
// };

void syncDateTime(time_t t) {
#if !defined(TARGET_PC)
	struct timeval tv = { t, 0 };
	settimeofday(&tv, NULL);
	system("hwclock --systohc");
#endif
}

const string getDateTime() {
	char buf[80];
	time_t now = time(0);
	struct tm tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%F %R", &tstruct);
	return buf;
}

void initDateTime() {
	time_t now = time(0);
	uint32_t t = __BUILDTIME__;

	if (now < t) {
		syncDateTime(t);
	}
}

void setDateTime(const char* timestamp) {
	int imonth, iday, iyear, ihour, iminute;

	sscanf(timestamp, "%d-%d-%d %d:%d", &iyear, &imonth, &iday, &ihour, &iminute);

	struct tm datetime = { 0 };

	datetime.tm_year = iyear - 1900;
	datetime.tm_mon  = imonth - 1;
	datetime.tm_mday = iday;
	datetime.tm_hour = ihour;
	datetime.tm_min  = iminute;
	datetime.tm_sec  = 0;

	if (datetime.tm_year < 0) datetime.tm_year = 0;

	time_t t = mktime(&datetime);

	syncDateTime(t);
}

static void quit_all(int err) {
	delete app;
	exit(err);
}

GMenu2X::~GMenu2X() {
	quit();
	delete menu;
	delete s;
	delete font;
	delete titlefont;
}

void GMenu2X::quit() {
	writeConfig();
	s->flip(); s->flip(); s->flip(); // flush buffers
	s->free();

	fflush(NULL);
	SDL_Quit();
	hwDeinit();
}

int main(int /*argc*/, char * /*argv*/[]) {
	INFO("GMenu2X starting: If you read this message in the logs, check http://mtorromeo.github.com/gmenu2x/troubleshooting.html for a solution");

	signal(SIGINT,  &quit_all);
	signal(SIGSEGV, &quit_all);
	signal(SIGTERM, &quit_all);

	int fd = open("/dev/tty0", O_RDONLY);
	if (fd > 0) {
		ioctl(fd, VT_UNLOCKSWITCH, 1);
		ioctl(fd, KDSETMODE, KD_TEXT);
		ioctl(fd, KDSKBMODE, K_XLATE);
		close(fd);
	}

	usleep(1000);

	app = new GMenuNX();
	DEBUG("Starting GMenuNX");
	app->main();

	return 0;
}

GMenu2X *GMenu2X::instance = NULL;
void GMenu2X::main() {
	instance = this;

	setenv("SDL_FBCON_DONT_CLEAR", "1", 0);
	setenv("SDL_NOMOUSE", "1", 1);

	hwInit();

	path = "";
	chdir(getExePath().c_str());

	initDateTime();

	readConfig();

	setCPU(confInt["cpuMenu"]);

	input.init(path + "input.conf");

	setInputSpeed();

	setScaleMode(0);

	setBacklight(confInt["backlight"]);
	setVolume(confInt["globalVolume"]);

	setSkin(confStr["skin"], true);
	powerManager = new PowerManager(this, confInt["backlightTimeout"], confInt["powerTimeout"]);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0) {
		ERROR("Could not initialize SDL: %s", SDL_GetError());
		quit();
		return;
	}
	SDL_ShowCursor(0);

	SDL_Surface *screen = SDL_SetVideoMode(this->w, this->h, 16, SDL_HWSURFACE |
		#ifdef SDL_TRIPLEBUF
			SDL_TRIPLEBUF
		#else
			SDL_DOUBLEBUF
		#endif
	);
	s = new Surface();

	s->enableVirtualDoubleBuffer(screen);

	currBackdrop = confStr["wallpaper"];
	setBackground(s, currBackdrop);

	bg = new Surface(s);

	MessageBox mb(this, tr["Loading"]);
	mb.setAutoHide(1);
	mb.setBgAlpha(0);
	mb.exec();

	initMenu();

	tvOutStatus = getTVOutStatus();
	mmcStatus = getMMCStatus();
	udcStatus = getUDCStatus();
	volumeModePrev = volumeMode = getVolumeMode(confInt["globalVolume"]);

	readTmp();

	SDL_TimerID hwCheckTimer = SDL_AddTimer(1000, hwCheck, NULL);

	section_changed = icon_changed = SDL_GetTicks();
	SDL_TimerID sectionChangedTimer = SDL_AddTimer(2000, input.wakeUp, (void*)false);
	SDL_TimerID iconChangedTimer = SDL_AddTimer(1000, input.wakeUp, (void*)false);

	// recover last session
	if (lastSelectorElement >= 0 && menu->selLinkApp() != NULL && (!menu->selLinkApp()->getSelectorDir().empty() || !lastSelectorDir.empty())) {
		if (confInt["skinBackdrops"] & BD_DIALOG)
			setBackground(bg, currBackdrop);
		else
			setBackground(bg, confStr["wallpaper"]);
		menu->selLinkApp()->selector(lastSelectorElement, lastSelectorDir);
	}

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

	while (true) {
		// Background
		if (confInt["skinBackdrops"] & BD_MENU){
			if (menu->selLink() != NULL && !menu->selLink()->getBackdropPath().empty()) {
				currBackdrop = menu->selLink()->getBackdropPath();
			} else if (menu->selLinkApp() != NULL && !menu->selLinkApp()->getBackdropPath().empty()) {
				currBackdrop = menu->selLinkApp()->getBackdropPath();
			} else {
				currBackdrop = confStr["wallpaper"];
			}
			setBackground(s, currBackdrop);
		} else {
			setBackground(s, confStr["wallpaper"]);
		}

		// SECTIONS
		if (skinConfInt["sectionBar"]) {
			s->box(sectionBarRect, skinConfColors[COLOR_TOP_BAR_BG]);

			x = sectionBarRect.x; y = sectionBarRect.y; ix = 0;
			sx = (menu->selSectionIndex() - menu->firstDispSection()) * skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				ix = (this->w - skinConfInt["sectionBarSize"] * min(menu->sectionNumItems(), menu->getSections().size())) / 2;
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
				if (skinConfInt["sectionBar"] == SB_LEFT)
					s->write(font, tr[menu->selSectionName()], sx, sy + skinConfInt["sectionBarSize"], HAlignLeft | VAlignBottom);
				else if (skinConfInt["sectionBar"] == SB_RIGHT)
					s->write(font, tr[menu->selSectionName()], sx + skinConfInt["sectionBarSize"], sy + skinConfInt["sectionBarSize"], HAlignRight | VAlignBottom);
				else
					s->write(font, tr[menu->selSectionName()], sx + skinConfInt["sectionBarSize"] / 2 , sy + skinConfInt["sectionBarSize"], HAlignCenter | VAlignBottom);
			} else {
				SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;
			}

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				iconL->blit(s, 0, 0, HAlignLeft | VAlignTop);
				iconR->blit(s, this->w, 0, HAlignRight | VAlignTop);
			}
		}

		// LINKS
		s->setClipRect(linksRect);
		s->box(linksRect, skinConfColors[COLOR_LIST_BG]);

		i = menu->firstDispRow() * linkCols;

		if (linkCols == 1 && linkRows > 1) { // LIST
			ix = linksRect.x;
			for (y = 0; y < linkRows && i < menu->sectionLinks()->size(); y++, i++) {
				iy = linksRect.y + y * linkHeight;

				if (i == (uint32_t)menu->selLinkIndex())
					s->box(ix, iy, linksRect.w, linkHeight, skinConfColors[COLOR_SELECTION_BG]);

				Surface *icon = sc[menu->sectionLinks()->at(i)->getIconPath()];
				if (icon == NULL) icon = sc["skin:icons/generic.png"];
				if (icon->width() > 32 || icon->height() > linkHeight - 4)
					icon->softStretch(32, linkHeight - 4, true, true);

				icon->blit(s, {ix + 2, iy + 2, 32, linkHeight - 4}, HAlignCenter | VAlignMiddle);
				s->write(titlefont, tr[menu->sectionLinks()->at(i)->getTitle()], ix + linkSpacing + 36, iy + titlefont->getHeight()/2, VAlignMiddle);
				s->write(font, tr[menu->sectionLinks()->at(i)->getDescription()], ix + linkSpacing + 36, iy + linkHeight - linkSpacing/2, VAlignBottom);
			}
		} else { // CLASSIC
			for (y = 0; y < linkRows; y++) {
				for (x = 0; x < linkCols && i < menu->sectionLinks()->size(); x++, i++) {
					ix = linksRect.x + x * linkWidth  + (x + 1) * linkSpacing;
					iy = linksRect.y + y * linkHeight + (y + 1) * linkSpacing;

					Surface *icon = sc[menu->sectionLinks()->at(i)->getIconPath()];
					if (icon == NULL) icon = sc["skin:icons/generic.png"];
					if (icon->width() > linkWidth || icon->height() > linkHeight)
						icon->softStretch(linkWidth, linkHeight, true, true);

					if (i == (uint32_t)menu->selLinkIndex()) {
						if (iconBGon != NULL && icon->width() <= iconBGon->width() && icon->height() <= iconBGon->height())
							iconBGon->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle, 50);
						else
							s->box(ix + (linkWidth - min(linkWidth, icon->width())) / 2 - 4, iy + (linkHeight - min(linkHeight, icon->height())) / 2 - 4, min(linkWidth, icon->width()) + 8, min(linkHeight, icon->height()) + 8, skinConfColors[COLOR_SELECTION_BG]);

					} else if (iconBGoff != NULL && icon->width() <= iconBGoff->width() && icon->height() <= iconBGoff->height())
						iconBGoff->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle);

					icon->blit(s, {ix + iconPadding/2, iy + iconPadding/2, linkWidth - iconPadding, linkHeight - iconPadding}, HAlignCenter | VAlignMiddle);

					if (skinConfInt["linkLabel"]) s->write(font, tr[menu->sectionLinks()->at(i)->getTitle()], ix + 2 + linkWidth/2, iy + (linkHeight + min(linkHeight, icon->height()))/2, HAlignCenter | VAlignMiddle);
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
		if (skinConfInt["sectionBar"]) {
			int iconTrayShift = 0;

			brightnessIcon = confInt["backlight"] / 20;
			if (brightnessIcon > 4 || iconBrightness[brightnessIcon] == NULL) brightnessIcon = 5;

			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				const int iconWidth = 16, pctWidth = font->getTextWidth("100");
				char buf[32]; int x = 0;

				s->box(bottomBarRect, skinConfColors[COLOR_BOTTOM_BAR_BG]);

				if (menu->selLinkApp() != NULL && !menu->selLinkApp()->getDescription().empty()
				&& SDL_GetTicks() - icon_changed < 300
				) {
					x = iconPadding;
					iconManual->blit(s, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
					x += iconWidth + iconPadding;
					s->write(font, menu->selLinkApp()->getDescription().c_str(), x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
				} else {
					SDL_RemoveTimer(iconChangedTimer); iconChangedTimer = NULL;

					// Volume indicator
					// TODO: use drawButton(s, iconVolume[volumeMode], confInt["globalVolume"], x);
					{ stringstream ss; ss << confInt["globalVolume"] /*<< "%"*/; ss.get(&buf[0], sizeof(buf)); }
					x = iconPadding; // 1 * (iconWidth + 2 * iconPadding) + iconPadding + 1 * pctWidth;
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

					// SD Card indicator
					if (mmcStatus == MMC_INSERT) {
						iconSD->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
						iconTrayShift++;
					}

					// Network indicator
					if (iconInet != NULL) {
						iconInet->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
						iconTrayShift++;
					}

					if (menu->selLink() != NULL) {
						if (menu->selLinkApp() != NULL) {
							if (!menu->selLinkApp()->getManualPath().empty()) {
								// Manual indicator
								iconManual->blit(s, bottomBarRect.w - iconTrayShift * (iconWidth + iconPadding) - iconPadding, bottomBarRect.y + bottomBarRect.h / 2, HAlignRight | VAlignMiddle);
							}

							// CPU indicator
							{ stringstream ss; ss << menu->selLinkApp()->getCPU() << "MHz"; ss.get(&buf[0], sizeof(buf)); }
							x += iconPadding + pctWidth;
							iconCPU->blit(s, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle);
							x += iconWidth + iconPadding;
							s->write(font, buf, x, bottomBarRect.y + bottomBarRect.h / 2, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
						}
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

		if (!menu->sectionLinks()->size()) {
			MessageBox mb(this, tr["This section is empty"]);
			mb.setAutoHide(1);
			mb.setBgAlpha(0);
			mb.exec();
		}

		if (!powerManager->suspendActive && !input.combo()) s->flip();

		bool inputAction = input.update();

		if (input.combo()) {
			skinConfInt["sectionBar"] = ((skinConfInt["sectionBar"] + 1) % 6);
			if (!skinConfInt["sectionBar"]) skinConfInt["sectionBar"]++;
			initMenu();
			MessageBox mb(this,tr["CHEATER! ;)"]);
			SDL_AddTimer(200, input.wakeUp, (void*)false);
			mb.setBgAlpha(0);
			mb.setAutoHide(100);
			mb.exec();
			input[CONFIRM] = false;
		}

		if (!powerManager->suspendActive && !input.combo()) s->flip();

		if (inputCommonActions(inputAction)) continue;

		if (input[CANCEL] || input[SETTINGS] || input[MENU] || input[CONFIRM]) {
			SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;
			SDL_RemoveTimer(iconChangedTimer); iconChangedTimer = NULL;
			icon_changed = section_changed = 0;
		}

		if (input[CONFIRM] && menu->selLink() != NULL) {
			if (confInt["skinBackdrops"] & BD_DIALOG)
				setBackground(bg, currBackdrop);
			else
				setBackground(bg, confStr["wallpaper"]);

			menu->selLink()->run();
		}
		else if (input[CANCEL])		continue;
		else if (input[SETTINGS])	settings();
		else if (input[MENU])		contextMenu();

		// LINK NAVIGATION
		else if (input[LEFT]  && linkCols == 1 && linkRows > 1) menu->pageUp();
		else if (input[RIGHT] && linkCols == 1 && linkRows > 1) menu->pageDown();
		else if (input[LEFT])	menu->linkLeft();
		else if (input[RIGHT])	menu->linkRight();
		else if (input[UP])		menu->linkUp();
		else if (input[DOWN])	menu->linkDown();

		// SECTION
		else if (input[SECTION_PREV]) menu->decSectionIndex();
		else if (input[SECTION_NEXT]) menu->incSectionIndex();

		// SELLINKAPP SELECTED
		else if (input[MANUAL] && menu->selLinkApp() != NULL && !menu->selLinkApp()->getManualPath().empty()) showManual();
		// On Screen Help
		// else if (input[MANUAL]) {
		// 	s->box(10,50,300,162, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		// 	s->rectangle(12,52,296,158, skinConfColors[COLOR_MESSAGE_BOX_BORDER]);
		// 	int line = 60; s->write(font, tr["CONTROLS"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["A: Select"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["B: Cancel"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Y: Show manual"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["L, R: Change section"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Start: Settings"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Select: Menu"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Select+Start: Save screenshot"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Select+L: Adjust volume level"], 20, line);
		// 	line += font->getHeight() + 5; s->write(font, tr["Select+R: Adjust backlight level"], 20, line);
		// 	s->flip();
		// 	bool close = false;
		// 	while (!close) {
		// 		input.update();
		// 		if (input[MODIFIER] || input[CONFIRM] || input[CANCEL]) close = true;
		// 	}
		// }

		if (
			menu->selLinkApp() != NULL && !menu->selLinkApp()->getDescription().empty()
			&& (input[LEFT] || input[RIGHT] || input[LEFT] || input[RIGHT] || input[UP] || input[DOWN] || input[SECTION_PREV] || input[SECTION_NEXT])
		) {
			icon_changed = SDL_GetTicks();
			SDL_RemoveTimer(iconChangedTimer); iconChangedTimer = NULL;
			iconChangedTimer = SDL_AddTimer(1000, input.wakeUp, (void*)false);
		}

		if (skinConfInt["sectionLabel"] && (input[SECTION_PREV] || input[SECTION_NEXT]) ) {
			section_changed = SDL_GetTicks();
			SDL_RemoveTimer(sectionChangedTimer); sectionChangedTimer = NULL;
			sectionChangedTimer = SDL_AddTimer(2000, input.wakeUp, (void*)false);
		}
	}
}

bool GMenu2X::inputCommonActions(bool &inputAction) {
	if (powerManager->suspendActive) {
		// SUSPEND ACTIVE
		while (!(input[POWER] || input[SETTINGS] || input[UDC_CONNECT])) {
			input.update();
		}

		powerManager->doSuspend(0);
		input[WAKE_UP] = true;
		return true;
	}

	if (inputAction) powerManager->resetSuspendTimer();

	uint32_t button_hold = SDL_GetTicks();

	int wasActive = 0;
	while (input[POWER] || input[SETTINGS]) {
		if (input[POWER])
			wasActive = POWER;
		else if (input[SETTINGS])
			wasActive = SETTINGS;

		input.update();

		if ((input[POWER] || input[SETTINGS]) && SDL_GetTicks() - button_hold > 1000)
			poweroffDialog(); // HOLD POWER BUTTON
		// else if (wasActive == POWER)
		// 	powerManager->doSuspend(1);
		else
			continue;
		return true;
	}

	while (input[MENU]) {
		wasActive = MENU;

		input.update();

		if (input[SETTINGS])
			input[SCREENSHOT] = true;
		else if (input[BACKLIGHT_HOTKEY])
			input[BACKLIGHT] = true;
		else if (input[VOLUME_HOTKEY])
			input[VOLUP] = true;
		else if (input[POWER])
			input[UDC_CONNECT] = true;
		else
			continue;
		break;
	}

	input[wasActive] = true;

	if (input[POWER])
		powerManager->doSuspend(1);

	else if (input[SCREENSHOT]) {
		if (!saveScreenshot()) { ERROR("Can't save screenshot"); return true; }
		MessageBox mb(this, tr["Screenshot saved"]);
		mb.setAutoHide(1000);
		mb.exec();

	} else if (input[VOLUP]) {
		setVolume(confInt["globalVolume"], true);

	} else if (input[BACKLIGHT]) {
		setBacklight(confInt["backlight"], true);

	} else if (input[UDC_CONNECT]) {
		powerManager->setPowerTimeout(0);
		batteryIcon = 6;
		udcDialog(UDC_CONNECT);

	} else if (input[UDC_REMOVE]) {
		udcDialog(UDC_REMOVE);
		iconInet = NULL;
		batteryIcon = getBatteryLevel();
		powerManager->setPowerTimeout(confInt["powerTimeout"]);

	} else if (input[TV_CONNECT]) {
		tvOutDialog();

	} else if (input[TV_REMOVE]) {
		tvOutDialog(TV_OFF);

	} else if (input[JOYSTICK_CONNECT]) {
		input.initJoysticks();
	// } else if (input[PHONES_CONNECT]) {
	// 	// tvOutDialog(TV_OFF);
	// 	WARNING("volume mode changed");
	// 	return true;

	} else {
		return false;
	}
	return true;
}

void GMenu2X::cls(Surface *s, bool flip) {
	if (s == NULL) {
		s = this->s;
	}

	s->box((SDL_Rect){0, 0, s->width(), s->height()}, (RGBAColor){0, 0, 0, 255});

	if (flip) {
		s->flip();
	}
}

void GMenu2X::setBackground(Surface *bg, string wallpaper) {
	if (!sc.exists(wallpaper)) { // search and scale background
		if (wallpaper.empty() || sc[wallpaper] == NULL) {
			DEBUG("Searching wallpaper");
			FileLister fl("skins/Default/wallpapers", false, true);
			fl.setFilter(".png,.jpg,.jpeg,.bmp");
			fl.browse();
			wallpaper = "skins/Default/wallpapers/" + fl.getFiles()[0];
		}
		if (sc[wallpaper] == NULL) return;
		if (confStr["bgscale"] == "Stretch") sc[wallpaper]->softStretch(this->w, this->h, false, true);
		else if (confStr["bgscale"] == "Aspect") sc[wallpaper]->softStretch(this->w, this->h, true, false);
		else if (confStr["bgscale"] == "Crop") sc[wallpaper]->softStretch(this->w, this->h, true, true);
	}

	cls(bg, false);
	sc[wallpaper]->blit(bg, (this->w - sc[wallpaper]->width()) / 2, (this->h - sc[wallpaper]->height()) / 2);
}

void GMenu2X::initLayout() {
	// LINKS rect
	linksRect = (SDL_Rect){0, 0, this->w, this->h};
	sectionBarRect = (SDL_Rect){0, 0, this->w, this->h};

	if (skinConfInt["sectionBar"]) {
		// x = 0; y = 0;
		if (skinConfInt["sectionBar"] == SB_LEFT || skinConfInt["sectionBar"] == SB_RIGHT) {
			sectionBarRect.x = (skinConfInt["sectionBar"] == SB_RIGHT) * (this->w - skinConfInt["sectionBarSize"]);
			sectionBarRect.w = skinConfInt["sectionBarSize"];
			linksRect.w = this->w - skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_LEFT) {
				linksRect.x = skinConfInt["sectionBarSize"];
			}
		} else {
			sectionBarRect.y = (skinConfInt["sectionBar"] == SB_BOTTOM) * (this->h - skinConfInt["sectionBarSize"]);
			sectionBarRect.h = skinConfInt["sectionBarSize"];
			linksRect.h = this->h - skinConfInt["sectionBarSize"];

			if (skinConfInt["sectionBar"] == SB_TOP || skinConfInt["sectionBar"] == SB_CLASSIC) {
				linksRect.y = skinConfInt["sectionBarSize"];
			}
			if (skinConfInt["sectionBar"] == SB_CLASSIC) {
				linksRect.h -= skinConfInt["bottomBarHeight"];
			}
		}
	}

	listRect = (SDL_Rect){0, skinConfInt["sectionBarSize"], this->w, this->h - skinConfInt["bottomBarHeight"] - skinConfInt["sectionBarSize"]};
	bottomBarRect = (SDL_Rect){0, this->h - skinConfInt["bottomBarHeight"], this->w, skinConfInt["bottomBarHeight"]};

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
		// Add virtual links in the applications section
		if (menu->getSections()[i] == "applications") {
			menu->addActionLink(i, tr["Explorer"], MakeDelegate(this, &GMenu2X::explorer), tr["Browse files and launch apps"], "explorer.png");

#if defined(HW_EXT_SD)
			menu->addActionLink(i, tr["Umount"], MakeDelegate(this, &GMenu2X::umountSdDialog), tr["Umount external media device"], "eject.png");
#endif
		}

		// Add virtual links in the setting section
		else if (menu->getSections()[i] == "settings") {
			menu->addActionLink(i, tr["Settings"], MakeDelegate(this, &GMenu2X::settings), tr["Configure system"], "configure.png");
			menu->addActionLink(i, tr["Skin"], MakeDelegate(this, &GMenu2X::skinMenu), tr["Appearance & skin settings"], "skin.png");
#if defined(TARGET_GP2X)
			if (fwType == "open2x") {
				menu->addActionLink(i, "Open2x", MakeDelegate(this, &GMenu2X::settingsOpen2x), tr["Configure Open2x system settings"], "o2xconfigure.png");
			}
			menu->addActionLink(i, "USB SD", MakeDelegate(this, &GMenu2X::activateSdUsb), tr["Activate USB on SD"], "usb.png");
			if (fwType == "gph" && !f200) {
				menu->addActionLink(i, "USB Nand", MakeDelegate(this, &GMenu2X::activateNandUsb), tr["Activate USB on NAND"], "usb.png");
			}
#endif
			if (file_exists(path + "log.txt"))
				menu->addActionLink(i, tr["Log Viewer"], MakeDelegate(this, &GMenu2X::viewLog), tr["Displays last launched program's output"], "ebook.png");

			menu->addActionLink(i, tr["About"], MakeDelegate(this, &GMenu2X::about), tr["Info about system"], "about.png");
			menu->addActionLink(i, tr["Power"], MakeDelegate(this, &GMenu2X::poweroffDialog), tr["Power menu"], "exit.png");
		}
	}
	menu->setSectionIndex(confInt["section"]);
	menu->setLinkIndex(confInt["link"]);
	// menu->loadIcons();
}

void GMenu2X::settings() {
//G
	// int prevgamma = confInt["gamma"];
	FileLister fl_tr("translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	string lang = tr.lang();
	if (lang == "") lang = "English";

	vector<string> opFactory;
	opFactory.push_back(">>");
	string tmp = ">>";

	SettingsDialog sd(this, ts, tr["Settings"], "skin:icons/configure.png");
	sd.allowCancel = false;

	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenuNX"], &lang, &fl_tr.getFiles()));

	string prevDateTime = getDateTime();
	string newDateTime = prevDateTime;
	sd.addSetting(new MenuSettingDateTime(this, tr["Date & Time"], tr["Set system's date & time"], &newDateTime));
	sd.addSetting(new MenuSettingDir(this, tr["Home path"],	tr["Set as home for launched links"], &confStr["homePath"]));

#if defined(HW_UDC)
	vector<string> usbMode;
	usbMode.push_back("Ask");
	usbMode.push_back("Storage");
	usbMode.push_back("Charger");
	sd.addSetting(new MenuSettingMultiString(this, tr["USB mode"], tr["Define default USB mode"], &confStr["usbMode"], &usbMode));
#endif

	sd.addSetting(new MenuSettingInt(this, tr["Suspend timeout"], tr["Seconds until suspend the device when inactive"], &confInt["backlightTimeout"], 30, 10, 300));
	// sd.addSetting(new MenuSettingInt(this, tr["Power timeout"], tr["Minutes to poweroff system if inactive"], &confInt["powerTimeout"], 10, 1, 300));
	sd.addSetting(new MenuSettingInt(this, tr["Backlight"], tr["Set LCD backlight"], &confInt["backlight"], 70, 1, 100));
	sd.addSetting(new MenuSettingInt(this, tr["Audio volume"], tr["Set the default audio volume"], &confInt["globalVolume"], 60, 0, 100));
	sd.addSetting(new MenuSettingBool(this, tr["Remember selection"], tr["Remember the last selected section, link and file"], &confInt["saveSelection"]));
	sd.addSetting(new MenuSettingBool(this, tr["Output logs"], tr["Logs the link's output to read with Log Viewer"], &confInt["outputLogs"]));
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

		if (prevDateTime != newDateTime) {
			powerManager->setPowerTimeout(0);
			powerManager->setSuspendTimeout(0);
			setDateTime(newDateTime.c_str());
			reinit();
		}
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
			reset_boxart = false,
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
	sd.addSetting(new MenuSettingBool(this, tr["Box art"], tr["Unset link's selector box art path"], &reset_boxart));
	sd.addSetting(new MenuSettingBool(this, tr["CPU speed"], tr["Reset link's custom CPU speed back to default"], &reset_cpu));

	if (sd.exec() && sd.edited() && sd.save) {
		MessageBox mb(this, tr["Changes will be applied to ALL\napps and GMenuNX. Are you sure?"], "skin:icons/exit.png");
		mb.setButton(CANCEL, tr["Cancel"]);
		mb.setButton(MANUAL,  tr["Confirm"]);
		if (mb.exec() != MANUAL) return;

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
				if (reset_boxart) 		menu->selLinkApp()->setSelectorScreens("");
				if (reset_backdrop) 	menu->selLinkApp()->setBackdrop("");
				if (reset_icon || reset_manual || reset_parameter || reset_backdrop || reset_filter || reset_directory || reset_boxart )
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
		reinit();
	}
}

// void GMenu2X::cpuSettings() {
// 	SettingsDialog sd(this, ts, tr["CPU settings"], "skin:icons/configure.png");
// 	sd.addSetting(new MenuSettingInt(this, tr["Default CPU clock"], tr["Set the default working CPU frequency"], &confInt["cpuMenu"], 528, 528, 600, 6));
// 	sd.addSetting(new MenuSettingInt(this, tr["Maximum CPU clock"], tr["Maximum overclock for launching links"], &confInt["cpuMax"], 740, 600, 1200, 6));
// 	// sd.addSetting(new MenuSettingInt(this, tr["Minimum CPU clock"], tr["Minimum underclock used in Suspend mode"], &confInt["cpuMin"], 342, 200, 528, 6));

// 	if (sd.exec() && sd.edited() && sd.save) {
// 		setCPU(confInt["cpuMenu"]);
// 		writeConfig();
// 	}
// }

void GMenu2X::readTmp() {
	lastSelectorElement = -1;
	if (!file_exists("/tmp/gmenu2x.tmp")) return;
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
		// else if (name == "TVOut") TVOut = atoi(value.c_str());
		else if (name == "tvOutPrev") tvOutPrev = atoi(value.c_str());
		else if (name == "udcPrev") udcPrev = atoi(value.c_str());
		else if (name == "currBackdrop") currBackdrop = value;
		else if (name == "explorerLastDir") confStr["explorerLastDir"] = value;
	}
	// if (TVOut > 2) TVOut = 0;
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
		// inf << "TVOut=" << TVOut << endl;
		inf << "currBackdrop=" << currBackdrop << endl;
		if (!confStr["explorerLastDir"].empty()) inf << "explorerLastDir=" << confStr["explorerLastDir"] << endl;
		inf.close();
	}
}

void GMenu2X::readConfig() {
	string conffile = path + "gmenu2x.conf";

	// Defaults *** Sync with default values in writeConfig
	confInt["saveSelection"] = 1;
	confInt["skinBackdrops"] = 0;
	confStr["homePath"] = CARD_ROOT;
	confInt["globalVolume"] = 60;
	confStr["bgscale"] = "Crop";
	confStr["skinFont"] = "Custom";

	confInt["cpuMenu"] = CPU_MENU;
	confInt["cpuMax"] = CPU_MAX;
	confInt["cpuMin"] = CPU_MIN;

	// input.update(false);

	// if (!input[SETTINGS] && file_exists(conffile)) {
	if (file_exists(conffile)) {
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
	if (!confStr["wallpaper"].empty() && !file_exists(confStr["wallpaper"])) confStr["wallpaper"] = "";
	if (confStr["skin"].empty() || !dir_exists("skins/" + confStr["skin"])) confStr["skin"] = "Default";

	evalIntConf(&confInt["backlightTimeout"], 30, 10, 300);
	evalIntConf(&confInt["powerTimeout"], 10, 1, 300);
	evalIntConf(&confInt["outputLogs"], 0, 0, 1 );
	// evalIntConf(&confInt["cpuMax"], 2000, 200, 2000 );
	// evalIntConf(&confInt["cpuMin"], 342, 200, 1200 );
	// evalIntConf(&confInt["cpuMenu"], 528, 200, 1200 );
	evalIntConf(&confInt["globalVolume"], 60, 0, 100 );
	evalIntConf(&confInt["backlight"], 70, 1, 100);
	evalIntConf(&confInt["minBattery"], 3550, 1, 10000);
	evalIntConf(&confInt["maxBattery"], 3720, 1, 10000);

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
				curr->first == "datetime" ||
				curr->first == "explorerLastDir" ||
				curr->first == "defaultDir" ||

				// defaults
				(curr->first == "homePath" && curr->second == CARD_ROOT) ||
				(curr->first == "skin" && curr->second == "Default") ||
				(curr->first == "previewMode" && curr->second == "Miniature") ||
				(curr->first == "skinFont" && curr->second == "Custom") ||
				(curr->first == "usbMode" && curr->second.empty()) ||
				(curr->first == "lang" && curr->second.empty()) ||
				(curr->first == "lang" && curr->second.empty()) ||
				(curr->first == "bgscale" && curr->second.empty()) ||
				(curr->first == "bgscale" && curr->second == "Crop") ||

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
				// (curr->first == "cpuMin" && curr->second == 342) ||
				// (curr->first == "cpuMax" && curr->second == 740) ||
				// (curr->first == "cpuMenu" && curr->second == 528) ||
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

				(curr->first == "previewWidth" && curr->second == 142) ||
				(curr->first == "linkCols" && curr->second == 4) ||
				(curr->first == "linkRows" && curr->second == 4) ||
				(curr->first == "sectionBar" && curr->second == SB_CLASSIC) ||
				(curr->first == "sectionLabel" && curr->second == 1) ||
				(curr->first == "linkLabel" && curr->second == 1) ||
				(curr->first == "showDialogIcon" && curr->second == 1) ||

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

void GMenu2X::setSkin(const string &skin, bool clearSC) {
	confStr["skin"] = skin;

	// Clear previous skin settings
	skinConfStr.clear();
	skinConfInt.clear();

	// Defaults *** Sync with default values in writeConfig
	skinConfInt["sectionBar"] = SB_CLASSIC;
	skinConfInt["sectionLabel"] = 1;
	skinConfInt["linkLabel"] = 1;
	skinConfInt["showDialogIcon"] = 1;

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
	skinConfColors[COLOR_PREVIEW_BG] = (RGBAColor){253,1,252,0};;
	skinConfColors[COLOR_MESSAGE_BOX_BG] = (RGBAColor){255,255,255,255};
	skinConfColors[COLOR_MESSAGE_BOX_BORDER] = (RGBAColor){80,80,80,255};
	skinConfColors[COLOR_MESSAGE_BOX_SELECTION] = (RGBAColor){160,160,160,255};
	skinConfColors[COLOR_FONT] = (RGBAColor){255,255,255,255};
	skinConfColors[COLOR_FONT_OUTLINE] = (RGBAColor){0,0,0,200};
	skinConfColors[COLOR_FONT_ALT] = (RGBAColor){253,1,252,0};
	skinConfColors[COLOR_FONT_ALT_OUTLINE] = (RGBAColor){253,1,252,0};

	// load skin settings
	string skinconfname = "skins/" + skin + "/skin.conf";
	if (file_exists(skinconfname)) {
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
		}
	}

	// (poor) HACK: ensure some colors have a default value
	if (skinConfColors[COLOR_FONT_ALT].r == 253 && skinConfColors[COLOR_FONT_ALT].g == 1 && skinConfColors[COLOR_FONT_ALT].b == 252 && skinConfColors[COLOR_FONT_ALT].a == 0) {
		skinConfColors[COLOR_FONT_ALT] = skinConfColors[COLOR_FONT];
	}
	if (skinConfColors[COLOR_FONT_ALT_OUTLINE].r == 253 && skinConfColors[COLOR_FONT_ALT_OUTLINE].g == 1 && skinConfColors[COLOR_FONT_ALT_OUTLINE].b == 252 && skinConfColors[COLOR_FONT_ALT_OUTLINE].a == 0) {
		skinConfColors[COLOR_FONT_ALT_OUTLINE] = skinConfColors[COLOR_FONT_OUTLINE];
	}
	if (skinConfColors[COLOR_PREVIEW_BG].r == 253 && skinConfColors[COLOR_PREVIEW_BG].g == 1 && skinConfColors[COLOR_PREVIEW_BG].b == 252 && skinConfColors[COLOR_PREVIEW_BG].a == 0) {
		skinConfColors[COLOR_PREVIEW_BG] = skinConfColors[COLOR_TOP_BAR_BG];
		skinConfColors[COLOR_PREVIEW_BG].a == 170;
	}

	// prevents breaking current skin until they are updated
	if (!skinConfInt["fontSizeTitle"] && skinConfInt["titleFontSize"] > 0) skinConfInt["fontSizeTitle"] = skinConfInt["titleFontSize"];

	evalIntConf(&skinConfInt["sectionBarSize"], 40, 1, this->w);
	evalIntConf(&skinConfInt["bottomBarHeight"], 16, 1, this->h);
	evalIntConf(&skinConfInt["previewWidth"], 142, 1, this->w);
	evalIntConf(&skinConfInt["fontSize"], 12, 6, 60);
	evalIntConf(&skinConfInt["fontSizeTitle"], 20, 6, 60);
	evalIntConf(&skinConfInt["sectionBar"], SB_CLASSIC, 0, 5);
	evalIntConf(&skinConfInt["linkCols"], 4, 1, 8);
	evalIntConf(&skinConfInt["linkRows"], 4, 1, 8);

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

	int linkColsPrev = skinConfInt["linkCols"];
	int linkRowsPrev = skinConfInt["linkRows"];

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

	vector<string> previewMode;
	previewMode.push_back("Miniature");
	previewMode.push_back("Backdrop");

	vector<string> wallpapers;
	string wpPath = confStr["wallpaper"];
	confStr["tmp_wallpaper"] = "";

	do {
		if (prevSkin != confStr["skin"] || skinFontPrev != confStr["skinFont"]) {

			prevSkin = confStr["skin"];
			skinFontPrev = confStr["skinFont"];

			setSkin(confStr["skin"], false);
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
		if (!file_exists(wpPath)) wpPath = "skins/Default/wallpapers/" + confStr["tmp_wallpaper"];
		if (!file_exists(wpPath)) wpPath = "skins/" + confStr["skin"] + "/wallpapers/" + wallpapers.at(0);
		if (!file_exists(wpPath)) wpPath = "skins/Default/wallpapers/" + wallpapers.at(0);

		setBackground(bg, wpPath);

		SettingsDialog sd(this, ts, tr["Skin"], "skin:icons/skin.png");
		sd.selected = selected;
		sd.allowCancel = false;
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenuNX"], &confStr["skin"], &fl_sk.getDirectories(), MakeDelegate(this, &GMenu2X::onChangeSkin)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Wallpaper"], tr["Select an image to use as a wallpaper"], &confStr["tmp_wallpaper"], &wallpapers, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::changeWallpaper)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Preview mode"], tr["How to show image preview and game art"], &confStr["previewMode"], &previewMode));
		sd.addSetting(new MenuSettingMultiString(this, tr["Background"], tr["How to scale wallpaper, backdrops and game art"], &confStr["bgscale"], &bgScale));
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin colors"], tr["Customize skin colors"], &tmp, &wpLabel, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::skinColors)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin backdrops"], tr["Automatic load backdrops from skin pack"], &skinBackdrops, &bdStr));
		sd.addSetting(new MenuSettingMultiString(this, tr["Font face"], tr["Override the skin font face"], &confStr["skinFont"], &skinFont, MakeDelegate(this, &GMenu2X::onChangeSkin)));
		sd.addSetting(new MenuSettingInt(this, tr["Font size"], tr["Size of text font"], &skinConfInt["fontSize"], 12, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Title font size"], tr["Size of title's text font"], &skinConfInt["fontSizeTitle"], 20, 6, 60));
		sd.addSetting(new MenuSettingMultiString(this, tr["Section bar layout"], tr["Set the layout and position of the Section Bar"], &sectionBar, &sbStr));
		sd.addSetting(new MenuSettingInt(this, tr["Section bar size"], tr["Size of section and top bar"], &skinConfInt["sectionBarSize"], 40, 1, this->w));
		sd.addSetting(new MenuSettingInt(this, tr["Bottom bar height"], tr["Height of bottom bar"], &skinConfInt["bottomBarHeight"], 16, 1, this->h));
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

	if (
		bdPrev != confInt["skinBackdrops"] ||
		initSkin != confStr["skin"] ||
		bgScalePrev != confStr["bgscale"] ||
		linkColsPrev != skinConfInt["linkCols"] ||
		linkRowsPrev != skinConfInt["linkRows"] ||
		sbPrev != skinConfInt["sectionBar"]
	) reinit();
}

void GMenu2X::skinColors() {
	bool save = false;
	do {
		setSkin(confStr["skin"], false);

		SettingsDialog sd(this, ts, tr["Skin Colors"], "skin:icons/skin.png");
		sd.allowCancel = false;
		sd.addSetting(new MenuSettingRGBA(this, tr["Top/Section Bar"], tr["Color of the top and section bar"], &skinConfColors[COLOR_TOP_BAR_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["List Body"], tr["Color of the list body"], &skinConfColors[COLOR_LIST_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Bottom Bar"], tr["Color of the bottom bar"], &skinConfColors[COLOR_BOTTOM_BAR_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Selection"], tr["Color of the selection and other interface details"], &skinConfColors[COLOR_SELECTION_BG]));
		sd.addSetting(new MenuSettingRGBA(this, tr["Box Art"], tr["Color of the box art background"], &skinConfColors[COLOR_PREVIEW_BG]));
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

	// float battlevel = getBatteryStatus();
	// { stringstream ss; ss.precision(2); ss << battlevel/1000; ss >> buf; }
	// temp += tr["Battery: "] + ((battlevel < 0 || battlevel > 10000) ? tr["Charging"] : buf + " V") + "\n";
	// temp += tr["Uptime: "] + ms2hms(SDL_GetTicks()) + "\n";

	// temp += "----\n";

	TextDialog td(this, "GMenuNX", tr["Info about GMenuNX"], "skin:icons/about.png");

	td.appendText(temp);
	td.appendFile("about.txt");
	td.exec();
}

void GMenu2X::viewLog() {
	string logfile = path + "log.txt";
	if (!file_exists(logfile)) return;

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
	string linkExec = menu->selLinkApp()->getExec();

	if (linkManual == "") return;

	TextDialog td(this, linkTitle, linkDescription, linkIcon); //, linkBackdrop);

	if (file_exists(linkManual)) {
		string ext = linkManual.substr(linkManual.size() - 4, 4);
		if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == "jpeg") {
			ImageViewerDialog im(this, linkTitle, linkDescription, linkIcon, linkManual);
			im.exec();
			return;
		}

		td.appendFile(linkManual);
#if defined(OPK_SUPPORT)
	} else if (file_ext(linkExec, true) == ".opk") {
		void *buf; size_t len;
		struct OPK *opk = opk_open(linkExec.c_str());

		if (!opk) {
			ERROR("Unable to open OPK");
			return;
		}

		if (opk_extract_file(opk, linkManual.c_str(), &buf, &len) < 0) {
			ERROR("Unable to extract file: %s\n", linkManual.c_str());
			return;
		}

		opk_close(opk);

		char* str = strdup((const char*)buf);
		str[len] = 0;

		td.appendText(str);
#endif // OPK_SUPPORT
	} else {
		return;
	}

	td.exec();
}

void GMenu2X::explorer() {
	BrowseDialog bd(this, tr["Explorer"], tr["Select a file or application"]);
	bd.showDirectories = true;
	bd.showFiles = true;

	if (confInt["saveSelection"]) bd.setPath(confStr["explorerLastDir"]);

	while (bd.exec()) {
		string ext = bd.getExt(bd.selected);
		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp") {
			ImageViewerDialog im(this, tr["Image viewer"], bd.getFile(bd.selected), "icons/explorer.png", bd.getFilePath(bd.selected));
			im.exec();
		} else if (ext == ".txt" || ext == ".conf" || ext == ".me" || ext == ".md" || ext == ".xml" || ext == ".log" || ext == ".ini") {
			TextDialog td(this, tr["Text viewer"], bd.getFile(bd.selected), "skin:icons/ebook.png");
			td.appendFile(bd.getFilePath(bd.selected));
			td.exec();
#if defined(IPK_SUPPORT)
		} else if (ext == ".ipk" && file_exists("/usr/bin/opkg")) {
			string cmd = "opkg install --force-reinstall --force-overwrite ";
			input.update(false);
			if (input[MANUAL]) cmd += "--force-downgrade ";
			TerminalDialog td(this, tr["Package installer"], "opkg install " + bd.getFileName(bd.selected), "skin:icons/configure.png");
			td.exec(cmd + cmdclean(bd.getFilePath(bd.selected)));
			initMenu();
#endif
#if defined(OPK_SUPPORT)
		} else if (ext == ".opk" && file_exists("/usr/bin/retrofw")) {
			string cmd = "retrofw opk install ";
			TerminalDialog td(this, tr["Package installer"], "opk install " + bd.getFileName(bd.selected), "skin:icons/terminal.png");
			td.exec(cmd + cmdclean(bd.getFilePath(bd.selected)));
			initMenu();
#endif
		} else if (ext == ".sh") {
			TerminalDialog td(this, tr["Terminal"], "sh " + cmdclean(bd.getFileName(bd.selected)), "skin:icons/terminal.png");
			td.exec(bd.getFilePath(bd.selected));
		} else if (ext == ".zip" && file_exists("/usr/bin/unzip") && (bd.getFile(bd.selected).rfind("gmenu2x-skin-", 0) == 0) || (bd.getFile(bd.selected).rfind("gmenunx-skin-", 0) == 0)) {
			TerminalDialog td(this, tr["Skin installer"], bd.getFileName(bd.selected), "skin:icons/skin.png");
			td.exec("rm -rf /tmp/skins; mkdir -p /tmp/skins; unzip -o " + cmdclean(bd.getFilePath(bd.selected)) + " -d /tmp/skins; cp -rf /tmp/skins/ " + getExePath() + "/skins 2> /dev/null; rm -rf /tmp/skins; sync");
		} else if (ext == ".zip" && file_exists("/usr/bin/unzip")) {
			TerminalDialog td(this, tr["Zip content"], bd.getFileName(bd.selected), "skin:icons/terminal.png");
			td.exec("unzip -l " + cmdclean(bd.getFilePath(bd.selected)));
		} else {
			if (confInt["saveSelection"] && (confInt["section"] != menu->selSectionIndex() || confInt["link"] != menu->selLinkIndex()))
				writeConfig();

			string command = cmdclean(bd.getFilePath(bd.selected));
			chdir(bd.getPath().c_str());
			quit();
			setCPU(confInt["cpuMenu"]);

			if (confInt["outputLogs"]) {
				if (file_exists("/usr/bin/gdb")) command = "gdb -batch -ex \"run\" -ex \"bt\" --args " + command;
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

	confStr["explorerLastDir"] = bd.getPath();
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
	} while (file_exists(fname));
	x = SDL_SaveBMP(s->raw, fname.c_str());
	sync();
	ledOff();
	return x == 0;
}

void GMenu2X::reinit(bool showDialog) {
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
#if !defined(TARGET_BITTBOY)
	MessageBox mb(this, tr["Poweroff or reboot the device?"], "skin:icons/exit.png");
	mb.setButton(SECTION_NEXT, tr["Reboot"]);
#else
	MessageBox mb(this, tr["Poweroff the device?"], "skin:icons/exit.png");
#endif
	mb.setButton(CONFIRM, tr["Poweroff"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	int response = mb.exec();
	writeConfig();
	if (response == CONFIRM) {
		MessageBox mb(this, tr["Poweroff"]);
		mb.setAutoHide(1);
		mb.exec();
		#if !defined(TARGET_PC)
			system("sync; mount -o remount,ro $HOME; poweroff");
		#endif
		setVolume(0);
		setBacklight(0);
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(this, tr["Rebooting"]);
		mb.setAutoHide(1);
		mb.exec();
		#if !defined(TARGET_PC)
			system("sync; mount -o remount,ro $HOME; reboot");
		#endif
		setVolume(0);
		SDL_Delay(2000);
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
	vector<MenuOption> options;
	if (menu->selLinkApp() != NULL) {
		options.push_back((MenuOption){tr["Edit"] + " " + menu->selLink()->getTitle().c_str(), MakeDelegate(this, &GMenu2X::editLink)});

		#if defined(OPK_SUPPORT) || defined(IPK_SUPPORT)
			string package = "";
			if (file_ext(menu->selLinkApp()->getExec(), true) == ".opk") {
				package = base_name(menu->selLinkApp()->getExec());
				options.push_back((MenuOption){tr["Uninstall"] + " " + package.c_str(), MakeDelegate(this, &GMenu2X::opkUninstall)});
			} else {
				package = ipkName(menu->selLinkApp()->getFile());
				if (!package.empty()) {
					options.push_back((MenuOption){tr["Uninstall"] + " " + package.c_str(), MakeDelegate(this, &GMenu2X::ipkUninstall)});
				}
			}
			if (package.empty())
		#endif
		options.push_back((MenuOption){tr["Delete"] + " " + menu->selLink()->getTitle().c_str(), MakeDelegate(this, &GMenu2X::deleteLink)});
	}

	options.push_back((MenuOption){tr["Add link"], 			MakeDelegate(this, &GMenu2X::addLink)});
	options.push_back((MenuOption){tr["Add section"],		MakeDelegate(this, &GMenu2X::addSection)});
	options.push_back((MenuOption){tr["Rename section"],	MakeDelegate(this, &GMenu2X::renameSection)});
	options.push_back((MenuOption){tr["Delete section"],	MakeDelegate(this, &GMenu2X::deleteSection)});

#if defined(OPK_SUPPORT)
#if !defined(TARGET_PC)
	if (file_exists("/usr/bin/retrofw"))
#endif
	{
		options.push_back((MenuOption){tr["Update OPKs"], MakeDelegate(this, &GMenu2X::opkScanner)});
	}
#endif
	MessageBox mb(this, options);
}

void GMenu2X::addLink() {
	BrowseDialog bd(this, tr["Add link"], tr["Select an application"]);
	bd.showDirectories = true;
	bd.showFiles = true;
	bd.setFilter(".dge,.gpu,.gpe,.sh,.bin,.elf,");
	if (bd.exec()) {
		ledOn();
		if (menu->addLink(bd.getFilePath(bd.selected))) {
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
	string dialogTitle = tr["Edit"] + " " + linkTitle;
	string dialogIcon = menu->selLinkApp()->getIconPath();
	string linkDir = dir_name(linkExec);

	vector<string> scaleMode;
	// scaleMode.push_back("Crop");
	scaleMode.push_back("Stretch");
	scaleMode.push_back("Aspect");
	scaleMode.push_back("Original");
	string linkScaleMode = scaleMode[menu->selLinkApp()->getScaleMode()];

	SettingsDialog sd(this, ts, dialogTitle, dialogIcon);
	sd.addSetting(new MenuSettingFile(			this, tr["Executable"],		tr["Application this link points to"], &linkExec, ".dge,.gpu,.gpe,.sh,.bin,.opk,.elf,", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Title"],			tr["Link title"], &linkTitle, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Description"],	tr["Link description"], &linkDescription, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingMultiString(	this, tr["Section"],		tr["The section this link belongs to"], &newSection, &menu->getSections()));
	sd.addSetting(new MenuSettingImage(			this, tr["Icon"],			tr["Select a custom icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg,.gif", linkExec, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingInt(			this, tr["CPU Clock"],		tr["CPU clock frequency when launching this link"], &linkClock, confInt["cpuMenu"], confInt["cpuMenu"], confInt["cpuMax"], CPU_STEP));
	sd.addSetting(new MenuSettingString(		this, tr["Parameters"],		tr["Command line arguments to pass to the application"], &linkParams, dialogTitle, dialogIcon));

	#if defined(HW_SCALER)
		#if !defined(TARGET_PC) && defined(HW_SCALER)
			if (file_exists("/proc/jz/ipu_ratio"))
		#endif
		{
			sd.addSetting(new MenuSettingMultiString(this, tr["Scale Mode"],		tr["Hardware scaling mode"], &linkScaleMode, &scaleMode));
		}
	#endif

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
		if (file_exists(ginge_prep))
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
			if (linkSelDir.empty()) linkSelDir = confStr["homePath"];
		} else if (confInt["saveSelection"]) {
			linkSelDir = "";
		}

		int scalemode = 0;
		if (linkScaleMode == "Aspect") scalemode = 1;
		else if (linkScaleMode == "Original") scalemode = 2;
		menu->selLinkApp()->setScaleMode(scalemode);

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
			while (file_exists(newFileName)) {
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
		MessageBox mb(this, tr["Delete"] + " " + menu->selLink()->getTitle().c_str() + "\n" + tr["Are you sure?"], menu->selLink()->getIconPath());
		mb.setButton(MANUAL, tr["Yes"]);
		mb.setButton(CANCEL,  tr["No"]);
		if (mb.exec() != MANUAL) return;

		ledOn();
		menu->deleteSelectedLink();
		sync();
		ledOff();
	}
}

void GMenu2X::addSection() {
	InputDialog id(this, /*ts,*/ tr["Insert a name for the new section"], "", tr["Add section"], "skin:icons/section.png");
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
	InputDialog id(this, /*ts,*/ tr["Insert a new name for this section"], menu->selSection(), tr["Rename section"], menu->getSectionIcon(menu->selSectionIndex()));
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

					if (!file_exists(newicon)) {
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
	MessageBox mb(this, tr["Delete section"] + " '" +  menu->selSectionName() + "'\n" + tr["THIS CAN'T BE UNDONE"] + "\n" + tr["Are you sure?"], menu->getSectionIcon(menu->selSectionIndex()));
	mb.setButton(MANUAL, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() != MANUAL) return;
	ledOn();
	if (rmtree(path + "sections/" + menu->selSection())) {
		menu->deleteSelectedSection();
		sync();
	}
	ledOff();
}

#if defined(OPK_SUPPORT)
void GMenu2X::opkScanner() {
	TerminalDialog td(this, tr["Update OPK links"], "opk update", "skin:icons/terminal.png");
	td.exec("retrofw opk update");
	initMenu();
}

void GMenu2X::opkUninstall() {
	if (menu->selLinkApp() != NULL) {
		string package = base_name(menu->selLinkApp()->getExec());
		MessageBox mb(this, tr["Uninstall"] + " " + package.c_str() + "\n" + tr["THIS CAN'T BE UNDONE"] + "\n" + tr["Are you sure?"], menu->selLink()->getIconPath());
		mb.setButton(MANUAL, tr["Yes"]);
		mb.setButton(CANCEL,  tr["No"]);
		if (mb.exec() != MANUAL) return;

		package = menu->selLinkApp()->getExec();
		menu->deleteSelectedLink();
		unlink(package.c_str());
		sync();
	}
}
#endif

#if defined(IPK_SUPPORT)
string GMenu2X::ipkName(string cmd) {
	if (!file_exists("/usr/bin/opkg"))
		return "";
	char package[128] = {0,0,0,0,0,0,0,0};
	cmd = "opkg search \"*" + base_name(cmd) + "\" | cut -f1 -d' '";
	FILE *fp = popen(cmd.c_str(), "r");
	if (fp == NULL)
		return "";

	fgets(package, sizeof(package) - 1, fp);
	pclose(fp);

	if (package == NULL)
		return "";

	return trim((string)package);
}

void GMenu2X::ipkUninstall() {
	if (menu->selLinkApp() != NULL) {
		string package = ipkName(menu->selLinkApp()->getFile());
		if (!package.empty()) {
			MessageBox mb(this, tr["Uninstall"] + " " + package.c_str() + ".ipk\n" + tr["THIS CAN'T BE UNDONE"] + "\n" + tr["Are you sure?"], menu->selLink()->getIconPath());
			mb.setButton(MANUAL, tr["Yes"]);
			mb.setButton(CANCEL,  tr["No"]);
			if (mb.exec() != MANUAL) return;

			TerminalDialog td(this, tr["Uninstall package"], "opkg remove " + package, "skin:icons/configure.png");
			td.exec("opkg remove " + package);
			initMenu();
			menu->deleteSelectedLink();
			sync();
		};
	}
}
#endif

void GMenu2X::setInputSpeed() {
	input.setInterval(180);
	input.setInterval(1000, SETTINGS);
	input.setInterval(100, MENU);
	input.setInterval(1000, CONFIRM);
	input.setInterval(1000, POWER);
	input.setInterval(1000, SECTION_PREV);
	input.setInterval(1000, SECTION_NEXT);
	// input.setInterval(30,  VOLDOWN);
	// input.setInterval(30,  VOLUP);
	// input.setInterval(300, CANCEL);
	// input.setInterval(300, MANUAL);
	// input.setInterval(100, INC);
	// input.setInterval(100, DEC);
	// input.setInterval(500, PAGEUP);
	// input.setInterval(500, PAGEDOWN);
	// input.setInterval(200, BACKLIGHT);
}

uint16_t GMenu2X::getBatteryLevel() {
	int32_t val = getBatteryStatus();

	val = constrain(val, 0, 5000);

	bool needWriteConfig = false;
	int32_t max = confInt["maxBattery"];
	int32_t min = confInt["minBattery"];

	if (val > max) {
		needWriteConfig = true;
		max = confInt["maxBattery"] = val;
	}
	if (val < min) {
		needWriteConfig = true;
		min = confInt["minBattery"] = val;
	}

	if (needWriteConfig)
		writeConfig();

	if (max == min)
		return 0;

	return 5 - 5 * (max - val) / (max - min);
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
	if (y < 0) y = this->h + y;
	btn->setPosition(x, y);
	btn->paint();
}

int GMenu2X::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = this->h + y;
	Surface *icon = sc.skinRes("imgs/buttons/" + btn + ".png");
	if (icon != NULL) {
		icon->blit(s, x, y, HAlignLeft | VAlignMiddle);
		x += 19;
		if (!text.empty()) {
			s->write(font, text, x, y, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
			x += font->getTextWidth(text);
		}
	}
	return x + 6;
}

int GMenu2X::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = this->h + y;
	Surface *icon = sc.skinRes("imgs/buttons/" + btn + ".png");
	if (icon != NULL) {
		if (!text.empty()) {
			x -= font->getTextWidth(text);
			s->write(font, text, x, y, HAlignLeft | VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
		}
		x -= 19;
		icon->blit(s, x, y, HAlignLeft | VAlignMiddle);
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

	if (bs < 8) bs = 8;

	by = scrollRect.y + 3 + by;
	if (by + bs > scrollRect.y + scrollRect.h - 4)
		by = scrollRect.y + scrollRect.h - 4 - bs;

	s->rectangle(scrollRect.x + scrollRect.w - 4, by, 4, bs, skinConfColors[COLOR_LIST_BG]);
	s->box(scrollRect.x + scrollRect.w - 3, by + 1, 2, bs - 2, skinConfColors[COLOR_SELECTION_BG]);
}

void GMenu2X::drawSlider(int val, int min, int max, Surface &icon, Surface &bg) {
	SDL_Rect progress = {52, 32, this->w - 84, 8};
	SDL_Rect box = {20, 20, this->w - 40, 32};

	val = constrain(val, min, max);

	bg.blit(s,0,0);
	s->box(box, skinConfColors[COLOR_MESSAGE_BOX_BG]);
	s->rectangle(box.x + 2, box.y + 2, box.w - 4, box.h - 4, skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

	icon.blit(s, 28, 28);

	s->box(progress, skinConfColors[COLOR_MESSAGE_BOX_BG]);
	s->box(progress.x + 1, progress.y + 1, val * (progress.w - 3) / max + 1, progress.h - 2, skinConfColors[COLOR_MESSAGE_BOX_SELECTION]);
	s->flip();
}
