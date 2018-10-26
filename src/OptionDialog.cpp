/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "precompiled.h"

#include "OptionDialog.h"
#include <QMessageBox>
#include "AppBase.h"
#include "ui_OptionDialog.h"
#include "LaunchyWidget.h"
#include "globals.h"
#include "plugin_handler.h"
#include "FileBrowserDelegate.h"
#include "SettingsManager.h"
#include "QLogger.h"
#include "catalog_builder.h"

QByteArray OptionDialog::s_windowGeometry;
int OptionDialog::s_currentTab;
int OptionDialog::s_currentPlugin;

OptionDialog::OptionDialog(QWidget * parent)
    : QDialog(parent),
      m_pUi(new Ui::OptionDialog),
      directoryItemDelegate(this, FileBrowser::Directory) {

    m_pUi->setupUi(this);

    Qt::WindowFlags windowsFlags = windowFlags();
    windowsFlags = windowsFlags & (~Qt::WindowContextHelpButtonHint);
    windowsFlags = windowsFlags | Qt::MSWindowsFixedSizeDialogHint;
    setWindowFlags(windowsFlags);

    curPlugin = -1;

    restoreGeometry(s_windowGeometry);
    m_pUi->tabWidget->setCurrentIndex(s_currentTab);

    // Load General Options		
    m_pUi->genAlwaysShow->setChecked(g_settings->value("GenOps/alwaysshow", false).toBool());
    m_pUi->genAlwaysTop->setChecked(g_settings->value("GenOps/alwaystop", false).toBool());
    m_pUi->genPortable->setChecked(SettingsManager::instance().isPortable());
    m_pUi->genHideFocus->setChecked(g_settings->value("GenOps/hideiflostfocus", false).toBool());
    m_pUi->genDecorateText->setChecked(g_settings->value("GenOps/decoratetext", false).toBool());

    int center = g_settings->value("GenOps/alwayscenter", 3).toInt();
    m_pUi->genHCenter->setChecked((center & 1) != 0);
    m_pUi->genVCenter->setChecked((center & 2) != 0);

    m_pUi->genShiftDrag->setChecked(g_settings->value("GenOps/dragmode", 0) == 1);
//    m_pUi->genUpdateCheck->setChecked(g_settings->value("GenOps/updatecheck", true).toBool());
    m_pUi->genLog->setCurrentIndex(g_settings->value("GenOps/logLevel", 2).toInt());
    connect(m_pUi->genLog, SIGNAL(currentIndexChanged(int)), this, SLOT(logLevelChanged(int)));

    m_pUi->genShowHidden->setChecked(g_settings->value("GenOps/showHiddenFiles", false).toBool());
    m_pUi->genShowNetwork->setChecked(g_settings->value("GenOps/showNetwork", true).toBool());
    m_pUi->genCondensed->setCurrentIndex(g_settings->value("GenOps/condensedView", 2).toInt());
    m_pUi->genAutoSuggestDelay->setValue(g_settings->value("GenOps/autoSuggestDelay", 1000).toInt());

    int updateInterval = g_settings->value("GenOps/updatetimer", 10).toInt();
    connect(m_pUi->genUpdateCatalog, SIGNAL(stateChanged(int)), this, SLOT(autoUpdateCheckChanged(int)));
    m_pUi->genUpdateMinutes->setValue(updateInterval);
    m_pUi->genUpdateCatalog->setChecked(updateInterval > 0);
    m_pUi->genMaxViewable->setValue(g_settings->value("GenOps/numviewable", 4).toInt());
    m_pUi->genNumResults->setValue(g_settings->value("GenOps/numresults", 10).toInt());
    m_pUi->genNumHistory->setValue(g_settings->value("GenOps/maxitemsinhistory", 20).toInt());
    m_pUi->genOpaqueness->setValue(g_settings->value("GenOps/opaqueness", 100).toInt());
    m_pUi->genFadeIn->setValue(g_settings->value("GenOps/fadein", 0).toInt());
    m_pUi->genFadeOut->setValue(g_settings->value("GenOps/fadeout", 20).toInt());
    connect(m_pUi->genOpaqueness, SIGNAL(sliderMoved(int)), g_mainWidget.data(), SLOT(setOpaqueness(int)));

#ifdef Q_OS_MAC
    metaKeys << QString("") << QString("Alt") << QString("Command") << QString("Shift") << QString("Control")
        << QString("Command+Alt") << QString("Command+Shift") << QString("Command+Control");
#else
    metaKeys << QString("") << QString("Alt") << QString("Control") << QString("Shift") << QString("Win")
        << QString("Ctrl+Alt") << QString("Ctrl+Shift") << QString("Ctrl+Win");
#endif
    iMetaKeys << Qt::NoModifier << Qt::AltModifier << Qt::ControlModifier << Qt::ShiftModifier << Qt::MetaModifier
        << (Qt::ControlModifier | Qt::AltModifier) << (Qt::ControlModifier | Qt::ShiftModifier)
        << (Qt::ControlModifier | Qt::MetaModifier);

    actionKeys << QString("Space") << QString("Tab") << QString("Caps Lock") << QString("Backspace")
        << QString("Enter") << QString("Esc") << QString("Insert") << QString("Delete") << QString("Home")
        << QString("End") << QString("Page Up") << QString("Page Down") << QString("Print") << QString("Scroll Lock")
        << QString("Pause") << QString("Num Lock")
        << tr("Up") << tr("Down") << tr("Left") << tr("Right")
        << QString("F1") << QString("F2") << QString("F3") << QString("F4") << QString("F5")
        << QString("F6") << QString("F7") << QString("F8") << QString("F9") << QString("F10")
        << QString("F11") << QString("F12") << QString("F13") << QString("F14") << QString("F15");

    iActionKeys << Qt::Key_Space << Qt::Key_Tab << Qt::Key_CapsLock << Qt::Key_Backspace << Qt::Key_Enter << Qt::Key_Escape <<
        Qt::Key_Insert << Qt::Key_Delete << Qt::Key_Home << Qt::Key_End << Qt::Key_PageUp << Qt::Key_PageDown <<
        Qt::Key_Print << Qt::Key_ScrollLock << Qt::Key_Pause << Qt::Key_NumLock <<
        Qt::Key_Up << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right <<
        Qt::Key_F1 << Qt::Key_F2 << Qt::Key_F3 << Qt::Key_F4 << Qt::Key_F5 << Qt::Key_F6 << Qt::Key_F7 << Qt::Key_F8 <<
        Qt::Key_F9 << Qt::Key_F10 << Qt::Key_F11 << Qt::Key_F12 << Qt::Key_F13 << Qt::Key_F14 << Qt::Key_F15;

    for (int i = '0'; i <= '9'; ++i) {
        actionKeys << QString(QChar(i));
        iActionKeys << i;
    }

    for (int i = 'A'; i <= 'Z'; ++i) {
        actionKeys << QString(QChar(i));
        iActionKeys << i;
    }

    actionKeys << "`" << "-" << "=" << "[" << "]" <<
        ";" << "'" << "#" << "\\" << "," << "." << "/";

    iActionKeys << '`' << '-' << '=' << '[' << ']' <<
        ';' << '\'' << '#' << '\\' << ',' << '.' << '/';

    // Find the current hotkey
    int hotkey = g_mainWidget->getHotkey();
    int meta = hotkey & (Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);
    hotkey &= ~(Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);

    for (int i = 0; i < metaKeys.count(); ++i) {
        m_pUi->genModifierBox->addItem(metaKeys[i]);
        if (iMetaKeys[i] == meta)
            m_pUi->genModifierBox->setCurrentIndex(i);
    }

    for (int i = 0; i < actionKeys.count(); ++i) {
        m_pUi->genKeyBox->addItem(actionKeys[i]);
        if (iActionKeys[i] == hotkey)
            m_pUi->genKeyBox->setCurrentIndex(i);
    }

    // Load up web proxy settings
    m_pUi->genProxyHostname->setText(g_settings->value("WebProxy/hostAddress").toString());
    m_pUi->genProxyPort->setText(g_settings->value("WebProxy/port").toString());

    // Load up the skins list
    QString skinName = g_settings->value("GenOps/skin", "Default").toString();

    int skinRow = 0;
    QHash<QString, bool> knownSkins;
    foreach(QString szDir, SettingsManager::instance().directory("skins")) {
        QDir dir(szDir);
        QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        foreach(QString d, dirs) {
            if (knownSkins.contains(d))
                continue;
            knownSkins[d] = true;

            QFile f(dir.absolutePath() + "/" + d + "/style.qss");
            // Only look for 2.0+ skins
            if (!f.exists())
                continue;

            QListWidgetItem* item = new QListWidgetItem(d);
            m_pUi->skinList->addItem(item);

            if (skinName.compare(d, Qt::CaseInsensitive) == 0)
                skinRow = m_pUi->skinList->count() - 1;
        }
    }
    m_pUi->skinList->setCurrentRow(skinRow);

    connect(m_pUi->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    // Load the directories and types
    m_pUi->catDirectories->setItemDelegate(&directoryItemDelegate);

    connect(m_pUi->catDirectories, SIGNAL(currentRowChanged(int)), this, SLOT(dirRowChanged(int)));
    connect(m_pUi->catDirectories, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(catDirDragEnter(QDragEnterEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(drop(QDropEvent*)), this, SLOT(catDirDrop(QDropEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catDirItemChanged(QListWidgetItem*)));
    connect(m_pUi->catDirPlus, SIGNAL(clicked(bool)), this, SLOT(catDirPlusClicked(bool)));
    connect(m_pUi->catDirMinus, SIGNAL(clicked(bool)), this, SLOT(catDirMinusClicked(bool)));
    connect(m_pUi->catTypes, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catTypesItemChanged(QListWidgetItem*)));
    connect(m_pUi->catTypesPlus, SIGNAL(clicked(bool)), this, SLOT(catTypesPlusClicked(bool)));
    connect(m_pUi->catTypesMinus, SIGNAL(clicked(bool)), this, SLOT(catTypesMinusClicked(bool)));
    connect(m_pUi->catCheckDirs, SIGNAL(stateChanged(int)), this, SLOT(catTypesDirChanged(int)));
    connect(m_pUi->catCheckBinaries, SIGNAL(stateChanged(int)), this, SLOT(catTypesExeChanged(int)));
    connect(m_pUi->catDepth, SIGNAL(valueChanged(int)), this, SLOT(catDepthChanged(int)));
    connect(m_pUi->catRescan, SIGNAL(clicked(bool)), this, SLOT(catRescanClicked(bool)));
    m_pUi->catProgress->setVisible(false);

    memDirs = SettingsManager::instance().readCatalogDirectories();
    for (int i = 0; i < memDirs.count(); ++i) {
        m_pUi->catDirectories->addItem(memDirs[i].name);
        QListWidgetItem* it = m_pUi->catDirectories->item(i);
        it->setFlags(it->flags() | Qt::ItemIsEditable);
    }

    if (m_pUi->catDirectories->count() > 0)
        m_pUi->catDirectories->setCurrentRow(0);

    m_pUi->genOpaqueness->setRange(15, 100);

    if (g_builder->getCatalog() != NULL) {
        m_pUi->catSize->setText(tr("Index has %n item(s)", "N/A", g_builder->getCatalog()->count()));
    }

    connect(g_builder.data(), SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
    connect(g_builder.data(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    if (g_builder->isRunning()) {
        catalogProgressUpdated(g_builder->getProgress());
    }

    // Load up the plugins		
    connect(m_pUi->plugList, SIGNAL(currentRowChanged(int)), this, SLOT(pluginChanged(int)));
    connect(m_pUi->plugList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(pluginItemChanged(QListWidgetItem*)));
    g_mainWidget->plugins.loadPlugins();
    foreach(PluginInfo info, g_mainWidget->plugins.getPlugins()) {
        m_pUi->plugList->addItem(info.name);
        QListWidgetItem* item = m_pUi->plugList->item(m_pUi->plugList->count()-1);
        item->setData(Qt::UserRole, info.id);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (info.loaded)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }
    m_pUi->plugList->sortItems();
    if (m_pUi->plugList->count() > 0) {
        m_pUi->plugList->setCurrentRow(s_currentPlugin);
    }
    m_pUi->aboutVer->setText(tr("This is Launchy %1").arg(LAUNCHY_VERSION_STRING));
    m_pUi->catDirectories->installEventFilter(this);

    needRescan = false;
}


OptionDialog::~OptionDialog() {
    if (g_builder != NULL) {
        disconnect(g_builder.data(), SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
        disconnect(g_builder.data(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    }

    s_currentTab = m_pUi->tabWidget->currentIndex();
    s_windowGeometry = saveGeometry();

    delete m_pUi;
    m_pUi = nullptr;
}


void OptionDialog::setVisible(bool visible) {
    QDialog::setVisible(visible);

    if (visible) {
        connect(m_pUi->skinList, SIGNAL(currentTextChanged(const QString)),
                this, SLOT(skinChanged(const QString)));
        skinChanged(m_pUi->skinList->currentItem()->text());
    }
}


void OptionDialog::accept() {
    if (g_settings == NULL)
        return;

    // See if the new hotkey works, if not we're not leaving the dialog.
    QKeySequence hotkey(iMetaKeys[m_pUi->genModifierBox->currentIndex()] + iActionKeys[m_pUi->genKeyBox->currentIndex()]);
    if (!g_mainWidget->setHotkey(hotkey)) {
        QMessageBox::warning(this, tr("Launchy"), tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
        return;
    }

    g_settings->setValue("GenOps/hotkey", hotkey.count() > 0 ? hotkey[0] : 0);

    // Save General Options
//	g_settings->setValue("GenOps/showtrayicon", genShowTrayIcon->isChecked());
    g_settings->setValue("GenOps/alwaysshow", m_pUi->genAlwaysShow->isChecked());
    g_settings->setValue("GenOps/alwaystop", m_pUi->genAlwaysTop->isChecked());
//    g_settings->setValue("GenOps/updatecheck", m_pUi->genUpdateCheck->isChecked());
    g_settings->setValue("GenOps/logLevel", m_pUi->genLog->currentIndex());
    g_settings->setValue("GenOps/decoratetext", m_pUi->genDecorateText->isChecked());
    g_settings->setValue("GenOps/hideiflostfocus", m_pUi->genHideFocus->isChecked());
    g_settings->setValue("GenOps/alwayscenter", (m_pUi->genHCenter->isChecked() ? 1 : 0) | (m_pUi->genVCenter->isChecked() ? 2 : 0));
    g_settings->setValue("GenOps/dragmode", m_pUi->genShiftDrag->isChecked() ? 1 : 0);
    g_settings->setValue("GenOps/showHiddenFiles", m_pUi->genShowHidden->isChecked());
    g_settings->setValue("GenOps/showNetwork", m_pUi->genShowNetwork->isChecked());
    g_settings->setValue("GenOps/condensedView", m_pUi->genCondensed->currentIndex());
    g_settings->setValue("GenOps/autoSuggestDelay", m_pUi->genAutoSuggestDelay->value());
    g_settings->setValue("GenOps/updatetimer", m_pUi->genUpdateCatalog->isChecked() ? m_pUi->genUpdateMinutes->value() : 0);
    g_settings->setValue("GenOps/numviewable", m_pUi->genMaxViewable->value());
    g_settings->setValue("GenOps/numresults", m_pUi->genNumResults->value());
    g_settings->setValue("GenOps/maxitemsinhistory", m_pUi->genNumHistory->value());
    g_settings->setValue("GenOps/opaqueness", m_pUi->genOpaqueness->value());
    g_settings->setValue("GenOps/fadein", m_pUi->genFadeIn->value());
    g_settings->setValue("GenOps/fadeout", m_pUi->genFadeOut->value());

    g_settings->setValue("WebProxy/hostAddress", m_pUi->genProxyHostname->text());
    g_settings->setValue("WebProxy/port", m_pUi->genProxyPort->text());

    // Apply General Options
    SettingsManager::instance().setPortable(m_pUi->genPortable->isChecked());
    g_mainWidget->startUpdateTimer();
    g_mainWidget->setSuggestionListMode(m_pUi->genCondensed->currentIndex());
    g_mainWidget->loadOptions();

    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(memDirs);

    if (curPlugin >= 0)
    {
        QListWidgetItem* item = m_pUi->plugList->item(curPlugin);
        g_mainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
    }

    g_settings->sync();

    QDialog::accept();

    // Now save the options that require launchy to be shown or redrawed
    bool show = g_mainWidget->setAlwaysShow(m_pUi->genAlwaysShow->isChecked());
    show |= g_mainWidget->setAlwaysTop(m_pUi->genAlwaysTop->isChecked());

    g_mainWidget->setOpaqueness(m_pUi->genOpaqueness->value());

    // Apply Skin Options
    QString prevSkinName = g_settings->value("GenOps/skin", "Default").toString();
    QString skinName = m_pUi->skinList->currentItem()->text();
    if (m_pUi->skinList->currentRow() >= 0 && skinName != prevSkinName)
    {
        g_settings->setValue("GenOps/skin", skinName);
        g_mainWidget->setSkin(skinName);
        show = false;
    }

    if (needRescan)
        g_mainWidget->buildCatalog();

    if (show)
        g_mainWidget->showLaunchy();
}


void OptionDialog::reject()
{
    if (curPlugin >= 0)
    {
        QListWidgetItem* item = m_pUi->plugList->item(curPlugin);
        g_mainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), false);
    }

    QDialog::reject();
}


void OptionDialog::tabChanged(int tab) {
    Q_UNUSED(tab)
        // Redraw the current skin (necessary because of dialog resizing issues)
        if (m_pUi->tabWidget->currentWidget()->objectName() == "Skins") {
            skinChanged(m_pUi->skinList->currentItem()->text());
        }
        else if (m_pUi->tabWidget->currentWidget()->objectName() == "Plugins") {
            // We've currently no way of checking if a plugin requires a catalog rescan
            // so assume that we need one if the user has viewed the plugins tab
            needRescan = true;
        }
}


void OptionDialog::autoUpdateCheckChanged(int state) {
    m_pUi->genUpdateMinutes->setEnabled(state > 0);
    if (m_pUi->genUpdateMinutes->value() <= 0)
        m_pUi->genUpdateMinutes->setValue(10);
}


void OptionDialog::skinChanged(const QString& newSkin)
{
    if (newSkin.count() == 0)
        return;

    // Find the skin with this name
    QString directory = SettingsManager::instance().skinPath(newSkin);

    // Load up the author file
    if (directory.length() == 0) {
        m_pUi->authorInfo->setText("");
        return;
    }
    QFile fileAuthor(directory + "author.txt");
    if (!fileAuthor.open(QIODevice::ReadOnly)) {
        m_pUi->authorInfo->setText("");
    }

    QString line, total;
    QTextStream in(&fileAuthor);
    line = in.readLine();
    while (!line.isNull()) {
        total += line + "\n";
        line = in.readLine();
    }
    m_pUi->authorInfo->setText(total);
    fileAuthor.close();

    // Set the image preview
    QPixmap pix;
    if (pix.load(directory + "background.png")) {
        if (!g_app->supportsAlphaBorder() && QFile::exists(directory + "background_nc.png"))
            pix.load(directory + "background_nc.png");
        if (pix.hasAlpha())
            pix.setMask(pix.mask());
        if (!g_app->supportsAlphaBorder() && QFile::exists(directory + "mask_nc.png"))
            pix.setMask(QPixmap(directory + "mask_nc.png"));
        else if (QFile::exists(directory + "mask.png"))
            pix.setMask(QPixmap(directory + "mask.png"));

        if (g_app->supportsAlphaBorder()) {
            // Compose the alpha image with the background
            QImage sourceImage(pix.toImage());
            QImage destinationImage(directory + "alpha.png");
            QImage resultImage(destinationImage.size(), QImage::Format_ARGB32_Premultiplied);

            QPainter painter(&resultImage);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(resultImage.rect(), Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawImage(0, 0, sourceImage);
            painter.drawImage(0, 0, destinationImage);
            painter.end();

            pix = QPixmap::fromImage(resultImage);
            QPixmap scaled = pix.scaled(m_pUi->skinPreview->size(),
                                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_pUi->skinPreview->setPixmap(scaled);
        }
    }
    else if (pix.load(directory + "frame.png")) {
        QPixmap scaled = pix.scaled(m_pUi->skinPreview->size(),
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_pUi->skinPreview->setPixmap(scaled);
    }
    else {
        m_pUi->skinPreview->clear();
    }
}


void OptionDialog::pluginChanged(int row)
{
    m_pUi->plugBox->setTitle(tr("Plugin options"));

    if (m_pUi->plugBox->layout() != NULL)
        for (int i = 1; i < m_pUi->plugBox->layout()->count(); i++)
            m_pUi->plugBox->layout()->removeItem(m_pUi->plugBox->layout()->itemAt(i));

    // Close any current plugin dialogs
    if (curPlugin >= 0)
    {
        QListWidgetItem* item = m_pUi->plugList->item(curPlugin);
        g_mainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
    }

    // Open the new plugin dialog
    curPlugin = row;
    s_currentPlugin = row;
    if (row >= 0)
    {
        loadPluginDialog(m_pUi->plugList->item(row));
    }
}


void OptionDialog::loadPluginDialog(QListWidgetItem* item)
{
    QWidget* win = g_mainWidget->plugins.doDialog(m_pUi->plugBox, item->data(Qt::UserRole).toUInt());
    if (win != NULL) {
        if (m_pUi->plugBox->layout() != NULL)
            m_pUi->plugBox->layout()->addWidget(win);
        win->show();
        if (win->windowTitle() != "Form")
            m_pUi->plugBox->setTitle(win->windowTitle());
    }
}


void OptionDialog::pluginItemChanged(QListWidgetItem* iz)
{
    int row = m_pUi->plugList->currentRow();
    if (row == -1)
        return;

    // Close any current plugin dialogs
    if (curPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(curPlugin);
        g_mainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
    }

    // Write out the new config
    g_settings->beginWriteArray("plugins");
    for (int i = 0; i < m_pUi->plugList->count(); i++) {
        QListWidgetItem* item = m_pUi->plugList->item(i);
        g_settings->setArrayIndex(i);
        g_settings->setValue("id", item->data(Qt::UserRole).toUInt());
        if (item->checkState() == Qt::Checked) {
            g_settings->setValue("load", true);
        }
        else {
            g_settings->setValue("load", false);
        }
    }
    g_settings->endArray();

    // Reload the plugins
    g_mainWidget->plugins.loadPlugins();

    // If enabled, reload the dialog
    if (iz->checkState() == Qt::Checked) {
        loadPluginDialog(iz);
    }
}


void OptionDialog::logLevelChanged(int index) {
    QLogger::setLogLevel(index);
}

void OptionDialog::catalogProgressUpdated(int value) {
    m_pUi->catSize->setVisible(false);
    m_pUi->catProgress->setValue(value);
    m_pUi->catProgress->setVisible(true);
    m_pUi->catRescan->setEnabled(false);
}


void OptionDialog::catalogBuilt()
{
    m_pUi->catProgress->setVisible(false);
    m_pUi->catRescan->setEnabled(true);
    if (g_builder->getCatalog() != NULL) {
        m_pUi->catSize->setText(tr("Index has %n items", "", g_builder->getCatalog()->count()));
        m_pUi->catSize->setVisible(true);
    }
}


void OptionDialog::catRescanClicked(bool val)
{
    val = val; // Compiler warning

    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(memDirs);

    needRescan = false;
    m_pUi->catRescan->setEnabled(false);
    g_mainWidget->buildCatalog();
}


void OptionDialog::catTypesDirChanged(int state)
{
    state = state; // Compiler warning
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    memDirs[row].indexDirs = m_pUi->catCheckDirs->isChecked();

    needRescan = true;
}


void OptionDialog::catTypesExeChanged(int state)
{
    state = state; // Compiler warning
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    memDirs[row].indexExe = m_pUi->catCheckBinaries->isChecked();

    needRescan = true;
}


void OptionDialog::catDirItemChanged(QListWidgetItem* item)
{
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    if (item != m_pUi->catDirectories->item(row))
        return;

    memDirs[row].name = item->text();

    needRescan = true;
}


void OptionDialog::catDirDragEnter(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls())
        event->acceptProposedAction();
}


void OptionDialog::catDirDrop(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls())
    {
        foreach(QUrl url, mimeData->urls())
        {
            QFileInfo info(url.toLocalFile());
            if (info.exists() && info.isDir())
            {
                addDirectory(info.filePath());
            }
        }
    }
}


void OptionDialog::dirRowChanged(int row)
{
    if (row == -1)
        return;

    m_pUi->catTypes->clear();
    foreach(QString str, memDirs[row].types)
    {
        QListWidgetItem* item = new QListWidgetItem(str, m_pUi->catTypes);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_pUi->catCheckDirs->setChecked(memDirs[row].indexDirs);
    m_pUi->catCheckBinaries->setChecked(memDirs[row].indexExe);
    m_pUi->catDepth->setValue(memDirs[row].depth);

    needRescan = true;
}


void OptionDialog::catDirMinusClicked(bool c) {
    Q_UNUSED(c)
    int dirRow = m_pUi->catDirectories->currentRow();

    delete m_pUi->catDirectories->takeItem(dirRow);
    m_pUi->catTypes->clear();

    memDirs.removeAt(dirRow);

    if (dirRow >= m_pUi->catDirectories->count()
        && m_pUi->catDirectories->count() > 0)
    {
        m_pUi->catDirectories->setCurrentRow(m_pUi->catDirectories->count() - 1);
        dirRowChanged(m_pUi->catDirectories->count() - 1);
    }
}

void OptionDialog::catDirPlusClicked(bool c) {
    Q_UNUSED(c)
    addDirectory("", true);
}

void OptionDialog::addDirectory(const QString& directory, bool edit) {
    QString nativeDir = QDir::toNativeSeparators(directory);
    Directory dir(nativeDir);
    memDirs.append(dir);

    m_pUi->catTypes->clear();
    QListWidgetItem* item = new QListWidgetItem(nativeDir, m_pUi->catDirectories);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_pUi->catDirectories->setCurrentItem(item);
    if (edit)
    {
        m_pUi->catDirectories->editItem(item);
    }

    needRescan = true;
}


void OptionDialog::catTypesItemChanged(QListWidgetItem* item) {
    Q_UNUSED(item);

    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    int typesRow = m_pUi->catTypes->currentRow();
    if (typesRow == -1)
        return;

    memDirs[row].types[typesRow] = m_pUi->catTypes->item(typesRow)->text();

    needRescan = true;
}


void OptionDialog::catTypesPlusClicked(bool c) {
    Q_UNUSED(c)
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;

    memDirs[row].types << "";
    QListWidgetItem* item = new QListWidgetItem(m_pUi->catTypes);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_pUi->catTypes->setCurrentItem(item);
    m_pUi->catTypes->editItem(item);

    needRescan = true;
}

void OptionDialog::catTypesMinusClicked(bool c) {
    Q_UNUSED(c)
    int dirRow = m_pUi->catDirectories->currentRow();
    if (dirRow == -1)
        return;

    int typesRow = m_pUi->catTypes->currentRow();
    if (typesRow == -1)
        return;

    memDirs[dirRow].types.removeAt(typesRow);
    delete m_pUi->catTypes->takeItem(typesRow);

    if (typesRow >= m_pUi->catTypes->count()
        && m_pUi->catTypes->count() > 0)
        m_pUi->catTypes->setCurrentRow(m_pUi->catTypes->count() - 1);

    needRescan = true;
}


void OptionDialog::catDepthChanged(int d) {
    int row = m_pUi->catDirectories->currentRow();
    if (row != -1)
        memDirs[row].depth = d;

    needRescan = true;
}
