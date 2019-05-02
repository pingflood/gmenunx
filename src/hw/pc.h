#ifndef HW_PC_H
#define HW_PC_H

volatile uint16_t *memregs;
uint8_t memdev = 0;
int SOUND_MIXER_READ = SOUND_MIXER_READ_PCM;
int SOUND_MIXER_WRITE = SOUND_MIXER_WRITE_PCM;

uint32_t hwCheck(unsigned int interval = 0, void *param = NULL) {
	printf("%s:%d: %s\n", __FILE__, __LINE__, __func__);
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
		resX = 480;
		resY = 272;
	}
};

#endif