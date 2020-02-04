#ifndef HW_PC_H
#define HW_PC_H

#define HW_UDC // hardware have UDC support
#define HW_EXT_SD // hardware have external sd card support
#define HW_SCALER // hardware have screen scaler (e.g., IPU)
// #define OPK_SUPPORT // firmware support opk
// #define IPK_SUPPORT // firmware support ipk

volatile uint16_t *memregs;
uint8_t memdev = 0;
int SOUND_MIXER_READ = SOUND_MIXER_READ_PCM;
int SOUND_MIXER_WRITE = SOUND_MIXER_WRITE_PCM;

const int CPU_MENU = 600;
const int CPU_MAX = 700;
const int CPU_MIN = 500;
const int CPU_STEP = 5;


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

uint8_t numJoyPrev = 0;

uint32_t hwCheck(unsigned int interval = 0, void *param = NULL) {
	printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
	numJoy = getDevStatus();
	if (numJoyPrev != numJoy) {
		numJoyPrev = numJoy;
		InputManager::pushEvent(JOYSTICK_CONNECT);
	}

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
	return 6;
}

uint8_t getVolumeMode(uint8_t vol) {
	return VOLUME_MODE_NORMAL;
}

class GMenuNX : public GMenu2X {
private:
	void hwInit() {
		w = 480;
		h = 272;
	}

	uint16_t getBatteryLevel() {
		return 6;
	};
};

#endif
