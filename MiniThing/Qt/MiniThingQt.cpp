#include "MiniThingQt.h"

MiniThingQt::MiniThingQt(QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);

    m_pMiniThingCore = new MiniThingCore(".\\MiniThing.db");
    m_pMiniThingQtWorkThread = new MiniThingQtWorkThread(m_pMiniThingCore);
    m_pMiniThingQtWorkThread->start();

    // ui.tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // ui.tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    UpdateTableView(true);

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

void MiniThingQt::UpdateTableView(bool isInitUpdate)
{
    QString searchStr = ui.lineEdit->text();
    std::wstring queryStr = StringToWstring(searchStr.toStdString());

    QueryInfo queryInfo;
    std::vector<UsnInfo> retVec;

    if (m_pMiniThingQtWorkThread->isMiniThingCoreReady())
    {
        queryInfo.type = QUERY_BY_NAME;
        queryInfo.info.fileNameWstr = queryStr;
        m_pMiniThingCore->SQLiteQueryV2(&queryInfo, retVec);
    }
    else
    {
        if (!isInitUpdate)
        {
            QMessageBox::warning(nullptr, "Warnning", "Scanning all volumes ...", QMessageBox::Close);
        }
    }

    m_model.clear();
    QStringList lHeader = { "Name", "Path", "Size", "Last Modified"};
    m_model.setHorizontalHeaderLabels(lHeader);

    QList<QStandardItem *> lRow;

    for (auto it = retVec.begin(); it != retVec.end(); it++)
    {
        QString filePath = QString::fromStdWString(it->filePathWstr);

        QStandardItem* pItemName = new QStandardItem(QString::fromStdWString(it->fileNameWstr));
        QStandardItem* pItemPath = new QStandardItem(filePath);
        QStandardItem* pItemSize = nullptr;
        QStandardItem* pItemLastMod = nullptr;


        QFileInfo fileInfo(filePath);
        if (fileInfo.exists())
        {
            pItemSize = new QStandardItem(QString::number(fileInfo.size() / 1024) + " KB");
            pItemLastMod = new QStandardItem(QString(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
        }

        lRow.clear();
        lRow << pItemName << pItemPath << pItemSize << pItemLastMod;


        m_model.appendRow(lRow);
    }

    ui.tableView->setModel(&m_model);
}

void MiniThingQt::ButtonSearchClicked()
{
    UpdateTableView();
}

void MiniThingQt::RefreshFunc(void)
{
    UpdateTableView();
}

