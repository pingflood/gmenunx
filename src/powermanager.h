#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <SDL.h>
#include "gmenu2x.h"

class PowerManager {
public:
	PowerManager(GMenu2X *gmenu2x, unsigned int suspendTimeout, unsigned int powerTimeout);
	~PowerManager();
	void setSuspendTimeout(unsigned int suspendTimeout);
	void setPowerTimeout(unsigned int powerTimeout);
	void clearTimer();
	void resetSuspendTimer();
	void resetPowerTimer();
	static Uint32 doSuspend(unsigned int interval, void *param = NULL);
	static Uint32 doPowerOff(unsigned int interval, void *param = NULL);
	bool suspendActive;
	SDL_TimerID powerTimer; // = NULL;

private:
	GMenu2X *gmenu2x;
	unsigned int suspendTimeout, powerTimeout;
	static PowerManager *instance;
};

#endif
