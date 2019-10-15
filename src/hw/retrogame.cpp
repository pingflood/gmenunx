#ifndef HW_RETROGAME_H
#define HW_RETROGAME_H

// #define HW_UDC // hardware have UDC support
// #define HW_EXT_SD // hardware have external sd card support
// #define HW_SCALER // hardware have screen scaler (e.g., IPU)
// #define OPK_SUPPORT // firmware support opk
// #define IPK_SUPPORT // firmware support ipk


/*	RetroGame Key Codes. pingflood, 2018
	BUTTON     GMENU          SDL             NUMERIC   GPIO
	-----------------------------------------------------------------------------
	X          MODIFIER       SDLK_SPACE      32        !(mem[PEPIN] >> 07 & 0b1)
	A          CONFIRM        SDLK_LCTRL      306       !(mem[PDPIN] >> 22 & 0b1)
	B          CANCEL         SDLK_LALT       308       !(mem[PDPIN] >> 23 & 0b1)
	Y          MANUAL         SDLK_LSHIFT     304       !(mem[PEPIN] >> 11 & 0b1)
	L          SECTION_PREV   SDLK_TAB        9         !(mem[PBPIN] >> 23 & 0b1)
	R          SECTION_NEXT   SDLK_BACKSPACE  8         !(mem[PDPIN] >> 24 & 0b1)
	START      SETTINGS       SDLK_RETURN     13         (mem[PDPIN] >> 18 & 0b1)
	SELECT     MENU           SDLK_ESCAPE     27         (mem[PDPIN] >> 17 & 0b1)
	BACKLIGHT  BACKLIGHT      SDLK_3          51        !(mem[PDPIN] >> 21 & 0b1)
	POWER      POWER          SDLK_END        279       !(mem[PAPIN] >> 30 & 0b1)
	UP         UP             SDLK_UP         273       !(mem[PBPIN] >> 25 & 0b1)
	DOWN       DOWN           SDLK_DOWN       274       !(mem[PBPIN] >> 24 & 0b1)
	RIGHT      RIGHT          SDLK_RIGHT      275       !(mem[PBPIN] >> 26 & 0b1)
	LEFT       LEFT           SDLK_LEFT       276       !(mem[PDPIN] >> 00 & 0b1)
*/

volatile uint32_t *memregs;
volatile uint8_t memdev = 0;
int SOUND_MIXER_READ = SOUND_MIXER_READ_VOLUME;
int SOUND_MIXER_WRITE = SOUND_MIXER_WRITE_VOLUME;
int32_t tickBattery = 0;
uint8_t numJoyPrev = 0;
// batteryIcon = 3;

const int CPU_MENU = 528;
const int CPU_MAX = CPU_MENU * 2;
const int CPU_MIN = CPU_MENU / 2;
const int CPU_STEP = 6;

uint16_t getMMCStatus() {
	if (memdev > 0 && !(memregs[0x10500 >> 2] >> 0 & 0b1)) return MMC_INSERT;
	return MMC_REMOVE;
}

uint16_t getUDCStatus() {
	// if (memdev > 0 && ((memregs[0x10300 >> 2] >> 7 & 0b1) || (memregs[0x10400 >> 2] >> 13 & 0b1))) return UDC_CONNECT;
	if (memdev > 0 && (memregs[0x10300 >> 2] >> 7 & 0b1)) return UDC_CONNECT;
	return UDC_REMOVE;
}

uint16_t getTVOutStatus() {
	// return TV_REMOVE;
	if (memdev > 0 && !(memregs[0x10300 >> 2] >> 25 & 0b1)) return TV_CONNECT;
	return TV_REMOVE;

	// if (memdev > 0 && fwType == "RETROARCADE") return !(memregs[0x10300 >> 2] >> 6 & 0b1);
	// else if (memdev > 0) return !(memregs[0x10300 >> 2] >> 25 & 0b1);
	// return false;
}

uint16_t getDevStatus() {
	FILE *f;
	char buf[10000];
	if (f = fopen("/proc/bus/input/devices", "r")) {
	// if (f = fopen("/proc/bus/input/handlers", "r")) {
		size_t sz = fread(buf, sizeof(char), 10000, f);
		fclose(f);
		return sz;
	}
	return 0;
}

