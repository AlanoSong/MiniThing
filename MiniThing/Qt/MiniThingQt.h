#pragma once

#include <QtWidgets/QMainWindow>
#include <qstandarditemmodel.h>
#include <qtimer.h>
#include <QDir.h>
#include <qdatetime.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qdesktopservices.h>
#include <QAction>
#include <qmenu.h>
#include <qprocess.h>
#include <qlabel.h>
#include <qclipboard.h>
#include <qmimedata.h>
#include <QThread>
#include <qinputdialog.h>

#include "ui_MiniThing.h"
#include "../Core/MiniThingCore.h"
#include "../Utility/Utility.h"
#include "MiniThingQtBackgroud.h"

class MiniThingQt : public QMainWindow
{
    Q_OBJECT

public:
    MiniThingQt(QWidget *parent = nullptr);
    ~MiniThingQt();

    void UpdateTableView(void);

    // Status bar funcs
    void SetStatusBar(QString str);

protected:
    MiniThingQtWorkThread* m_pMiniThingQtWorkThread;

private:
    Ui::MiniThingClass m_ui;
    QStandardItemModel m_model;

    MiniThingCore* m_pMiniThingCore;

    // Usn info to be show
    std::vector<UsnInfo> m_usnSet;

    // Short keys
    QAction* m_shortKeySearch;
    QAction* m_shortKeyOpen;
    QAction* m_shortKeyOpenPath;
    QAction* m_shortKeyFocusOnInput;
    QAction* m_shortKeyDelete;
    QAction* m_shortKeyCopy;

    // Right keys
    QMenu* m_rightKeyMenu;
    QAction* m_rightKeyActionOpen;
    QAction* m_rightKeyActionOpenPath;

    // Status bar
    QLabel* m_statusBar;

    bool OpenFile(const QString& filePath);
    void OpenFilePath(const QString& filePath);
    void SetupUIComponents();
    void SetupActionsAndMenus();
    void UpdateUiFont(QFont& font);

public slots:
    void UpdateStatusBar(const std::string& msg)
    {
        const QString qStr = QString::fromStdString(msg);
        statusBar()->showMessage(qStr);
    }

private slots:
    // Button funcs
    void ButtonSearchClicked();

    void ButtonOpenClicked();

    // Right key funcs
    void RightKeyMenu(QPoint pos);

    // Short key funcs
    void ShortKeySearch();
    void ShortKeyOpen();
    void ShortKeyOpenPath();
    void ShortKeyDelete();
    void ShortKeyCopy();

    // Enter press funcs for line text
    void EnterPressDown(void);

    // Menu & Action Press Functions
    void FontActionPress();
};


