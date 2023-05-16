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

class MiniThingQt : public QMainWindow
{
    Q_OBJECT

public:
    MiniThingQt(QWidget *parent = nullptr);
    ~MiniThingQt();

    void UpdateFiles(void);

private:
    Ui::MiniThingClass ui;
    MiniThingCore *m_pMiniThingCore;
    QStandardItemModel m_model;
    QAction* m_actionSearch;

private slots:
    void ButtonSearchClicked();
    void RefreshFunc();
};
