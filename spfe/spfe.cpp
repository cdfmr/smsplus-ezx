/*
 * SMS Plus / EZX frontend
 * Copyright (C) 2006 Lin Fan (cnlinfan@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Further, this software is distributed without any warranty that it is
 * free of the rightful claim of any third person regarding infringement
 * or the like.  Any license provided herein, whether implied or
 * otherwise, applies only to this software file.  Patent licenses, if
 * any, provided herein do not apply to combinations of this program with
 * other software, or any other product whatsoever.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <qwsevent_qws.h>
#include <zapplication.h>
#include <zmainwidget.h>
#include <zmessagebox.h>
#include <ezxutilcst.h>
#include <ezxres.h>
#include <qlistbox.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qtextcodec.h>

#define SMSPLUS_ABOUT	"<p align=\"center\"><b>SMS Plus / EZX 1.1</b><br>Under GPL v2<br><br>(C) 2005-2006 Lin Fan<br>cnlinfan@gmail.com</p>"
#define CONFIG_FILE	"smsplus.cfg"
#define CONFIG_HEAD	"#    CONFIGURATION FILE OF SMS PLUS / EZX    #"
#define FILE_FILTER	"*.zip *.sms *.gg"
#define SMSPLUS_BIN	"smsplus"
#define SMSPLUS_DOC	"README"
#define DEF_ROM_DIR	"/mmc/mmca1/Game/SMS.GG"

#define EZX_RES_PIXMAP_FOLDER	"FMMS_Personal_Folder_S.gif"
#define EZX_RES_PIXMAP_FILE	"FMMS_Personal_File_S.gif"

QString appPath;

class QFileBrowser : public QListBox
{
	Q_OBJECT
	public:
		QFileBrowser(const QString &filter, QWidget *parent = 0, const char *name = 0);
		void setDir(const QString &path);
		QString filename() const;
	private slots:
		void itemSelected(int index);
		void itemClicked(QListBoxItem *item);
	private:
		QString nameFilter;
		QString basePath;
};

typedef struct {
	int	country;
	int	fullscreen;
	int	volume;
	int	mute;
	int	enablefm;
	int	keymap;
	char	romdir[1024];
} sp_option;

class ZSMSPlusWidget : public ZMainWidget
{
	Q_OBJECT
	private:
		int	menu_id_overseas,
			menu_id_domestic,
			menu_id_fullscreen,
			menu_id_vol[11],
			menu_id_mute,
			menu_id_enablefm,
			menu_id_key[4];
		QPopupMenu *menuPopup, *menuMachine, *menuVolume, *menuKey;
		QFileBrowser *fileBrowser;
		sp_option smsOption;
		void getOption();
		void setOption();
		void loadOption();
		void saveOption();
		void killSmsProc();
	public slots:
		void launchEmulator();
		void showAbout();
		void openDoc();
		void menuSelect(int param);
	public:
		ZSMSPlusWidget(QWidget *parent);
		~ZSMSPlusWidget();
};

QFileBrowser::QFileBrowser(const QString &filter, QWidget *parent, const char *name)
	: QListBox(parent, name)
{
	nameFilter = filter;
	setDir(QDir::currentDirPath());
	connect(this, SIGNAL(selected(int)), this, SLOT(itemSelected(int)));
	connect(this, SIGNAL(clicked(QListBoxItem *)), this, SLOT(itemClicked(QListBoxItem *)));
}

void QFileBrowser::setDir(const QString &path)
{
	QDir dir(path, nameFilter, QDir::DirsFirst);
	dir.setMatchAllDirs(true);
	if (!dir.isReadable())
		return;
	clear();

	RES_ICON_Reader ir;
	QPixmap folder = ir.getIcon(EZX_RES_PIXMAP_FOLDER);
	QPixmap file = ir.getIcon(EZX_RES_PIXMAP_FILE);
	
	basePath = dir.canonicalPath();
	QStringList entries = dir.entryList();
	for (QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it) {
		if (*it != ".") {
			if (QFileInfo(basePath + "/" + *it).isDir())
				insertItem(folder, *it);
			else
				insertItem(file, *it);
		}
	}
}

void QFileBrowser::itemSelected(int index)
{
	QString path = basePath + "/" + text(index);
	if (QFileInfo(path).isDir())
		setDir(path);
}

void QFileBrowser::itemClicked(QListBoxItem *item)
{
	if (item == 0) return;
	QString path = basePath + "/" + item->text();
	if (QFileInfo(path).isDir())
		setDir(path);
}

QString QFileBrowser::filename() const
{
	QString path;
	int index = currentItem();
	if (index != -1) {
		path = basePath + "/" + text(index);
		if (QFileInfo(path).isFile())
			return path;
	}
	path = "";
	return path;
}

ZSMSPlusWidget::ZSMSPlusWidget(QWidget *parent)
	: ZMainWidget(parent, NULL, 0)
{
	UTIL_CST	*widgetCST;
	ZPushButton	*buttonCST;

	fileBrowser = new QFileBrowser(FILE_FILTER, this);
	fileBrowser->setFrameStyle(QFrame::NoFrame);
	setContentWidget(fileBrowser);
	fileBrowser->show();
	connect(fileBrowser, SIGNAL(selected(int)), SLOT(launchEmulator()));

	widgetCST = new UTIL_CST(this, tr("Load & Run"));
	setCSTWidget(widgetCST);
	widgetCST->show();

	buttonCST = widgetCST->getRightBtn();
	connect(buttonCST, SIGNAL(clicked()), qApp, SLOT(quit()));

	buttonCST = widgetCST->getMidBtn();
	connect(buttonCST, SIGNAL(clicked()), SLOT(launchEmulator()));

	menuPopup = new QPopupMenu(widgetCST, NULL);
	menuPopup->insertItem(tr("About ..."), this, SLOT(showAbout()));
	menuPopup->insertItem(tr("Documentation"), this, SLOT(openDoc()));
	menuPopup->insertSeparator();
	menuMachine = new QPopupMenu;
	menu_id_overseas = menuMachine->insertItem(tr("OVERSEAS"));
	menu_id_domestic = menuMachine->insertItem(tr("DOMESTIC"));
	menuPopup->insertItem(tr("Machine Type"), menuMachine);
	menu_id_fullscreen = menuPopup->insertItem(tr("Full Screen"));
	menuVolume = new QPopupMenu;
	for (int i = 0; i < 10; i++)
		menu_id_vol[i] = menuVolume->insertItem(QString(QChar(i + 0x30)));
	menu_id_vol[10] = menuVolume->insertItem("10");
	menuPopup->insertItem(tr("Volume"), menuVolume);
	menu_id_mute = menuPopup->insertItem(tr("Mute"));
	menu_id_enablefm = menuPopup->insertItem(tr("Enable FM"));
	menuKey = new QPopupMenu;
	for (int i = 0; i <= 3; i++)
		menu_id_key[i] = menuKey->insertItem(QString(QChar(i + 0x30)));
	menuPopup->insertItem(tr("Key Mapper"), menuKey);
	connect(menuPopup, SIGNAL(activated(int)), SLOT(menuSelect(int)));
	connect(menuMachine, SIGNAL(activated(int)), SLOT(menuSelect(int)));
	connect(menuVolume, SIGNAL(activated(int)), SLOT(menuSelect(int)));
	connect(menuKey, SIGNAL(activated(int)), SLOT(menuSelect(int)));
	buttonCST = widgetCST->getLeftBtn();
	buttonCST->setPopup(menuPopup);
	
	memset(&smsOption, 0, sizeof(sp_option));
	smsOption.volume = 4;
	loadOption();
	setOption();
	fileBrowser->setDir(QString(smsOption.romdir));
}

ZSMSPlusWidget::~ZSMSPlusWidget()
{
	killSmsProc();
	getOption();
	saveOption();
}

void ZSMSPlusWidget::showAbout()
{
	RES_ICON_Reader ir;
	ZMessageBox::information(
		this,
		ir.getIcon(RES_ICON_DLG_EXCLAMATORY_MARK),
		QString(SMSPLUS_ABOUT),
		QString(tr("OK"))
	);
}

void ZSMSPlusWidget::openDoc()
{
	char cmdline[1024];

	strcpy(cmdline, "/usr/SYSqtapp/notepad/notepad -d ");
	strcat(cmdline, "\"");
	strcat(cmdline, appPath + SMSPLUS_DOC);
	strcat(cmdline, "\" &");
	printf("%s\n", cmdline);
	system(cmdline);
}

void ZSMSPlusWidget::menuSelect(int param)
{
	if (param == menu_id_fullscreen || param == menu_id_mute || param == menu_id_enablefm)
		menuPopup->setItemChecked(param, !menuPopup->isItemChecked(param));
	else if (param == menu_id_overseas || param == menu_id_domestic) {
		menuMachine->setItemChecked(menu_id_overseas, false);
		menuMachine->setItemChecked(menu_id_domestic, false);
		menuMachine->setItemChecked(param, true);
	}
	else {
		for (int i = 0; i <= 10; i++) {
			if (param == menu_id_vol[i]) {
				for (int j = 0; j <= 10; j++)
					menuVolume->setItemChecked(menu_id_vol[j], false);
				menuVolume->setItemChecked(menu_id_vol[i], true);
				return;
			}
		}
		for (int i = 0; i <= 3; i++) {
			if (param == menu_id_key[i]) {
				for (int j = 0; j <= 3; j++)
					menuKey->setItemChecked(menu_id_key[j], false);
				menuKey->setItemChecked(menu_id_key[i], true);
				return;
			}
		}
	}
}

void ZSMSPlusWidget::getOption()
{
	smsOption.country = menuMachine->isItemChecked(menu_id_domestic);
	smsOption.fullscreen = menuPopup->isItemChecked(menu_id_fullscreen);
	smsOption.mute = menuPopup->isItemChecked(menu_id_mute);
	smsOption.enablefm = menuPopup->isItemChecked(menu_id_enablefm);
	smsOption.volume = 4;
	for (int i = 0; i <= 10; i++) {
		if (menuVolume->isItemChecked(menu_id_vol[i])) {
			smsOption.volume = i;
			break;
		}
	}
	smsOption.keymap = 0;
	for (int i = 0; i <= 3; i++) {
		if (menuKey->isItemChecked(menu_id_key[i])) {
			smsOption.keymap = i;
			break;
		}
	}
}

void ZSMSPlusWidget::setOption()
{
	menuMachine->setItemChecked(menu_id_overseas, !smsOption.country);
	menuMachine->setItemChecked(menu_id_domestic, smsOption.country);
	menuPopup->setItemChecked(menu_id_fullscreen, smsOption.fullscreen);
	menuPopup->setItemChecked(menu_id_mute, smsOption.mute);
	menuPopup->setItemChecked(menu_id_enablefm, smsOption.enablefm);
	for (int i = 0; i <= 10; i++)
		menuVolume->setItemChecked(menu_id_vol[i], false);
	menuVolume->setItemChecked(menu_id_vol[smsOption.volume], true);
	for (int i = 0; i <= 3; i++)
		menuKey->setItemChecked(menu_id_key[i], false);
	menuKey->setItemChecked(menu_id_key[smsOption.keymap], true);
}

void ZSMSPlusWidget::loadOption()
{
	QFile file (appPath + CONFIG_FILE);
	if (file.open(IO_ReadOnly)) {
		QTextStream stream (&file);
		QString line, key, value;
		int p;
		while ( !stream.atEnd() ) {
			line = stream.readLine();
			p = line.find('=');
			if (p == -1)
				continue;
			key = line.left(p).stripWhiteSpace();
			value = line.right(line.length() - p - 1).stripWhiteSpace();
			if (!strcmp(key, "country"))
				smsOption.country = value.toInt();
			else if (!strcmp(key, "fullscreen"))
				smsOption.fullscreen = value.toInt();
			else if (!strcmp(key, "volume"))
				smsOption.volume = value.toInt();
			else if (!strcmp(key, "mute"))
				smsOption.mute = value.toInt();
			else if (!strcmp(key, "enablefm"))
				smsOption.enablefm = value.toInt();
			else if (!strcmp(key, "keymap"))
				smsOption.keymap = value.toInt();
			else if (!strcmp(key, "romdir"))
				strcpy(smsOption.romdir, value);
		}
		file.close();
	}

	if ((!strcmp(smsOption.romdir, "")) || (smsOption.romdir[0] != '/'))
		strcpy(smsOption.romdir, DEF_ROM_DIR);
	if ((smsOption.volume < 0) || (smsOption.volume > 10))
		smsOption.volume = 4;
	if ((smsOption.keymap < 0) || (smsOption.keymap > 3))
		smsOption.keymap = 0;
}

void ZSMSPlusWidget::saveOption()
{
	QFile file (appPath + CONFIG_FILE);
	if (file.open(IO_WriteOnly)) {
		QTextStream stream (&file);
		QString line;
		line.sprintf("%s\n\n", CONFIG_HEAD);
		stream << line;
		line.sprintf("country=%d\n", smsOption.country);
		stream << line;
		line.sprintf("fullscreen=%d\n", smsOption.fullscreen);
		stream << line;
		line.sprintf("volume=%d\n", smsOption.volume);
		stream << line;
		line.sprintf("mute=%d\n", smsOption.mute);
		stream << line;
		line.sprintf("enablefm=%d\n", smsOption.enablefm);
		stream << line;
		line.sprintf("keymap=%d\n", smsOption.keymap);
		stream << line;
		line.sprintf("romdir=%s\n", smsOption.romdir);
		stream << line;
		file.close();
	}
}

void ZSMSPlusWidget::killSmsProc()
{
	char cmdline[1024];

	strcpy(cmdline, "kill -9 `pidof ");
	strcat(cmdline, SMSPLUS_BIN);
	strcat(cmdline, "` 2>/dev/null");
	system(cmdline);
}

void ZSMSPlusWidget::launchEmulator()
{
	char cmdline[1024], filename[1024], key[32], volume[32];

	strcpy(filename, (const char *)fileBrowser->filename().utf8());
	if (!strcmp(filename, "")) return;
	
	killSmsProc();
	
	getOption();

	strcpy(cmdline, appPath + SMSPLUS_BIN);
	strcat(cmdline, " \"");
	strcat(cmdline, filename);
	strcat(cmdline, "\"");
	if (smsOption.country)
		strcat(cmdline, " --japan");
	if (smsOption.fullscreen)
		strcat(cmdline, " --fullscreen");
	if (smsOption.mute)
		strcat(cmdline, " --mute");
	if (smsOption.enablefm)
		strcat(cmdline, " --enablefm");
	sprintf(key, " --keymap %d", smsOption.keymap);
	strcat(cmdline, key);
	sprintf(volume, " --volume %d", smsOption.volume);
	strcat(cmdline, volume);
	printf("%s\n", cmdline);
	system(cmdline);
}

int main(int argc, char** argv)
{
	ZApplication app (argc, argv);

	appPath = argv[0];
	appPath.truncate(appPath.findRev('/') + 1);

        QTranslator translator(0);
        translator.load(appPath + "spfe.qm");
        app.installTranslator(&translator);

	ZSMSPlusWidget mainWidget (NULL);
	app.setMainWidget(&mainWidget);
	mainWidget.show();
	
	return app.exec();
}

#include "spfe.moc"
