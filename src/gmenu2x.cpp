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

#include <sys/fcntl.h> //for battery

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

//for soundcard
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "linkapp.h"
// #include "linkaction.h"
#include "menu.h"
#include "fonthelper.h"
#include "surface.h"
#include "filedialog.h"
#include "powermanager.h"
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

#include "batteryloggerdialog.h"
#include "linkscannerdialog.h"
#include "menusettingdatetime.h"


#include "debug.h"

#include <sys/mman.h>

#include <ctime>
#include <sys/time.h>   /* for settimeofday() */

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

enum mmc_status{
	MMC_REMOVE, MMC_INSERT, MMC_ERROR
};

mmc_status getMMCStatus(void) {
#if defined(TARGET_RS97)
	char buf[32] = {0};

	FILE *f = fopen("/proc/jz/mmc", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);

		if (memcmp(buf, "REMOVE", 6) == 0) {
			return MMC_REMOVE;
		} else if (memcmp(buf, "INSERT", 6) == 0) {
			return MMC_INSERT;
		}
	}
#endif
	return MMC_ERROR;
}

enum udc_status{
	UDC_REMOVE, UDC_CONNECT, UDC_ERROR
};

udc_status getUDCStatus(void) {
	char buf[32] = {0};

#if defined(TARGET_RS97)
	FILE *f = fopen("/proc/jz/udc", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);

		if (memcmp(buf, "REMOVE", 6) == 0) {
			return UDC_REMOVE;
		} else if (memcmp(buf, "CONNECT", 6) == 0) {
			return UDC_CONNECT;
		}
	}
// #else
//   FILE *f = fopen("/sys/devices/platform/musb_hdrc.0/uh_cable", "r");
//   fgets(buf, sizeof(buf), f);
//   fclose(f);

//   if (memcmp(buf, "offline", 7) == 0) {
// 	return UDC_REMOVE;
//   }
//   else if (memcmp(buf, "usb", 3) == 0) {
// 	return UDC_CONNECT;
//   }
#endif
	return UDC_ERROR;
}

int udcConnectedOnBoot;


int main(int /*argc*/, char * /*argv*/[]) {
	INFO("GMenu2X starting: If you read this message in the logs, check http://mtorromeo.github.com/gmenu2x/troubleshooting.html for a solution");

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);

	// system("df &"); // HACK: try to cache disk free information on background.
	// system("nohup sleep 10 &"); // HACK: try to cache disk free information on background.

	udcConnectedOnBoot = getUDCStatus();

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
#if defined(TARGET_GP2X)
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
#if defined(TARGET_GP2X)
	if (memdev > 0) {
		memregs[0x28DA >> 1] = 0x4AB;
		memregs[0x290C >> 1] = 640;
	}
	if (f200) ts.deinit();
	if (batteryHandle != 0) close(batteryHandle);
#endif

	if (memdev > 0) {
		memregs = NULL;
		close(memdev);
	}
}

#if defined(TARGET_GP2X)
void GMenu2X::gp2x_tvout_on(bool pal) {
	if (memdev!=0) {
		/*Ioctl_Dummy_t *msg;
		#define FBMMSP2CTRL 0x4619
		int TVHandle = ioctl(SDL_videofd, FBMMSP2CTRL, msg);*/
		if (cx25874!=0) gp2x_tvout_off();
		//if tv-out is enabled without cx25874 open, stop
		//if (memregs[0x2800 >> 1]&0x100) return;
		cx25874 = open("/dev/cx25874",O_RDWR);
		ioctl(cx25874, _IOW('v', 0x02, unsigned char), pal ? 4 : 3);
		memregs[0x2906 >> 1] = 512;
		memregs[0x28E4 >> 1] = memregs[0x290C >> 1];
		memregs[0x28E8 >> 1] = 239;
	}
}
#endif

#if defined(TARGET_GP2X)
void GMenu2X::gp2x_tvout_off() {
	if (memdev != 0) {
		close(cx25874);
		cx25874 = 0;
		memregs[0x2906 >> 1] = 1024;
	}
}
#endif

GMenu2X::GMenu2X() {
	//Detect firmware version and type
	if (fileExists("/etc/open2x")) {
		fwType = "open2x";
		// fwVersion = "";
	} else {
		fwType = "gph";
		// fwVersion = "";
	}
#if defined(TARGET_GP2X)
	f200 = fileExists("/dev/touchscreen/wm97xx");

	//open2x
	savedVolumeMode = 0;
	volumeScalerNormal = VOLUME_SCALER_NORMAL;
	volumeScalerPhones = VOLUME_SCALER_PHONES;

	o2x_usb_net_on_boot = false;
	o2x_usb_net_ip = "";
	o2x_ftp_on_boot = false;
	o2x_telnet_on_boot = false;
	o2x_gp2xjoy_on_boot = false;
	o2x_usb_host_on_boot = false;
	o2x_usb_hid_on_boot = false;
	o2x_usb_storage_on_boot = false;

	usbnet = samba = inet = web = false;

	// useSelectionPng = false;
#elif defined(TARGET_RS97)
	fwType = "rs97";
#else
	f200 = true;
#endif

	volumeMode = VOLUME_MODE_NORMAL;

	//load config data
	readConfig();

#if defined(TARGET_GP2X)
	if (fwType=="open2x") {
		readConfigOpen2x();
		//	VOLUME MODIFIER
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones);	break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
	}
	readCommonIni();
	cx25874 = 0;
	batteryHandle = 0;
#endif

	halfX = resX/2;
	halfY = resY/2;
	// bottomBarIconY = resY-18;

	path = "";
	getExePath();

	memdev = 0;

#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO) || defined(TARGET_RS97)
	gp2x_init();
#endif

#if defined(TARGET_GP2X)
	//Fix tv-out
	if (memdev > 0) {
		if (memregs[0x2800 >> 1] & 0x100) {
			memregs[0x2906 >> 1] = 512;
			//memregs[0x290C >> 1]=640;
			memregs[0x28E4 >> 1] = memregs[0x290C >> 1];
		}
		memregs[0x28E8 >> 1] = 239;
	}
#endif

#if !defined(TARGET_PC)
	setenv("SDL_NOMOUSE", "1", 1);
#endif

	setDateTime();

	//Screen
	if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK) < 0 ) {
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
	s->raw = SDL_CreateRGBSurface(SDL_SWSURFACE, resX, resY, confInt["videoBpp"], 0, 0, 0, 0);

#else
	s->raw = SDL_SetVideoMode(resX, resY, confInt["videoBpp"], SDL_HWSURFACE|SDL_DOUBLEBUF);
#endif

	bg = NULL;
	// btnContextMenu = NULL;
	font = NULL;
	menu = NULL;

	if (!fileExists(confStr["wallpaper"])) {
		DEBUG("Searching wallpaper");

		FileLister fl("skins/" + confStr["skin"] + "/wallpapers", false, true);
		fl.setFilter(".png,.jpg,.jpeg,.bmp");
		fl.browse();
		if (fl.getFiles().size() <= 0 && confStr["skin"] != "Default")
			fl.setPath("skins/Default/wallpapers", true);
		if (fl.getFiles().size() > 0)
			confStr["wallpaper"] = fl.getPath() + "/" + fl.getFiles()[0];
	}

	setSkin(confStr["skin"], false, false);

	sc[confStr["wallpaper"]]->blit(s,0,0);

	powerManager = new PowerManager(this, confInt["backlightTimeout"], confInt["powerTimeout"]);

	MessageBox mb(this,tr["Loading..."]);
	mb.setAutoHide(1);
	mb.exec();

	setBacklight(confInt["backlight"]);

	initBG();

	initLayout();

	initMenu();

	input.init(path + "input.conf");
	setInputSpeed();
	// input.setWakeUpInterval(1000);

#if defined(TARGET_GP2X)
	initServices();
	setGamma(confInt["gamma"]);
	applyDefaultTimings();
#endif

	// setVolume(confInt["globalVolume"]);
	setCPU(confInt["cpuMenu"]);
	// setCPU(CPU_CLK_DEFAULT);
	//recover last session
	readTmp();

	tickSuspend = 0;

	if (lastSelectorElement >- 1 && menu->selLinkApp() != NULL && (!menu->selLinkApp()->getSelectorDir().empty() || !lastSelectorDir.empty()))
		menu->selLinkApp()->selector(lastSelectorElement,lastSelectorDir);
}

GMenu2X::~GMenu2X() {
	confStr["datetime"] = getDateTime();

	writeConfig();

#if defined(TARGET_GP2X)
	if (fwType=="open2x") writeConfigOpen2x();
#endif
	quit();

	delete menu;
	delete s;
	delete font;
	delete titlefont;
}

