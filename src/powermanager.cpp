#include "powermanager.h"
#include "messagebox.h"
#include "debug.h"

PowerManager *PowerManager::instance = NULL;

PowerManager::PowerManager(GMenu2X *gmenu2x, unsigned int suspendTimeout, unsigned int powerTimeout) {
	instance = this;
	this->suspendTimeout = suspendTimeout;
	this->powerTimeout = powerTimeout;
	this->gmenu2x = gmenu2x;
	this->suspendActive = false;

	this->powerTimer = NULL;

	resetSuspendTimer();
	// resetPowerTimeout(powerTimeout);
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
	ERROR("clearTimer");
	if (powerTimer != NULL) SDL_RemoveTimer(powerTimer);
	powerTimer = NULL;
};

void PowerManager::resetSuspendTimer() {
	ERROR("resetSuspendTimer");
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

		PowerManager::instance->gmenu2x->setBacklight(0);
		PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMin"]);
		PowerManager::instance->resetPowerTimer();

		PowerManager::instance->suspendActive = true;

		return interval;
	}

	PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMenu"]);
	PowerManager::instance->gmenu2x->setBacklight(max(10, PowerManager::instance->gmenu2x->confInt["backlight"]));
	PowerManager::instance->suspendActive = false;
	PowerManager::instance->resetSuspendTimer();
};

Uint32 PowerManager::doPowerOff(unsigned int interval, void *param) {
	if (interval > 0) {
#if !defined(TARGET_PC)
		system("poweroff");
#endif
		return interval;
	}

	MessageBox mb(PowerManager::instance->gmenu2x, PowerManager::instance->gmenu2x->tr["Poweroff or reboot the device?"], "skin:icons/exit.png");
	mb.setButton(SECTION_NEXT, PowerManager::instance->gmenu2x->tr["Reboot"]);
	mb.setButton(CONFIRM, PowerManager::instance->gmenu2x->tr["Poweroff"]);
	mb.setButton(CANCEL,  PowerManager::instance->gmenu2x->tr["Cancel"]);
	int response = mb.exec();
	if (response == CONFIRM) {
		MessageBox mb(PowerManager::instance->gmenu2x, PowerManager::instance->gmenu2x->tr["Poweroff"]);
		mb.setAutoHide(500);
		mb.exec();
		// setSuspend(true);
		// SDL_Delay(500);

#if !defined(TARGET_PC)
		PowerManager::instance->gmenu2x->setBacklight(0);
		system("poweroff");
#endif
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(PowerManager::instance->gmenu2x, PowerManager::instance->gmenu2x->tr["Rebooting"]);
		mb.setAutoHide(500);
		mb.exec();
		// setSuspend(true);
		// SDL_Delay(500);

#if !defined(TARGET_PC)
		PowerManager::instance->gmenu2x->setBacklight(0);
		system("reboot");
#endif
	}
};
