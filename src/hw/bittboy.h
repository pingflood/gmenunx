#ifndef HW_BITTBOY_H
#define HW_BITTBOY_H

/*	BittBoy Key Codes. pingflood, 2019
	BUTTON     GMENU          SDL             NUMERIC   GPIO
	-----------------------------------------------------------------------------
	A          CONFIRM        SDLK_LCTRL      306
	B          CANCEL         SDLK_SPACE      32
	TA         MANUAL         SDLK_LALT       308
	TB         MODIFIER       SDLK_LSHIFT     304
	R          SECTION_NEXT   SDLK_RCTRL      305
	START      SETTINGS       SDLK_RETURN     13
	SELECT     MENU           SDLK_ESCAPE     27
	UP         UP             SDLK_UP         273
	DOWN       DOWN           SDLK_DOWN       274
	RIGHT      RIGHT          SDLK_RIGHT      275
	LEFT       LEFT           SDLK_LEFT       276

	Pocket-Go Key Codes. pingflood, 2019
	BUTTON     GMENU          SDL             NUMERIC   GPIO
	-----------------------------------------------------------------------------
	A          CONFIRM        SDLK_LALT       308
	B          CANCEL         SDLK_LCTRL      306
	X          MODIFIER       SDLK_LSHIFT     304
	Y          MANUAL         SDLK_SPACE      32
	L          SECTION_PREV   SDLK_TAB        9
	R          SECTION_NEXT   BACKSPACE       8
	RESET      POWER          SDLK_RCTRL      305
	START      SETTINGS       SDLK_RETURN     13
	SELECT     MENU           SDLK_ESCAPE     27
	UP         UP             SDLK_UP         273
	DOWN       DOWN           SDLK_DOWN       274
	RIGHT      RIGHT          SDLK_RIGHT      275
	LEFT       LEFT           SDLK_LEFT       276
*/

#undef VOLUME_HOTKEY
#undef BACKLIGHT_HOTKEY

#define VOLUME_HOTKEY		MANUAL
#define BACKLIGHT_HOTKEY	MODIFIER

#define MIYOO_SND_SET_VOLUME  _IOWR(0x100, 0, unsigned long)
#define MIYOO_KBD_GET_HOTKEY  _IOWR(0x100, 0, unsigned long)
#define MIYOO_KBD_SET_VER     _IOWR(0x101, 0, unsigned long)
#define MIYOO_FB0_GET_VER     _IOWR(0x102, 0, unsigned long)