void GMenu2X::quit() {
	fflush(NULL);
	sc.clear();
	s->free();
	SDL_Quit();
#if defined(TARGET_GP2X)
	if (memdev != 0) {
		//Fix tv-out
		if (memregs[0x2800 >> 1] & 0x100) {
			memregs[0x2906 >> 1] = 512;
			memregs[0x28E4 >> 1] = memregs[0x290C >> 1];
		}
		gp2x_deinit();
	}
#elif defined(TARGET_RETROGMAE)
	gp2x_deinit();
#endif
}

void GMenu2X::initBG(const string &imagePath) {
	// sc.del("bgmain");

	if (bg != NULL) delete bg;

	bg = new Surface(s);
	bg->box(0, 0, resX, resY, 0, 0, 0);

	if (!imagePath.empty() && sc.add(imagePath) != NULL) {
		// Surface wall(imagePath, false);
		// wall.blit(bg, 0, 0);
		sc[imagePath]->blit(bg, 0, 0);
	} else if (sc.add(confStr["wallpaper"]) != NULL) {
		sc[confStr["wallpaper"]]->blit(bg, 0, 0);
		// Surface wall(confStr["wallpaper"], false);
		// wall.blit(bg, 0, 0);
	}
}

void GMenu2X::initLayout() {
	// LINKS rect
	linksRect = (SDL_Rect){0, 0, resX, resY};
	sectionBarRect = (SDL_Rect){0, 0, resX, resY};

	if (confInt["sectionBar"]) {
		// x = 0; y = 0;
		if (confInt["sectionBar"] == SB_LEFT || confInt["sectionBar"] == SB_RIGHT) {
			sectionBarRect.x = (confInt["sectionBar"] == SB_RIGHT)*(resX - skinConfInt["sectionBarSize"]);
			sectionBarRect.w = skinConfInt["sectionBarSize"];
			linksRect.w = resX - skinConfInt["sectionBarSize"];

			if (confInt["sectionBar"] == SB_LEFT) {
				linksRect.x = skinConfInt["sectionBarSize"];
			}
		} else {
			sectionBarRect.y = (confInt["sectionBar"] == SB_BOTTOM)*(resY - skinConfInt["sectionBarSize"]);
			sectionBarRect.h = skinConfInt["sectionBarSize"];
			linksRect.h = resY - skinConfInt["sectionBarSize"];

			if (confInt["sectionBar"] == SB_TOP) {
				linksRect.y = skinConfInt["sectionBarSize"];
			}
		}
	}

	listRect = (SDL_Rect){0, skinConfInt["topBarHeight"], resX, resY - skinConfInt["bottomBarHeight"] - skinConfInt["topBarHeight"]};

	// Surface *bgmain = new Surface(bg);
	// sc.add(bgmain,"bgmain");

	//Surface sd("imgs/sd.png", confStr["skin"]);
	// Surface cpu("imgs/cpu.png", confStr["skin"]);
	//Surface volume("imgs/volume.png", confStr["skin"]);
	//string df = getDiskFree();

  //sd.blit( sc["bgmain"], 3, bottomBarIconY );
	//sc["bgmain"]->write( font, df, 22, bottomBarTextY, HAlignLeft, VAlignMiddle );
	//volumeX = 27+font->getTextWidth(df);
	//volume.blit( sc["bgmain"], volumeX, bottomBarIconY );
	//volumeX += 19;
	//cpuX = volumeX+font->getTextWidth("1")+5;
 //  cpuX = 3;
	// cpu.blit( sc["bgmain"], cpuX, bottomBarIconY );
	// cpuX += 19;
	// manualX = cpuX+font->getTextWidth("300Mhz")+5;

// #if defined(TARGET_GP2X)
// 	int serviceX = resX-38;
// 	if (usbnet) {
// 		if (web) {
// 			Surface webserver("imgs/webserver.png", confStr["skin"]);
// 			webserver.blit( sc["bgmain"], serviceX, bottomBarIconY );
// 			serviceX -= 19;
// 		}
// 		if (samba) {
// 			Surface sambaS("imgs/samba.png", confStr["skin"]);
// 			sambaS.blit( sc["bgmain"], serviceX, bottomBarIconY );
// 			serviceX -= 19;
// 		}
// 		if (inet) {
// 			Surface inetS("imgs/inet.png", confStr["skin"]);
// 			inetS.blit( sc["bgmain"], serviceX, bottomBarIconY );
// 			serviceX -= 19;
// 		}
// 	}
// #endif
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
	titlefont = new FontHelper(sc.getSkinFilePath("font.ttf"), skinConfInt["fontSizeTitle"], skinConfColors[COLOR_FONT], skinConfColors[COLOR_FONT_OUTLINE]);
}

