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
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <signal.h>

#include <sys/statvfs.h>
#include <errno.h>

// #include <sys/fcntl.h> //for battery

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// for soundcard
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "linkapp.h"
#include "linkaction.h"
#include "menu.h"
#include "fonthelper.h"
#include "surface.h"
#include "filedialog.h"
#include "gmenu2x.h"
#include "filelister.h"

#include "iconbutton.h"
#include "messagebox.h"
#include "inputdialog.h"
#include "settingsdialog.h"
#include "wallpaperdialog.h"
#include "textdialog.h"
#include "menusettingint.h"
#include "menusettingbool.h"
#include "menusettingrgba.h"
#include "menusettingstring.h"
#include "menusettingmultistring.h"
#include "menusettingfile.h"
#include "menusettingimage.h"
#include "menusettingdir.h"

#include "debug.h"

#include <sys/mman.h>

const char *CARD_ROOT = "/"; //Note: Add a trailing /!
const int CARD_ROOT_LEN = 1;

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
	for (unsigned int i = 0; i < NUM_COLORS; i++) {
		if (strcmp(colorNames[i], name.c_str()) == 0) {
			return (enum color)i;
		}
	}
	return (enum color)-1;
}

static const char *colorToString(enum color c) {
	return colorNames[c];
}

static void quit_all(int err) {
	delete app;
	exit(err);
}

int main(int /*argc*/, char * /*argv*/[]) {
	INFO("GMenu2X starting: If you read this message in the logs, check http://mtorromeo.github.com/gmenu2x/troubleshooting.html for a solution");

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);


	app = new GMenu2X();
	DEBUG("Starting main()");
	app->main();

	return 0;
}

void GMenu2X::gp2x_init() {
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO) || (TARGET_RS97)
	memdev = open("/dev/mem", O_RDWR);
	if (memdev < 0){
		WARNING("Could not open /dev/mem");
	}
#endif

	if (memdev > 0) {
#ifdef TARGET_GP2X
		memregs = (unsigned short*)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);
		MEM_REG=&memregs[0];
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
		memregs = (unsigned short*)mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);
#elif defined(TARGET_RS97)
		memregs = (unsigned long*)mmap(0, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, memdev, 0x10000000);
#endif
		if (memregs == MAP_FAILED) {
			ERROR("Could not mmap hardware registers!");
			close(memdev);
		}
	}

#if defined(TARGET_GP2X)
	batteryHandle = open(f200 ? "/dev/mmsp2adc" : "/dev/batt", O_RDONLY);
	if (f200) {
		//if wm97xx fails to open, set f200 to false to prevent any further access to the touchscreen
		f200 = ts.init();
	}
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	/* get access to battery device */
	batteryHandle = open("/dev/pollux_batt", O_RDONLY);
#endif
	INFO("System Init Done!");
}

void GMenu2X::gp2x_deinit() {
#ifdef TARGET_GP2X
	if (memdev > 0) {
		memregs[0x28DA>>1]=0x4AB;
		memregs[0x290C>>1]=640;
	}
	if (f200) ts.deinit();
#endif

	if (memdev > 0) {
		memregs = NULL;
		close(memdev);
	}
	if (batteryHandle!=0) close(batteryHandle);
}

void GMenu2X::gp2x_tvout_on(bool pal) {
#ifdef TARGET_GP2X
	if (memdev!=0) {
		/*Ioctl_Dummy_t *msg;
		#define FBMMSP2CTRL 0x4619
		int TVHandle = ioctl(SDL_videofd, FBMMSP2CTRL, msg);*/
		if (cx25874!=0) gp2x_tvout_off();
		//if tv-out is enabled without cx25874 open, stop
		//if (memregs[0x2800>>1]&0x100) return;
		cx25874 = open("/dev/cx25874",O_RDWR);
		ioctl(cx25874, _IOW('v', 0x02, unsigned char), pal ? 4 : 3);
		memregs[0x2906>>1]=512;
		memregs[0x28E4>>1]=memregs[0x290C>>1];
		memregs[0x28E8>>1]=239;
	}
#endif
}

void GMenu2X::gp2x_tvout_off() {
#ifdef TARGET_GP2X
	if (memdev!=0) {
		close(cx25874);
		cx25874 = 0;
		memregs[0x2906>>1]=1024;
	}
#endif
}

GMenu2X::GMenu2X() {
	//Detect firmware version and type
	if (fileExists("/etc/open2x")) {
		fwType = "open2x";
		fwVersion = "";
	} else {
		fwType = "gph";
		fwVersion = "";
	}
#ifdef TARGET_GP2X
	f200 = fileExists("/dev/touchscreen/wm97xx");
#elif defined(TARGET_RS97)
	fwType = "rs97";
#else
	f200 = true;
#endif

	//open2x
	savedVolumeMode = 0;
	volumeMode = VOLUME_MODE_NORMAL;
	volumeScalerNormal = VOLUME_SCALER_NORMAL;
	volumeScalerPhones = VOLUME_SCALER_PHONES;

	backlightStep = 10;

	o2x_usb_net_on_boot = false;
	o2x_usb_net_ip = "";
	o2x_ftp_on_boot = false;
	o2x_telnet_on_boot = false;
	o2x_gp2xjoy_on_boot = false;
	o2x_usb_host_on_boot = false;
	o2x_usb_hid_on_boot = false;
	o2x_usb_storage_on_boot = false;

	usbnet = samba = inet = web = false;
	useSelectionPng = false;

	//load config data
	readConfig();
	if (fwType=="open2x") {
		readConfigOpen2x();
		//	VOLUME MODIFIER
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones);	break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
	}
#if !defined(TARGET_GP2x)
	readCommonIni();
#endif

	halfX = resX/2;
	halfY = resY/2;
	// bottomBarIconY = resY-18;

	path = "";
	getExePath();

#if defined(TARGET_GP2X)
	cx25874 = 0;
#endif
	batteryHandle = 0;
	memdev = 0;
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO) || defined(TARGET_RS97)
	gp2x_init();
#endif

#ifdef TARGET_GP2X
	//Fix tv-out
	if (memdev > 0) {
		if (memregs[0x2800>>1]&0x100) {
			memregs[0x2906>>1]=512;
			//memregs[0x290C>>1]=640;
			memregs[0x28E4>>1]=memregs[0x290C>>1];
		}
		memregs[0x28E8>>1]=239;
	}
#endif

#if !defined(TARGET_PC)
	setenv("SDL_NOMOUSE", "1", 1);
#endif
	//Screen
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK)<0 ) {
		ERROR("Could not initialize SDL: %s", SDL_GetError());
		quit();
	}

	s = new Surface();
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	{
		//I'm forced to use SW surfaces since with HW there are issuse with changing the clock frequency
		SDL_Surface *dbl = SDL_SetVideoMode(resX, resY, confInt["videoBpp"], SDL_SWSURFACE);
		s->enableVirtualDoubleBuffer(dbl);
		SDL_ShowCursor(0);
	}
#elif defined(TARGET_RS97)
	SDL_ShowCursor(0);
	s->ScreenSurface = SDL_SetVideoMode(320, 480, confInt["videoBpp"], SDL_HWSURFACE/*|SDL_DOUBLEBUF*/);
	s->raw = SDL_CreateRGBSurface(SDL_SWSURFACE, resX, resY, 16, 0, 0, 0, 0);
#else
	s->raw = SDL_SetVideoMode(resX, resY, confInt["videoBpp"], SDL_HWSURFACE|SDL_DOUBLEBUF);
#endif

	bg = NULL;
	btnContextMenu = NULL;
	font = NULL;
	menu = NULL;
	setSkin(confStr["skin"], false);
	initMenu();

	if (!fileExists(confStr["wallpaper"])) {
		DEBUG("Searching wallpaper");

		FileLister fl("skins/"+confStr["skin"]+"/wallpapers",false,true);
		fl.setFilter(".png,.jpg,.jpeg,.bmp");
		fl.browse();
		if (fl.getFiles().size()<=0 && confStr["skin"] != "Default")
			fl.setPath("skins/Default/wallpapers",true);
		if (fl.getFiles().size()>0)
			confStr["wallpaper"] = fl.getPath()+"/"+fl.getFiles()[0];
	}

	initBG();
	input.init(path+"input.conf");
	setInputSpeed();
	initServices();

	setGamma(confInt["gamma"]);
	setVolume(confInt["globalVolume"]);
	applyDefaultTimings();
	setClock(confInt["menuClock"]);

	//recover last session
	readTmp();
	if (lastSelectorElement>-1 && menu->selLinkApp()!=NULL && (!menu->selLinkApp()->getSelectorDir().empty() || !lastSelectorDir.empty()))
		menu->selLinkApp()->selector(lastSelectorElement,lastSelectorDir);

	// bottomBarTextY = resY - (skinConfInt["bottomBarHeight"]/2) - 2; //10;
	// bottomBarIconY = bottomBarTextY - 6; //10;

	// LIST rect
	listRect = (SDL_Rect){0, skinConfInt["topBarHeight"], resX, resY - skinConfInt["bottomBarHeight"] - skinConfInt["topBarHeight"]};

}

GMenu2X::~GMenu2X() {
	writeConfig();
	if (fwType=="open2x") writeConfigOpen2x();

	quit();

	delete menu;
	delete s;
	delete font;
	delete titlefont;
	// delete bottombarfont;
}

void GMenu2X::quit() {
	fflush(NULL);
	sc.clear();
	s->free();
	SDL_Quit();
#ifdef TARGET_GP2X
	if (memdev!=0) {
		//Fix tv-out
		if (memregs[0x2800>>1]&0x100) {
			memregs[0x2906>>1]=512;
			memregs[0x28E4>>1]=memregs[0x290C>>1];
		}
		gp2x_deinit();
	}
#elif defined(TARGET_RETROGMAE)
	gp2x_deinit();
#endif
}

void GMenu2X::initBG(const string &imagePath) {
	if (bg != NULL) delete bg;

	bg = new Surface(s);
	bg->box(0, 0, resX, resY, 0, 0, 0);

	if (!imagePath.empty() && sc.add(imagePath) != NULL) {
		sc[imagePath]->blit(bg, 0, 0);
	} else if (sc.add(confStr["wallpaper"]) != NULL) {
		sc[confStr["wallpaper"]]->blit(bg, 0, 0);
	}

	// LINKS rect
	linksRect = (SDL_Rect){0, 0, resX, resY};
	sectionBarRect = (SDL_Rect){0, 0, resX, resY};



}

void GMenu2X::initFont() {
	if (font != NULL) {
		delete font;
		font = NULL;
	}
	if (titlefont != NULL) {
		delete titlefont;
		titlefont = NULL;
	}

	font = new FontHelper(sc.getSkinFilePath("font.ttf"), skinConfInt["fontSize"], skinConfColors[COLOR_FONT], skinConfColors[COLOR_FONT_OUTLINE]);
	titlefont = new FontHelper(sc.getSkinFilePath("font.ttf"), skinConfInt["titleFontSize"], skinConfColors[COLOR_FONT], skinConfColors[COLOR_FONT_OUTLINE]);
}

