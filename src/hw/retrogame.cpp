#ifndef HW_RETROGAME_H
#define HW_RETROGAME_H

#define HW_UDC // hardware have UDC support
#define HW_EXT_SD // hardware have external sd card support
#define HW_SCALER // hardware have screen scaler (e.g., IPU)

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

int32_t getBatteryStatus() {
	char buf[32] = "-1";
	FILE *f = fopen("/proc/jz/battery", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	return atol(buf);
}

uint16_t getTVOut() {
	char buf[32] = "0";
	FILE *f = fopen("/proc/jz/tvout", "r");
	if (f) {
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

	if (memdev > 0) {
		// printf("\e[s\e[1;0f\e[1;32m\n");
		// printf("               3          2          1          0\n");
		// printf("              10987654 32109876 54321098 76543210\n");
		// printbin("0", memregs[0x09600 >> 2]);
		// printbin("1", memregs[0x09700 >> 2]);
		// printbin("2", memregs[0x09800 >> 2]);
		// printbin("3", memregs[0x09900 >> 2]);
		// printbin("4", memregs[0x09a00 >> 2]);
		// printbin("5", memregs[0x09b00 >> 2]);
		// printbin("6", memregs[0x09c00 >> 2]);
		// printbin("7", memregs[0x09d00 >> 2]);
		// printbin("8", memregs[0x09e00 >> 2]);
		// printbin("9", memregs[0x09f00 >> 2]);
		// printbin("A", memregs[0x10000 >> 2]);
		// printbin("B", memregs[0x10100 >> 2]);
		// printbin("C", memregs[0x10200 >> 2]);
		// printbin("D", memregs[0x10300 >> 2]);
		// printbin("E", memregs[0x10400 >> 2]);
		// printbin("F", memregs[0x10500 >> 2]);
		// printf("\n\e[30;0m\e[K\e[u");

		udcStatus = getUDCStatus();
		if (udcPrev != udcStatus) {
			udcPrev = udcStatus;
			InputManager::pushEvent(udcStatus);
		}

		mmcStatus = getMMCStatus();
		if (mmcPrev != mmcStatus) {
			mmcPrev = mmcStatus;
			InputManager::pushEvent(mmcStatus);
		}

		tvOutStatus = getTVOutStatus();
		if (tvOutPrev != tvOutStatus) {
			tvOutPrev = tvOutStatus;
			InputManager::pushEvent(tvOutStatus);
		}

		volumeMode = getVolumeMode(1);
		if (volumeModePrev != volumeMode) {
			volumeModePrev = volumeMode;
			InputManager::pushEvent(PHONES_CONNECT);
		}


// SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
// SDL_InitSubSystem(SDL_INIT_JOYSTICK);
// SDL_JOYDEVICEADDED
// SDL_JoystickUpdate()
		numJoy = SDL_NumJoysticks();
		if (numJoyPrev != numJoy) {
			numJoyPrev = numJoy;
			InputManager::pushEvent(JOYSTICK_CONNECT);
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

		setenv("SDL_NOMOUSE", "1", 1);
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
		}
		close(fbdev);

		if (w == 320 && h == 480) h = 240;

		INFO("RETROGAME");
	}

	void udcDialog(int udcStatus) {
		if (udcStatus == UDC_REMOVE) {
			INFO("USB Disconnected. Unloading modules...");
			system("/usr/bin/retrofw stop; /etc/init.d/S99recovery stop; /etc/init.d/S80recovery stop");
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
			INFO("Enabling gadget-lun storage device");
			quit();

			if (file_exists("/usr/bin/retrofw")) execlp("/bin/sh", "/bin/sh", "-c", "/usr/bin/retrofw storage on", NULL);
			else if (file_exists("/etc/init.d/S99recovery")) execlp("/bin/sh", "/bin/sh", "-c", "/etc/init.d/S99recovery storage on", NULL);
			else if (file_exists("/etc/init.d/S80recovery")) execlp("/bin/sh", "/bin/sh", "-c", "/etc/init.d/S80recovery storage on", NULL);
			return;
		}
		INFO("Enabling usb0 networking device");
		system("/usr/bin/retrofw network on &");
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
		FILE *f = fopen("/proc/jz/lcd_backlight", "r");
		if (f) {
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
		val = GMenu2X::setBacklight(val, popup);

		char buf[128] = {0};
		sprintf(buf, "echo %d > /proc/jz/lcd_backlight; echo %d > /proc/jz/backlight_control", val, val);
		system(buf);

		return val;
	}

	void setScaleMode(unsigned int mode) {
		if (!file_exists("/proc/jz/ipu_ratio")) return;

		char buf[128] = {0};
		sprintf(buf, "echo %d > /proc/jz/ipu_ratio", mode);
		system(buf);
	}

	void setTVOut(unsigned int mode) {
		if (!file_exists("/proc/jz/tvout") || mode > 2) return;
		char buf[128] = {0};
		// sprintf(buf, "echo %d > /proc/jz/tvout; echo 1 > /proc/jz/tvout &> /dev/null", mode);
		sprintf(buf, "echo %d > /proc/jz/tvout", mode);
		system(buf);
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
			INFO("Set CPU clock: %d", mhz);
		}
	}

	string hwPreLinkLaunch() {
		system("[ -d /home/retrofw ] && mount -o remount,sync /home/retrofw");
		return "";
	}
};

#endif
