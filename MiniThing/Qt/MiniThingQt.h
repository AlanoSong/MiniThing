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

private:
    Ui::MiniThingClass m_ui;
    QStandardItemModel m_model;

    MiniThingCore* m_pMiniThingCore;
    MiniThingQtWorkThread* m_pMiniThingQtWorkThread;

    QString m_searchBefore;

    // Usn info to be show
    std::vector<UsnInfo> m_usnSet;

    // Short keys
    QAction* m_shortKeySearch;
    QAction* m_shortKeyOpen;
    QAction* m_shortKeyOpenPath;
    QAction* m_shortKeyFocusOnInput;

    // Right keys
    QMenu* m_rightKeyMenu;
    QAction* m_rightKeyActionOpen;
    QAction* m_rightKeyActionOpenPath;

    // Status bar
    QLabel* m_statusBar;

private slots:
    // Button funcs
    void ButtonSearchClicked();

    // Right key funcs
    void RightKeyMenu(QPoint pos);

    // Short key funcs
    void ShortKeySearch();
    void ShortKeyOpen();
    void ShortKeyOpenPath();

    // Status bar funcs
    void SetStatusBar(QString str);

    // Enter press funcs for line text
    void EnterPressDown(void);
};