uint32_t oc_table[] = {
	0x00c81802,
	0x00cc1013,
	0x00cc1001,
	0x00d01902,
	0x00d00c12,
	0x00d80b23,
	0x00d81101,
	0x00d80833,
	0x00d81a02,
	0x00d80811,
	0x00d81113,
	0x00d80220,
	0x00d80521,
	0x00d80822,
	0x00d80800,
	0x00e01b02,
	0x00e00d12,
	0x00e00632,
	0x00e41201,
	0x00e41213,
	0x00e81c02,
	0x00ea0c23,
	0x00f00922,
	0x00f00900,
	0x00f01301,
	0x00f00933,
	0x00f00911,
	0x00f01313,
	0x00f01d02,
	0x00f00431,
	0x00f00e12,
	0x00f00410,
	0x00f81e02,
	0x00fc0d23,
	0x00fc0621,
	0x00fc1413,
	0x00fc1401,
	0x01000732,
	0x01001f02,
	0x01000f12,
	0x01081501,
	0x01080a11,
	0x01080a22,
	0x01081513,
	0x01080a00,
	0x01080a33,
	0x010e0e23,
	0x01101012,
	0x01141601,
	0x01141613,
	0x01200531,
	0x01200f23,
	0x01200832,
	0x01200b11,
	0x01200230,
	0x01200721,
	0x01200b22,
	0x01200320,
	0x01201701,
	0x01200b33,
	0x01200b00,
	0x01201713,
	0x01201112,
	0x01200510,
	0x012c1801,
	0x012c1813,
	0x01301212,
	0x01321023,
	0x01380c11,
	0x01380c22,
	0x01381901,
	0x01380c00,
	0x01380c33,
	0x01381913,
	0x01401312,
	0x01400932,
	0x01441123,
	0x01441a13,
	0x01441a01,
	0x01440821,
	0x01501b13,
	0x01501412,
	0x01500631,
	0x01500610,
	0x01500d22,
	0x01500d00,
	0x01501b01,
	0x01500d33,
	0x01500d11,
	0x01561223,
	0x015c1c01,
	0x015c1c13,
	0x01601512,
	0x01600a32,
	0x01680921,
	0x01681d01,
	0x01680420,
	0x01680e22,
	0x01681d13,
	0x01680e00,
	0x01681323,
	0x01680e33,
	0x01680e11,
	0x01701612,
	0x01741e01,
	0x01741e13,
	0x017a1423,
	0x01800f33,
	0x01800710,
	0x01800731,
	0x01800f11,
	0x01801712,
	0x01800330,
	0x01800f00,
	0x01801f01,
	0x01800f22,
	0x01800b32,
	0x01801f13,
	0x018c0a21,
	0x018c1523,
	0x01901812,
	0x01981022,
	0x01981000,
	0x01981033,
	0x01981011,
	0x019e1623,
	0x01a01912,
	0x01a00c32,
	0x01b00520,
	0x01b00810,
	0x01b01111,
	0x01b01723,
	0x01b00831,
	0x01b01100,
	0x01b01122,
	0x01b00b21,
	0x01b01a12,
	0x01b01133,
	0x01c01b12,
	0x01c00d32,
	0x01c21823,
	0x01c81233,
	0x01c81211,
	0x01c81222,
	0x01c81200,
	0x01d01c12,
	0x01d41923,
	0x01d40c21,
	0x01e00931,
	0x01e01333,
	0x01e00430,
	0x01e00e32,
	0x01e01311,
	0x01e00910,
	0x01e01300,
	0x01e01322,
	0x01e01d12,
	0x01e61a23,
	0x01f01e12,
	0x01f81400,
	0x01f81b23,
	0x01f81433,
	0x01f81411,
	0x01f80d21,
	0x01f80620,
	0x01f81422,
	0x02001f12,
	0x02000f32,
	0x020a1c23,
	0x02101522,
	0x02101500,
	0x02101533,
	0x02101511,
	0x02100a10,
	0x02100a31,
	0x021c0e21,
	0x021c1d23,
	0x02201032,
	0x02281611,
	0x02281622,
	0x02281600,
	0x02281633,
	0x022e1e23,
	0x02401722,
	0x02400b10,
	0x02401733,
	0x02401700,
	0x02400b31,
	0x02400530,
	0x02401132,
	0x02401711,
	0x02400f21,
	0x02400720,
	0x02401f23,
	0x02581800,
	0x02581833,
	0x02581811,
	0x02581822,
	0x02601232,
	0x02641021,
	0x02701911,
	0x02700c10,
	0x02700c31,
	0x02701922,
	0x02701900,
	0x02701933,
	0x02801332,
	0x02881a11,
	0x02880820,
	0x02881121,
	0x02881a22,
	0x02881a00,
	0x02881a33,
	0x02a00d31,
	0x02a01432,
	0x02a01b11,
	0x02a00630,
	0x02a01b00,
	0x02a01b22,
	0x02a00d10,
	0x02a01b33,
	0x02ac1221,
	0x02b81c00,
	0x02b81c33,
	0x02b81c11,
	0x02b81c22,
	0x02c01532,
	0x02d01d00,
	0x02d01d22,
	0x02d00920,
	0x02d01d33,
	0x02d00e10,
	0x02d00e31,
	0x02d01d11,
	0x02d01321,
	0x02e01632,
	0x02e81e00,
	0x02e81e33,
	0x02e81e11,
	0x02e81e22,
	0x02f41421,
	0x03000730,
	0x03000f10,
	0x03001f11,
	0x03000f31,
	0x03001f00,
	0x03001f22,
	0x03001732,
	0x03001f33,
	0x03180a20,
	0x03181521,
	0x03201832,
	0x03301031,
	0x03301010,
	0x033c1621,
	0x03401932,
	0x03600830,
	0x03601721,
	0x03601a32,
	0x03600b20,
	0x03601110,
	0x03601131,
	0x03801b32,
	0x03841821,
};

#define MIYOO_LID_FILE "/mnt/.backlight.conf"
static int read_conf(const char *file)
{
	int val = 5;
	char buf[10] = {0};
	int fd = open(file, O_RDWR);
	if (fd < 0) val = -1;
	else {
		read(fd, buf, sizeof(buf));
		for (int i = 0; i < strlen(buf); i++) {
			if(buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ') {
				buf[i] = 0;
			}
		}
		val = atoi(buf);
	}
	close(fd);
	return val;
}

volatile uint32_t *memregs;
uint8_t memdev = 0;
int SOUND_MIXER_READ = SOUND_MIXER_READ_PCM;
// int SOUND_MIXER_WRITE = SOUND_MIXER_WRITE_PCM;

