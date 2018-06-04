#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <SDL.h>
#include "gmenu2x.h"

class PowerManager {
public:
	PowerManager(GMenu2X *gmenu2x, unsigned int suspendTimeout, unsigned int powerTimeout);
	~PowerManager();
	// void resetScreenTimer();
	void resetSuspendTimeout();
	void resetPowerTimeout();
	void doRestart(bool showDialog);
	// void doPoweroff(bool showDialog);
	// void doSuspend(bool suspend);
	static Uint32 doSuspend(unsigned int interval, void * param);
	static Uint32 doPowerOff(unsigned int interval, void * param);
	bool suspendActive = false;

private:
	unsigned int suspendTimeout, powerTimeout;
	GMenu2X *gmenu2x;

	// void addScreenTimer();
	// void removeScreenTimer();
	// void setScreenBlanking(bool state);
	// void enableScreen();
	// void disableScreen();

	static PowerManager *instance;
	// bool screenState;
	// unsigned int screenTimeout;
	// unsigned int timeout_startms;
	SDL_TimerID powerTimer = NULL;

	// friend 
	// friend 
};

#endif