void GMenu2X::initMenu() {
	//Menu structure handler
	menu = new Menu(this);
	for (uint i=0; i < menu->getSections().size(); i++) {
		//Add virtual links in the applications section
		if (menu->getSections()[i]=="applications") {
			menu->addActionLink(i, tr["Explorer"], MakeDelegate(this, &GMenu2X::explorer), tr["Launch an application"], "skin:icons/explorer.png");
#if !defined(TARGET_PC)
			if (getBatteryLevel() > 5) // show only if charging
#endif
				menu->addActionLink(i, tr["Battery Logger"], MakeDelegate(this, &GMenu2X::batteryLogger), tr["Log battery power to battery.csv"], "skin:icons/ebook.png");
		}

		//Add virtual links in the setting section
		else if (menu->getSections()[i]=="settings") {
			menu->addActionLink(i, tr["Settings"], MakeDelegate(this, &GMenu2X::settings), tr["Configure settings"], "skin:icons/configure.png");
			menu->addActionLink(i, tr["Skin"], MakeDelegate(this, &GMenu2X::skinMenu), tr["Configure skin"], "skin:icons/skin.png");
			menu->addActionLink(i, tr["Wallpaper"], MakeDelegate(this, &GMenu2X::changeWallpaper), tr["Set background image"], "skin:icons/wallpaper.png");
#if defined(TARGET_GP2X)
			if (fwType=="open2x")
				menu->addActionLink(i, "Open2x", MakeDelegate(this, &GMenu2X::settingsOpen2x), tr["Configure Open2x system settings"], "skin:icons/o2xconfigure.png");
			// menu->addActionLink(i, "TV", MakeDelegate(this, &GMenu2X::toggleTvOut), tr["Activate/deactivate tv-out"], "skin:icons/tv.png");
			menu->addActionLink(i, "USB SD", MakeDelegate(this, &GMenu2X::activateSdUsb), tr["Activate USB on SD"], "skin:icons/usb.png");
			if (fwType=="gph" && !f200)
				menu->addActionLink(i, "USB Nand", MakeDelegate(this, &GMenu2X::activateNandUsb), tr["Activate USB on NAND"], "skin:icons/usb.png");
			//menu->addActionLink(i, "USB Root", MakeDelegate(this, &GMenu2X::activateRootUsb), tr["Activate USB on the root of the Gp2x Filesystem"], "skin:icons/usb.png");
			//menu->addActionLink(i, "Speaker", MakeDelegate(this, &GMenu2X::toggleSpeaker), tr["Activate/deactivate Speaker"], "skin:icons/speaker.png");
#elif defined(TARGET_RS97)
			// menu->addActionLink(i, tr["TV"], MakeDelegate(this, &GMenu2X::toggleTvOut), tr["Activate/deactivate tv-out"], "skin:icons/tv.png");
			//menu->addActionLink(i, "Format", MakeDelegate(this, &GMenu2X::formatSd), tr["Format internal SD"], "skin:icons/format.png");
			menu->addActionLink(i, tr["Umount"], MakeDelegate(this, &GMenu2X::umountSd), tr["Umount external SD"], "skin:icons/eject.png");
#endif

			if (fileExists(path + "log.txt"))
				menu->addActionLink(i, tr["Log Viewer"], MakeDelegate(this, &GMenu2X::viewLog), tr["Displays last launched program's output"], "skin:icons/ebook.png");


			menu->addActionLink(i, tr["About"], MakeDelegate(this, &GMenu2X::about), tr["Info about system"], "skin:icons/about.png");
			// menu->addActionLink(i, "Reboot", MakeDelegate(this, &GMenu2X::reboot), tr["Reboot device"], "skin:icons/reboot.png");
			menu->addActionLink(i, tr["Power"], MakeDelegate(this, &GMenu2X::poweroffDialog), tr["Power menu"], "skin:icons/exit.png");
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
	// int d = (t % (86400 * 30)) / 86400;

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
	TextDialog td(this, "GMenuNext", tr["Info about system"], "skin:icons/about.png", &text);
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

	TextDialog td(this, tr["Log Viewer"], tr["Displays last launched program's output"], "skin:icons/ebook.png", &log);
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

void GMenu2X::batteryLogger() {
	BatteryLoggerDialog bl(this, tr["Battery Logger"], tr["Log battery power to battery.csv"], "skin:icons/ebook.png");
	bl.exec();
}

void GMenu2X::linkScanner() {
	LinkScannerDialog ls(this, tr["Link scanner"], tr["Scan for applications and games"], "skin:icons/configure.png");
	ls.exec();
}

void GMenu2X::changeWallpaper() {
	WallpaperDialog wp(this, tr["Wallpaper"], tr["Select an image to use as a wallpaper"], "skin:icons/wallpaper.png");
	if (wp.exec() && confStr["wallpaper"] != wp.wallpaper) {
		confStr["wallpaper"] = wp.wallpaper;
		initBG();
		writeConfig();
	}
}

void GMenu2X::readConfig() {
	string conffile = path+"gmenu2x.conf";

	// Defaults
	confStr["batteryType"] = "BL-5B";
	confStr["datetime"] = __BUILDTIME__;

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
	evalIntConf( &confInt["powerTimeout"], 10, 1, 300);
	evalIntConf( &confInt["outputLogs"], 0, 0, 1 );
// #if defined(TARGET_GP2X)
// 	evalIntConf( &confInt["cpuMax"], 300, 200, 300 );
// 	evalIntConf( &confInt["cpuMenu"], 140, 50, 300 );
// #elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
// 	evalIntConf( &confInt["cpuMax"], 900, 200, 900 );
// 	evalIntConf( &confInt["cpuMenu"], CPU_CLK_DEFAULT, 250, 300 );
// #elif defined(TARGET_RS97)
	evalIntConf( &confInt["cpuMax"], 642, 200, 1200 );
	evalIntConf( &confInt["cpuMin"], 252, 200, 1200 );
	evalIntConf( &confInt["cpuMenu"], 528, 200, 1200 );
// #endif
	evalIntConf( &confInt["globalVolume"], 60, 1, 100 );
	evalIntConf( &confInt["gamma"], 10, 1, 100 );
	evalIntConf( &confInt["videoBpp"], 16, 8, 32 );
	evalIntConf( &confInt["backlight"], 70, 1, 100);

	evalIntConf( &confInt["minBattery"], 0, 1, 10000);
	evalIntConf( &confInt["maxBattery"], 4500, 1, 10000);

	// evalIntConf( &confInt["sectionBar"], SB_LEFT, 1, 4);
	confInt["sectionBar"] = SB_LEFT;

	if (!confInt["saveSelection"]) {
		confInt["section"] = 0;
		confInt["link"] = 0;
	}

	confStr["TVOut"] = "OFF";
	resX = constrain( confInt["resolutionX"], 320, 1920 );
	resY = constrain( confInt["resolutionY"], 240, 1200 );
}

void GMenu2X::writeConfig() {
	ledOn();
	if (confInt["saveSelection"] && menu != NULL) {
		confInt["section"] = menu->selSectionIndex();
		confInt["link"] = menu->selLinkIndex();
	}

	string conffile = path+"gmenu2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		for (ConfStrHash::iterator curr = confStr.begin(); curr != confStr.end(); curr++) {
			if (curr->first == "sectionBarPosition" || curr->first == "sectionBar" || curr->first == "tvoutEncoding" ) continue;
			inf << curr->first << "=\"" << curr->second << "\"" << endl;
		}

		for (ConfIntHash::iterator curr = confInt.begin(); curr != confInt.end(); curr++) {
			if (curr->first == "batteryLog" || curr->first == "sectionBar" || curr->first == "maxClock" || curr->first == "minClock" || curr->first == "menuClock") continue;
			inf << curr->first << "=" << curr->second << endl;
		}
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
		for (ConfStrHash::iterator curr = skinConfStr.begin(); curr != skinConfStr.end(); curr++)
			inf << curr->first << "=\"" << curr->second << "\"" << endl;

		for (ConfIntHash::iterator curr = skinConfInt.begin(); curr != skinConfInt.end(); curr++) {
			if (curr->first == "titleFontSize" || curr->first == "sectionBarHeight" || curr->first == "linkHeight" ) continue;
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

void GMenu2X::setSkin(const string &skin, bool setWallpaper, bool clearSC) {
	confStr["skin"] = skin;

//Clear previous skin settings
	skinConfStr.clear();
	skinConfInt.clear();

//clear collection and change the skin path
	if (clearSC) sc.clear();
	sc.setSkin(skin);
	// if (btnContextMenu != NULL) btnContextMenu->setIcon( btnContextMenu->getIcon() );

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

				if (value.length() > 0) {
					if (value.length() > 1 && value.at(0) == '"' && value.at(value.length() - 1) == '"') {
							skinConfStr[name] = value.substr(1, value.length() - 2);
					} else if (value.at(0) == '#') {
						// skinConfColor[name] = strtorgba( value.substr(1,value.length()) );
						skinConfColors[stringToColor(name)] = strtorgba(value);
					} else if (name.length() > 6 && name.substr( name.length() - 6, 5 ) == "Color") {
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

			if (setWallpaper && !skinConfStr["wallpaper"].empty() && fileExists("skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"])) {
				confStr["wallpaper"] = "skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"];
				sc[confStr["wallpaper"]]->blit(bg,0,0);
			}
		}
	}

// (poor) HACK: ensure font alt colors have a default value
	if (skinConfColors[COLOR_FONT_ALT].r == 253 && skinConfColors[COLOR_FONT_ALT].g == 1 && skinConfColors[COLOR_FONT_ALT].b == 252 && skinConfColors[COLOR_FONT_ALT].a == 0) skinConfColors[COLOR_FONT_ALT] = skinConfColors[COLOR_FONT];
	if (skinConfColors[COLOR_FONT_ALT_OUTLINE].r == 253 && skinConfColors[COLOR_FONT_ALT_OUTLINE].g == 1 && skinConfColors[COLOR_FONT_ALT_OUTLINE].b == 252 && skinConfColors[COLOR_FONT_ALT_OUTLINE].a == 0) skinConfColors[COLOR_FONT_ALT_OUTLINE] = skinConfColors[COLOR_FONT_OUTLINE];

// prevents breaking current skin until they are updated
	if (!skinConfInt["fontSizeTitle"] && skinConfInt["titleFontSize"] > 0) skinConfInt["fontSizeTitle"] = skinConfInt["titleFontSize"];

	evalIntConf( &skinConfInt["topBarHeight"], 40, 1, resY);
	// evalIntConf( &skinConfInt["sectionBarHeight"], 200, 32, resY);
	evalIntConf( &skinConfInt["sectionBarSize"], 40, 1, resX);
	// evalIntConf( &skinConfInt["linkHeight"], 40, 16, resY);
	evalIntConf( &skinConfInt["linkItemHeight"], 40, 32, resY);
	evalIntConf( &skinConfInt["bottomBarHeight"], 16, 1, resY);
	evalIntConf( &skinConfInt["selectorX"], 142, 1, resX);
	evalIntConf( &skinConfInt["selectorPreviewX"], 7, 1, resX);
	evalIntConf( &skinConfInt["selectorPreviewY"], 56, 1, resY);
	evalIntConf( &skinConfInt["selectorPreviewWidth"], 128, 32, resY);
	evalIntConf( &skinConfInt["selectorPreviewHeight"], 128, 32, resX);
	evalIntConf( &skinConfInt["fontSize"], 9, 6, 60);
	evalIntConf( &skinConfInt["fontSizeTitle"], 14, 6, 60);

//recalculate some coordinates based on the new element sizes
	// linkRows = resY/skinConfInt["linkItemHeight"];
	// needed until refactor menu.cpp
	linkColumns = 1;//(resX-10)/skinConfInt["linkWidth"];
	linkRows = resY / skinConfInt["linkItemHeight"];
	// linkRows = linksRect.h / skinConfInt["linkItemHeight"];

	if (menu != NULL && clearSC) menu->loadIcons();

//font
	initFont();
}

// void GMenu2X::writeCommonIni() {}

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

void GMenu2X::ledOn() {
#if defined(TARGET_GP2X)
	if (memdev != 0 && !f200) memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_ON);
#endif
}

void GMenu2X::ledOff() {
#if defined(TARGET_GP2X)
	if (memdev != 0 && !f200) memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_OFF);
#endif
}

long GMenu2X::getBatteryStatus() {
	char buf[32] = "-1";
#if defined(TARGET_RS97)
	FILE *f = fopen("/proc/jz/battery", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
	}
	fclose(f);
#endif
	return atol(buf);
}