uint32_t hwCheck(unsigned int interval = 0, void *param = NULL) {

  unsigned long ret;

int fb0 = open("/dev/miyoo_fb0", O_RDWR);
int kbd = open("/dev/miyoo_kbd", O_RDWR);
  ioctl(fb0, MIYOO_FB0_GET_VER, &ret);
  ioctl(kbd, MIYOO_KBD_SET_VER, ret);
// int vir = open("/dev/miyoo_vir", O_RDWR);
  // ioctl(vir, MIYOO_VIR_SET_VER, ret);
  // close(vir);
    ioctl(kbd, MIYOO_KBD_GET_HOTKEY, &ret);
    if (!ret) return 0;

    switch(ret) {
    case 1:
      printf("backlight++\n");
      // if(lid < 10){
        // lid+= 1;
        // write_conf(MIYOO_LID_FILE, lid);
        // sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF);
        // system(buf);
        // info_fb0(fb0, lid, vol, 1);
      // }
      break;
    case 2:
      printf("backlight--\n");
      // if(lid > 1){
      //   lid-= 1;
      //   write_conf(MIYOO_LID_FILE, lid);
      //   sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF);
      //   system(buf);
      //   info_fb0(fb0, lid, vol, 1);
      // }
      break;
    case 3:
      printf("sound++\n");
      // if(vol < 9){
      //   vol+= 1;
      //   write_conf(MIYOO_VOL_FILE, vol);
      //   ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
      //   info_fb0(fb0, lid, vol, 1);
      // }
      break;
    case 4:
      printf("sound--\n");
      // if(vol > 0){
      //   vol-= 1;
      //   write_conf(MIYOO_VOL_FILE, vol);
      //   ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
      //   info_fb0(fb0, lid, vol, 1);
      // }
      break;
    }
  close(fb0);
  close(kbd);
  // close(snd);

	return 0;
}

uint8_t getMMCStatus() {
	return MMC_REMOVE;
}

uint8_t getUDCStatus() {
	return UDC_REMOVE;
}

uint8_t getTVOutStatus() {
	return TV_REMOVE;
}

int32_t getBatteryStatus() {
	char buf[32] = "-1";
	FILE *f = fopen("/sys/devices/platform/soc/1c23400.battery/power_supply/miyoo-battery/voltage_now", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	return atol(buf);
}

uint8_t getVolumeMode(uint8_t vol) {
	return VOLUME_MODE_NORMAL;
}

class GMenuNX : public GMenu2X {
private:
	void hwInit() {
		system("mount -o remount,async /mnt");

		setenv("SDL_NOMOUSE", "1", 1);
		memdev = open("/dev/mem", O_RDWR);
		if (memdev > 0) {
			memregs = (uint32_t*)mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, memdev, 0x01c20000);
			if (memregs == MAP_FAILED) {
				ERROR("Could not mmap hardware registers!");
				close(memdev);
			}
		} else {
			WARNING("Could not open /dev/mem");
		}
		resX = 320;
		resY = 240;
		INFO("BITTBOY Init Done!");
	}

	int getBacklight() {
		char buf[32] = "-1";
		FILE *f = fopen("/sys/devices/platform/backlight/backlight/backlight/brightness", "r");
		if (f) {
			fgets(buf, sizeof(buf), f);
			fclose(f);
		}
		return atoi(buf);
	}

public:
	int setVolume(int val, bool popup = false) {
		val = GMenu2X::setVolume(val, popup);

		uint32_t snd = open("/dev/miyoo_snd", O_RDWR);

		if (snd) {
			int vol = val / 20;
			ioctl(snd, MIYOO_SND_SET_VOLUME, &vol);
			close(snd);
		}
		volumeMode = getVolumeMode(val);

		return val;
	}

	int setBacklight(int val, bool popup = false) {
		val = GMenu2X::setBacklight(val, popup);
		char buf[128] = {0};
		// int lid = read_conf(MIYOO_LID_FILE);
		// return 80;
		// if (val == 0) lid = 0;
		// sprintf(buf, "echo %d > /sys/devices/platform/backlight/backlight/backlight/brightness", lid);
		// sprintf(buf, "echo %d > /sys/devices/platform/backlight/backlight/backlight/brightness", val);
		// system(buf);
		return val;
	}

	void setCPU(uint32_t mhz) {
		uint32_t v;
		uint32_t total = sizeof(oc_table) / sizeof(oc_table[0]);

		for (int x = 0; x < total; x++) {
			if ((oc_table[x] >> 16) >= mhz) {
				v = memregs[0];
				v &= 0xffff0000;
				v |= (oc_table[x] &  0x0000ffff);
				memregs[0] = v;
				break;
			}
		}

		INFO("Set CPU clock: %d(0x%08x)", mhz, v);
	}

	string hwPreLinkLaunch() {
		system("mount -o remount,sync /mnt");
		return "";
	}
};

#endif
