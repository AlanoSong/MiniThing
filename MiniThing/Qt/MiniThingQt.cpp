#include "MiniThingQt.h"

MiniThingQt::MiniThingQt(QWidget *parent) : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_usnSet.clear();

    m_pMiniThingCore = new MiniThingCore(".\\MiniThing.db");
    m_pMiniThingQtWorkThread = new MiniThingQtWorkThread(m_pMiniThingCore);
    m_pMiniThingQtWorkThread->start();

    m_ui.tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui.tableView->verticalHeader()->setVisible(false);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // ui.tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui.tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_rightKeyMenu = new QMenu(m_ui.tableView);
    m_rightKeyActionOpen = new QAction();
    m_rightKeyActionOpenPath = new QAction();

    m_rightKeyActionOpen->setText(QString("Open(ctrl+o)"));
    m_rightKeyActionOpenPath->setText(QString("Open path(ctrl+p)"));
    m_rightKeyMenu->addAction(m_rightKeyActionOpen);
    m_rightKeyMenu->addAction(m_rightKeyActionOpenPath);
    connect(m_ui.tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(RightKeyMenu(QPoint)));

    UpdateTableView();

    // Registe all shortcut keys
    m_shortKeySearch = new QAction(this);
    m_shortKeySearch->setShortcut(tr("ctrl+f"));
    this->addAction(m_shortKeySearch);
    connect(m_shortKeySearch, SIGNAL(triggered()), this, SLOT(ShortKeySearch()));

    m_shortKeyOpen = new QAction(this);
    m_shortKeyOpen->setShortcut(tr("ctrl+o"));
    this->addAction(m_shortKeyOpen);
    connect(m_shortKeyOpen, SIGNAL(triggered()), this, SLOT(ShortKeyOpen()));

    m_shortKeyOpenPath = new QAction(this);
    m_shortKeyOpenPath->setShortcut(tr("ctrl+p"));
    this->addAction(m_shortKeyOpenPath);
    connect(m_shortKeyOpenPath, SIGNAL(triggered()), this, SLOT(ShortKeyOpenPath()));
}

MiniThingQt::~MiniThingQt()
{
    delete m_shortKeySearch;
    delete m_pMiniThingCore;
}

void MiniThingQt::UpdateTableView(void)
{
    m_model.clear();
    QStringList lHeader = { "Name", "Path", "Size", "Last Modified"};
    m_model.setHorizontalHeaderLabels(lHeader);

    QList<QStandardItem *> lRow;

    for (auto it = m_usnSet.begin(); it != m_usnSet.end(); it++)
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
            pItemLastMod = new QStandardItem(QString(fileInfo.lastModified().toString("MM/dd/yyyy hh:mm:ss")));
        }

        lRow.clear();
        lRow << pItemName << pItemPath << pItemSize << pItemLastMod;

        m_model.appendRow(lRow);
    }

    m_ui.tableView->setModel(&m_model);
}

//==========================================================================
//                        Button Press Functions                          //
//==========================================================================
void MiniThingQt::ButtonSearchClicked()
{
    // Here reuse short key func
    ShortKeySearch();
}

//==========================================================================
//                        Short Key Functions                             //
//==========================================================================
void MiniThingQt::ShortKeySearch()
{
    // Clear m_usnSet firstly, cause SQLiteQueryV2 use push back to add file node
    m_usnSet.clear();

    if (m_pMiniThingQtWorkThread->isMiniThingCoreReady())
    {
        QString searchStr = m_ui.lineEdit->text();
        std::wstring queryStr = StringToWstring(searchStr.toStdString());

        QueryInfo queryInfo;
        queryInfo.type = QUERY_BY_NAME;
        queryInfo.info.fileNameWstr = queryStr;

        m_pMiniThingCore->SQLiteQueryV2(&queryInfo, m_usnSet);
    }
    else
    {
        QMessageBox::warning(nullptr, "Warnning", "Scanning all volumes ...", QMessageBox::Close);
        // TODO: show status msg
    }

    // Call update table view to refresh all msg show
    UpdateTableView();
}

void MiniThingQt::ShortKeyOpen()
{
    auto currentIndex = m_ui.tableView->currentIndex();
    if (currentIndex.isValid())
    {
        // Get file path info from current line
        QModelIndex index = m_model.index(currentIndex.row(), 1);
        QString filePath = m_model.data(index).toString();

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath())))
        {
            QMessageBox::information(this, tr("warning"), tr("Failed to open file."), QMessageBox::Ok);
        }
    }
}

void MiniThingQt::ShortKeyOpenPath()
{
    auto currentIndex = m_ui.tableView->currentIndex();
    if (currentIndex.isValid())
    {
        // Get file path info from current line
        QModelIndex index = m_model.index(currentIndex.row(), 1);
        QString filePath = m_model.data(index).toString();

        //if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath())))
        //{
        //    QMessageBox::information(this, tr("warning"), tr("Failed to open file."), QMessageBox::Ok);
        //}

        QProcess process;
        filePath.replace("/", "\\");
        QString cmd = QString("explorer.exe /select,\"%1\"").arg(filePath);
        process.startDetached(cmd);
    }
}

//==========================================================================
//                        Right Key Functions                             //
//==========================================================================
void MiniThingQt::RightKeyMenu(QPoint pos)
{
    auto currentIndex = m_ui.tableView->currentIndex();
    if (currentIndex.isValid())
    {
        m_rightKeyMenu->exec(QCursor::pos());

        // Get file path info from current line
        QModelIndex index = m_model.index(currentIndex.row(), 1);
        QString filePath = m_model.data(index).toString();

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath())))
        {
            QMessageBox::information(this, tr("warning"), tr("Failed to open file."), QMessageBox::Ok);
        }
    }
}