bool exitMainThread = false;
void* mainThread(void* param) {
	GMenu2X *menu = (GMenu2X*)param;
	while(!exitMainThread) {
		sleep(1);
	}
	return NULL;
}

void GMenu2X::setSuspend(bool suspend) {
	if (suspend) {
		setCPU(confInt["cpuMin"]);
		// input.setWakeUpInterval(60e3);
		setBacklight(0);
		INFO("Enter suspend mode. Current backlight: %d", getBacklight());
	} else {
		setCPU(confInt["cpuMenu"]);
		setBacklight(max(10, confInt["backlight"]));
		INFO("Exit from suspend mode. Restore backlight to: %d", confInt["backlight"]);
	}
	suspendActive = suspend;
}

void GMenu2X::main() {
	unsigned short battlevel = 0; //getBatteryLevel();
	pthread_t thread_id;
	// uint linksPerPage = linkColumns*linkRows;
	// int linkSpacingX = (resX-10 - linkColumns*(resX - skinConfInt["sectionBarX"]))/linkColumns;
	// int linkSpacingY = (resY-35 - skinConfInt["sectionBarY"] - linkRows*skinConfInt["sectionBarHeight"])/linkRows;
	uint sectionLinkPadding = 4; //max(skinConfInt["sectionBarHeight"] - 32 - font->getHeight(), 0) / 3;

	mmc_status curMMCStatus = MMC_REMOVE;
	mmc_status preMMCStatus = MMC_REMOVE;

	bool quit = false;
	int x = 0, y = 0; //, helpBoxHeight = fwType=="open2x" ? 154 : 139;//, offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
	uint i;
	Uint32 tickBattery = -4800, tickNow, tickMMC = 0; //, tickUSB = 0;

	string batteryIcon = "imgs/battery/3.png"; //, backlightIcon = "imgs/backlight.png";
	string prevBackdrop = confStr["wallpaper"], currBackdrop = confStr["wallpaper"];

	stringstream ss;

	if (pthread_create(&thread_id, NULL, mainThread, this)) {
		ERROR("%s, failed to create main thread\n", __func__);
	}

#if defined(TARGET_RS97)
	if (udcConnectedOnBoot == UDC_CONNECT) {
		sc[currBackdrop]->blit(s,0,0);
		checkUDC();
	}
#endif


	while (!quit) {
		tickNow = SDL_GetTicks();

		sc[currBackdrop]->blit(s,0,0);

		// SECTIONS
		if (confInt["sectionBar"]) {
			s->box(sectionBarRect, skinConfColors[COLOR_TOP_BAR_BG]);

			x = sectionBarRect.x; y = sectionBarRect.y;
			for (i = menu->firstDispSection(); i < menu->getSections().size() && i < menu->firstDispSection() + menu->sectionNumItems(); i++) {
				string sectionIcon = "skin:sections/"+menu->getSections()[i]+".png";
				if (!sc.exists(sectionIcon))
					sectionIcon = "skin:icons/section.png";

				if (confInt["sectionBar"] == SB_LEFT || confInt["sectionBar"] == SB_RIGHT) {
					y = (i - menu->firstDispSection()) * skinConfInt["sectionBarSize"];
				} else {
					x = (i - menu->firstDispSection()) * skinConfInt["sectionBarSize"];
				}

				if (menu->selSectionIndex()==(int)i)
					s->box(x, y, skinConfInt["sectionBarSize"], skinConfInt["sectionBarSize"], skinConfColors[COLOR_SELECTION_BG]);

				sc[sectionIcon]->blitCenter(s, x + skinConfInt["sectionBarSize"]/2, y + skinConfInt["sectionBarSize"]/2, skinConfInt["sectionBarSize"], skinConfInt["sectionBarSize"]);
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

			if (i==(uint)menu->selLinkIndex())
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

			// curUDCStatus = getUDCStatus();
			// if (preUDCStatus != curUDCStatus) {
			// 	if (curUDCStatus == UDC_REMOVE) {
			// 		if (needUSBUmount) {
			// 			// system("/usr/bin/usb_disconn_int_sd.sh");
			// 			// system("mount -o remount,rw /dev/mmcblk0p4");
			// 			system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
			// 			system("mount /dev/mmcblk0p4 /mnt/int_sd -t vfat -o rw,utf8");
			// 			INFO("%s, disconnect usbdisk for internal sd", __func__);
			// 			if (curMMCStatus == MMC_INSERT) {
			// 				// system("/usr/bin/usb_disconn_ext_sd.sh");
			// 				system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
			// 				system("mount /dev/mmcblk1p1 /mnt/ext_sd -t vfat -o rw,utf8 -t vfat -o rw,utf8");
			// 				INFO("%s, disconnect USB disk for external SD", __func__);
			// 			}
			// 			needUSBUmount = 0;
			// 		}
			// 	}
			// 	else if (curUDCStatus == UDC_CONNECT) {
			// 		MessageBox mb(this, tr["Which action do you want?"], "skin:icons/usb.png");
			// 		mb.setButton(CONFIRM, tr["USB disk"]);
			// 		mb.setButton(CANCEL,  tr["Charge only"]);
			// 		if (mb.exec() == CONFIRM) {
			// 			needUSBUmount = 1;
			// 			// system("/usr/bin/usb_conn_int_sd.sh");
			// 			// system("mount -o remount,ro /dev/mmcblk0p4");
			// 			system("umount -fl /dev/mmcblk0p4");
			// 			system("echo '/dev/mmcblk0p4' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
			// 			INFO("%s, connect USB disk for internal SD", __func__);
			// 			if (curMMCStatus == MMC_INSERT) {
			// 				// system("/usr/bin/usb_conn_ext_sd.sh");
			// 				system("umount -fl /mnt/ext_sd");
			// 				system("echo '/dev/mmcblk1p1' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
			// 				INFO("%s, connect USB disk for external SD", __func__);
			// 			}

			// 			MessageBox mb(this, tr["USB Disk Connected"], "skin:icons/usb.png");
			// 			mb.setAutoHide(500);
			// 			mb.exec();

			// 			while (getUDCStatus() == UDC_CONNECT) {
			// 				SDL_Delay(200);
			// 			}
			// 		}
			// 	}
			// 	else {
			// 		WARNING("%s, unexpected USB status!", __func__);
			// 	}
			// 	preUDCStatus = curUDCStatus;
			// 	tickSuspend = SDL_GetTicks(); // prevent immediate suspend
			// 	continue;
			// }
		}

		currBackdrop = confStr["wallpaper"];
		if (menu->selLink() != NULL && menu->selLinkApp() != NULL && !menu->selLinkApp()->getBackdrop().empty() && sc.add(menu->selLinkApp()->getBackdrop()) != NULL) {
			currBackdrop = menu->selLinkApp()->getBackdrop();
		}
		//Background
		if (prevBackdrop != currBackdrop) {
			INFO("New backdrop: %s", currBackdrop.c_str());
			sc.del(prevBackdrop);
			prevBackdrop = currBackdrop;
			// input.setWakeUpInterval(1);
			continue;
		}

		if (confInt["sectionBar"]) {
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
					batteryIcon = "imgs/battery/"+batteryIcon+".png";
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
				backlightIcon = "imgs/brightness/"+backlightIcon+".png";

				if (backlightLevel > 4 || sc.skinRes(backlightIcon)==NULL) 
					backlightIcon = "imgs/brightness.png";

				sc.skinRes(backlightIcon)->blit(s, sectionBarRect.x + sectionBarRect.w - 38 + iconTrayShift * 20, sectionBarRect.y + sectionBarRect.h - 18);
				iconTrayShift++;
			}
		}

		s->flip();

		bool inputAction = input.update();
		if (input.combo()) {
			confInt["sectionBar"] = ((confInt["sectionBar"] + 1) % 5);
			if (!confInt["sectionBar"]) confInt["sectionBar"]++;
			initLayout();
			initMenu();

			MessageBox mb(this,tr["CHEATER! ;)"]);
			mb.setBgAlpha(0);
			mb.setAutoHide(200);
			mb.exec();

			input.setWakeUpInterval(1); //25FPS

			continue;
		}
		input.setWakeUpInterval(0); //25FPS

		if (inputCommonActions(inputAction)) continue;

		if ( input[CONFIRM] && menu->selLink() != NULL ) {
			setVolume(confInt["globalVolume"]);

			if (menu->selLinkApp() != NULL && menu->selLinkApp()->getSelectorDir().empty()) {
				// MessageBox mb(this, tr.translate("Launching $1", menu->selLink()->getTitle().c_str(), NULL), menu->selLinkApp()->getIconPath());
				MessageBox mb(this, tr["Launching "] + menu->selLink()->getTitle().c_str(), menu->selLink()->getIconPath());
				mb.setAutoHide(500);
				mb.exec();
			}

			menu->selLink()->run();
		}
		else if ( input[SETTINGS] ) settings();
		else if ( input[MENU]     ) contextMenu();
		// LINK NAVIGATION
		else if ( input[LEFT ]  ) menu->pageUp();
		else if ( input[RIGHT]  ) menu->pageDown();
		else if ( input[UP   ]  ) menu->linkUp();
		else if ( input[DOWN ]  ) menu->linkDown();
		// SECTION
		else if ( input[SECTION_PREV] ) menu->decSectionIndex();
		else if ( input[SECTION_NEXT] ) menu->incSectionIndex();

		// VOLUME SCALE MODIFIER
#if defined(TARGET_GP2X)
		else if ( fwType=="open2x" && input[CANCEL] ) {
			volumeMode = constrain(volumeMode-1, -VOLUME_MODE_MUTE-1, VOLUME_MODE_NORMAL);
			if (volumeMode < VOLUME_MODE_MUTE)
				volumeMode = VOLUME_MODE_NORMAL;
			switch(volumeMode) {
				case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
				case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones); break;
				case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
			}
			setVolume(confInt["globalVolume"]);
		}
#endif
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
		// 			menu->selLinkApp()->setCPU( constrain(menu->selLinkApp()->clock()-10,50,confInt["cpuMax"]) );
		// 		if ( input[VOLUP] && !input.isActive(VOLDOWN) )
		// 			menu->selLinkApp()->setCPU( constrain(menu->selLinkApp()->clock()+10,50,confInt["cpuMax"]) );
		// 		if ( input.isActive(VOLUP) && input.isActive(VOLDOWN) ) menu->selLinkApp()->setCPU(CPU_CLK_DEFAULT);
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
		// tickSuspend = SDL_GetTicks();
	}

	exitMainThread = true;
	pthread_join(thread_id, NULL);
	// delete btnContextMenu;
	// btnContextMenu = NULL;
}

