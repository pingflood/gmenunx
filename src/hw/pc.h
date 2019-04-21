#ifndef HW_PC_H
#define HW_PC_H

uint32_t default_keymap[NUM_ACTIONS] = {
	0, // DO_NOTHING,
	SDLK_UP, // UP,
	SDLK_DOWN, // DOWN,
	SDLK_LEFT, // LEFT,
	SDLK_RIGHT, // RIGHT,
	SDLK_a, // CONFIRM,
	SDLK_b, // CANCEL,
	SDLK_y, // MANUAL,
	SDLK_x, // MODIFIER,
	SDLK_PAGEUP, // SECTION_PREV,
	SDLK_PAGEDOWN, // SECTION_NEXT,
	SDLK_EQUALS, // INC,
	SDLK_MINUS, // DEC,
	SDLK_PAGEUP, // PAGEUP,
	SDLK_PAGEDOWN, // PAGEDOWN,
	SDLK_RETURN, // SETTINGS,
	SDLK_MENU, // MENU,
	SDLK_LEFTBRACKET, // VOLUP,
	SDLK_RIGHTBRACKET, // VOLDOWN,
	SDLK_3, // BACKLIGHT,
	SDLK_END, // POWER,
	0, // UDC_CONNECT,
	0, // UDC_REMOVE,
	0, // MMC_INSERT,
	0, // MMC_REMOVE,
	0, // TV_CONNECT,
	0, // TV_REMOVE,
	0, // PHONES_CONNECT,
	0, // PHONES_REMOVE,
};


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
};

#endif