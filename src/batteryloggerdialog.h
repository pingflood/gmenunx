#ifdef BATTERYLOGGER

#ifndef BATTERYLOGGERDIALOG_H_
#define BATTERYLOGGERDIALOG_H_

#include <string>
#include <fstream>

#include "gmenu2x.h"
#include "dialog.h"
#include "messagebox.h"

using namespace std;
using std::string;
using std::vector;
using std::ifstream;
using std::ios_base;

class BatteryLoggerDialog : protected Dialog {
protected:
	string title, description, icon;

public:
	BatteryLoggerDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon);
	void exec();
};

#endif /*BATTERYLOGGERDIALOG_H_*/

#endif