void GMenu2X::initMenu() {
	//Menu structure handler
	menu = new Menu(this);
	for (uint i=0; i < menu->getSections().size(); i++) {
		//Add virtual links in the applications section
		if (menu->getSections()[i]=="applications") {
			menu->addActionLink(i, tr["Explorer"], MakeDelegate(this, &GMenu2X::explorer), tr["Launch an application"], "skin:icons/explorer.png");
#if !defined(TARGET_PC)
			if (getBatteryStatus() > 5) // show only if charging
#endif
				menu->addActionLink(i, tr["Battery Logger"], MakeDelegate(this, &GMenu2X::batteryLogger), tr["Log battery power to battery.csv"], "skin:icons/ebook.png");
		}

		//Add virtual links in the setting section
		else if (menu->getSections()[i]=="settings") {
			menu->addActionLink(i, tr["Settings"], MakeDelegate(this, &GMenu2X::options), tr["Configure options"], "skin:icons/configure.png");
			menu->addActionLink(i, tr["Skin"], MakeDelegate(this, &GMenu2X::skinMenu), tr["Configure skin"], "skin:icons/skin.png");
			menu->addActionLink(i, tr["Wallpaper"], MakeDelegate(this, &GMenu2X::changeWallpaper), tr["Select an image to use as a wallpaper"], "skin:icons/wallpaper.png");
#ifdef TARGET_GP2X
			if (fwType=="open2x")
				menu->addActionLink(i, "Open2x", MakeDelegate(this, &GMenu2X::settingsOpen2x), tr["Configure Open2x system settings"], "skin:icons/o2xconfigure.png");
			menu->addActionLink(i, "TV", MakeDelegate(this, &GMenu2X::toggleTvOut), tr["Activate/deactivate tv-out"], "skin:icons/tv.png");
			menu->addActionLink(i, "USB SD", MakeDelegate(this, &GMenu2X::activateSdUsb), tr["Activate USB on SD"], "skin:icons/usb.png");
			if (fwType=="gph" && !f200)
				menu->addActionLink(i, "USB Nand", MakeDelegate(this, &GMenu2X::activateNandUsb), tr["Activate USB on NAND"], "skin:icons/usb.png");
			//menu->addActionLink(i, "USB Root", MakeDelegate(this, &GMenu2X::activateRootUsb), tr["Activate USB on the root of the Gp2x Filesystem"], "skin:icons/usb.png");
#elif defined(TARGET_RS97)
			//menu->addActionLink(i, "Speaker", MakeDelegate(this, &GMenu2X::toggleSpeaker), tr["Activate/deactivate Speaker"], "skin:icons/speaker.png");
			menu->addActionLink(i, tr["TV"], MakeDelegate(this, &GMenu2X::toggleTvOut), tr["Activate/deactivate tv-out"], "skin:icons/tv.png");
			//menu->addActionLink(i, "USB", MakeDelegate(this, &GMenu2X::activateSdUsb), tr["Activate USB on SD"], "skin:icons/usb.png");
			//menu->addActionLink(i, "Format", MakeDelegate(this, &GMenu2X::formatSd), tr["Format internal SD"], "skin:icons/format.png");
			menu->addActionLink(i, tr["Umount"], MakeDelegate(this, &GMenu2X::umountSd), tr["Umount external SD"], "skin:icons/eject.png");
#endif

			if (fileExists(path + "log.txt"))
				menu->addActionLink(i, tr["Log Viewer"], MakeDelegate(this, &GMenu2X::viewLog), tr["Displays last launched program's output"], "skin:icons/ebook.png");


			menu->addActionLink(i, tr["About"], MakeDelegate(this, &GMenu2X::about), tr["Info about system"], "skin:icons/about.png");
			// menu->addActionLink(i, "Reboot", MakeDelegate(this, &GMenu2X::reboot), tr["Reboot device"], "skin:icons/reboot.png");
			menu->addActionLink(i, tr["Power"], MakeDelegate(this, &GMenu2X::poweroff), tr["Power menu"], "skin:icons/exit.png");
		}
	}
	menu->setSectionIndex(confInt["section"]);
	menu->setLinkIndex(confInt["link"]);
	menu->loadIcons();
}

char *ms2hms(unsigned long t, bool mm = true, bool ss = true) {
	static char buf[10];

	t = t / 1000;
	int s = (t % 60);
	int m = (t % 3600) / 60;
	int h = (t % 86400) / 3600;
	int d = (t % (86400 * 30)) / 86400;

	if (!ss) sprintf(buf, "%02d:%02d", h, m);
	else if (!mm) sprintf(buf, "%02d", h);
	else sprintf(buf, "%02d:%02d:%02d", h, m, s);
	return buf;
};



void GMenu2X::about() {
	vector<string> text;
	string temp, batt;

	char *hms = ms2hms(SDL_GetTicks());
	long battlevel = getBatteryStatus();

	stringstream ss; ss << battlevel; ss >> batt;

	temp = tr["Build date: "] + __DATE__ + "\n";
	temp += tr["Uptime: "] + hms + "\n";
	temp += tr["Battery: "] + ((battlevel < 0 || battlevel > 10000) ? tr["Charging"] : batt);// + "\n";
	// temp += tr["Storage:"];
	// temp += "\n    " + tr["Root: "] + getDiskFree("/");
	// temp += "\n    " + tr["Internal: "] + getDiskFree("/mnt/int_sd");
	// temp += "\n    " + tr["External: "] + getDiskFree("/mnt/ext_sd");
	temp += "\n----\n";

#if defined(TARGET_CAANOO)
	string versionFile = "";
	if (fileExists("/usr/gp2x/version"))
		versionFile = "/usr/gp2x/version";
	else if (fileExists("/tmp/gp2x/version"))
		versionFile = "/tmp/gp2x/version";
	if (!versionFile.empty()) {
		ifstream f(versionFile.c_str(), ios_base::in);
		if (f.is_open()) {
			string line;
			if (getline(f, line, '\n'))
				temp += "\nFirmware version: " + line + "\n" + exec("uname -srm");
			f.close();
		}
	}
#endif

	ifstream f("about.txt", ios_base::in);
	if (f.is_open()) {
		string line;
		while (getline(f, line, '\n'))
			temp += line + "\n"; // + exec("uname -srm");
		f.close();
	}

	split(text, temp, "\n");
	TextDialog td(this, "GMenuNext", tr["Info about system"], "icons/about.png", &text);
	td.exec();
}

void GMenu2X::viewLog() {
	string logfile = path + "log.txt";
	if (!fileExists(logfile)) return;
	ifstream inf(logfile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	vector<string> log;

	string line;
	while (getline(inf, line, '\n'))
		log.push_back(line);
	inf.close();

	TextDialog td(this, tr["Log Viewer"], tr["Displays last launched program's output"], "icons/ebook.png", &log);
	td.exec();

	MessageBox mb(this, tr["Do you want to delete the log file?"], "icons/ebook.png");
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

void GMenu2X::batteryLogger() {
	long tickNow = 0, tickStart = SDL_GetTicks(), tickBatteryLogger = -1000000;
	string logfile = path+"battery.csv";

	char buf[100];
	sprintf(buf, "echo '----' >> %s/battery.csv; sync", cmdclean(getExePath()).c_str());
	system(buf);

	if (!fileExists(logfile)) return;

	ifstream inf(logfile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	vector<string> log;

	string line;
	while (getline(inf, line, '\n'))
		log.push_back(line);
	inf.close();

	initBG();

	setBacklight(100);
	setClock(528);

	bool close = false;

	drawTopBar(bg);
	titlefont->setColor(skinConfColors[COLOR_FONT_ALT])->setOutlineColor(skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	bg->write(titlefont, tr["Battery Logger"], 40, 4 + titlefont->getHeight()/2, HAlignLeft, VAlignMiddle);
	bg->write(font, tr["Log battery power to battery.csv"], 40, 38, HAlignLeft, VAlignBottom);
	titlefont->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);
	sc.skinRes("icons/ebook.png")->blit(bg, 4, 4, 32, 32);

	drawBottomBar(bg);
	drawButton(bg, "b", tr["Back"],
	drawButton(bg, "select", tr["Del battery.csv"],
	drawButton(bg, "down", tr["Scroll"],
	drawButton(bg, "up", "", 5)-10)));

	bg->box(listRect, skinConfColors[COLOR_LIST_BG]);

	bg->blit(s,0,0);

// // skinConfColor
// 	int i = 0, j = 0;
// 	for(ConfColorHash::iterator curr = skinConfColor.begin(); curr != skinConfColor.end(); curr++) {
// 		i++;
// 		if (i > 31) {
// 			j++;
// 			i=0;
// 		}
// 		if (j > 14) break;

// 		DEBUG("COLOR: %d,%d %s = %d", i,j, curr->first.c_str(),  (unsigned short)curr->second.r);
// 		bg->box(2+i*10,2+j*10,8,8, curr->second);
// 	}

	bg->flip();

	MessageBox mb(this, tr["Welcome to the Battery Logger.\nMake sure the battery is fully charged.\nAfter pressing OK, leave the device ON until\nthe battery has been fully discharged.\nThe log will be saved in 'battery.csv'."]);
	mb.exec();

	uint firstRow = 0, rowsPerPage = listRect.h/font->getHeight();
		// s->setClipRect(rect);
	while (!close) {
		tickNow = SDL_GetTicks();
		if ((tickNow - tickBatteryLogger) >= 60000) {
			tickBatteryLogger = tickNow;

			char buf[100];
			sprintf(buf, "echo '%s,%d,%d' >> %s/battery.csv; sync", ms2hms(tickNow - tickStart, true, false), getBatteryStatus(), getBatteryLevel(), cmdclean(getExePath()).c_str());
			system(buf);

			ifstream inf(logfile.c_str(), ios_base::in);
			log.clear();

			// string line;
			while (getline(inf, line, '\n'))
				log.push_back(line);
			inf.close();
		}

		bg->blit(s,0,0);

		for (uint i=firstRow; i<firstRow+rowsPerPage && i<log.size(); i++) {
			int rowY, j = log.size() - i - 1;
			if (log.at(j)=="----") { //draw a line
				rowY = 42+(int)((i-firstRow+0.5)*font->getHeight());
				s->hline(5,rowY,resX-16,255,255,255,130);
				s->hline(5,rowY+1,resX-16,0,0,0,130);
			} else {
				rowY = 42+(i-firstRow)*font->getHeight();
				font->write(s, log.at(j), 5, rowY);
			}
		}

		drawScrollBar(rowsPerPage, log.size(), firstRow, listRect);

		s->flip();

		input.update(0);

// COMMON ACTIONS
		if ( input.isActive(MODIFIER) ) {
			if (input.isActive(SECTION_NEXT)) {
				if (!saveScreenshot()) { ERROR("Can't save screenshot"); continue; }
				MessageBox mb(this, tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
			} else if (input.isActive(SECTION_PREV)) {
				int vol = getVolume();
				if (vol) {
					vol = 0;
					volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					volumeMode = VOLUME_MODE_NORMAL;
				}
				confInt["globalVolume"] = vol;
				setVolume(vol);
				writeConfig();
			}
		}
// END OF COMMON ACTIONS
		else if ( input[UP  ] && firstRow > 0 ) firstRow--;
		else if ( input[DOWN] && firstRow + rowsPerPage < log.size() ) firstRow++;
		else if ( input[PAGEUP] || input[LEFT]) {
			if (firstRow >= rowsPerPage - 1)
				firstRow -= rowsPerPage - 1;
			else
				firstRow = 0;
		}
		else if ( input[PAGEDOWN] || input[RIGHT]) {
			if (firstRow + rowsPerPage * 2 - 1 < log.size())
				firstRow += rowsPerPage - 1;
			else
				firstRow = max(0,log.size()-rowsPerPage);
		}
		else if ( input[SETTINGS] || input[CANCEL] ) close = true;
		else if (input[MENU]) {
			MessageBox mb(this, tr.translate("Deleting $1", "battery.csv", NULL) + "\n" + tr["Are you sure?"]);
			mb.setButton(CONFIRM, tr["Yes"]);
			mb.setButton(CANCEL,  tr["No"]);
			if (mb.exec() == CONFIRM) {
				system("rm battery.csv");
				log.clear();
			}
		}
	}

		// s->clearClipRect();
	setBacklight(confInt["backlight"]);
}


void GMenu2X::readConfig() {
	string conffile = path+"gmenu2x.conf";

	// Defaults
	confStr["batteryType"] = "BL-5B";
	confStr["sectionBarPosition"] = "Left";

	if (fileExists(conffile)) {
		ifstream inf(conffile.c_str(), ios_base::in);
		if (inf.is_open()) {
			string line;
			while (getline(inf, line, '\n')) {
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (value.length()>1 && value.at(0)=='"' && value.at(value.length()-1)=='"')
					confStr[name] = value.substr(1,value.length()-2);
				else
					confInt[name] = atoi(value.c_str());
			}
			inf.close();
		}
	}

	if (!confStr["lang"].empty()) tr.setLang(confStr["lang"]);
	if (!confStr["wallpaper"].empty() && !fileExists(confStr["wallpaper"])) confStr["wallpaper"] = "";
	if (confStr["skin"].empty() || !dirExists("skins/"+confStr["skin"])) confStr["skin"] = "Default";

	// evalIntConf( &confInt["batteryLog"], 0, 0, 1 );
	evalIntConf( &confInt["backlightTimeout"], 30, 10, 300);
	evalIntConf( &confInt["outputLogs"], 0, 0, 1 );
#ifdef TARGET_GP2X
	evalIntConf( &confInt["maxClock"], 300, 200, 300 );
	evalIntConf( &confInt["menuClock"], 140, 50, 300 );
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	evalIntConf( &confInt["maxClock"], 900, 200, 900 );
	evalIntConf( &confInt["menuClock"], DEFAULT_CPU_CLK, 250, 300 );
#elif defined(TARGET_RS97)
	evalIntConf( &confInt["maxClock"], 750, 750, 750 );
	evalIntConf( &confInt["menuClock"], DEFAULT_CPU_CLK, 528, 528 );
#endif
	evalIntConf( &confInt["globalVolume"], 60, 1, 100 );
	evalIntConf( &confInt["gamma"], 10, 1, 100 );
	evalIntConf( &confInt["videoBpp"], 16, 8, 32 );
	evalIntConf( &confInt["backlight"], 70, 1, 100);

	if (confStr["tvoutEncoding"] != "PAL") confStr["tvoutEncoding"] = "NTSC";
	resX = constrain( confInt["resolutionX"], 320, 1920 );
	resY = constrain( confInt["resolutionY"], 240, 1200 );
}

void GMenu2X::writeConfig() {
	ledOn();
	string conffile = path+"gmenu2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		ConfStrHash::iterator endS = confStr.end();
		for(ConfStrHash::iterator curr = confStr.begin(); curr != endS; curr++) {
			inf << curr->first << "=\"" << curr->second << "\"" << endl;
		}

		ConfIntHash::iterator endI = confInt.end();
		for(ConfIntHash::iterator curr = confInt.begin(); curr != endI; curr++) {
			inf << curr->first << "=" << curr->second << endl;
		}
		inf.close();
		sync();
	}
	ledOff();
}


void GMenu2X::readConfigOpen2x() {
	string conffile = "/etc/config/open2x.conf";
	if (!fileExists(conffile)) return;
	ifstream inf(conffile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	string line;
	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		string name = trim(line.substr(0,pos));
		string value = trim(line.substr(pos+1,line.length()));

		if (name=="USB_NET_ON_BOOT") o2x_usb_net_on_boot = value == "y" ? true : false;
		else if (name=="USB_NET_IP") o2x_usb_net_ip = value;
		else if (name=="TELNET_ON_BOOT") o2x_telnet_on_boot = value == "y" ? true : false;
		else if (name=="FTP_ON_BOOT") o2x_ftp_on_boot = value == "y" ? true : false;
		else if (name=="GP2XJOY_ON_BOOT") o2x_gp2xjoy_on_boot = value == "y" ? true : false;
		else if (name=="USB_HOST_ON_BOOT") o2x_usb_host_on_boot = value == "y" ? true : false;
		else if (name=="USB_HID_ON_BOOT") o2x_usb_hid_on_boot = value == "y" ? true : false;
		else if (name=="USB_STORAGE_ON_BOOT") o2x_usb_storage_on_boot = value == "y" ? true : false;
		else if (name=="VOLUME_MODE") volumeMode = savedVolumeMode = constrain( atoi(value.c_str()), 0, 2);
		else if (name=="PHONES_VALUE") volumeScalerPhones = constrain( atoi(value.c_str()), 0, 100);
		else if (name=="NORMAL_VALUE") volumeScalerNormal = constrain( atoi(value.c_str()), 0, 150);
	}
	inf.close();
}
void GMenu2X::writeConfigOpen2x() {
	ledOn();
	string conffile = "/etc/config/open2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "USB_NET_ON_BOOT=" << ( o2x_usb_net_on_boot ? "y" : "n" ) << endl;
		inf << "USB_NET_IP=" << o2x_usb_net_ip << endl;
		inf << "TELNET_ON_BOOT=" << ( o2x_telnet_on_boot ? "y" : "n" ) << endl;
		inf << "FTP_ON_BOOT=" << ( o2x_ftp_on_boot ? "y" : "n" ) << endl;
		inf << "GP2XJOY_ON_BOOT=" << ( o2x_gp2xjoy_on_boot ? "y" : "n" ) << endl;
		inf << "USB_HOST_ON_BOOT=" << ( (o2x_usb_host_on_boot || o2x_usb_hid_on_boot || o2x_usb_storage_on_boot) ? "y" : "n" ) << endl;
		inf << "USB_HID_ON_BOOT=" << ( o2x_usb_hid_on_boot ? "y" : "n" ) << endl;
		inf << "USB_STORAGE_ON_BOOT=" << ( o2x_usb_storage_on_boot ? "y" : "n" ) << endl;
		inf << "VOLUME_MODE=" << volumeMode << endl;
		if (volumeScalerPhones != VOLUME_SCALER_PHONES) inf << "PHONES_VALUE=" << volumeScalerPhones << endl;
		if (volumeScalerNormal != VOLUME_SCALER_NORMAL) inf << "NORMAL_VALUE=" << volumeScalerNormal << endl;
		inf.close();
		sync();
	}
	ledOff();
}

void GMenu2X::writeSkinConfig() {
	ledOn();
	string conffile = path+"skins/"+confStr["skin"]+"/skin.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		ConfStrHash::iterator endS = skinConfStr.end();
		for(ConfStrHash::iterator curr = skinConfStr.begin(); curr != endS; curr++)
			inf << curr->first << "=\"" << curr->second << "\"" << endl;

		ConfIntHash::iterator endI = skinConfInt.end();
		for(ConfIntHash::iterator curr = skinConfInt.begin(); curr != endI; curr++)
			inf << curr->first << "=" << curr->second << endl;

		for (int i = 0; i < NUM_COLORS; ++i) {
			inf << colorToString((enum color)i) << "=" << rgbatostr(skinConfColors[i]) << endl;
		}

		inf.close();
		sync();
	}
	ledOff();
}

void GMenu2X::readCommonIni() {
	if (!fileExists("/usr/gp2x/common.ini")) return;
	ifstream inf("/usr/gp2x/common.ini", ios_base::in);
	if (!inf.is_open()) return;
	string line;
	string section = "";
	while (getline(inf, line, '\n')) {
		line = trim(line);
		if (line[0]=='[' && line[line.length()-1]==']') {
			section = line.substr(1,line.length()-2);
		} else {
			string::size_type pos = line.find("=");
			string name = trim(line.substr(0,pos));
			string value = trim(line.substr(pos+1,line.length()));

			if (section=="usbnet") {
				if (name=="enable")
					usbnet = value=="true" ? true : false;
				else if (name=="ip")
					ip = value;

			} else if (section=="server") {
				if (name=="inet")
					inet = value=="true" ? true : false;
				else if (name=="samba")
					samba = value=="true" ? true : false;
				else if (name=="web")
					web = value=="true" ? true : false;
			}
		}
	}
	inf.close();
}

void GMenu2X::readTmp() {
	lastSelectorElement = -1;
	if (!fileExists("/tmp/gmenu2x.tmp")) return;
	ifstream inf("/tmp/gmenu2x.tmp", ios_base::in);
	if (!inf.is_open()) return;
	string line;
	string section = "";
	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		string name = trim(line.substr(0,pos));
		string value = trim(line.substr(pos+1,line.length()));

		if (name == "section") menu->setSectionIndex(atoi(value.c_str()));
		else if (name == "link") menu->setLinkIndex(atoi(value.c_str()));
		else if (name == "selectorelem") lastSelectorElement = atoi(value.c_str());
		else if (name == "selectordir") lastSelectorDir = value;
	}
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
		inf.close();
	}
}

