#include "powermanager.h"
#include "messagebox.h"
// #include "debug.h"

PowerManager *PowerManager::instance = NULL;

PowerManager::PowerManager(GMenu2X *gmenu2x, unsigned int suspendTimeout, unsigned int powerTimeout) {
	instance = this;
	this->suspendTimeout = suspendTimeout;
	this->powerTimeout = powerTimeout;
	this->gmenu2x = gmenu2x;
	this->suspendActive = false;

	this->powerTimer = NULL;

	resetSuspendTimer();
}

PowerManager::~PowerManager() {
	clearTimer();
	instance = NULL;
}

void PowerManager::setSuspendTimeout(unsigned int suspendTimeout) {
	this->suspendTimeout = suspendTimeout;
	resetSuspendTimer();
};

void PowerManager::setPowerTimeout(unsigned int powerTimeout) {
	this->powerTimeout = powerTimeout;
	resetSuspendTimer();
};

void PowerManager::clearTimer() {
	SDL_RemoveTimer(powerTimer); powerTimer = NULL;
};

void PowerManager::resetSuspendTimer() {
	clearTimer();
	powerTimer = SDL_AddTimer(this->suspendTimeout * 1e3, doSuspend, NULL);
};

void PowerManager::resetPowerTimer() {
	clearTimer();
	powerTimer = SDL_AddTimer(this->powerTimeout * 60e3, doPowerOff, NULL);
};

Uint32 PowerManager::doSuspend(unsigned int interval, void *param) {
	if (interval > 0) {
		MessageBox mb(PowerManager::instance->gmenu2x, PowerManager::instance->gmenu2x->tr["Suspend"]);
		mb.setAutoHide(500);
		mb.exec();

		PowerManager::instance->gmenu2x->setTVOut("OFF");
		PowerManager::instance->gmenu2x->setBacklight(0);
		PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMin"]);
		PowerManager::instance->resetPowerTimer();

		PowerManager::instance->suspendActive = true;

		return interval;
	}

	PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMenu"]);
	PowerManager::instance->gmenu2x->setTVOut(PowerManager::instance->gmenu2x->TVOut);
	PowerManager::instance->gmenu2x->setBacklight(max(10, PowerManager::instance->gmenu2x->confInt["backlight"]));
	PowerManager::instance->suspendActive = false;
	PowerManager::instance->resetSuspendTimer();
	return interval;
};

Uint32 PowerManager::doPowerOff(unsigned int interval, void *param) {
	if (interval > 0) {
#if !defined(TARGET_PC)
		system("poweroff");
#endif
		return interval;
	}

	PowerManager::instance->gmenu2x->poweroffDialog();
	return 0;
};
