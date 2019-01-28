#ifndef HW_PC_H
#define HW_PC_H

volatile uint16_t *memregs;
int SOUND_MIXER = SOUND_MIXER_READ_PCM;

static uint32_t hwCheck(unsigned int interval = 0, void *param = NULL) {
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

class hwGMenu2X : public GMenu2X {
};

#endif