void GMenu2X::initServices() {
#ifdef TARGET_GP2X
	if (usbnet) {
		string services = "scripts/services.sh "+ip+" "+(inet?"on":"off")+" "+(samba?"on":"off")+" "+(web?"on":"off")+" &";
		system(services.c_str());
	}
#endif
}

void GMenu2X::ledOn() {
#ifdef TARGET_GP2X
	if (memdev!=0 && !f200) memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_ON);
#endif
}

void GMenu2X::ledOff() {
#ifdef TARGET_GP2X
	if (memdev!=0 && !f200) memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_OFF);
#endif
}

int exitMainThread=0;
enum mmc_status{
	MMC_REMOVE, MMC_INSERT, MMC_ERROR
};

long GMenu2X::getBatteryStatus() {
	char buf[32]={0};

	FILE *f = fopen("/proc/jz/battery", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
		return atol(buf);
	}
	return -1;
}

mmc_status getMMCStatus(void) {
	char buf[32]={0};

	FILE *f = fopen("/proc/jz/mmc", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);

		if (memcmp(buf, "REMOVE", 6) == 0) {
			return MMC_REMOVE;
		}
		else if (memcmp(buf, "INSERT", 6) == 0) {
			return MMC_INSERT;
		}
	}
	return MMC_ERROR;
}

enum udc_status{
	UDC_REMOVE, UDC_CONNECT, UDC_ERROR
};

udc_status getUDCStatus(void) {
	char buf[32]={0};

#if 1
	FILE *f = fopen("/proc/jz/udc", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);

		if (memcmp(buf, "REMOVE", 6) == 0) {
			return UDC_REMOVE;
		}
		else if (memcmp(buf, "CONNECT", 6) == 0) {
			return UDC_CONNECT;
		}
	}
#else
  FILE *f = fopen("/sys/devices/platform/musb_hdrc.0/uh_cable", "r");
  fgets(buf, sizeof(buf), f);
  fclose(f);

  if (memcmp(buf, "offline", 7) == 0) {
	return UDC_REMOVE;
  }
  else if (memcmp(buf, "usb", 3) == 0) {
	return UDC_CONNECT;
  }
#endif
	return UDC_ERROR;
}

void* mainThread(void* param) {
	GMenu2X *menu = (GMenu2X*)param;
	while(exitMainThread == 0) {
		sleep(1);
	}
	return NULL;
}

