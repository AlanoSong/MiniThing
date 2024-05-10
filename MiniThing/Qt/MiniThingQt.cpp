#include "MiniThingQt.h"

//==========================================================================
//                        Common Functions                                //
//==========================================================================

// Opens the file explorer and highlights the specified file path. The path is adjusted
// to the system's file path format before opening.
void MiniThingQt::OpenFilePath(const QString& filePath)
{
    QString adjustedPath = filePath;
    adjustedPath.replace("/", "\\");
    QString command = QString("explorer.exe /select,\"%1\"").arg(adjustedPath);
    QProcess::startDetached(command);
}


// Attempts to open a file at the specified path. Returns true if the file is successfully opened,
// and false with an error message if the file cannot be opened.
bool MiniThingQt::OpenFile(const QString& filePath)
{
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath())))
    {
        QMessageBox::information(this, tr("Warning"), tr("Failed to open file."), QMessageBox::Ok);
        return false;
    }
    return true;
}

MiniThingQt::MiniThingQt(QWidget* parent) : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_usnSet.clear();

    // Setup UI components
    this->SetupUIComponents();
}

void MiniThingQt::SetupUIComponents()
{
    // Setup logo and other UI elements
    QIcon logo("Logo.ico");
    this->setWindowIcon(logo);
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(255, 255, 255));
    this->setPalette(palette);

    // Status Bar setup
    m_statusBar = new QLabel;
    statusBar()->addWidget(m_statusBar);
    statusBar()->setStyleSheet("QLabel { color: black }");

    // Setup actions, menus, and other signal-slot connections
    this->SetupActionsAndMenus();
    m_pMiniThingCore = new MiniThingCore();
    m_pMiniThingQtWorkThread = new MiniThingQtWorkThread(m_pMiniThingCore, statusBar());

    // Connect UpdateStatusBar(), so we could update status bar in diff thread
    connect(m_pMiniThingQtWorkThread, &MiniThingQtWorkThread::UpdateStatusBar, this, &MiniThingQt::UpdateStatusBar);
    m_pMiniThingQtWorkThread->start();

    // Setup table view
    m_ui.tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui.tableView->verticalHeader()->setVisible(true);
    // Uncomment the next lines if you need to set specific resize modes for headers
    // m_ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // m_ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // m_ui.tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui.tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect line edit returnPressed signal to EnterPressDown slot
    connect(m_ui.lineEdit, SIGNAL(returnPressed()), this, SLOT(EnterPressDown()));

    // Connect push button clicked signal to ButtonSearchClicked slot
    connect(m_ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonSearchClicked()));

    // Setup table and update view
    UpdateTableView();
}

void MiniThingQt::SetupActionsAndMenus()
{
    // Setup right-click menu
    m_rightKeyMenu = new QMenu(m_ui.tableView);
    m_rightKeyActionOpen = new QAction(tr("Open file (ctrl+o)"), this);
    m_rightKeyActionOpenPath = new QAction(tr("Open file path (ctrl+p)"), this);
    m_rightKeyMenu->addAction(m_rightKeyActionOpen);
    m_rightKeyMenu->addAction(m_rightKeyActionOpenPath);
    connect(m_ui.tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(RightKeyMenu(QPoint)));

    // Setup shortcut keys
    m_shortKeySearch = new QAction(tr("Search (ctrl+f)"), this);
    m_shortKeyOpen = new QAction(tr("Open (ctrl+o)"), this);
    m_shortKeyOpenPath = new QAction(tr("Open Path (ctrl+p)"), this);
    m_shortKeyDelete = new QAction(tr("Delete (ctrl+d)"), this);
    m_shortKeyCopy = new QAction(tr("Copy (ctrl+c)"), this);

    addAction(m_shortKeySearch);
    addAction(m_shortKeyOpen);
    addAction(m_shortKeyOpenPath);
    addAction(m_shortKeyDelete);
    addAction(m_shortKeyCopy);

    connect(m_shortKeySearch, SIGNAL(triggered()), this, SLOT(ShortKeySearch()));
    connect(m_shortKeyOpen, SIGNAL(triggered()), this, SLOT(ShortKeyOpen()));
    connect(m_shortKeyOpenPath, SIGNAL(triggered()), this, SLOT(ShortKeyOpenPath()));
    connect(m_shortKeyDelete, SIGNAL(triggered()), this, SLOT(ShortKeyDelete()));
    connect(m_shortKeyCopy, SIGNAL(triggered()), this, SLOT(ShortKeyCopy()));
}

