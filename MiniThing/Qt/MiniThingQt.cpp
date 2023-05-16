#include "MiniThingQt.h"

MiniThingQt::MiniThingQt(QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);

    m_pMiniThingCore = new MiniThingCore(".\\MiniThing.db");

    ui.tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // ui.tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    UpdateFiles();

    m_actionSearch = new QAction(this);
    m_actionSearch->setShortcut(tr("ctrl+f"));
    this->addAction(m_actionSearch);
    connect(m_actionSearch, SIGNAL(triggered()), this, SLOT(RefreshFunc()));
}

MiniThingQt::~MiniThingQt()
{
    delete m_actionSearch;
    delete m_pMiniThingCore;
}

void MiniThingQt::UpdateFiles(void)
{
    QString searchStr = ui.lineEdit->text();
    std::wstring queryInfo = StringToWstring(searchStr.toStdString());
    std::vector<std::wstring> searchRes;
    m_pMiniThingCore->SQLiteQuery(queryInfo, searchRes);

    m_model.clear();
    QStringList lHeader = { "Path" };
    m_model.setHorizontalHeaderLabels(lHeader);

    QList<QStandardItem *> lRow;

    for (auto it = searchRes.begin(); it != searchRes.end(); it++)
    {
        QStandardItem* pItem = new QStandardItem(QString::fromStdWString(*it));

        lRow.clear();
        lRow << pItem;

        m_model.appendRow(lRow);
    }

    ui.tableView->setModel(&m_model);
}

void MiniThingQt::ButtonSearchClicked()
{
    UpdateFiles();
}

void MiniThingQt::RefreshFunc(void)
{
    UpdateFiles();
}

