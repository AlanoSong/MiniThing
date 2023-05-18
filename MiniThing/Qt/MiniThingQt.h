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

    void UpdateTableView(bool isInitUpdate = false);

private:
    Ui::MiniThingClass m_ui;
    QStandardItemModel m_model;
    QAction* m_actionSearch;

    MiniThingCore *m_pMiniThingCore;
    MiniThingQtWorkThread* m_pMiniThingQtWorkThread;

    QMenu* m_rightKeyMenu;
    QAction* m_rightKeyActionOpen;
    QAction* m_rightKeyActionLocation;

private slots:
    void ButtonSearchClicked();
    void RefreshFunc();

    void RightKeyMenu(QPoint pos);
};
