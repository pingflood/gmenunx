#include "powermanager.h"
#include "messagebox.h"
#include "debug.h"


PowerManager *PowerManager::instance = NULL;


PowerManager::PowerManager(GMenu2X *gmenu2x, unsigned int suspendTimeout, unsigned int powerTimeout) {
	// this->gmenu2x = gmenu2x;
	ERROR("POWER START");

	// assert(!instance);
	instance = this;
	this->suspendTimeout = suspendTimeout;
	this->powerTimeout = powerTimeout;
	this->gmenu2x = gmenu2x;

	// suspendActive = false;

	resetSuspendTimeout();
	// resetPowerTimeout(powerTimeout);
}

PowerManager::~PowerManager() {
	if (powerTimer != NULL) SDL_RemoveTimer(powerTimer);
	instance = NULL;
	// enableScreen();
}

void PowerManager::resetSuspendTimeout() {
	ERROR("resetSuspendTimeout");

	if (powerTimer != NULL) SDL_RemoveTimer(powerTimer);
	powerTimer = SDL_AddTimer(this->suspendTimeout * 1000, doSuspend, NULL);
};

void PowerManager::resetPowerTimeout() {
	if (powerTimer != NULL) SDL_RemoveTimer(powerTimer);
	powerTimer = SDL_AddTimer(this->powerTimeout * 1000, doPowerOff, NULL);
};

void PowerManager::doRestart(bool showDialog = false) {
};


Uint32 PowerManager::doSuspend(unsigned int interval, void * param) {
	if (interval > 0) {
		ERROR("POWER MANAGER ENTER SUSPEND");
	
		// MessageBox mb(this, tr["Suspend PM"]);
		MessageBox mb(PowerManager::instance->gmenu2x, PowerManager::instance->gmenu2x->tr["Suspend PM"]);
		mb.setAutoHide(500);
		mb.exec();

		PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMin"]);
		PowerManager::instance->gmenu2x->setBacklight(0);
		INFO("Enter suspend mode.");
	
		PowerManager::instance->resetPowerTimeout();

		PowerManager::instance->suspendActive = true;

		return interval;
	}

	PowerManager::instance->gmenu2x->setCPU(PowerManager::instance->gmenu2x->confInt["cpuMenu"]);
	PowerManager::instance->gmenu2x->setBacklight(max(10, PowerManager::instance->gmenu2x->confInt["backlight"]));
	INFO("Exit from suspend mode. Restore backlight to: %d", PowerManager::instance->gmenu2x->confInt["backlight"]);

	PowerManager::instance->suspendActive = false;

	ERROR("POWER MANAGER EXIT SUSPEND");
	PowerManager::instance->resetSuspendTimeout();
};

Uint32 PowerManager::doPowerOff(unsigned int interval, void * param) {
	if (interval > 0) {
		ERROR("POWER MANAGER DO POWEROFF");
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
		system("reboot");
#endif
	}};






	// PowerManager(GMenu2X *gmenu2x);
	// ~PowerManager();
	// // void resetScreenTimer();