bool GMenu2X::inputCommonActions(bool &inputAction) {
	INFO("SDL_GetTicks(): %d\tsuspendActive: %d", SDL_GetTicks(), powerManager->suspendActive);

	if (powerManager->suspendActive) {
		// SUSPEND ACTIVE
		while (!input[POWER]) {
			WARNING("SUSPEND ACTIVE");
			input.update();
		}
		powerManager->doSuspend(0, NULL);
		return true;
	}

	if (inputAction) powerManager->resetSuspendTimer();

	bool wasActive = false;
	while (input[POWER]) {
		WARNING("POWER HOLD");

		wasActive = true;

		input.update();

		// HOLD POWER BUTTON
		if (input[POWER]) {
			powerManager->doPowerOff(0, NULL);
			return true;
		}
	}

	if (wasActive) {
		powerManager->doSuspend(1, NULL);
		return true;
	}


	wasActive = false;
	while (input[MENU]) {
		input.update();
		wasActive = true;
		if (input[SECTION_NEXT]) {
			// SCREENSHOT
			if (!saveScreenshot()) { ERROR("Can't save screenshot"); return true; }
			MessageBox mb(this, tr["Screenshot saved"]);
			mb.setAutoHide(500);
			mb.exec();
			wasActive = false;
		} else if (input[SECTION_PREV]) {
			// VOLUME / MUTE
			setVolume(confInt["globalVolume"], true);
			wasActive = false;
		}
	}

	input[MENU] = wasActive; // Key was active but no combo was pressed

	if ( input[BACKLIGHT] ) {
		setBacklight(confInt["backlight"], true);
		return true; 
	}

	return false;
}

