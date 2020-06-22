#ifndef SKINDIALOG_H_
#define SKINDIALOG_H_

#include <string>
#include "gmenu2x.h"
#include "dialog.h"
#include "filelister.h"

using std::string;
using std::vector;

class SkinDialog : protected Dialog, public FileLister {
public:
	SkinDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon);
	~SkinDialog();

	string skin;
	vector<string> skins;

	bool exec();
};

#endif /*SKINDIALOG_H_*/
