#include "MiniThingQt.h"

//==========================================================================
//                        Common Functions                                //
//==========================================================================
MiniThingQt::MiniThingQt(QWidget* parent) : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_usnSet.clear();

    // Setup logo icon
    QIcon logo("Logo.ico");
    this->setWindowIcon(logo);

    // Setup status bar
    m_statusBar = new QLabel;
    statusBar()->addWidget(m_statusBar);
    statusBar()->setStyleSheet("QLabel{ color: black }");

    this->SetStatusBar(QString("scanning files..."));

    // Create MiniThingCore instance, put it in background
    m_pMiniThingCore = new MiniThingCore(".\\MiniThing.db");
    m_pMiniThingQtWorkThread = new MiniThingQtWorkThread(m_pMiniThingCore);
    m_pMiniThingQtWorkThread->start();

    // Monitor enter press in line edit
    connect(m_ui.lineEdit, SIGNAL(returnPressed()), this, SLOT(EnterPressDown()));

    // Setup right key
    m_rightKeyMenu = new QMenu(m_ui.tableView);
    m_rightKeyActionOpen = new QAction();
    m_rightKeyActionOpenPath = new QAction();

    m_rightKeyActionOpen->setText(QString("Open(ctrl+o)"));
    m_rightKeyActionOpenPath->setText(QString("Open path(ctrl+p)"));
    m_rightKeyMenu->addAction(m_rightKeyActionOpen);
    m_rightKeyMenu->addAction(m_rightKeyActionOpenPath);
    connect(m_ui.tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(RightKeyMenu(QPoint)));

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

    m_shortKeyDelete = new QAction(this);
    m_shortKeyDelete->setShortcut(tr("ctrl+d"));
    this->addAction(m_shortKeyDelete);
    connect(m_shortKeyDelete, SIGNAL(triggered()), this, SLOT(ShortKeyDelete()));

    m_shortKeyCopy = new QAction(this);
    m_shortKeyCopy->setShortcut(tr("ctrl+c"));
    this->addAction(m_shortKeyCopy);
    connect(m_shortKeyCopy, SIGNAL(triggered()), this, SLOT(ShortKeyCopy()));

    // Setup table view
    m_ui.tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui.tableView->verticalHeader()->setVisible(true);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // ui.tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui.tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    UpdateTableView();
}

MiniThingQt::~MiniThingQt()
{
    delete m_pMiniThingCore;
    delete m_pMiniThingQtWorkThread;

    delete m_rightKeyMenu;
    delete m_rightKeyActionOpen;
    delete m_rightKeyActionOpenPath;

    delete m_shortKeySearch;
    delete m_shortKeyOpen;
    delete m_shortKeyOpenPath;
    delete m_shortKeyDelete;
    delete m_shortKeyCopy;

    delete m_statusBar;
}

void MiniThingQt::UpdateTableView(void)
{
    m_model.clear();
    QStringList lHeader = { "Name", "Path", "Size", "Last Modified" };
    m_model.setHorizontalHeaderLabels(lHeader);

    QList<QStandardItem*> lRow;

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
        else
        {
            pItemSize = new QStandardItem(tr(""));
            pItemLastMod = new QStandardItem(tr(""));
        }

        lRow.clear();
        lRow << pItemName << pItemPath << pItemSize << pItemLastMod;

        m_model.appendRow(lRow);
    }

    m_ui.tableView->setModel(&m_model);
}

//==========================================================================
//                        Status Bar Functions                            //
//==========================================================================
void MiniThingQt::SetStatusBar(QString str)
{
    statusBar()->showMessage(str);
}

//==========================================================================
//                 Enter Press Down For Line Edit Functions               //
//==========================================================================
void MiniThingQt::EnterPressDown(void)
{
    // Here reuse short key func
    ShortKeySearch();
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
    if (m_pMiniThingQtWorkThread->isMiniThingCoreReady())
    {
        QString search = m_ui.lineEdit->text();

        //if (search == m_searchBefore)
        //{
        //    // Search content is the same with before, do nothing
        //    goto JUST_SHOW_MSG;
        //}
        //else
        {
            // Clear m_usnSet firstly, cause SQLiteQueryV2 use push back to add file node
            m_usnSet.clear();

            QueryInfo queryInfo;
            queryInfo.type = QUERY_BY_NAME;
            queryInfo.info.fileNameWstr = StringToWstring(search.toStdString());

            m_pMiniThingCore->SQLiteQueryV2(&queryInfo, m_usnSet);

            m_searchBefore = search;
        }

        // Call update table view to refresh all msg show
        UpdateTableView();

        QString status;
        status.append(QString::number(m_usnSet.size()));
        status.append(tr(" objests"));
        this->SetStatusBar(status);
    }
    else
    {
        this->SetStatusBar(QString("scanning files..."));
    }
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

        this->SetStatusBar(tr("file opened"));
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

        this->SetStatusBar(tr("file path opened"));
    }
}

void MiniThingQt::ShortKeyDelete()
{
    auto currentIndex = m_ui.tableView->currentIndex();
    if (currentIndex.isValid())
    {
        // Get file path info from current line
        QModelIndex index = m_model.index(currentIndex.row(), 1);
        QString filePath = m_model.data(index).toString();
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile())
        {
            QFile::remove(filePath);
            this->SetStatusBar(tr("file deleted"));
        }
        else if (fileInfo.isDir())
        {
            QDir qDir(filePath);
            qDir.removeRecursively();
            this->SetStatusBar(tr("directory deleted"));
        }
    }
}

void MiniThingQt::ShortKeyCopy()
{
    auto currentIndex = m_ui.tableView->currentIndex();
    if (currentIndex.isValid())
    {
        // Get file path info from current line
        QModelIndex index = m_model.index(currentIndex.row(), 1);
        QString filePath = m_model.data(index).toString();

        QList<QUrl> copyfile;
        copyfile.push_back(QUrl::fromLocalFile(filePath));

        QMimeData* data = new QMimeData;
        data->setUrls(copyfile);

        QClipboard* clip = QApplication::clipboard();
        clip->setMimeData(data);

        this->SetStatusBar("file copied");
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

        this->SetStatusBar("file opened");
    }
}
