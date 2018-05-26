#include "batteryloggerdialog.h"

BatteryLoggerDialog::BatteryLoggerDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
	: Dialog(gmenu2x)
{
	this->title = title;
	this->description = description;
	this->icon = icon;
}

void BatteryLoggerDialog::exec() {
	gmenu2x->initBG();

	bool close = false;

	drawTopBar(gmenu2x->bg, title, description, icon);

	gmenu2x->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	drawBottomBar(gmenu2x->bg);
	gmenu2x->drawButton(gmenu2x->bg, "b", gmenu2x->tr["Back"],
	gmenu2x->drawButton(gmenu2x->bg, "select", gmenu2x->tr["Del battery.csv"],
	gmenu2x->drawButton(gmenu2x->bg, "down", gmenu2x->tr["Scroll"],
	gmenu2x->drawButton(gmenu2x->bg, "up", "", 5)-10)));

	gmenu2x->bg->blit(gmenu2x->s,0,0);

	gmenu2x->setBacklight(100);
	gmenu2x->setClock(CPU_CLK_MAX);

// // skinConfColor
// 	int i = 0, j = 0;
// 	for(ConfColorHash::iterator curr = skinConfColor.begin(); curr != skinConfColor.end(); curr++) {
// 		i++;
// 		if (i > 31) {
// 			j++;
// 			i=0;
// 		}
// 		if (j > 14) break;

// 		DEBUG("COLOR: %d,%d %s = %d", i,j, curr->first.c_str(),  (unsigned short)curr->second.r);
// 		gmenu2x->bg->box(2+i*10,2+j*10,8,8, curr->second);
// 	}

	gmenu2x->s->flip();

	MessageBox mb(gmenu2x, gmenu2x->tr["Welcome to the Battery Logger.\nMake sure the battery is fully charged.\nAfter pressing OK, leave the device ON until\nthe battery has been fully discharged.\nThe log will be saved in 'battery.csv'."]);
	mb.exec();

	uint firstRow = 0, rowsPerPage = gmenu2x->listRect.h/gmenu2x->font->getHeight();

	long tickNow = 0, tickStart = SDL_GetTicks(), tickBatteryLogger = -1000000;
	string logfile = gmenu2x->getExePath()+"battery.csv";

	char buf[100];
	sprintf(buf, "echo '----' >> %s/battery.csv; sync", cmdclean(gmenu2x->getExePath()).c_str());
	system(buf);

	if (!fileExists(logfile)) return;

	ifstream inf(logfile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	vector<string> log;

	string line;
	while (getline(inf, line, '\n'))
		log.push_back(line);
	inf.close();

	while (!close) {
		tickNow = SDL_GetTicks();
		if ((tickNow - tickBatteryLogger) >= 60000) {
			tickBatteryLogger = tickNow;

			char buf[100];
			sprintf(buf, "echo '%s,%d,%d' >> %s/battery.csv; sync", ms2hms(tickNow - tickStart, true, false), gmenu2x->getBatteryStatus(), gmenu2x->getBatteryLevel(), cmdclean(gmenu2x->getExePath()).c_str());
			system(buf);

			ifstream inf(logfile.c_str(), ios_base::in);
			log.clear();

			// string line;
			while (getline(inf, line, '\n'))
				log.push_back(line);
			inf.close();
		}

		gmenu2x->bg->blit(gmenu2x->s,0,0);

		for (uint i = firstRow; i < firstRow + rowsPerPage && i < log.size(); i++) {
			int rowY, j = log.size() - i - 1;
			if (log.at(j)=="----") { //draw a line
				rowY = 42 + (int)((i - firstRow + 0.5) * gmenu2x->font->getHeight());
				gmenu2x->s->hline(5, rowY, gmenu2x->resX - 16, 255, 255, 255, 130);
				gmenu2x->s->hline(5, rowY + 1, gmenu2x->resX - 16, 0, 0, 0, 130);
			} else {
				rowY = 42 + (i - firstRow) * gmenu2x->font->getHeight();
				gmenu2x->font->write(gmenu2x->s, log.at(j), 5, rowY);
			}
		}

		gmenu2x->drawScrollBar(rowsPerPage, log.size(), firstRow, gmenu2x->listRect);

		gmenu2x->s->flip();

		gmenu2x->input.update(false);

		if (gmenu2x->inputCommonActions()) continue;

		if ( gmenu2x->input[UP  ] && firstRow > 0 ) firstRow--;
		else if ( gmenu2x->input[DOWN] && firstRow + rowsPerPage < log.size() ) firstRow++;
		else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT]) {
			if (firstRow >= rowsPerPage - 1)
				firstRow -= rowsPerPage - 1;
			else
				firstRow = 0;
		}
		else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT]) {
			if (firstRow + rowsPerPage * 2 - 1 < log.size())
				firstRow += rowsPerPage - 1;
			else
				firstRow = max(0,log.size()-rowsPerPage);
		}
		else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) close = true;
		else if (gmenu2x->input[MENU]) {
			MessageBox mb(gmenu2x, gmenu2x->tr.translate("Deleting $1", "battery.csv", NULL) + "\n" + gmenu2x->tr["Are you sure?"]);
			mb.setButton(CONFIRM, gmenu2x->tr["Yes"]);
			mb.setButton(CANCEL,  gmenu2x->tr["No"]);
			if (mb.exec() == CONFIRM) {
				system("rm battery.csv");
				log.clear();
			}
		}
	}
	gmenu2x->setBacklight(gmenu2x->confInt["backlight"]);
}