int32_t getBatteryStatus() {
	char buf[32] = "-1";
	FILE *f;
	if (f = fopen("/proc/jz/battery", "r")) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	return atol(buf);
}

uint16_t getTVOut() {
	char buf[32] = "0";
	FILE *f;
	if (f = fopen("/proc/jz/tvout", "r")) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	return atoi(buf);
}

uint16_t getVolumeMode(uint8_t vol) {
	if (!vol) return VOLUME_MODE_MUTE;
	else if (memdev > 0 && !(memregs[0x10300 >> 2] >> 6 & 0b1)) return VOLUME_MODE_PHONES;
	return VOLUME_MODE_NORMAL;
}

void printbin(const char *id, int n) {
	printf("%s: 0x%08x ", id, n);
	for(int i = 31; i >= 0; i--) {
		printf("%d", !!(n & 1 << i));
		if (!(i % 8)) printf(" ");
	}
	printf("\e[0K\n");
}

uint32_t hwCheck(unsigned int interval = 0, void *param = NULL) {
	tickBattery++;
	if (tickBattery > 30) { // update battery level every 30 hwChecks
		tickBattery = 0;
		batteryIcon = 0; // 0% :(
		int32_t val = getBatteryStatus();
		if ((val > 10000) || (val < 0)) batteryIcon = 6;
		else if (val > 4000) batteryIcon = 5; // 100%
		else if (val > 3900) batteryIcon = 4; // 80%
		else if (val > 3800) batteryIcon = 3; // 60%
		else if (val > 3730) batteryIcon = 2; // 40%
		else if (val > 3600) batteryIcon = 1; // 20%
	}

	if (memdev > 0 && tickBattery > 2) {
		udcStatus = getUDCStatus();
		if (udcPrev != udcStatus) {
			udcPrev = udcStatus;
			InputManager::pushEvent(udcStatus);
			return 2000;
		}

		mmcStatus = getMMCStatus();
		if (mmcPrev != mmcStatus) {
			mmcPrev = mmcStatus;
			InputManager::pushEvent(mmcStatus);
			return 2000;
		}

		volumeMode = getVolumeMode(1);
		if (volumeModePrev != volumeMode) {
			volumeModePrev = volumeMode;
			InputManager::pushEvent(PHONES_CONNECT);
			return 2000;
		}

		numJoy = getDevStatus();
		if (numJoyPrev != numJoy) {
			numJoyPrev = numJoy;
			SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
			SDL_InitSubSystem(SDL_INIT_JOYSTICK);
			InputManager::pushEvent(JOYSTICK_CONNECT);
			return 5000;
		}

		tvOutStatus = getTVOutStatus();
		if (tvOutPrev != tvOutStatus) {
			tvOutPrev = tvOutStatus;
			InputManager::pushEvent(tvOutStatus);
			return 2000;
		}

		// volumeMode = getVolumeMode(confInt["globalVolume"]);
		// if (volumeModePrev != volumeMode && volumeMode == VOLUME_MODE_PHONES) {
		// 	setVolume(min(70, confInt["globalVolume"]), true);
		// }
		// volumeModePrev = volumeMode;
	}
	return interval;
}

class GMenuNX : public GMenu2X {
private:
	void hwInit() {
		system("[ -d /home/retrofw ] && mount -o remount,async /home/retrofw");

		memdev = open("/dev/mem", O_RDWR);
		if (memdev > 0) {
			memregs = (uint32_t*)mmap(0, 0x20000, PROT_READ | PROT_WRITE, MAP_SHARED, memdev, 0x10000000);
			if (memregs == MAP_FAILED) {
				ERROR("Could not mmap hardware registers!");
				close(memdev);
			}
		} else {
			WARNING("Could not open /dev/mem");
		}

		struct fb_var_screeninfo vinfo;
		int fbdev = open("/dev/fb0", O_RDWR);
		if (fbdev >= 0 && ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo) >= 0) {
			w = vinfo.width;
			h = vinfo.height;
			close(fbdev);
		}

		if (w == 320 && h == 480) h = 240;

