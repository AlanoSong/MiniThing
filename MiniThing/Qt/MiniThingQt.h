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

#include "ui_MiniThing.h"
#include "../Core/MiniThingCore.h"
#include "../Utility/Utility.h"
#include "MiniThingBackgroud.h"

class MiniThingQt : public QMainWindow
{
    Q_OBJECT

public:
    MiniThingQt(QWidget *parent = nullptr);
    ~MiniThingQt();

    void UpdateTableView(bool isInitUpdate = false);

private:
    Ui::MiniThingClass ui;
    QStandardItemModel m_model;
    QAction* m_actionSearch;

    MiniThingCore *m_pMiniThingCore;
    MiniThingQtWorkThread* m_pMiniThingQtWorkThread;

private slots:
    void ButtonSearchClicked();
    void RefreshFunc();
};