void GMenu2X::explorer() {
	FileDialog fd(this,tr["Select an application"],".gpu,.gpe,.sh,", "", tr["Explorer"]);
	if (fd.exec()) {
		if (confInt["saveSelection"] && (confInt["section"]!=menu->selSectionIndex() || confInt["link"]!=menu->selLinkIndex()))
			writeConfig();

#if defined(TARGET_GP2X)
		if (fwType == "open2x" && savedVolumeMode != volumeMode)
			writeConfigOpen2x();
#endif

	//string command = cmdclean(fd.path()+"/"+fd.file) + "; sync & cd "+cmdclean(getExePath())+"; exec ./gmenu2x";
		string command = cmdclean(fd.getPath()+"/"+fd.getFile());
		chdir(fd.getPath().c_str());
		quit();
		setCPU(confInt["cpuMenu"]);
		execlp("/bin/sh","/bin/sh","-c",command.c_str(),NULL);

	//if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
	//try relaunching gmenu2x
		WARNING("Error executing selected application, re-launching gmenu2x");
		chdir(getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}
}

void GMenu2X::settings() {
	int curMenuClock = confInt["cpuMenu"];
	int curGlobalVolume = confInt["globalVolume"];
//G
	// int prevgamma = confInt["gamma"];
	// bool showRootFolder = fileExists("/mnt/root");

	FileLister fl_tr("translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	string lang = tr.lang();

	vector<string> encodings;
	encodings.push_back("OFF");
	encodings.push_back("NTSC");
	encodings.push_back("PAL");

	vector<string> batteryType;
	batteryType.push_back("BL-5B");
	batteryType.push_back("Linear");

	int prevSkinBackdrops = confInt["skinBackdrops"];

	vector<string> sectionBar;
	sectionBar.push_back("OFF");
	sectionBar.push_back("Left");
	sectionBar.push_back("Right");
	sectionBar.push_back("Top");
	sectionBar.push_back("Bottom");

	string sb_sel = sectionBar[confInt["sectionBar"]];
	string prevDateTime = confStr["datetime"] = getDateTime();
	string prevTVOut = confStr["TVOut"];

	SettingsDialog sd(this, ts, tr["Settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenu2X"], &lang, &fl_tr.getFiles()));
	sd.addSetting(new MenuSettingDateTime(this, tr["Date & Time"], tr["Set system's date time"], &confStr["datetime"]));
	sd.addSetting(new MenuSettingInt(this,tr["Screen timeout"], tr["Seconds to turn display off if inactive"], &confInt["backlightTimeout"], 30, 10, 300));
	sd.addSetting(new MenuSettingInt(this,tr["Power timeout"], tr["Minutes to poweroff system if inactive"], &confInt["powerTimeout"], 10, 1, 300));
	sd.addSetting(new MenuSettingMultiString(this, tr["Battery profile"], tr["Set the battery discharge profile"], &confStr["batteryType"], &batteryType));
	sd.addSetting(new MenuSettingBool(this, tr["Skin backdrops"], tr["Automatic load backdrops from skin pack"], &confInt["skinBackdrops"]));
	// sd.addSetting(new MenuSettingMultiString(this, tr["Section Bar Postition"], tr["Set the position of the Section Bar"], &sb_sel, &sectionBar));
	// sd.addSetting(new MenuSettingMultiString(this, tr["Section Bar Postition"], tr["Set the position of the Section Bar"], &confInt["sectionBar"], &sectionBar));

	sd.addSetting(new MenuSettingBool(this, tr["Save last selection"], tr["Save the last selected link and section on exit"], &confInt["saveSelection"]));
	sd.addSetting(new MenuSettingBool(this, tr["Output logs"], tr["Logs the output of the links. Use the Log Viewer to read them."], &confInt["outputLogs"]));

#if defined(TARGET_GP2X)
	sd.addSetting(new MenuSettingInt(this, tr["Clock for GMenu2X"], tr["Set the cpu working frequency when running GMenu2X"], &confInt["cpuMenu"], 140, 50, 325));
	// sd.addSetting(new MenuSettingInt(this, tr["Maximum overclock"], tr["Set the maximum overclock for launching links"], &confInt["cpuMax"], CPU_CLK_DEFAULT, CPU_CLK_MIN, CPU_CLK_MAX));
//G
//sd.addSetting(new MenuSettingInt(this, tr["Gamma"], tr["Set gp2x gamma value (default: 10)"], &confInt["gamma"], 1, 100));
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	sd.addSetting(new MenuSettingInt(this, tr["Clock for GMenu2X"], tr["Set the cpu working frequency when running GMenu2X"], &confInt["cpuMenu"], 200, 50, 900, 10));
	// sd.addSetting(new MenuSettingInt(this, tr["Maximum overclock"], tr["Set the maximum overclock for launching links"], &confInt["cpuMax"], CPU_CLK_DEFAULT, CPU_CLK_MIN, CPU_CLK_MAX, 10));
#elif defined(TARGET_RS97)
	// sd.addSetting(new MenuSettingInt(this, tr["Maximum overclock"], tr["Set the maximum overclock for launching links"], &confInt["cpuMax"], 528, 528, 642, 6));
	sd.addSetting(new MenuSettingMultiString(this, tr["TV-out"], tr["TV-out signal"], &confStr["TVOut"], &encodings));
	// sd.addSetting(new MenuSettingInt(this, tr["Clock for GMenu2X"], tr["Set the cpu working frequency when running GMenu2X"], &confInt["cpuMenu"], CPU_CLK_DEFAULT, CPU_CLK_MIN, CPU_CLK_MAX, 6));
#endif
	sd.addSetting(new MenuSettingInt(this,tr["Backlight"], tr["Set LCD backlight"], &confInt["backlight"], 70, 1, 100));
	sd.addSetting(new MenuSettingInt(this, tr["Global volume"], tr["Set the default volume for the soundcard"], &confInt["globalVolume"], 60, 0, 100));
	// sd.addSetting(new MenuSettingBool(this,tr["Show root"],tr["Show root folder in the file selection dialogs"],&showRootFolder));

	if (sd.exec() && sd.edited() && sd.save) {
//G
#if defined(TARGET_GP2X)
		if (prevgamma != confInt["gamma"]) setGamma(confInt["gamma"]);
#endif

		if (curMenuClock != confInt["cpuMenu"]) setCPU(confInt["cpuMenu"]);
		if (curGlobalVolume != confInt["globalVolume"]) setVolume(confInt["globalVolume"]);
		if (lang == "English") lang = "";
		if (lang != tr.lang()) tr.setLang(lang);
		// if (fileExists("/mnt/root") && !showRootFolder)
			// unlink("/mnt/root");
		// else if (!fileExists("/mnt/root") && showRootFolder)
			// symlink("/","/mnt/root");

		if (confStr["lang"] != lang) confStr["lang"] = lang;
		if (sb_sel == "OFF") confInt["sectionBar"] = SB_OFF;
		else if (sb_sel == "Right") confInt["sectionBar"] = SB_RIGHT;
		else if (sb_sel == "Top") confInt["sectionBar"] = SB_TOP;
		else if (sb_sel == "Bottom") confInt["sectionBar"] = SB_BOTTOM;
		else confInt["sectionBar"] = SB_LEFT;

		setBacklight(confInt["backlight"], false);

		writeConfig();

		powerManager->setSuspendTimeout(confInt["backlightTimeout"]);
		powerManager->setPowerTimeout(confInt["powerTimeout"]);
		powerManager->resetSuspendTimer();

#if defined(TARGET_RS97)
		if (prevTVOut != confStr["TVOut"]) setTVOut();
#endif

		if (prevSkinBackdrops != confInt["skinBackdrops"] || prevDateTime != confStr["datetime"]) restartDialog();
	}
}

const string GMenu2X::getDateTime() {
	char       buf[80];
	time_t     now = time(0);
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
	settimeofday(&newtime,NULL);
#endif
}

uint GMenu2X::onChangeSkin() {
	return 1;
}

void GMenu2X::skinMenu() {
	FileLister fl_sk("skins",true,false);
	fl_sk.addExclude("..");
	fl_sk.browse();
	string curSkin = confStr["skin"];
	bool save = false;

	do {
		sc.del("skin:icons/skin.png");
		sc.del("skin:imgs/buttons/left.png");
		sc.del("skin:imgs/buttons/right.png");

		setSkin(confStr["skin"], true, false);

		SettingsDialog sd(this, ts, tr["Skin"], "skin:icons/skin.png");
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenu2X"], &confStr["skin"], &fl_sk.getDirectories(), MakeDelegate(this, &GMenu2X::onChangeSkin)));
		// sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenu2X"], &confStr["skin"], &fl_sk.getDirectories()));
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

		sd.addSetting(new MenuSettingInt(this, tr["Font Size"], tr["Size of text font"], &skinConfInt["fontSize"], 9, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Title Font Size"], tr["Size of title's text font"], &skinConfInt["fontSizeTitle"], 14, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Top Bar Height"], tr["Height of top bar"], &skinConfInt["topBarHeight"], 40, 1, resY));
		sd.addSetting(new MenuSettingInt(this, tr["Bottom Bar Height"], tr["Height of bottom bar"], &skinConfInt["bottomBarHeight"], 16, 1, resY));
		sd.addSetting(new MenuSettingInt(this, tr["Link Height"], tr["Height of link item"], &skinConfInt["linkItemHeight"], 40, 16, resY));
		sd.addSetting(new MenuSettingInt(this, tr["Section Bar Size"], tr["Size of section bar"], &skinConfInt["sectionBarSize"], 40, 1, resX));

		sd.exec();

		save = sd.save;
	} while (!save);

	writeSkinConfig();
	writeConfig();

	setSkin(confStr["skin"], true, true);
	initBG();
}

void GMenu2X::restartDialog() {
	MessageBox mb(this, tr["GMenuNext will restart to apply\nthe settings. Continue?"], "skin:icons/exit.png");
	mb.setButton(CONFIRM, tr["Restart"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	if (mb.exec() == CONFIRM) {
		quit();
		WARNING("Re-launching gmenu2x");
		chdir(getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}
}

void GMenu2X::poweroffDialog() {
	MessageBox mb(this, tr["   Poweroff or reboot the device?   "], "skin:icons/exit.png");
	mb.setButton(SECTION_NEXT, tr["Reboot"]);
	mb.setButton(CONFIRM, tr["Poweroff"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	int response = mb.exec();
	if (response == CONFIRM) {
		MessageBox mb(this, tr["Poweroff"]);
		mb.setAutoHide(500);
		mb.exec();
		// setSuspend(true);
		// SDL_Delay(500);

#if !defined(TARGET_PC)
		system("poweroff");
#endif
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(this, tr["Rebooting"]);
		mb.setAutoHide(500);
		mb.exec();
		// setSuspend(true);
		// SDL_Delay(500);

#if !defined(TARGET_PC)
		system("reboot");
#endif
	}
}

#if defined(TARGET_RS97)
void GMenu2X::checkUDC() {
	if (getUDCStatus() == UDC_CONNECT) {
		MessageBox mb(this, tr["Select USB mode:"], "skin:icons/usb.png");
		mb.setButton(CONFIRM, tr["USB Drive"]);
		mb.setButton(CANCEL,  tr["Charger"]);
		if (mb.exec() == CONFIRM) {
			// needUSBUmount = 1;
			// system("/usr/bin/usb_conn_int_sd.sh");
			// system("mount -o remount,ro /dev/mmcblk0p4");
			system("umount -fl /dev/mmcblk0p4");
			system("echo '/dev/mmcblk0p4' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
			INFO("%s, connect USB disk for internal SD", __func__);

			if (getMMCStatus() == MMC_INSERT) {
				// system("/usr/bin/usb_conn_ext_sd.sh");
				system("umount -fl /mnt/ext_sd");
				system("echo '/dev/mmcblk1p1' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
				INFO("%s, connect USB disk for external SD", __func__);
			}

			sc[confStr["wallpaper"]]->blit(s,0,0);

			MessageBox mb(this, tr["USB Drive Connected"], "skin:icons/usb.png");
			mb.setAutoHide(500);
			mb.exec();

			while (udcConnectedOnBoot = getUDCStatus() == UDC_CONNECT) {
				SDL_Delay(200);
			}

			system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
			system("mount /dev/mmcblk0p4 /mnt/int_sd -t vfat -o rw,utf8");
			INFO("%s, disconnect usbdisk for internal sd", __func__);
			if (getMMCStatus() == MMC_INSERT) {
				// system("/usr/bin/usb_disconn_ext_sd.sh");
				system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
				system("mount /dev/mmcblk1p1 /mnt/ext_sd -t vfat -o rw,utf8 -t vfat -o rw,utf8");
				INFO("%s, disconnect USB disk for external SD", __func__);
			}
			powerManager->resetSuspendTimer();
			// tickSuspend = SDL_GetTicks(); // prevent immediate suspend
		}
	}
}

void GMenu2X::setTVOut() {
	char buf[2] = "0";

	// int tvout = open("/proc/jz/tvout", O_RDWR);
	FILE *f = fopen("/proc/jz/tvselect", "w");
	if (f) {
		if (confStr["TVOut"] == "PAL") {
			sprintf(buf, "1");
		} else if (confStr["TVOut"] == "NTSC") {
			sprintf(buf, "2");
		}
		fputs(buf, f);

	}
	fclose(f);

	if (strcmp(buf, "0") == 0) return;

	MessageBox mb(this, tr["TV-out enabled.\nContinue?"], "skin:icons/tv.png");
	mb.setButton(SETTINGS, tr["Yes"]);
	mb.setButton(CONFIRM,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		confStr["TVOut"] = "OFF";
		setTVOut();
	}
}

void GMenu2X::umountSd() {
	MessageBox mb(this, tr["Umount external SD card?"], "skin:icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		system("/usr/bin/umount_ext_sd.sh");
		MessageBox mb(this,tr["Complete!"]);
		mb.exec();
	}
}

void GMenu2X::formatSd() {
	MessageBox mb(this, tr["Format internal SD card?"], "skin:icons/format.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		MessageBox mb(this, tr["Formatting internal SD card..."], "skin:icons/format.png");
		mb.setAutoHide(100);
		mb.exec();

		system("/usr/bin/format_int_sd.sh");
		{ // new mb scope
			MessageBox mb(this,tr["Complete!"]);
			mb.setAutoHide(0);
			mb.exec();
		}
	}
}
#endif


// void GMenu2X::toggleTvOut() {
// #if defined(TARGET_GP2X)
// 	if (cx25874!=0)
// 		gp2x_tvout_off();
// 	else
// 		gp2x_tvout_on(confStr["TVOut"] == "PAL");
// #elif defined(TARGET_RS97)
// 	int tvout=0;
// 	char buf[32]={0};
// 	int fd = open("/mnt/game/gmenu2x/tvout", O_RDWR);
// 	if (fd > 0){
// 		read(fd, buf, sizeof(buf));
// 		INFO("Current TV-out value: %s", buf);
// 		if (memcmp(buf, "1", 1) == 0){
// 			tvout = 0;
// 			sprintf(buf, "0");
// 			INFO("Turn off TV-out");
// 		}
// 		else{
// 			tvout = 1;
// 			sprintf(buf, "1");
// 			INFO("Turn on TV-out");
// 		}
// 		lseek(fd, 0, SEEK_SET);
// 		write(fd, buf, 1);
// 		close(fd);
// 	}
// 	else{
// 		tvout = 1;
// 		fd = open("/mnt/game/gmenu2x/tvout", O_CREAT);
// 		sprintf(buf, "1");
// 		write(fd, buf, 1);
// 		close(fd);
// 		INFO("Create new TV-out file");
// 	}

// 	sprintf(buf, "/usr/bin/tvout.sh %d %d", tvout, confStr["TVOut"] == "PAL" ? 1 : 2);
// 	system(buf);
// 	INFO("run cmd: %s", buf);
// #endif
// }

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
	voices.push_back((MenuOption){tr["Link scanner"],	MakeDelegate(this, &GMenu2X::linkScanner)});

	bool close = false;
	uint i, fadeAlpha = 0;
	int sel = 0;

	int h = font->getHeight();
	int h2 = font->getHalfHeight();
	SDL_Rect box;
	box.h = h * voices.size() + 8;
	box.w = 0;
	for (i = 0; i < voices.size(); i++) {
		int w = font->getTextWidth(voices[i].text);
		if (w > box.w) box.w = w;
	}
	box.w += 23;
	box.x = halfX - box.w/2;
	box.y = halfY - box.h/2;

	SDL_Rect selbox = {box.x + 4, 0, box.w - 8, h};
	Uint32 tickStart = SDL_GetTicks();

	Surface bg(s);
	input.setWakeUpInterval(45); //25FPS

	while (!close) {
		selbox.y = box.y + 4 + h * sel;
		bg.blit(s, 0, 0);

		s->box(0, 0, resX, resY, 0,0,0,fadeAlpha);
		s->box(box.x, box.y, box.w, box.h, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		s->rectangle( box.x+2, box.y+2, box.w-4, box.h-4, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );

	//draw selection rect
		s->box( selbox.x, selbox.y, selbox.w, selbox.h, skinConfColors[COLOR_MESSAGE_BOX_SELECTION] );
		for (i = 0; i < voices.size(); i++)
			s->write( font, voices[i].text, box.x + 12, box.y + h2 + 3 + h * i, HAlignLeft, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
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

		if (fadeAlpha < 200) {
			fadeAlpha = intTransition(0, 200, tickStart, 200);
			continue; 
		}

		input.setWakeUpInterval(0);

		bool inputAction = input.update();

		if (inputCommonActions(inputAction)) continue;

		if ( input[MENU] || input[CANCEL]) close = true;
		else if ( input[UP] ) sel = (sel-1 < 0) ? (int)voices.size()-1 : sel - 1 ;
		else if ( input[DOWN] ) sel = (sel+1 > (int)voices.size()-1) ? 0 : sel + 1;
		else if ( input[LEFT] || input[PAGEUP] ) sel = 0;
		else if ( input[RIGHT] || input[PAGEDOWN] ) sel = (int)voices.size() - 1;
		else if ( input[SETTINGS] || input[CONFIRM] ) { voices[sel].action(); close = true; }
	}
	// input.setWakeUpInterval(0);
	// tickSuspend = SDL_GetTicks(); // prevent immediate suspend
	// powerManager->resetSuspendTimer();
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
	// ERROR("FILE: %s", menu->selLinkApp()->getFile().c_str());
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

	string diagTitle = tr.translate("Edit $1", linkTitle.c_str(), NULL);
	string diagIcon = menu->selLinkApp()->getIconPath();

	// string strClock;
	// stringstream ss;
	// ss << CPU_CLK_DEFAULT;
	// ss >> strClock;

	SettingsDialog sd(this, ts, diagTitle, diagIcon);
	sd.addSetting(new MenuSettingString(      this, tr["Title"],                tr["Link title"], &linkTitle, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingString(      this, tr["Description"],          tr["Link description"], &linkDescription, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingMultiString( this, tr["Section"],              tr["The section this link belongs to"], &newSection, &menu->getSections() ));
	sd.addSetting(new MenuSettingImage(       this, tr["Icon"],                 tr["Select an icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg", dir_name(linkIcon) ));
	sd.addSetting(new MenuSettingFile(        this, tr["Manual"],               tr["Select a manual or README file"], &linkManual, ".man.png,.txt", dir_name(linkManual)));

	// sd.addSetting(new MenuSettingInt(         this, tr.translate("Clock (default: $1)","528", NULL), tr["Cpu clock frequency to set when launching this link"], &linkClock, 50, confInt["cpuMax"] ));
	sd.addSetting(new MenuSettingInt(         this, tr["CPU Clock"], tr["CPU clock frequency when launching this link"], &linkClock, confInt["cpuMenu"], confInt["cpuMin"], confInt["cpuMax"], 6));
	//sd.addSetting(new MenuSettingBool(        this, tr["Tweak RAM Timings"],    tr["This usually speeds up the application at the cost of stability"], &linkUseRamTimings ));
	//sd.addSetting(new MenuSettingInt(         this, tr["Volume"],               tr["Volume to set for this link"], &linkVolume, 0, 1 ));
	sd.addSetting(new MenuSettingString(      this, tr["Parameters"],           tr["Parameters to pass to the application"], &linkParams, diagTitle, diagIcon ));

	sd.addSetting(new MenuSettingBool(        this, tr["Selector Browser"],     tr["Allow the selector to change directory"], &linkSelBrowser ));
	sd.addSetting(new MenuSettingDir(         this, tr["Selector Directory"],   tr["Directory to scan for the selector"], &linkSelDir, real_path(linkSelDir) ));
	sd.addSetting(new MenuSettingString(      this, tr["Selector Filter"],      tr["Filter file type (separate with commas)"], &linkSelFilter, diagTitle, diagIcon ));
	sd.addSetting(new MenuSettingDir(         this, tr["Selector Screenshots"], tr["Directory of the screenshots for the selector"], &linkSelScreens, dir_name(linkSelScreens) ));
	sd.addSetting(new MenuSettingFile(        this, tr["Selector Aliases"],     tr["File containing a list of aliases for the selector"], &linkSelAliases, dir_name(linkSelAliases) ));
	sd.addSetting(new MenuSettingImage(       this, tr["Backdrop"],             tr["Select an image backdrop"], &linkBackdrop, ".png,.bmp,.jpg,.jpeg", real_path(linkBackdrop)));

#if defined(TARGET_WIZ) || defined(TARGET_CAANOO)
	bool linkUseGinge = menu->selLinkApp()->getUseGinge();
	string ginge_prep = getExePath() + "/ginge/ginge_prep";
	if (fileExists(ginge_prep))
		sd.addSetting(new MenuSettingBool(        this, tr["Use Ginge"],            tr["Compatibility layer for running GP2X applications"], &linkUseGinge ));
#elif defined(TARGET_GP2X)
	//G
	int linkGamma = menu->selLinkApp()->gamma();
	sd.addSetting(new MenuSettingInt(         this, tr["Gamma (default: 0)"],   tr["Gamma value to set when launching this link"], &linkGamma, 0, 100 ));
#endif

	//G
	sd.addSetting(new MenuSettingBool(        this, tr["Wrapper"],              tr["Relaunch GMenu2X after this link's execution ends"], &menu->selLinkApp()->needsWrapperRef() ));
	//sd.addSetting(new MenuSettingBool(        this, tr["Don't Leave"],          tr["Don't quit GMenu2X when launching this link"], &menu->selLinkApp()->runsInBackgroundRef() ));

	if (sd.exec() && sd.edited() && sd.save) {
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
		menu->selLinkApp()->setCPU(linkClock);
		menu->selLinkApp()->setVolume(linkVolume);
		//G
#if defined(TARGET_GP2X)
		menu->selLinkApp()->setGamma(linkGamma);
#elif defined(TARGET_WIZ) || defined(TARGET_CAANOO)
		menu->selLinkApp()->setUseGinge(linkUseGinge);
#endif

		INFO("New Section: '%s'", newSection.c_str());

		//if section changed move file and update link->file
		if (oldSection!=newSection) {
			vector<string>::const_iterator newSectionIndex = find(menu->getSections().begin(),menu->getSections().end(),newSection);
			if (newSectionIndex==menu->getSections().end()) return;
			string newFileName = "sections/"+newSection+"/"+linkTitle;
			uint x=2;
			while (fileExists(newFileName)) {
				string id = "";
				stringstream ss; ss << x; ss >> id;
				newFileName = "sections/"+newSection+"/"+linkTitle+id;
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
	InputDialog id(this, ts, tr["Insert a name for the new section"], "", tr["Add section"], "skin:icons/section.png");
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
	InputDialog id(this, ts, tr["Insert a new name for this section"],menu->selSection(),tr ["Rename section"], "skin:sections/" + menu->selSection() + ".png");
	if (id.exec()) {
		//only if a section with the same name does not exist & !samename
		if (menu->selSection() != id.getInput() && find(menu->getSections().begin(),menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
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
	MessageBox mb(this,tr["All links in this section will be removed."]+"\n"+tr["Are you sure?"]);
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

unsigned short GMenu2X::getBatteryLevel() {
	long val = getBatteryStatus();

if (confStr["batteryType"] == "BL-5B") {
	if ((val > 10000) || (val < 0)) return 6;
	else if (val > 4000) return 5; // 100%
	else if (val > 3900) return 4; // 80%
	else if (val > 3800) return 3; // 60%
	else if (val > 3730) return 2; // 40%
	else if (val > 3600) return 1; // 20%
	return 0; // 0% :(
}

#if defined(TARGET_GP2X)
	//if (batteryHandle<=0) return 6; //AC Power
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

	val = constrain(val, 0, 4500);

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
	input.setInterval(180);
	// input.setInterval(30,  VOLDOWN);
	// input.setInterval(30,  VOLUP);
	input.setInterval(1000, SETTINGS);
	input.setInterval(1000, MENU);
	// input.setInterval(300, CANCEL);
	// input.setInterval(300, MANUAL);
	// input.setInterval(100, INC);
	// input.setInterval(100, DEC);
	input.setInterval(1000, CONFIRM);
	// input.setInterval(500, SECTION_PREV);
	// input.setInterval(500, SECTION_NEXT);
	// input.setInterval(500, PAGEUP);
	// input.setInterval(500, PAGEDOWN);
	// input.setInterval(200, BACKLIGHT);
	input.setInterval(1500, POWER);
}

void GMenu2X::setCPU(unsigned mhz) {
	// mhz = constrain(mhz, CPU_CLK_MIN, CPU_CLK_MAX);
	if (memdev > 0) {
		DEBUG("Setting clock to %d", mhz);

#if defined(TARGET_GP2X)
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

#elif defined(TARGET_RS97)
		#define CPPCR     (0x10 >> 2)
		unsigned long m = mhz / 6;
		memregs[CPPCR] = (m << 24) | 0x090520;
		INFO("Set CPU clock: %d", mhz);
#endif
	}
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
		while (!close) {
			input.setWakeUpInterval(3000);
			drawSlider(val, 0, 100, *iconVolume[val > 0 ? 2 : 0], bg);

			close = !input.update();

			if (input[SETTINGS] || input[CONFIRM] || input[CANCEL]) close = true;
			if ( input[LEFT] || input[DEC] )		val = max(0, val - volumeStep);
			else if ( input[RIGHT] || input[INC] )	val = min(100, val + volumeStep);
			else if ( input[SECTION_PREV] )		{
													val += volumeStep;
													if (val > 100) val = 0;
												}
		}
		powerManager->resetSuspendTimer();
		input.setWakeUpInterval(0);
		confInt["globalVolume"] = val;
		writeConfig();
	}

	unsigned long soundDev = open("/dev/mixer", O_RDWR);
	if (soundDev) {
		int vol = (val << 8) | val;
#if defined(TARGET_RS97)
		ioctl(soundDev, SOUND_MIXER_WRITE_VOLUME, &vol);
#else
		ioctl(soundDev, SOUND_MIXER_WRITE_PCM, &vol);
#endif
		close(soundDev);

		volumeMode = vol ? VOLUME_MODE_NORMAL : VOLUME_MODE_MUTE;
	}

	return val;
}

int GMenu2X::getBacklight() {
	char buf[32] = "-1";
#if defined(TARGET_RS97)
	FILE *f = fopen("/proc/jz/lcd_backlight", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
	}
	fclose(f);
#endif
	return atoi(buf);
}

int GMenu2X::setBacklight(int val, bool popup) {
	int backlightStep = 10;

	if (val < 0) val = 100;
	else if (val > 100) val = backlightStep;

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

		powerManager->clearTimer();
		while (!close) {
			input.setWakeUpInterval(3000);
			int backlightIcon = val/20;

			if (backlightIcon > 4 || iconBrightness[backlightIcon] == NULL) backlightIcon = 5;

			drawSlider(val, 0, 100, *iconBrightness[backlightIcon], bg);

			close = !input.update();

			if ( input[SETTINGS] || input[MENU] || input[CONFIRM] || input[CANCEL] ) close = true;
			if ( input[LEFT] || input[DEC] )			val = setBacklight(max(1, val - backlightStep), false);
			else if ( input[RIGHT] || input[INC] )		val = setBacklight(min(100, val + backlightStep), false);
			else if ( input[BACKLIGHT] )				val = setBacklight(val + backlightStep, false);
		}
		powerManager->resetSuspendTimer();
		input.setWakeUpInterval(0);
		confInt["backlight"] = val;
		writeConfig();
	}

#if defined(TARGET_RS97)
	char buf[4];
	FILE *f = fopen("/proc/jz/lcd_backlight", "w");
	if (f) {
		sprintf(buf, "%d", val);
		fputs(buf, f);
	}
	fclose(f);
#endif

	return val;
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
		s->write(font, text, x+re.w, y, HAlignLeft, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
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
		s->write(font, text, x, y, HAlignRight, VAlignMiddle, skinConfColors[COLOR_FONT_ALT], skinConfColors[COLOR_FONT_ALT_OUTLINE]);
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

#if defined(TARGET_GP2X)
void GMenu2X::settingsOpen2x() {
	SettingsDialog sd(this, ts, tr["Open2x Settings"]);
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

void GMenu2X::activateSdUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		MessageBox mb(this,tr["USB Enabled (SD)"],"skin:icons/usb.png");
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
		MessageBox mb(this,tr["USB Enabled (Nand)"],"skin:icons/usb.png");
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
		MessageBox mb(this,tr["USB Enabled (Root)"],"skin:icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh root");
	}
}

void GMenu2X::applyRamTimings() {
	// 6 4 1 1 1 2 2
	if (memdev!=0) {
		int tRC = 5, tRAS = 3, tWR = 0, tMRD = 0, tRFC = 0, tRP = 1, tRCD = 1;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
}

void GMenu2X::applyDefaultTimings() {
	// 8 16 3 8 8 8 8
	if (memdev!=0) {
		int tRC = 7, tRAS = 15, tWR = 2, tMRD = 7, tRFC = 7, tRP = 7, tRCD = 7;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
}

void GMenu2X::setGamma(int gamma) {
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

void GMenu2X::initServices() {
	if (usbnet) {
		string services = "scripts/services.sh "+ip+" "+(inet?"on":"off")+" "+(samba?"on":"off")+" "+(web?"on":"off")+" &";
		system(services.c_str());
	}
}
#endif