int GMenu2X::setBacklight(int val, bool popup) {
	char buf[64];
	unsigned long tickStart = 0;
	int backlightIcon;


	if(val < 0) val = 100;
	else if(val > 100) val = backlightStep;

	sprintf(buf, "echo %d > /proc/jz/lcd_backlight", val);
	system(buf);
	// DEBUG("%s", buf);

	if (popup) {
		bool close = false;
		input.setWakeUpInterval(40);
		SDL_Rect progress = {52, 32, resX-84, 8};
		SDL_Rect window = {20, 20, resX-40, 32};

		Surface *iconBrightness[6] = {
			sc.skinRes("imgs/brightness/0.png"),
			sc.skinRes("imgs/brightness/1.png"),
			sc.skinRes("imgs/brightness/2.png"),
			sc.skinRes("imgs/brightness/3.png"),
			sc.skinRes("imgs/brightness/4.png"),
			sc.skinRes("imgs/brightness.png")
		};
		while (!close) {
			tickStart = SDL_GetTicks();
	
			s->box(window, 255,255,255,255);
			s->box(window, skinConfColors[COLOR_MESSAGE_BOX_BG]);
			s->rectangle(window, skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

			backlightIcon = val/20;

			if (backlightIcon > 4 || iconBrightness[backlightIcon]==NULL)
				backlightIcon = 5;

			iconBrightness[backlightIcon]->blit(s, 28, 28);

			while (!close) {
				s->box(progress, skinConfColors[COLOR_MESSAGE_BOX_BG]);
				s->box(progress.x + 1, progress.y + 1, val * (progress.w - 3) / 100 + 1, progress.h - 2, skinConfColors[COLOR_MESSAGE_BOX_SELECTION]);
				s->flip();
				input.update(0);
				if ((SDL_GetTicks()-tickStart) >= 3000 || input[MODIFIER] || input[CONFIRM] || input[CANCEL]) close = true;

				if (input[LEFT]) {
					val = setBacklight(max(1, val - backlightStep));
					break;
				} else if (input[RIGHT]) {
					val = setBacklight(min(100, val + backlightStep));
					break;
				} else if (input[BACKLIGHT]) {
					val = setBacklight(val + backlightStep);
					break;
				}
			}
		}

		confInt["backlight"] = val;
		writeConfig();
	}
	return val;
}

bool GMenu2X::setSuspend(bool suspend) {
	if(suspend) {
		setBacklight(0);
		INFO("Enter suspend mode. Current backlight: %d", getBacklight());
	} else{
		setBacklight(max(10, confInt["backlight"]));
		INFO("Exit from suspend mode. Restore backlight to: %d", confInt["backlight"]);
		setClock(528);
	}
	return suspend;
}

void GMenu2X::main() {
	// int ret;
	bool suspendActive = false;
	unsigned short battlevel = 0; //getBatteryLevel();
	pthread_t thread_id;
	// uint linksPerPage = linkColumns*linkRows;
	// int linkSpacingX = (resX-10 - linkColumns*(resX - skinConfInt["sectionBarX"]))/linkColumns;
	// int linkSpacingY = (resY-35 - skinConfInt["sectionBarY"] - linkRows*skinConfInt["sectionBarHeight"])/linkRows;
	uint sectionLinkPadding = 4; //max(skinConfInt["sectionBarHeight"] - 32 - font->getHeight(), 0) / 3;

	mmc_status curMMCStatus = MMC_REMOVE;
	mmc_status preMMCStatus = MMC_REMOVE;
	udc_status curUDCStatus = UDC_REMOVE;
	udc_status preUDCStatus = UDC_REMOVE;
	int needUSBUmount = 0;
	// int backlightOffset;
	bool inputAction;
	bool quit = false;
	int x = 0, y = 0; //, helpBoxHeight = fwType=="open2x" ? 154 : 139;//, offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
	uint i;
	unsigned long tickSuspend = 0, tickPowerOff = 0, tickBattery = -4800, tickNow, tickMMC = 0, tickUSB = 0;
	string batteryIcon = "imgs/battery/3.png"; //, backlightIcon = "imgs/backlight.png";
	string prevBackdrop = confStr["wallpaper"], currBackdrop = confStr["wallpaper"];
	// char backlightMsg[16]={0};
	stringstream ss;
	// uint sectionsCoordX = 24;
	uint sectionsCoordY = 0;//24;
	// SDL_Rect re = {0,0,0,0};

	setBacklight(confInt["backlight"]);
	// btnContextMenu = new IconButton(this,"skin:imgs/menu.png");
	// btnContextMenu->setPosition(resX-18, resY-18);
	// btnContextMenu->setAction(MakeDelegate(this, &GMenu2X::contextMenu));
	exitMainThread = 0;
	if (pthread_create(&thread_id, NULL, mainThread, this)) {
		ERROR("%s, failed to create main thread\n", __func__);
	}
	setClock(528);


	while (!quit) {
		tickNow = SDL_GetTicks();
		inputAction = input.update(0);
		if(suspendActive) {
			// SUSPEND ACTIVE
			if (input[POWER]) {
				tickPowerOff = 0;
				tickSuspend = tickNow;
				suspendActive = setSuspend(false);
			}
			usleep(50000);
			continue;
		}
		// SUSPEND NOT ACTIVE


		//Background
		if (prevBackdrop != currBackdrop) {
			INFO("New backdrop: %s", currBackdrop.c_str());
			sc.del(prevBackdrop);
			prevBackdrop = currBackdrop;
		}
		sc[currBackdrop]->blit(s,0,0);

		// SECTIONS
		if (confStr["sectionBarPosition"] != "OFF") {
				x = 0; y = 0;
				if (confStr["sectionBarPosition"] == "Left" || confStr["sectionBarPosition"] == "Right") {
					sectionBarRect.x = (confStr["sectionBarPosition"] == "Right")*(resX - skinConfInt["sectionBarWidth"]);
					sectionBarRect.w = skinConfInt["sectionBarWidth"];
						linksRect.w = resX - skinConfInt["sectionBarWidth"];

						if (confStr["sectionBarPosition"] == "Left") {
							linksRect.x = skinConfInt["sectionBarWidth"];
						} else {
							// VERTICAL RIGHT
							x = resX - skinConfInt["sectionBarWidth"];
						}
				} else {
					sectionBarRect.y = (confStr["sectionBarPosition"] == "Bottom")*(resY - skinConfInt["sectionBarWidth"]);
					sectionBarRect.h = skinConfInt["sectionBarWidth"];
						linksRect.h = resY - skinConfInt["sectionBarWidth"];

						if (confStr["sectionBarPosition"] == "Top") {
							linksRect.y = skinConfInt["sectionBarWidth"];
						} else {
							// HORIZONTAL BOTTOM
							y = resY - skinConfInt["sectionBarWidth"];
						}
				}
				s->box(sectionBarRect, skinConfColors[COLOR_TOP_BAR_BG]);

				for (i = menu->firstDispSection(); i < menu->getSections().size() && i < menu->firstDispSection() + menu->sectionNumItems(); i++) {
					string sectionIcon = "skin:sections/"+menu->getSections()[i]+".png";
					if (!sc.exists(sectionIcon))
						sectionIcon = "skin:icons/section.png";

					if (confStr["sectionBarPosition"] == "Left" || confStr["sectionBarPosition"] == "Right") {
						y = (i - menu->firstDispSection()) * skinConfInt["sectionBarWidth"];
					} else {
						x = (i - menu->firstDispSection()) * skinConfInt["sectionBarWidth"];
					}

					if (menu->selSectionIndex()==(int)i)
						s->box(x, y, skinConfInt["sectionBarWidth"], skinConfInt["sectionBarWidth"], skinConfColors[COLOR_SELECTION_BG]);

					sc[sectionIcon]->blitCenter(s, x + skinConfInt["sectionBarWidth"]/2, y + skinConfInt["sectionBarWidth"]/2, skinConfInt["sectionBarWidth"], skinConfInt["sectionBarWidth"]);
				}
		}

		// LINKS
		s->setClipRect(linksRect);
		s->box(linksRect, skinConfColors[COLOR_LIST_BG]);

		linkRows = linksRect.h / skinConfInt["linkItemHeight"];
		x = linksRect.x;
		// for (i = menu->firstDispRow() * linkColumns; i < (menu->firstDispRow() * linkColumns) && i < menu->sectionLinks()->size(); i++) {
		// for (i = menu->firstDispRow() * linkColumns; i < (menu->firstDispRow() * linkColumns) + linksPerPage && i < menu->sectionLinks()->size(); i++) {
		// for (i = menu->firstDispRow() * linkColumns; i < (menu->firstDispRow() * linkColumns) + linkRows && i < menu->sectionLinks()->size(); i++) {
		for (i = menu->firstDispRow(); i < menu->firstDispRow() + linkRows && i < menu->sectionLinks()->size(); i++) {
			// int ir = i - menu->firstDispRow();
			int ir = i - menu->firstDispRow() * linkColumns;

			// y = (ir) * skinConfInt["linkItemHeight"] + linksRect.y; //+skinConfInt["sectionBarY"];//ir/linkColumns*(skinConfInt["linkItemHeight"]+linkSpacingY)+skinConfInt["linkItemHeight"]+2;
			y = (ir % linkRows) * skinConfInt["linkItemHeight"] + linksRect.y; //+skinConfInt["sectionBarY"];//ir/linkColumns*(skinConfInt["linkItemHeight"]+linkSpacingY)+skinConfInt["linkItemHeight"]+2;

			menu->sectionLinks()->at(i)->setPosition(x,y);

			if (i == (uint)menu->selLinkIndex())
				menu->sectionLinks()->at(i)->paintHover();
			menu->sectionLinks()->at(i)->paint();
		}
		s->clearClipRect();

		drawScrollBar(linkRows, menu->sectionLinks()->size()/linkColumns + ((menu->sectionLinks()->size()%linkColumns==0) ? 0 : 1), menu->firstDispRow(), linksRect);

		// TRAY DEBUG
		// s->box(sectionBarRect.x + sectionBarRect.w - 38 + 0 * 20, sectionBarRect.y + sectionBarRect.h - 18,16,16, strtorgba("ffff00ff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 38 + 1 * 20, sectionBarRect.y + sectionBarRect.h - 18,16,16, strtorgba("00ff00ff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38,16,16, strtorgba("0000ffff"));
		// s->box(sectionBarRect.x + sectionBarRect.w - 18, sectionBarRect.y + sectionBarRect.h - 38,16,16, strtorgba("ff00ffff"));
		if (tickNow - tickMMC >= 1000) {
			tickMMC = tickNow;
			curMMCStatus = getMMCStatus();
			if (preMMCStatus != curMMCStatus) {
				if (curMMCStatus == MMC_REMOVE) {
					system("/usr/bin/umount_ext_sd.sh");
					INFO("%s: umount external SD from /mnt/ext_sd", __func__);
				}
				else if (curMMCStatus == MMC_INSERT) {
					system("/usr/bin/mount_ext_sd.sh");
					INFO("%s: mount external SD on /mnt/ext_sd", __func__);
				}
				else {
					WARNING("%s: unexpected MMC status!", __func__);
				}
				preMMCStatus = curMMCStatus;
			}
		}
		if (tickNow - tickUSB >= 1000) {
			tickUSB = tickNow;
			curUDCStatus = getUDCStatus();
			if (preUDCStatus != curUDCStatus) {
				if (curUDCStatus == UDC_REMOVE) {
					if (needUSBUmount) {
						system("/usr/bin/usb_disconn_int_sd.sh");
						INFO("%s, disconnect usbdisk for internal sd", __func__);
						if (curMMCStatus == MMC_INSERT) {
							system("/usr/bin/usb_disconn_ext_sd.sh");
							INFO("%s, disconnect USB disk for external SD", __func__);
						}
						needUSBUmount = 0;
					}
				}
				else if(curUDCStatus == UDC_CONNECT) {
					MessageBox mb(this, tr["Which action do you want ?"], "icons/usb.png");
					mb.setButton(CONFIRM, tr["USB disk"]);
					mb.setButton(CANCEL,  tr["Charge only"]);
					if (mb.exec() == CONFIRM) {
						needUSBUmount = 1;
						system("/usr/bin/usb_conn_int_sd.sh");
						INFO("%s, connect USB disk for internal SD", __func__);
						if (curMMCStatus == MMC_INSERT) {
							system("/usr/bin/usb_conn_ext_sd.sh");
							INFO("%s, connect USB disk for external SD", __func__);
						}
					}
				}
				else {
					WARNING("%s, unexpected USB status!", __func__);
				}
				preUDCStatus = curUDCStatus;
			}
		}

		currBackdrop = confStr["wallpaper"];
		if (menu->selLink() != NULL && menu->selLinkApp() != NULL && !menu->selLinkApp()->getBackdrop().empty() && sc.add(menu->selLinkApp()->getBackdrop()) != NULL) {
			currBackdrop = menu->selLinkApp()->getBackdrop();
		}


		if (confStr["sectionBarPosition"] != "OFF") {

			// TRAY 0,0
			switch(volumeMode) {
				case VOLUME_MODE_PHONES: sc.skinRes("imgs/phones.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38); break;
				case VOLUME_MODE_MUTE:   sc.skinRes("imgs/mute.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38); break;
				default: sc.skinRes("imgs/volume.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38, sectionBarRect.y + sectionBarRect.h - 38); break;
			}

			// TRAY 1,0
			if (tickNow - tickBattery >= 5000) {
				tickBattery = tickNow;
				battlevel = getBatteryLevel();
				if (battlevel > 5) {
					batteryIcon = "imgs/battery/ac.png";
				} else {
					ss.clear();
					ss << battlevel;
					ss >> batteryIcon;
					batteryIcon = "imgs/battery/" + batteryIcon + ".png";
				}
			}
			sc.skinRes(batteryIcon)->blit(s, sectionBarRect.x + sectionBarRect.w - 18, sectionBarRect.y + sectionBarRect.h - 38);

			// TRAY iconTrayShift,1
			int iconTrayShift = 0;
			if (preMMCStatus == MMC_INSERT) {
				sc.skinRes("imgs/sd1.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
				iconTrayShift++;
			}

			if (menu->selLink() != NULL) {
				if (menu->selLinkApp() != NULL) {
					if (!menu->selLinkApp()->getManual().empty() && iconTrayShift < 2) {
						// Manual indicator
						sc.skinRes("imgs/manual.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
						iconTrayShift++;
					}

					if (iconTrayShift < 2) {
						// CPU indicator
						sc.skinRes("imgs/cpu.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
						iconTrayShift++;
					}
				}
			}

			if (iconTrayShift < 2) {
				// menu indicator
				sc.skinRes("imgs/menu.png")->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
				iconTrayShift++;
			}

			if (iconTrayShift < 2) {
				int backlightLevel = confInt["backlight"]/20;
				string backlightIcon;
				ss.clear();
				ss << backlightLevel;
				ss >> backlightIcon;
				backlightIcon = "imgs/brightness/" + backlightIcon + ".png";

				if (backlightLevel > 4 || sc.skinRes(backlightIcon)==NULL) 
					backlightIcon = "imgs/brightness.png";

				sc.skinRes(backlightIcon)->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
				iconTrayShift++;
			}
		}

		s->flip();

		if (inputAction == 0) {
			// INFO("NOW: %d\tSUSPEND: %d\tPOWER: %d", tickNow, tickSuspend, tickPowerOff);

			usleep(50000);
			
			if(input.isActive(POWER)) {
				if (tickPowerOff >= 5) { //5 * 500ms
					poweroff();
					tickPowerOff = 0;
				}
			} else if (tickPowerOff >= 2 || tickNow - tickSuspend >= confInt["backlightTimeout"] * 1000) {
					if(!suspendActive) {
						MessageBox mb(this, tr["Suspend"]);
						mb.setAutoHide(1000);
						mb.exec();

						SDL_Delay(1000);
						suspendActive = setSuspend(true);
						tickPowerOff = 0;
					}
			} else {
				tickPowerOff = 0;
			}
			continue;
		}

		if ( input[CONFIRM] && menu->selLink() != NULL ) {
			setVolume(confInt["globalVolume"]);
			menu->selLink()->run();
		}

// COMMON ACTIONS
		else if ( input.isActive(MODIFIER) ) {
			if (input.isActive(SECTION_NEXT)) {
				if (!saveScreenshot()) { ERROR("Can't save screenshot"); continue; }
				MessageBox mb(this, tr["Screenshot saved"]);
				mb.setAutoHide(1000);
				mb.exec();
			} else if (input.isActive(SECTION_PREV)) {
				int vol = getVolume();
				if (vol) {
					vol = 0;
					volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					volumeMode = VOLUME_MODE_NORMAL;
				}
				confInt["globalVolume"] = vol;
				setVolume(vol);
				writeConfig();
			}
		}
		// BACKLIGHT
		else if ( input[BACKLIGHT] ) setBacklight(confInt["backlight"], true);
// END OF COMMON ACTIONS

		else if ( input[SETTINGS] ) options();
		else if ( input[MENU]     ) contextMenu();
		// LINK NAVIGATION
		else if ( input[LEFT ]  ) menu->linkLeft();
		else if ( input[RIGHT]  ) menu->linkRight();
		else if ( input[UP   ]  ) menu->linkUp();
		else if ( input[DOWN ]  ) menu->linkDown();
		// SECTION
		else if ( input[SECTION_PREV] ) menu->decSectionIndex();
		else if ( input[SECTION_NEXT] ) menu->incSectionIndex();
		// POWER || SUSPEND
		else if ( input[POWER] ) { tickPowerOff++; }

		// VOLUME SCALE MODIFIER
		else if ( fwType=="open2x" && input[CANCEL] ) {
			volumeMode = constrain(volumeMode-1, -VOLUME_MODE_MUTE-1, VOLUME_MODE_NORMAL);
			if(volumeMode < VOLUME_MODE_MUTE)
				volumeMode = VOLUME_MODE_NORMAL;
			switch(volumeMode) {
				case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
				case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones); break;
				case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
			}
			setVolume(confInt["globalVolume"]);
		}

		// SELLINKAPP SELECTED
		else if (menu->selLinkApp()!=NULL) {
			if ( input[MANUAL] ) menu->selLinkApp()->showManual();
		// 	else if ( input.isActive(MODIFIER) ) {
		// 		// VOLUME
		// 		if ( input[VOLDOWN] && !input.isActive(VOLUP) )
		// 			menu->selLinkApp()->setVolume( constrain(menu->selLinkApp()->volume()-1,0,100) );
		// 		if ( input[VOLUP] && !input.isActive(VOLDOWN) )
		// 			menu->selLinkApp()->setVolume( constrain(menu->selLinkApp()->volume()+1,0,100) );
		// 		if ( input.isActive(VOLUP) && input.isActive(VOLDOWN) ) menu->selLinkApp()->setVolume(-1);
		// 	} else {
		// 		// CLOCK
		// 		if ( input[VOLDOWN] && !input.isActive(VOLUP) )
		// 			menu->selLinkApp()->setClock( constrain(menu->selLinkApp()->clock()-10,50,confInt["maxClock"]) );
		// 		if ( input[VOLUP] && !input.isActive(VOLDOWN) )
		// 			menu->selLinkApp()->setClock( constrain(menu->selLinkApp()->clock()+10,50,confInt["maxClock"]) );
		// 		if ( input.isActive(VOLUP) && input.isActive(VOLDOWN) ) menu->selLinkApp()->setClock(DEFAULT_CPU_CLK);
		// 	}
		}


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

		// tickSuspend = tickPowerOff = SDL_GetTicks();
		tickSuspend = SDL_GetTicks();
	}
	
	exitMainThread = 1;
	pthread_join(thread_id, NULL);
	delete btnContextMenu;
	btnContextMenu = NULL;
}

void GMenu2X::explorer() {
	FileDialog fd(this,tr["Select an application"],".gpu,.gpe,.sh,", "", tr["Explorer"]);
	if (fd.exec()) {
		if (confInt["saveSelection"] && (confInt["section"]!=menu->selSectionIndex() || confInt["link"]!=menu->selLinkIndex()))
			writeConfig();
		if (fwType == "open2x" && savedVolumeMode != volumeMode)
			writeConfigOpen2x();

	//string command = cmdclean(fd.path()+"/"+fd.file) + "; sync & cd "+cmdclean(getExePath())+"; exec ./gmenu2x";
		string command = cmdclean(fd.getPath()+"/"+fd.getFile());
		chdir(fd.getPath().c_str());
		quit();
		setClock(DEFAULT_CPU_CLK);
		execlp("/bin/sh","/bin/sh","-c",command.c_str(),NULL);

	//if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
	//try relaunching gmenu2x
		WARNING("Error executing selected application, re-launching gmenu2x");
		chdir(getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}
}

void GMenu2X::options() {
	int curMenuClock = confInt["menuClock"];
	int curGlobalVolume = confInt["globalVolume"];
//G
	int prevgamma = confInt["gamma"];
	bool showRootFolder = fileExists("/mnt/root");

	FileLister fl_tr("translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	string lang = tr.lang();

	vector<string> encodings;
	encodings.push_back("NTSC");
	encodings.push_back("PAL");

	vector<string> batteryType;
	batteryType.push_back("BL-5B");
	batteryType.push_back("Linear");

	int prevSkinBackdrops = confInt["skinBackdrops"];

	vector<string> sectionBarPosition;
	sectionBarPosition.push_back("Left");
	sectionBarPosition.push_back("Right");
	sectionBarPosition.push_back("Top");
	sectionBarPosition.push_back("Bottom");
	sectionBarPosition.push_back("OFF");

	SettingsDialog sd(this, input, ts, tr["Settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenu2X"], &lang, &fl_tr.getFiles()));
	sd.addSetting(new MenuSettingInt(this,tr["Screen timeout"], tr["Set the screen timeout and suspend delay"], &confInt["backlightTimeout"], 30, 10, 300));
	sd.addSetting(new MenuSettingMultiString(this, tr["Battery profile"], tr["Set the battery discharge profile"], &confStr["batteryType"], &batteryType));
	sd.addSetting(new MenuSettingBool(this, tr["Skin Backdrops"], tr["Automatic load of backdrops from skin pack"], &confInt["skinBackdrops"]));
	// sd.addSetting(new MenuSettingMultiString(this, tr["Section Bar Postition"], tr["Set the position of the Section Bar"], &confStr["sectionBarPosition"], &sectionBarPosition));

	sd.addSetting(new MenuSettingBool(this, tr["Save last selection"], tr["Save the last selected link and section on exit"], &confInt["saveSelection"]));
#ifdef TARGET_GP2X
	sd.addSetting(new MenuSettingInt(this, tr["Clock for GMenu2X"], tr["Set the cpu working frequency when running GMenu2X"], &confInt["menuClock"], 140, 50, 325));
	sd.addSetting(new MenuSettingInt(this, tr["Maximum overclock"], tr["Set the maximum overclock for launching links"], &confInt["maxClock"], 300, 50, 325));
#endif
#if defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	sd.addSetting(new MenuSettingInt(this, tr["Clock for GMenu2X"], tr["Set the cpu working frequency when running GMenu2X"], &confInt["menuClock"], 200, 50, 900, 10));
	sd.addSetting(new MenuSettingInt(this, tr["Maximum overclock"], tr["Set the maximum overclock for launching links"], &confInt["maxClock"], 900, 50, 900, 10));
#endif
	sd.addSetting(new MenuSettingInt(this, tr["Global volume"], tr["Set the default volume for the soundcard"], &confInt["globalVolume"], 60, 0, 100));
	sd.addSetting(new MenuSettingBool(this, tr["Output logs"], tr["Logs the output of the links. Use the Log Viewer to read them."], &confInt["outputLogs"]));
//G
//sd.addSetting(new MenuSettingInt(this, tr["Gamma"], tr["Set gp2x gamma value (default: 10)"], &confInt["gamma"], 1, 100));
	sd.addSetting(new MenuSettingMultiString(this, tr["TV-out encoding"], tr["Encoding of the TV-out signal"], &confStr["tvoutEncoding"], &encodings));
//sd.addSetting(new MenuSettingBool(this,tr["Show root"],tr["Show root folder in the file selection dialogs"],&showRootFolder));

	if (sd.exec() && sd.edited()) {
	//G
		if (prevgamma != confInt["gamma"]) setGamma(confInt["gamma"]);








		setGamma(confInt["gamma"]);
		if (curMenuClock!=confInt["menuClock"]) {
			setClock(confInt["menuClock"]);
		}
		if (curGlobalVolume!=confInt["globalVolume"]) setVolume(confInt["globalVolume"]);
		if (lang == "English") lang = "";
		if (lang != tr.lang()) tr.setLang(lang);
		if (fileExists("/mnt/root") && !showRootFolder)
			unlink("/mnt/root");
		else if (!fileExists("/mnt/root") && showRootFolder)
			symlink("/","/mnt/root");

		if (confStr["lang"] != lang) {
			confStr["lang"] = lang;
		}

		writeConfig();
		if (prevSkinBackdrops != confInt["skinBackdrops"] && menu != NULL) {
			restart();
		}
	}
}

void GMenu2X::settingsOpen2x() {
	SettingsDialog sd(this, input, ts, tr["Open2x Settings"]);
	sd.addSetting(new MenuSettingBool(this,tr["USB net on boot"],tr["Allow USB networking to be started at boot time"],&o2x_usb_net_on_boot));
	sd.addSetting(new MenuSettingString(this,tr["USB net IP"],tr["IP address to be used for USB networking"],&o2x_usb_net_ip));
	sd.addSetting(new MenuSettingBool(this,tr["Telnet on boot"],tr["Allow telnet to be started at boot time"],&o2x_telnet_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["FTP on boot"],tr["Allow FTP to be started at boot time"],&o2x_ftp_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["GP2XJOY on boot"],tr["Create a js0 device for GP2X controls"],&o2x_gp2xjoy_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB host on boot"],tr["Allow USB host to be started at boot time"],&o2x_usb_host_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB HID on boot"],tr["Allow USB HID to be started at boot time"],&o2x_usb_hid_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB storage on boot"],tr["Allow USB storage to be started at boot time"],&o2x_usb_storage_on_boot));
//sd.addSetting(new MenuSettingInt(this,tr["Speaker Mode on boot"],tr["Set Speaker mode. 0 = Mute, 1 = Phones, 2 = Speaker"],&volumeMode,0,2,1));
	sd.addSetting(new MenuSettingInt(this,tr["Speaker Scaler"],tr["Set the Speaker Mode scaling 0-150\% (default is 100\%)"],&volumeScalerNormal,100, 0,150));
	sd.addSetting(new MenuSettingInt(this,tr["Headphones Scaler"],tr["Set the Headphones Mode scaling 0-100\% (default is 65\%)"],&volumeScalerPhones,65, 0,100));

	if (sd.exec() && sd.edited()) {
		writeConfigOpen2x();
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones); break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
		setVolume(confInt["globalVolume"]);
	}
}

void GMenu2X::skinMenu() {
	FileLister fl_sk("skins", true, false);
	fl_sk.addExclude("..");
	fl_sk.browse();
	string curSkin = confStr["skin"];


	SettingsDialog sd(this, input, ts, tr["Skin"], "skin:icons/skin.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenu2X"], &confStr["skin"], &fl_sk.getDirectories()));
	sd.addSetting(new MenuSettingRGBA(this, tr["Top Bar"], tr["Color of the top bar"], &skinConfColors[COLOR_TOP_BAR_BG]));
	sd.addSetting(new MenuSettingRGBA(this, tr["List Body"], tr["Color of the list body"], &skinConfColors[COLOR_LIST_BG]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Bottom Bar"], tr["Color of the bottom bar"], &skinConfColors[COLOR_BOTTOM_BAR_BG]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Selection"], tr["Color of the selection and other interface details"], &skinConfColors[COLOR_SELECTION_BG]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Message Box"], tr["Background color of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_BG]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Message Box Border"], tr["Border color of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_BORDER]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Message Box Selection"], tr["Color of the selection of the message box"], &skinConfColors[COLOR_MESSAGE_BOX_SELECTION]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Font"], tr["Color of the font"], &skinConfColors[COLOR_FONT]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Font Outline"], tr["Color of the font's outline"], &skinConfColors[COLOR_FONT_OUTLINE]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font"], tr["Color of the alternative font"], &skinConfColors[COLOR_FONT_ALT]));
	sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font Outline"], tr["Color of the alternative font outline"], &skinConfColors[COLOR_FONT_ALT_OUTLINE]));

// sd.addSetting(new MenuSettingRGBA(this, tr["Bottom Bar Font Color"], tr["Color of the font"], &skinConfColors[COLOR_BOTTOM_BAR_FONT]));

	if (sd.exec() && sd.edited()) {
		if (curSkin != confStr["skin"]) {
			setSkin(confStr["skin"]);
			writeConfig();
		}
		font->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);

		writeSkinConfig();
		// setSkin(confStr["skin"]);
		initBG();
	}
}

void GMenu2X::umountSd() {
#ifdef TARGET_RS97
	MessageBox mb(this, tr["Do you want to umount external sdcard ?"], "icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		system("/usr/bin/umount_ext_sd.sh");
		MessageBox mb(this,tr["Complete !"]);
		mb.exec();
	}
#endif
}

void GMenu2X::formatSd() {
#ifdef TARGET_RS97
	MessageBox mb(this, tr["Do you want to format internal SD card?"], "icons/format.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		s->box(10, 80, 300, 52, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		s->rectangle( 12, 82, 296, 48, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );
		s->write( font, tr["Formatting internal SD card..."], 55, 90 );
		s->flip();
		system("/usr/bin/format_int_sd.sh");
		MessageBox mb(this,tr["Complete!"]);
		mb.exec();
	}
#endif
}

void GMenu2X::restart() {
	MessageBox mb(this, tr["GMenuNext will restart to apply\nthe settings. Continue?"], "icons/exit.png");
	mb.setButton(CONFIRM, tr["Restart"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	if (mb.exec() == CONFIRM) {
		quit();
		WARNING("Re-launching gmenu2x");
		chdir(getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}
}

void GMenu2X::poweroff() {
	MessageBox mb(this, tr["   Poweroff or reboot the device?   "], "icons/exit.png");
	mb.setButton(SECTION_NEXT, tr["Reboot"]);
	mb.setButton(CONFIRM, tr["Poweroff"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	int response = mb.exec();
	// del(mb);
	if (response == CONFIRM) {
		MessageBox mb(this, tr["Poweroff"]);
		mb.setAutoHide(1000);
		mb.exec();
		setSuspend(true);
		SDL_Delay(1000);

#if !defined(TARGET_PC)
		system("poweroff");
#endif
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(this, tr["Rebooting"]);
		mb.setAutoHide(1000);
		mb.exec();
		setSuspend(true);
		SDL_Delay(1000);

#if !defined(TARGET_PC)
		system("reboot");
#endif

	}
}

void GMenu2X::toggleTvOut() {
#ifdef TARGET_GP2X
	if (cx25874!=0)
		gp2x_tvout_off();
	else
		gp2x_tvout_on(confStr["tvoutEncoding"] == "PAL");
#else
	int tvout=0;
	char buf[32]={0};
	int fd = open("/mnt/game/gmenu2x/tvout", O_RDWR);
	if(fd > 0){
		read(fd, buf, sizeof(buf));
		INFO("Current TV-out value: %s", buf);
		if(memcmp(buf, "1", 1) == 0){
			tvout = 0;
			sprintf(buf, "0");
			INFO("Turn off TV-out");
		}
		else{
			tvout = 1;
			sprintf(buf, "1");
			INFO("Turn on TV-out");
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, buf, 1);
		close(fd);
	}
	else{
		tvout = 1;
		fd = open("/mnt/game/gmenu2x/tvout", O_CREAT);
		sprintf(buf, "1");
		write(fd, buf, 1);
		close(fd);
		INFO("Create new TV-out file");
	}

	sprintf(buf, "/usr/bin/tvout.sh %d %d", tvout, confStr["tvoutEncoding"] == "PAL" ? 1 : 2);
	system(buf);
	INFO("run cmd: %s", buf);
#endif
}

void GMenu2X::setSkin(const string &skin, bool setWallpaper) {
	confStr["skin"] = skin;

//Clear previous skin settings
	skinConfStr.clear();
	skinConfInt.clear();

//clear collection and change the skin path
	sc.clear();
	sc.setSkin(skin);
	if (btnContextMenu != NULL)
		btnContextMenu->setIcon( btnContextMenu->getIcon() );

//reset colors to the default values
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

//load skin settings
	string skinconfname = "skins/"+skin+"/skin.conf";
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

				if (value.length()>0) {
					if (value.length()>1 && value.at(0)=='"' && value.at(value.length()-1)=='"')
						skinConfStr[name] = value.substr(1,value.length()-2);
					else if (value.at(0) == '#') {
						// skinConfColor[name] = strtorgba( value.substr(1,value.length()) );
						skinConfColors[stringToColor(name)] = strtorgba( value.substr(1,value.length()) );
					}
					else
						skinConfInt[name] = atoi(value.c_str());
				}
			}
			skinconf.close();

			if (setWallpaper && !skinConfStr["wallpaper"].empty() && fileExists("skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"]))
				confStr["wallpaper"] = "skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"];
		}
	}

// (poor) HACK: ensure font alt colors have a default value
	if (skinConfColors[COLOR_FONT_ALT].r == 253 && skinConfColors[COLOR_FONT_ALT].g == 1 && skinConfColors[COLOR_FONT_ALT].b == 252 && skinConfColors[COLOR_FONT_ALT].a == 0) skinConfColors[COLOR_FONT_ALT] = skinConfColors[COLOR_FONT];
	if (skinConfColors[COLOR_FONT_ALT_OUTLINE].r == 253 && skinConfColors[COLOR_FONT_ALT_OUTLINE].g == 1 && skinConfColors[COLOR_FONT_ALT_OUTLINE].b == 252 && skinConfColors[COLOR_FONT_ALT_OUTLINE].a == 0) skinConfColors[COLOR_FONT_ALT_OUTLINE] = skinConfColors[COLOR_FONT_OUTLINE];

	evalIntConf( &skinConfInt["topBarHeight"], 40, 32, resY);
	// evalIntConf( &skinConfInt["sectionBarHeight"], 200, 32, resY);
	evalIntConf( &skinConfInt["sectionBarWidth"], 40, 32, resX);
	evalIntConf( &skinConfInt["linkHeight"], 40, 32, resY);
	evalIntConf( &skinConfInt["linkItemHeight"], 40, 32, resY);
	evalIntConf( &skinConfInt["bottomBarHeight"], 16, 1, resY);
	evalIntConf( &skinConfInt["selectorX"], 142, 1, resX);
	evalIntConf( &skinConfInt["selectorPreviewX"], 7, 1, resX);
	evalIntConf( &skinConfInt["selectorPreviewY"], 56, 1, resY);
	evalIntConf( &skinConfInt["selectorPreviewWidth"], 128, 32, resY);
	evalIntConf( &skinConfInt["selectorPreviewHeight"], 128, 32, resX);
	evalIntConf( &skinConfInt["fontSize"], 9, 6, 60);
	evalIntConf( &skinConfInt["titleFontSize"], 14, 6, 60);

//recalculate some coordinates based on the new element sizes
	// linkRows = resY/skinConfInt["linkItemHeight"];
	// needed until refactor menu.cpp
	linkColumns = 1;//(resX-10)/skinConfInt["linkWidth"];
	linkRows = resY / skinConfInt["linkItemHeight"];
	// linkRows = linksRect.h / skinConfInt["linkItemHeight"];

	if (menu != NULL) menu->loadIcons();

//font
	initFont();
}

void GMenu2X::activateSdUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		MessageBox mb(this,tr["USB Enabled (SD)"],"icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usbon.sh nand");
	}
}

