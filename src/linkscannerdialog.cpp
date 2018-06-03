#include "linkscannerdialog.h"

LinkScannerDialog::LinkScannerDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
	: Dialog(gmenu2x)
{
	this->title = title;
	this->description = description;
	this->icon = icon;
}

void LinkScannerDialog::exec() {
	gmenu2x->initBG();

	bool close = false;

	drawTopBar(gmenu2x->bg, title, description, icon);
	drawBottomBar(gmenu2x->bg);
	gmenu2x->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	gmenu2x->drawButton(gmenu2x->bg, "b", gmenu2x->tr["Back"]);

	gmenu2x->bg->blit(gmenu2x->s,0,0);

#if defined(TARGET_RS97)
	uint lineY = 80;
#else
	uint lineY = 42;
#endif

	// if (gmenu2x->confInt["menuClock"] < CPU_CLK_DEFAULT) {
		string strClock;
		stringstream ss;
		ss << gmenu2x->confInt["maxClock"];
		ss >> strClock;
		gmenu2x->s->write(gmenu2x->font, gmenu2x->tr.translate("Raising cpu clock to $1Mhz",  strClock.c_str(),  NULL), 5, lineY);
		gmenu2x->s->flip();
		lineY += 26;
		gmenu2x->setClock(gmenu2x->confInt["maxClock"]);
	// }

	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["Scanning SD filesystem..."], 5, lineY);
	gmenu2x->s->flip();
	lineY += 26;

	vector<string> files;
	scanPath("/mnt/sd", &files);

	//Onyl gph firmware has nand
	if (gmenu2x->fwType == "gph" && !gmenu2x->f200) {
		gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["Scanning NAND filesystem..."], 5, lineY);
		gmenu2x->s->flip();
		lineY += 26;
		scanPath("/mnt/nand", &files);
	}

	// stringstream ss;
	ss << files.size();
	string str = "";
	ss >> str;
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr.translate("$1 files found.", str.c_str(), NULL), 5, lineY);
	lineY += 26;
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["Creating links..."], 5, lineY);
	gmenu2x->s->flip();
	lineY += 26;

	string path, file;
	string::size_type pos;
	uint linkCount = 0;

	// ledOn();
	for (uint i = 0; i < files.size(); i++) {
		pos = files[i].rfind("/");
		if (pos != string::npos && pos > 0) {
			path = files[i].substr(0, pos + 1);
			file = files[i].substr(pos + 1, files[i].length());
			if (gmenu2x->menu->addLink(path, file, "found " + file.substr(file.length()-3, 3)))
				linkCount++;
		}
	}

	ss.clear();
	ss << linkCount;
	ss >> str;
	gmenu2x->s->write(gmenu2x->font, gmenu2x->tr.translate("$1 links created.", str.c_str(), NULL), 5, lineY);
	gmenu2x->s->flip();
	lineY += 26;

	// if (gmenu2x->confInt["menuClock"] < CPU_CLK_DEFAULT) {
		gmenu2x->s->write(gmenu2x->font, gmenu2x->tr["Decreasing CPU clock"], 5, lineY);
		gmenu2x->s->flip();
		lineY += 26;
		gmenu2x->setClock(gmenu2x->confInt["menuClock"]);
	// }

	sync();
	// ledOff();

	while (!close) {
		bool inputAction = gmenu2x->input.update();
		if (gmenu2x->powerManager(inputAction) || gmenu2x->inputCommonActions()) continue;
		else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) close = true;
	}
}


void LinkScannerDialog::scanPath(string path, vector<string> *files) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath, ext;

	if (path[path.length()-1]!='/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0]=='.')
			continue;
		filepath = path+dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			scanPath(filepath, files);
		if (statRet != -1) {
			ext = filepath.substr(filepath.length()-4,4);
#if defined(TARGET_GP2X) || defined(TARGET_WIZ) || defined(TARGET_CAANOO)
			if (ext==".gpu" || ext==".gpe")
#endif
				files->push_back(filepath);
		}
	}

	closedir(dirp);
}