MiniThingQt::~MiniThingQt()
{
    // Destroy qt work thread firstly, cause there are some data strcture
    //  we need use to terminate thread and so on
    //  after we destroy qt work thread, we can destroy core safely
    delete m_pMiniThingQtWorkThread;
    delete m_pMiniThingCore;

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
    this->SetStatusBar(QString("Preparing show ..."));

    m_model.clear();
    QStringList lHeader = { "Name", "Path", "Size", "Last Modified" };
    m_model.setHorizontalHeaderLabels(lHeader);
    m_ui.tableView->setColumnWidth(0, 200);
    m_ui.tableView->setColumnWidth(1, 800);
    m_ui.tableView->setColumnWidth(2, 50);
    m_ui.tableView->setColumnWidth(3, 150);

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

void MiniThingQt::ButtonOpenClicked()
{
    const TCHAR szOperation[] = _T("open");
    const TCHAR szAddress[] = MINITHING_GITHUB_URL;
    HINSTANCE hRslt = ShellExecute(NULL, szOperation, szAddress, NULL, NULL, SW_SHOWNORMAL);
}

//==========================================================================
//                        Short Key Functions                             //
//==========================================================================
void MiniThingQt::ShortKeySearch()
{
    if (m_pMiniThingQtWorkThread->isMiniThingCoreReady())
    {
        this->SetStatusBar(QString("Searching ..."));

        QString search = m_ui.lineEdit->text();

        // Clear m_usnSet firstly, cause SQLiteQueryV2 use push back to add file node
        m_usnSet.clear();

        if(!search.isEmpty())
        {
            QueryInfo queryInfo;
            queryInfo.type = QUERY_BY_NAME;
            queryInfo.info.fileNameWstr = search.toStdWString();

            m_pMiniThingCore->SQLiteQueryV2(&queryInfo, m_usnSet);
        }

        // Call update table view to refresh all msg show
        UpdateTableView();

        if (!search.isEmpty())
        {
            QString status;
            status.append(QString::number(m_usnSet.size()));
            status.append(tr(" objests"));
            this->SetStatusBar(status);
        }
        else
        {
            this->SetStatusBar(QString("Input something to search"));
        }
    }
    else
    {
        this->SetStatusBar(QString("Scanning files..."));
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

        // Use openFile to attempt opening the file
        if (this->OpenFile(filePath))
        {
            this->SetStatusBar(tr("File opened"));
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

        this->OpenFilePath(filePath); // Use openFilePath to open the file path in the explorer
        this->SetStatusBar(tr("File path opened"));
    }
}

void MiniThingQt::ShortKeyDelete()
{
    // Deletes the selected file or directory after confirming its existence
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
            this->SetStatusBar(tr("File deleted"));
        }
        else if (fileInfo.isDir())
        {
            QDir qDir(filePath);
            qDir.removeRecursively();
            this->SetStatusBar(tr("Directory deleted"));
        }
    }
}

void MiniThingQt::ShortKeyCopy()
{
    // Copies the file path of the selected file to the clipboard
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

        this->SetStatusBar(tr("File copied"));
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

        // Use openFile to attempt opening the file
        if (this->OpenFile(filePath))
        {
            this->SetStatusBar(tr("File opened"));
        }
    }
}