void GMenu2X::activateNandUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh nand");
		MessageBox mb(this,tr["USB Enabled (Nand)"],"icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh nand");
	}
}

void GMenu2X::activateRootUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh root");
		MessageBox mb(this,tr["USB Enabled (Root)"],"icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh root");
	}
}

void GMenu2X::contextMenu() {
	vector<MenuOption> voices;

	if (menu->selLinkApp()!=NULL) {
		voices.push_back((MenuOption){tr.translate("Edit $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::editLink)});
		voices.push_back((MenuOption){tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL), MakeDelegate(this, &GMenu2X::deleteLink)});
	}
	voices.push_back((MenuOption){tr["Add link"], 		MakeDelegate(this, &GMenu2X::addLink)});
	voices.push_back((MenuOption){tr["Add section"],	MakeDelegate(this, &GMenu2X::addSection)});
	voices.push_back((MenuOption){tr["Rename section"],	MakeDelegate(this, &GMenu2X::renameSection)});
	voices.push_back((MenuOption){tr["Delete section"],	MakeDelegate(this, &GMenu2X::deleteSection)});
	voices.push_back((MenuOption){tr["Link scanner"],	MakeDelegate(this, &GMenu2X::scanner)});

	bool close = false;
	uint i, fadeAlpha=0;
	int sel=0;

	int h = font->getHeight();
	int h2 = font->getHalfHeight();
	SDL_Rect box;
	box.h = h*voices.size()+8;
	box.w = 0;
	for (i=0; i < voices.size(); i++) {
		int w = font->getTextWidth(voices[i].text);
		if (w > box.w) box.w = w;
	}
	box.w += 23;
	box.x = halfX - box.w/2;
	box.y = halfY - box.h/2;

	SDL_Rect selbox = {box.x+4, 0, box.w-8, h};
	long tickStart = SDL_GetTicks();

	Surface bg(s);
	input.setWakeUpInterval(40); //25FPS

	while (!close) {
		tickNow = SDL_GetTicks();

		selbox.y = box.y+4+h*sel;
		bg.blit(s,0,0);

		if (fadeAlpha<200)
			fadeAlpha = intTransition(0,200,tickStart,500,tickNow);
		else
			input.setWakeUpInterval(0);
		s->box(0, 0, resX, resY, 0,0,0,fadeAlpha);
		s->box(box.x, box.y, box.w, box.h, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		s->rectangle( box.x+2, box.y+2, box.w-4, box.h-4, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );

	//draw selection rect
		s->box( selbox.x, selbox.y, selbox.w, selbox.h, skinConfColors[COLOR_MESSAGE_BOX_SELECTION] );
		for (i=0; i<voices.size(); i++)
			s->write( font, voices[i].text, box.x+12, box.y+h2+3+h*i, HAlignLeft, VAlignMiddle );
		s->flip();

#if defined(TARGET_GP2X)
		//touchscreen
		if (f200) {
			ts.poll();
			if (ts.released()) {
				if (!ts.inRect(box))
					close = true;
				else if (ts.getX() >= selbox.x
					&& ts.getX() <= selbox.x + selbox.w)
					for (i=0; i<voices.size(); i++) {
						selbox.y = box.y+4+h*i;
						if (ts.getY() >= selbox.y
							&& ts.getY() <= selbox.y + selbox.h) {
							voices[i].action();
						close = true;
						i = voices.size();
					}
				}
			} else if (ts.pressed() && ts.inRect(box)) {
				for (i=0; i<voices.size(); i++) {
					selbox.y = box.y+4+h*i;
					if (ts.getY() >= selbox.y
						&& ts.getY() <= selbox.y + selbox.h) {
						sel = i;
					i = voices.size();
					}
				}
			}
		}
#endif
		input.update();
	// COMMON ACTIONS
		if ( input.isActive(MODIFIER) ) {
			if (input.isActive(SECTION_NEXT)) {
				if (!saveScreenshot()) { ERROR("Can't save screenshot"); continue; }
				MessageBox mb(this, tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
			} else if (input.isActive(SECTION_PREV)) {
				int vol = getVolume();
				if (vol) {
					vol = 0;
					volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					volumeMode = VOLUME_MODE_NORMAL;
				}
				confInt["globalVolume"] = vol;
				setVolume(vol);
				writeConfig();
			}
		}
		// BACKLIGHT
		else if ( input[BACKLIGHT] ) setBacklight(confInt["backlight"], true);
// END OF COMMON ACTIONS
		else if ( input[MENU]    ) close = true;
		else if ( input[UP]      ) sel = (sel-1 < 0) ? (int)voices.size()-1 : sel -1 ;
		else if ( input[DOWN]    ) sel = (sel+1 > (int)voices.size()-1) ? 0 : sel + 1;
		else if ( input[CONFIRM] ) { voices[sel].action(); close = true; }
		else if ( input[LEFT]  || input[PAGEUP]   ) sel = 0;
		else if ( input[RIGHT] || input[PAGEDOWN] ) sel = (int)voices.size()-1;
	}
	input.setWakeUpInterval(0);
}

void GMenu2X::changeWallpaper() {
	WallpaperDialog wp(this);
	if (wp.exec() && confStr["wallpaper"] != wp.wallpaper) {
		confStr["wallpaper"] = wp.wallpaper;
		initBG();
		writeConfig();
	}
}

bool GMenu2X::saveScreenshot() {
	ledOn();
	uint x = 0;
	stringstream ss;
	string fname;
	
	mkdir("screenshots/",0777);

	do {
		x++;
		fname = "";
		ss.clear();
		ss << x;
		ss >> fname;
		fname = "screenshots/screen"+fname+".bmp";
	} while (fileExists(fname));
	x = SDL_SaveBMP(s->raw, fname.c_str());
	sync();
	ledOff();
	return x == 0;
}

void GMenu2X::addLink() {
	FileDialog fd(this,tr["Select an application"],"","",tr["File Dialog"]);
	if (fd.exec()) {
		ledOn();
		menu->addLink(fd.getPath(), fd.getFile());
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

	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManual();
	string linkParams = menu->selLinkApp()->getParams();
	string linkSelFilter = menu->selLinkApp()->getSelectorFilter();
	string linkSelDir = menu->selLinkApp()->getSelectorDir();
	bool linkSelBrowser = menu->selLinkApp()->getSelectorBrowser();
	bool linkUseRamTimings = menu->selLinkApp()->getUseRamTimings();
	string linkSelScreens = menu->selLinkApp()->getSelectorScreens();
	string linkSelAliases = menu->selLinkApp()->getAliasFile();
	int linkClock = menu->selLinkApp()->clock();
	int linkVolume = menu->selLinkApp()->volume();
	string linkBackdrop = menu->selLinkApp()->getBackdrop();
	//G
	int linkGamma = menu->selLinkApp()->gamma();

	string diagTitle = tr.translate("Edit $1", linkTitle.c_str(), NULL);
	string diagIcon = menu->selLinkApp()->getIconPath();

	string strClock;
	stringstream ss;
	ss << DEFAULT_CPU_CLK;
	ss >> strClock;

	SettingsDialog sd(this, input, ts, diagTitle, diagIcon);
	sd.addSetting(new MenuSettingString(      this, tr["Title"],                tr["Link title"], &linkTitle, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingString(      this, tr["Description"],          tr["Link description"], &linkDescription, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingMultiString( this, tr["Section"],              tr["The section this link belongs to"], &newSection, &menu->getSections() ));
	sd.addSetting(new MenuSettingImage(       this, tr["Icon"],                 tr["Select an icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg", dir_name(linkIcon) ));
	sd.addSetting(new MenuSettingFile(        this, tr["Manual"],               tr["Select a graphic/textual manual or a readme"], &linkManual, ".man.png,.txt", dir_name(linkManual)));

	//sd.addSetting(new MenuSettingInt(         this, tr.translate("Clock (default: $1)","528", NULL), tr["Cpu clock frequency to set when launching this link"], &linkClock, 50, confInt["maxClock"] ));
	sd.addSetting(new MenuSettingInt(         this, tr["CPU Clock"], tr["CPU clock frequency to set when launching this link"], &linkClock, 528, 528, 750));
	//sd.addSetting(new MenuSettingBool(        this, tr["Tweak RAM Timings"],    tr["This usually speeds up the application at the cost of stability"], &linkUseRamTimings ));
	//sd.addSetting(new MenuSettingInt(         this, tr["Volume"],               tr["Volume to set for this link"], &linkVolume, 0, 1 ));
	sd.addSetting(new MenuSettingString(      this, tr["Parameters"],           tr["Parameters to pass to the application"], &linkParams, diagTitle, diagIcon ));

	sd.addSetting(new MenuSettingBool(        this, tr["Selector Browser"],     tr["Allow the selector to change directory"], &linkSelBrowser ));
	sd.addSetting(new MenuSettingDir(         this, tr["Selector Directory"],   tr["Directory to scan for the selector"], &linkSelDir, real_path(linkSelDir) ));
	sd.addSetting(new MenuSettingString(      this, tr["Selector Filter"],      tr["Filter for the selector (separate with commas)"], &linkSelFilter, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingDir(         this, tr["Selector Screenshots"], tr["Directory of the screenshots for the selector"], &linkSelScreens, dir_name(linkSelScreens) ));
	sd.addSetting(new MenuSettingFile(        this, tr["Selector Aliases"],     tr["File containing a list of aliases for the selector"], &linkSelAliases, dir_name(linkSelAliases) ));
	sd.addSetting(new MenuSettingImage(       this, tr["Backdrop"],             tr["Select an image backdrop"], &linkBackdrop, ".png,.bmp,.jpg,.jpeg", real_path(linkBackdrop)));

#	if defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	bool linkUseGinge = menu->selLinkApp()->getUseGinge();
	string ginge_prep = getExePath() + "/ginge/ginge_prep";
	if (fileExists(ginge_prep))
		sd.addSetting(new MenuSettingBool(        this, tr["Use Ginge"],            tr["Compatibility layer for running GP2X applications"], &linkUseGinge ));
#	endif

	//G
	//sd.addSetting(new MenuSettingInt(         this, tr["Gamma (default: 0)"],   tr["Gamma value to set when launching this link"], &linkGamma, 0, 100 ));
	sd.addSetting(new MenuSettingBool(        this, tr["Wrapper"],              tr["Relaunch GMenu2X after this link's execution ends"], &menu->selLinkApp()->needsWrapperRef() ));
	//sd.addSetting(new MenuSettingBool(        this, tr["Don't Leave"],          tr["Don't quit GMenu2X when launching this link"], &menu->selLinkApp()->runsInBackgroundRef() ));

	if (sd.exec() && sd.edited()) {
		ledOn();

		menu->selLinkApp()->setTitle(linkTitle);
		menu->selLinkApp()->setDescription(linkDescription);
		menu->selLinkApp()->setIcon(linkIcon);
		menu->selLinkApp()->setManual(linkManual);
		menu->selLinkApp()->setParams(linkParams);
		menu->selLinkApp()->setSelectorFilter(linkSelFilter);
		menu->selLinkApp()->setSelectorDir(linkSelDir);
		menu->selLinkApp()->setSelectorBrowser(linkSelBrowser);
		menu->selLinkApp()->setUseRamTimings(linkUseRamTimings);
		menu->selLinkApp()->setSelectorScreens(linkSelScreens);
		menu->selLinkApp()->setAliasFile(linkSelAliases);
		menu->selLinkApp()->setBackdrop(linkBackdrop);

		menu->selLinkApp()->setClock(linkClock);
		menu->selLinkApp()->setVolume(linkVolume);
		//G
		menu->selLinkApp()->setGamma(linkGamma);

#		if defined(TARGET_WIZ) || defined(TARGET_CAANOO)
		menu->selLinkApp()->setUseGinge(linkUseGinge);
#		endif

		INFO("New Section: '%s'", newSection.c_str());

		//if section changed move file and update link->file
		if (oldSection != newSection) {
			vector<string>::const_iterator newSectionIndex = find(menu->getSections().begin(), menu->getSections().end(), newSection);
			if (newSectionIndex == menu->getSections().end()) return;
			string newFileName = "sections/" + newSection + "/" + linkTitle;
			uint x = 2;
			while (fileExists(newFileName)) {
				string id = "";
				stringstream ss; ss << x; ss >> id;
				newFileName = "sections/" + newSection + "/" + linkTitle + id;
				x++;
			}
			rename(menu->selLinkApp()->getFile().c_str(),newFileName.c_str());
			menu->selLinkApp()->renameFile(newFileName);

			INFO("New section index: %i.", newSectionIndex - menu->getSections().begin());

			menu->linkChangeSection(menu->selLinkIndex(), menu->selSectionIndex(), newSectionIndex - menu->getSections().begin());
		}
		menu->selLinkApp()->save();
		sync();

		ledOff();
	}
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
	InputDialog id(this, input, ts, tr["Insert a name for the new section"], "", tr["Add section"]);
	if (id.exec()) {
		//only if a section with the same name does not exist
		if (find(menu->getSections().begin(), menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			//section directory doesn't exists
				ledOn();
			if (menu->addSection(id.getInput())) {
				menu->setSectionIndex( menu->getSections().size()-1 ); //switch to the new section
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::renameSection() {
	InputDialog id(this, input, ts, tr["Insert a new name for this section"],menu->selSection(),tr ["Rename section"]);
	if (id.exec()) {
		//only if a section with the same name does not exist & !samename
		if (menu->selSection() != id.getInput()
			&& find(menu->getSections().begin(),menu->getSections().end(), id.getInput())
			== menu->getSections().end()) {
			//section directory doesn't exists
			string newsectiondir = "sections/" + id.getInput();
			string sectiondir = "sections/" + menu->selSection();
			ledOn();
			if (rename(sectiondir.c_str(), "tmpsection")==0 && rename("tmpsection", newsectiondir.c_str())==0) {
				string oldpng = sectiondir+".png", newpng = newsectiondir+".png";
				string oldicon = sc.getSkinFilePath(oldpng), newicon = sc.getSkinFilePath(newpng);
				if (!oldicon.empty() && newicon.empty()) {
					newicon = oldicon;
					newicon.replace(newicon.find(oldpng), oldpng.length(), newpng);

					if (!fileExists(newicon)) {
						rename(oldicon.c_str(), "tmpsectionicon");
						rename("tmpsectionicon", newicon.c_str());
						sc.move("skin:"+oldpng, "skin:"+newpng);
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
	MessageBox mb(this, tr["All links in this section will be removed."] + "\n" + tr["Are you sure?"]);
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

void GMenu2X::scanner() {
	initBG();

	drawTopBar(bg);
	titlefont->setColor(skinConfColors[COLOR_FONT_ALT])->setOutlineColor(skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	bg->write(titlefont, tr["Link scanner"], 40, 4 + titlefont->getHeight()/2, HAlignLeft, VAlignMiddle);
	titlefont->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);
	font->setColor(skinConfColors[COLOR_FONT_ALT])->setOutlineColor(skinConfColors[COLOR_FONT_ALT_OUTLINE]);
	bg->write(font, tr["Scan for applications and games"], 40, 38, HAlignLeft, VAlignBottom);
	font->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);
	sc.skinRes("icons/configure.png")->blit(bg, 4, 4, 32, 32);

	drawBottomBar(bg);
	drawButton(bg, "b", tr["Back"], 5);

	bg->box(listRect, skinConfColors[COLOR_LIST_BG]);


	// Surface bg(bg);
	// bg.write(font,tr["Link Scanner"],halfX,7,HAlignCenter,VAlignMiddle);

#if defined(TARGET_RS97)
	uint lineY = 80;
#else
	uint lineY = 42;
#endif

	if (confInt["menuClock"] < DEFAULT_CPU_CLK) {
		setClock(DEFAULT_CPU_CLK);
		string strClock;
		stringstream ss;
		ss << DEFAULT_CPU_CLK;
		ss >> strClock;
		bg->write(font,tr.translate("Raising cpu clock to $1Mhz", strClock.c_str(), NULL),5,lineY);
		bg->blit(s,0,0);
		s->flip();
		lineY += 26;
	}

	bg->write(font,tr["Scanning SD filesystem..."],5,lineY);
	bg->blit(s,0,0);
	s->flip();
	lineY += 26;

	vector<string> files;
	scanPath("/mnt/sd",&files);

	//Onyl gph firmware has nand
	if (fwType=="gph" && !f200) {
		bg->write(font,tr["Scanning NAND filesystem..."],5,lineY);
		bg->blit(s,0,0);
		s->flip();
		lineY += 26;
		scanPath("/mnt/nand",&files);
	}

	stringstream ss;
	ss << files.size();
	string str = "";
	ss >> str;
	bg->write(font,tr.translate("$1 files found.",str.c_str(),NULL),5,lineY);
	lineY += 26;
	bg->write(font,tr["Creating links..."],5,lineY);
	bg->blit(s,0,0);
	s->flip();
	lineY += 26;

	string path, file;
	string::size_type pos;
	uint linkCount = 0;

	ledOn();
	for (uint i = 0; i<files.size(); i++) {
		pos = files[i].rfind("/");
		if (pos!=string::npos && pos>0) {
			path = files[i].substr(0, pos+1);
			file = files[i].substr(pos+1, files[i].length());
			if (menu->addLink(path,file,"found "+file.substr(file.length()-3,3)))
				linkCount++;
		}
	}

	ss.clear();
	ss << linkCount;
	ss >> str;
	bg->write(font,tr.translate("$1 links created.",str.c_str(),NULL),5,lineY);
	bg->blit(s,0,0);
	s->flip();
	lineY += 26;

	if (confInt["menuClock"]<DEFAULT_CPU_CLK) {
		setClock(confInt["menuClock"]);
		bg->write(font,tr["Decreasing cpu clock"],5,lineY);
		bg->blit(s,0,0);
		s->flip();
		lineY += 26;
	}

	sync();
	ledOff();

	bool close = false;
	while (!close) {
		input.update();

// COMMON ACTIONS
		if ( input.isActive(MODIFIER) ) {
			if (input.isActive(SECTION_NEXT)) {
				if (!saveScreenshot()) { ERROR("Can't save screenshot"); continue; }
				MessageBox mb(this, tr["Screenshot Saved"]);
				mb.setAutoHide(1000);
				mb.exec();
			} else if (input.isActive(SECTION_PREV)) {
				int vol = getVolume();
				if (vol) {
					vol = 0;
					volumeMode = VOLUME_MODE_MUTE;
				} else {
					vol = 100;
					volumeMode = VOLUME_MODE_NORMAL;
				}
				confInt["globalVolume"] = vol;
				setVolume(vol);
				writeConfig();
			}
		}
		// BACKLIGHT
		else if ( input[BACKLIGHT] ) setBacklight(confInt["backlight"], true);
// END OF COMMON ACTIONS
		else if (input[SETTINGS] || input[CONFIRM] || input[CANCEL]) close = true;
	}
}

void GMenu2X::scanPath(string path, vector<string> *files) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath, ext;

	if (path[path.length()-1]!='/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0]=='.')
			continue;
		filepath = path+dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			scanPath(filepath, files);
		if (statRet != -1) {
			ext = filepath.substr(filepath.length()-4,4);
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
			if (ext==".gpu" || ext==".gpe")
#endif
				files->push_back(filepath);
		}
	}

	closedir(dirp);
}

unsigned short GMenu2X::getBatteryLevel() {
	//if (batteryHandle<=0) return 6; //AC Power
	long val = getBatteryStatus();

if (confStr["batteryType"] == "RS-97") {
	if ((val > 10000) || (val < 0)) return 6;
	else if (val > 4000) return 5; // 100%
	else if (val > 3900) return 4; // 80%
	else if (val > 3800) return 3; // 60%
	else if (val > 3730) return 2; // 40%
	else if (val > 3600) return 1; // 20%
	return 0; // 0% :(
}

#if defined(TARGET_GP2X)
	if (f200) {
		MMSP2ADC val;
		read(batteryHandle, &val, sizeof(MMSP2ADC));

		if (val.batt==0) return 5;
		if (val.batt==1) return 3;
		if (val.batt==2) return 1;
		if (val.batt==3) return 0;
		return 6;
	} else {
		int battval = 0;
		unsigned short cbv, min=900, max=0;

		for (int i = 0; i < BATTERY_READS; i ++) {
			if ( read(batteryHandle, &cbv, 2) == 2) {
				battval += cbv;
				if (cbv>max) max = cbv;
				if (cbv<min) min = cbv;
			}
		}

		battval -= min+max;
		battval /= BATTERY_READS-2;

		if (battval>=850) return 6;
		if (battval>780) return 5;
		if (battval>740) return 4;
		if (battval>700) return 3;
		if (battval>690) return 2;
		if (battval>680) return 1;
	}

#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	unsigned short cbv;
	if ( read(batteryHandle, &cbv, 2) == 2) {
		// 0=fail, 1=100%, 2=66%, 3=33%, 4=0%
		switch (cbv) {
			case 4: return 1;
			case 3: return 2;
			case 2: return 4;
			case 1: return 5;
			default: return 6;
		}
	}
#else

	bool needWriteConfig = false;
	long max = confInt["maxBattery"];
	long min = confInt["minBattery"];

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

	// return 5 - 5*(100-val)/(100);
	return -5*(max-val)/(max-min)+5;
#endif

}

void GMenu2X::setInputSpeed() {
	input.setInterval(150);
	input.setInterval(30,  VOLDOWN);
	input.setInterval(30,  VOLUP);
	input.setInterval(30,  CANCEL);
	input.setInterval(500, SETTINGS);
	input.setInterval(500, MENU);
	input.setInterval(300, CANCEL);
	input.setInterval(300, MANUAL);
	input.setInterval(200, INC);
	input.setInterval(200, DEC);
	input.setInterval(1000,CONFIRM);
	input.setInterval(150, SECTION_PREV);
	input.setInterval(150, SECTION_NEXT);
	input.setInterval(150, PAGEUP);
	input.setInterval(150, PAGEDOWN);
	input.setInterval(500, BACKLIGHT);
	input.setInterval(500, POWER);
}

void GMenu2X::applyRamTimings() {
#ifdef TARGET_GP2X
	// 6 4 1 1 1 2 2
	if (memdev!=0) {
		int tRC = 5, tRAS = 3, tWR = 0, tMRD = 0, tRFC = 0, tRP = 1, tRCD = 1;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
#endif
}

void GMenu2X::applyDefaultTimings() {
#ifdef TARGET_GP2X
	// 8 16 3 8 8 8 8
	if (memdev!=0) {
		int tRC = 7, tRAS = 15, tWR = 2, tMRD = 7, tRFC = 7, tRP = 7, tRCD = 7;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
#endif
}

void GMenu2X::setClock(unsigned mhz) {
	mhz = constrain(mhz,250,confInt["maxClock"]);
	if (memdev > 0) {
		DEBUG("Setting clock to %d", mhz);

#if defined(TARGET_RS97)
		#define CPPCR     (0x10 >> 2)
		unsigned long m = mhz / 6;
		memregs[CPPCR] = (m << 24) | 0x090520;
		INFO("Set CPU clock: %d", mhz);

#elif defined(TARGET_GP2X)
		unsigned v;
		unsigned mdiv, pdiv=3, scale=0;

		#define SYS_CLK_FREQ 7372800
		mhz *= 1000000;
		mdiv = (mhz * pdiv) / SYS_CLK_FREQ;
		mdiv = ((mdiv-8)<<8) & 0xff00;
		pdiv = ((pdiv-2)<<2) & 0xfc;
		scale &= 3;
		v = mdiv | pdiv | scale;
		MEM_REG[0x910>>1] = v;

#elif defined(TARGET_CAANOO) || defined(TARGET_WIZ)
		volatile unsigned int *memregl = static_cast<volatile unsigned int*>((volatile void*)memregs);
		int mdiv, pdiv = 9, sdiv = 0;
		unsigned long v;

		#define SYS_CLK_FREQ 27
		#define PLLSETREG0   (memregl[0xF004>>2])
		#define PWRMODE      (memregl[0xF07C>>2])
		mdiv = (mhz * pdiv) / SYS_CLK_FREQ;
		if (mdiv & ~0x3ff) return;
		v = pdiv<<18 | mdiv<<8 | sdiv;

		PLLSETREG0 = v;
		PWRMODE |= 0x8000;
		for (int i = 0; (PWRMODE & 0x8000) && i < 0x100000; i++);
#endif
	}
}

void GMenu2X::setGamma(int gamma) {
#ifdef TARGET_GP2X
	float fgamma = (float)constrain(gamma,1,100)/10;
	fgamma = 1 / fgamma;
	MEM_REG[0x2880>>1]&=~(1<<12);
	MEM_REG[0x295C>>1]=0;

	for (int i=0; i<256; i++) {
		unsigned char g = (unsigned char)(255.0*pow(i/255.0,fgamma));
		unsigned short s = (g<<8) | g;
		MEM_REG[0x295E>>1]= s;
		MEM_REG[0x295E>>1]= g;
	}
#endif
}

int GMenu2X::getVolume() {
	int vol = -1;
	unsigned long soundDev = open("/dev/mixer", O_RDONLY);

	if (soundDev) {
#if defined(TARGET_RS97)
		ioctl(soundDev, SOUND_MIXER_READ_VOLUME, &vol);
#else
		ioctl(soundDev, SOUND_MIXER_READ_PCM, &vol);
#endif
		close(soundDev);
		if (vol != -1) {
			//just return one channel , not both channels, they're hopefully the same anyways
			return vol & 0xFF;
		}
	}
	return vol;
}

void GMenu2X::setVolume(int vol) {
	vol = constrain(vol,0,100);
	unsigned long soundDev = open("/dev/mixer", O_RDWR);

	vol = vol ? 100 : 0;
	if (soundDev) {
		vol = (vol << 8) | vol;
#if defined(TARGET_RS97)
		ioctl(soundDev, SOUND_MIXER_WRITE_VOLUME, &vol);
#else
		ioctl(soundDev, SOUND_MIXER_WRITE_PCM, &vol);
#endif
		close(soundDev);

		if (vol) {
			volumeMode = VOLUME_MODE_NORMAL;
		}
		else{
			volumeMode = VOLUME_MODE_MUTE;
		}
	}
}

void GMenu2X::setVolumeScaler(int scale) {
	scale = constrain(scale,0,MAX_VOLUME_SCALE_FACTOR);
	unsigned long soundDev = open("/dev/mixer", O_WRONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE2, &scale);
		close(soundDev);
	}
}

int GMenu2X::getVolumeScaler() {
	int currentscalefactor = -1;
	unsigned long soundDev = open("/dev/mixer", O_RDONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE1, &currentscalefactor);
		close(soundDev);
	}
	return currentscalefactor;
}

const string &GMenu2X::getExePath() {
	if (path.empty()) {
		char buf[255];
		memset(buf, 0, 255);
		int l = readlink("/proc/self/exe", buf, 255);

		path = buf;
		path = path.substr(0,l);
		l = path.rfind("/");
		path = path.substr(0,l+1);
	}
	return path;
}

int GMenu2X::getBacklight() {
	char buf[32];

	FILE *f = fopen("/proc/jz/lcd_backlight", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
		return atoi(buf);
	}
	return -1;
}

string GMenu2X::getDiskFree(const char *path) {
	string df = "N/A";
	struct statvfs b;

	if (statvfs(path, &b) == 0) {
		// Make sure that the multiplication happens in 64 bits.
		unsigned long freeMiB = ((unsigned long long)b.f_bfree * b.f_bsize) / (1024 * 1024);
		unsigned long totalMiB = ((unsigned long long)b.f_blocks * b.f_frsize) / (1024 * 1024);
		stringstream ss;
		if (totalMiB >= 10000) {
			ss << (freeMiB / 1024) << "." << ((freeMiB % 1024) * 10) / 1024 << "/"
			   << (totalMiB / 1024) << "." << ((totalMiB % 1024) * 10) / 1024 << "GiB";
		} else {
			ss << freeMiB << "/" << totalMiB << "MiB";
		}
		ss >> df;
	} else WARNING("statvfs failed with error '%s'.\n", strerror(errno));
	return df;
}

int GMenu2X::drawButton(Button *btn, int x, int y) {
	if (y<0) y = resY+y;
	btn->setPosition(x, y-7);
	btn->paint();
	return x+btn->getRect().w+6;
}

int GMenu2X::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y<0) y = resY+y;
	SDL_Rect re = {x, y-7, 0, 16};
	if (sc.skinRes("imgs/buttons/"+btn+".png") != NULL) {
		sc["imgs/buttons/"+btn+".png"]->blit(s, x, y-7);
		re.w = sc["imgs/buttons/"+btn+".png"]->raw->w+3;
		font->setColor(skinConfColors[COLOR_FONT_ALT])->setOutlineColor(skinConfColors[COLOR_FONT_ALT_OUTLINE]);
		s->write(font, text, x+re.w, y, HAlignLeft, VAlignMiddle);
		font->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);
		re.w += font->getTextWidth(text);
	}
	return x+re.w+6;
}

int GMenu2X::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y<0) y = resY+y;
	if (sc.skinRes("imgs/buttons/"+btn+".png") != NULL) {
		x -= 16;
		sc["imgs/buttons/"+btn+".png"]->blit(s, x, y-7);
		x -= 3;
		font->setColor(skinConfColors[COLOR_FONT_ALT])->setOutlineColor(skinConfColors[COLOR_FONT_ALT_OUTLINE]);
		s->write(font, text, x, y, HAlignRight, VAlignMiddle);
		font->setColor(skinConfColors[COLOR_FONT])->setOutlineColor(skinConfColors[COLOR_FONT_OUTLINE]);
		return x-6-font->getTextWidth(text);
	}
	return x-6;
}

void GMenu2X::drawScrollBar(uint pagesize, uint totalsize, uint pagepos, SDL_Rect scrollRect) {
	if (totalsize <= pagesize) return;

	//internal bar total height = height-2
	//bar size
	uint bs = (scrollRect.h - 4) * pagesize / totalsize;
	//bar y position
	uint by = (scrollRect.h - 4) * pagepos / totalsize;
	by = scrollRect.y + 4 + by;
	if ( by + bs > scrollRect.y + scrollRect.h - 4) by = scrollRect.y + scrollRect.h - 4 - bs;

	s->rectangle(scrollRect.x + scrollRect.w - 5, by, 5, bs, skinConfColors[COLOR_LIST_BG]);
	s->box(scrollRect.x + scrollRect.w - 4, by + 1, 2, bs - 2, skinConfColors[COLOR_SELECTION_BG]);
}

void GMenu2X::drawTopBar(Surface *s) {
	if (s==NULL) s = this->s;

	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL)
	// 	bar->blit(s, 0, 0);
	// else
	s->box(0, 0, resX, skinConfInt["topBarHeight"], skinConfColors[COLOR_TOP_BAR_BG]);
}

void GMenu2X::drawBottomBar(Surface *s) {
	if (s==NULL) s = this->s;

	// Surface *bar = sc.skinRes("imgs/bottombar.png");
	// if (bar != NULL)
	// 	bar->blit(s, 0, resY-bar->raw->h);
	// else
	s->box(0, resY - skinConfInt["bottomBarHeight"], resX, skinConfInt["bottomBarHeight"], skinConfColors[COLOR_BOTTOM_BAR_BG]);
}