		INFO("RETROGAME");
	}

	void udcDialog(int udcStatus) {
		if (udcStatus == UDC_REMOVE) {
			INFO("USB Disconnected. Disabling devices...");
			system("/usr/bin/retrofw stop");
			return;
		}

		int option;
		if (confStr["usbMode"] == "Storage") option = CONFIRM;
		else if (confStr["usbMode"] == "Charger") option = CANCEL;
		else {
			MessageBox mb(this, tr["USB mode"], "skin:icons/usb.png");
			mb.setButton(CANCEL,  tr["Charger"]);
			mb.setButton(CONFIRM, tr["Storage"]);
			option = mb.exec();
		}

		if (option == CONFIRM) { // storage
			INFO("Enabling storage device");
			quit();
			execlp("/bin/sh", "/bin/sh", "-c", "exec /usr/bin/retrofw storage on", NULL);
			return;
		}

		INFO("Enabling networking device");
		system("/usr/bin/retrofw network on");
		iconInet = sc.skinRes("imgs/inet.png");
	}

	void tvOutDialog(int16_t mode) {
		if (!file_exists("/proc/jz/tvout")) return;

		if (mode < 0) {
			MessageBox mb(this, tr["TV-out connected. Enable?"], "skin:icons/tv.png");
			mb.setButton(CONFIRM, tr["NTSC"]);
			mb.setButton(MANUAL,  tr["PAL"]);
			mb.setButton(CANCEL,  tr["OFF"]);

			mode = TV_OFF;
			switch (mb.exec()) {
				case CONFIRM:
					mode = TV_NTSC;
					break;
				case MANUAL:
					mode = TV_PAL;
					break;
			}
		}

		if (mode != getTVOut()) {
			setTVOut(mode);
			setBacklight(confInt["backlight"]);
			writeTmp();
			exit(0);
		}
	}

	int getBacklight() {
		char buf[32] = "-1";
		FILE *f;
		if (f = fopen("/proc/jz/lcd_backlight", "r")) {
			fgets(buf, sizeof(buf), f);
			fclose(f);
		}
		return atoi(buf);
	}

public:
	int setVolume(int val, bool popup = false) {
		val = GMenu2X::setVolume(val, popup);

		uint32_t soundDev = open("/dev/mixer", O_RDWR);
		if (soundDev) {
			int vol = (val << 8) | val;
			ioctl(soundDev, SOUND_MIXER_WRITE_VOLUME, &vol);
			close(soundDev);

		}
		volumeMode = getVolumeMode(val);

		return val;
	}

	int setBacklight(int val, bool popup = false) {
		if (val < 1 && getUDCStatus() != UDC_REMOVE) {
			val = 0; // suspend only if not charging
		}

		if (val >= 0) {
			val = GMenu2X::setBacklight(val, popup);
		}

		FILE *f;
		if (f = fopen("/proc/jz/lcd_backlight", "w")) {
			fprintf(f, "%d", val); // fputs(val, f);
			fclose(f);
		}

		if (f = fopen("/proc/jz/backlight_control", "w")) {
			fprintf(f, "%d", (val > 0)); // fputs(val, f);
			fclose(f);
		}

		return val;
	}

	void setScaleMode(unsigned int mode) {
		FILE *f;
		if (f = fopen("/proc/jz/ipu", "w")) {
			fprintf(f, "%d", mode); // fputs(val, f);
			fclose(f);
		}
	}

	void setTVOut(unsigned int mode) {
		FILE *f;
		if (f = fopen("/proc/jz/tvout", "w")) {
			fprintf(f, "%d", mode); // fputs(val, f);
			fclose(f);
		}
	}

	uint16_t getBatteryLevel() {
		int32_t val = getBatteryStatus();
		if ((val > 10000) || (val < 0)) return 6;
		else if (val > 4000) return 5; // 100%
		else if (val > 3900) return 4; // 80%
		else if (val > 3800) return 3; // 60%
		else if (val > 3730) return 2; // 40%
		else if (val > 3600) return 1; // 20%
		return 0; // 0% :(
	}

	void setCPU(uint32_t mhz) {
		mhz = constrain(mhz, confInt["cpuMenu"], confInt["cpuMax"]);
		if (memdev > 0) {
			DEBUG("Setting clock to %d", mhz);
			uint32_t m = mhz / 6;
			memregs[0x10 >> 2] = (m << 24) | 0x090520;
			INFO("CPU clock: %d MHz", mhz);
		}
	}

	string hwPreLinkLaunch() {
		system("[ -d /home/retrofw ] && mount -o remount,sync /home/retrofw");
		return "";
	}
};

#endif
