#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
    initData();
    initConnect();

    // 确认文件关联状态(只确认dat即可)
    ui->label_relation->setText(QString::fromLocal8Bit("未关联"));
    ui->Button_relation->setText(QString::fromLocal8Bit("设置关联"));
    QSettings settingRegClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    if (settingRegClasses.value(".dat/.", "errer").toString() == "300DATEditorFJJ.open.dat")
    {
        ui->label_relation->setText(QString::fromLocal8Bit("已关联"));
        ui->Button_relation->setText(QString::fromLocal8Bit("取消关联"));
        QSettings settingRegDat("HKEY_CURRENT_USER\\Software\\Classes\\300DATEditorFJJ.open.dat\\Shell\\Open\\Command\\", QSettings::NativeFormat);
        QString strPath = settingRegDat.value(".", "error").toString();
        if (strPath != QString("\"" + QCoreApplication::applicationFilePath().replace('/', '\\') + "\" \"%1\""))
        {
            ui->label_relation->setText(QString::fromLocal8Bit("已关联(非本程序)"));
            ui->Button_relation->setText(QString::fromLocal8Bit("设置关联"));
        }
    }

    // 获取游戏路径
    QSettings settingRegGamePath("HKEY_CURRENT_USER\\Software\\300DATEditorFJJ", QSettings::NativeFormat);
    QString strGamePath = settingRegGamePath.value("gamepath", "error").toString();
    if (strGamePath != "error") ui->line_gamePath->setText(strGamePath);
    else ui->line_gamePath->setText("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 设置传入参数
void MainWindow::setArg(int argc, char *argv[])
{
    if (argc < 2) return;
    QString strFileName = QString::fromLocal8Bit(argv[1]).replace('\\', '/');
    int result = 0;
    if (strFileName.right(4) == ".dat")
    {
        QFile inFile(strFileName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QByteArray&& data = inFile.readAll();
            int&& ret = CPatch::ReadDatData(data.data(), data.size(), m_datList);
            inFile.close();
            if (!ret) result = 1;
            else result = -1;
            m_strFileName = strFileName.mid(strFileName.lastIndexOf('/') + 1);
        }
    }
    else if (strFileName.right(4) == ".dpk")
    {
        QFile inFile(strFileName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QByteArray&& data = inFile.readAll();
            int&& ret = CPatch::ReadDpkData(data.data(), data.size(), m_dpkList);
            inFile.close();
            if (!ret) result = 2;
            else result = -1;
            m_strFileName = strFileName.mid(strFileName.lastIndexOf('/') + 1);
        }
    }
    else QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("不支持的文件类型！"));
    if (result == -1)  // erorr
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("读取数据出错！"));
        initUI();
        initData();
    }
    else openData(result);
}

// 初始化UI
void MainWindow::initUI()
{
    // 菜单栏
    ui->menu_open->setEnabled(1);
    ui->menu_import->setEnabled(0);
    ui->menu_save->setEnabled(0);
    ui->menu_lookup->setEnabled(0);
    ui->menu_reset->setEnabled(0);
    ui->menu_toolbar->setEnabled(1);
    ui->label_datName->setText(QString::fromLocal8Bit("Dat编辑器 (右键获取开源代码)"));

    // 主表格
    m_bDisableChanged = true;
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(0);

    // 查找栏
    ui->line_lookup->clear();
    ui->list_lookup->clear();

    // 工具栏
    ui->label_indexMode->clear();
    ui->list_modify->clear();
    ui->Button_indexMode->setEnabled(0);
    ui->Button_delSelect->setEnabled(0);
    ui->Button_delAll->setEnabled(0);

    // 主页
    ui->label_home->setText(QString::fromLocal8Bit("首页什么都没有！"));
    ui->stacked2->setVisible(0);
    ui->stacked1->setCurrentIndex(0);
}

// 初始化数据
void MainWindow::initData()
{
    m_readMode = 0;
    m_indexMode = 0;
    m_strFileName.clear();
    m_datList.clear();
    m_dpkList.mode = 0;
    m_dpkList.list.clear();
    m_datpkList.clear();
    m_handTypeList.clear();
}

// 初始化事件绑定
void MainWindow::initConnect()
{
    // 菜单栏
    connect(ui->menu_open, &QPushButton::clicked, this, &MainWindow::menuOpen);
    connect(ui->menu_import, &QPushButton::clicked, this, &MainWindow::menuImport);
    connect(ui->menu_save, &QPushButton::clicked, this, &MainWindow::menuSave);
    connect(ui->menu_lookup, &QPushButton::clicked, this, &MainWindow::menuLookup);
    connect(ui->menu_reset, &QPushButton::clicked, this, &MainWindow::menuReset);
    connect(ui->menu_toolbar, &QPushButton::clicked, this, &MainWindow::menuToolbar);
    connect(ui->label_datName, &QWidget::customContextMenuRequested, this,  &MainWindow::getSource);

    // 主表格
    connect(ui->tableWidget, &QWidget::customContextMenuRequested, this, &MainWindow::tableRightMenu);
    connect(ui->tableWidget, &QTableWidget::cellChanged, this, &MainWindow::tableChanged);

    // 查找
    connect(ui->line_lookup, &QLineEdit::returnPressed, this, &MainWindow::buttonLookup);
    connect(ui->Button_lookup, &QPushButton::clicked, this, &MainWindow::buttonLookup);
    connect(ui->list_lookup, &QListWidget::itemDoubleClicked, this, &MainWindow::listLookupDoubleClicked);

    // 工具栏
    connect(ui->Button_indexMode, &QPushButton::clicked, this, &MainWindow::buttonIndexMode);
    connect(ui->Button_relation, &QPushButton::clicked, this, &MainWindow::buttonRelation);
    connect(ui->Button_autoPath, &QPushButton::clicked, this, &MainWindow::buttonAutoPath);
    connect(ui->Button_gamePath, &QPushButton::clicked, this, &MainWindow::buttonGamePath);
    connect(ui->Button_delSelect, &QPushButton::clicked, this, &MainWindow::buttonDelSelect);
    connect(ui->Button_delAll, &QPushButton::clicked, this, &MainWindow::buttonDelAll);
    connect(ui->list_modify, &QListWidget::itemDoubleClicked, this, &MainWindow::listModifyDoubleClicked);
}

// 打开dat/dpk数据
void MainWindow::openData(int readMode)
{
    if (readMode == 1)    // dat
    {
        initUI();
        ui->label_home->setText(QString::fromLocal8Bit("#_# 正在装载数据ing..."));
        if (m_datList.size() == 0)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("dat数据为空！"));
            ui->label_home->setText(QString::fromLocal8Bit("首页什么都没有！"));
            initData();
            return;
        }
        if (m_datList[0].size() == 0)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("dat数据为空！"));
            ui->label_home->setText(QString::fromLocal8Bit("首页什么都没有！"));
            initData();
            return;
        }
        ui->label_datName->setText(m_strFileName);
        m_strFileName = m_strFileName.left(m_strFileName.size() - 4);

        // 设置索引模式
        m_indexMode = CPatch::GetDpkMode(m_datList);
        if (m_indexMode == -1)
        {
            if (const_datIndexList.contains(m_strFileName)) m_indexMode = const_datIndexList[m_strFileName];
            else m_indexMode = 0;
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("未知的索引类型！此文件制作dpk可能会丢失部分数据\n请在工具栏中手动设置索引模式，当前为默认模式(") + QString::number(m_indexMode) + ")");
        }
        const QString sstrIndexModeList[4] { QString::fromLocal8Bit("[0] 1列整数索引"),  QString::fromLocal8Bit("[1] 1、2列索引"),  QString::fromLocal8Bit("[2] 1、3列索引"), QString::fromLocal8Bit("[3] 1列字符索引") };
        ui->label_indexMode->setText(sstrIndexModeList[m_indexMode]);

        // 添加dat数据到表格
        int &&rowSize{ m_datList.size() }, &&columnSize{ m_datList[0].size() };
        ui->tableWidget->setColumnCount(m_datList[0].size());
        ui->tableWidget->setRowCount(m_datList.size());
        for (int &&row{ 0 }; row < rowSize; ++row)
        {
            for (int &&column{ 0 }; column < columnSize; ++column)
            {
                if (m_datList[row][column].type == 0) ui->tableWidget->setItem(row, column, new QTableWidgetItem(""));
                else if (m_datList[row][column].type == 1) ui->tableWidget->setItem(row, column, new QTableWidgetItem(QString::number(m_datList[row][column].Int)));
                else if (m_datList[row][column].type == 2) ui->tableWidget->setItem(row, column, new QTableWidgetItem(QString::number(m_datList[row][column].Float)));
                else if (m_datList[row][column].type == 3) ui->tableWidget->setItem(row, column, new QTableWidgetItem(m_datList[row][column].Str));
            }
        }

        // 设置列类型
        m_handTypeList.resize(columnSize);
        for (int &&column{ 0 }; column < columnSize; ++column)
        {
            m_handTypeList[column] = 0;
            for (int &&row{ 0 }; row < rowSize; ++row)
            {
                if (m_datList[row][column].type > m_handTypeList[column]) m_handTypeList[column] = m_datList[row][column].type;
                if (m_datList[row][column].type == 3) break;
            }
        }

        // 调整列名
        QStringList strHanderList;
        for (int &&column{ 0 }; column < columnSize; ++column)
        {
            if      (m_handTypeList[column] == 0) strHanderList << QString::number(column + 1) + QString::fromLocal8Bit(":空值");
            else if (m_handTypeList[column] == 1) strHanderList << QString::number(column + 1) + QString::fromLocal8Bit(":整数");
            else if (m_handTypeList[column] == 2) strHanderList << QString::number(column + 1) + QString::fromLocal8Bit(":小数");
            else if (m_handTypeList[column] == 3) strHanderList << QString::number(column + 1) + QString::fromLocal8Bit(":字符");
        }
        ui->tableWidget->setHorizontalHeaderLabels(strHanderList);

        // 调整列宽
        ui->tableWidget->resizeColumnsToContents();
        for (int &&column{ 0 }; column < columnSize; ++column)
            if (ui->tableWidget->columnWidth(column) > 500)
                ui->tableWidget->setColumnWidth(column, 500);

        // 调整索引列颜色
        for (int &&row{ 0 }; row < rowSize; ++row) ui->tableWidget->item(row, 0)->setForeground(QColor(255, 0, 0, 255));
        if (m_indexMode == 1) for (int &&row{ 0 }; row < rowSize; ++row) ui->tableWidget->item(row, 1)->setForeground(QColor(255, 0, 0, 255));
        if (m_indexMode == 2) for (int &&row{ 0 }; row < rowSize; ++row) ui->tableWidget->item(row, 2)->setForeground(QColor(255, 0, 0, 255));

        // 调整空值列不可编辑
        for (int &&column{ 0 }; column < columnSize; ++column)
            if (m_handTypeList[column] == 0) for (int &&row{ 0 }; row < rowSize; ++row)
                ui->tableWidget->item(row, column)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // 调整UI界面
        m_readMode = 1;
        m_bDisableChanged = false;
        ui->menu_import->setEnabled(1);
        ui->menu_save->setEnabled(1);
        ui->menu_lookup->setEnabled(1);
        ui->menu_reset->setEnabled(1);
        ui->menu_toolbar->setEnabled(1);
        ui->Button_indexMode->setEnabled(1);
        ui->Button_delSelect->setEnabled(1);
        ui->Button_delAll->setEnabled(1);
        ui->stacked1->setCurrentIndex(1);
    }
    else if (readMode == 2)   // dpk
    {
        initUI();
        ui->label_home->setText(QString::fromLocal8Bit("#_# 正在装载数据ing..."));
        if (m_dpkList.list.size() == 0)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("dpk数据为空！"));
            ui->label_home->setText(QString::fromLocal8Bit("首页什么都没有！"));
            initData();
            return;
        }
        ui->label_datName->setText(m_strFileName);
        m_strFileName = m_strFileName.left(m_strFileName.size() - 4);

        // 设置索引模式
        m_indexMode = m_dpkList.mode;
        const QString sstrIndexModeList[4] { QString::fromLocal8Bit("[0] 1列整数索引"),  QString::fromLocal8Bit("[1] 1、2列索引"),  QString::fromLocal8Bit("[2] 1、3列索引"), QString::fromLocal8Bit("[3] 1列字符索引") };
        ui->label_indexMode->setText(sstrIndexModeList[m_indexMode]);

        // 添加dpk数据到表格
        int &&dpkSize{ m_dpkList.list.size() };
        if (m_indexMode == 0 || m_indexMode == 3) ui->tableWidget->setColumnCount(4);
        else ui->tableWidget->setColumnCount(5);
        ui->tableWidget->setRowCount(dpkSize);
        if (m_indexMode == 0)
        {
            for(int&& row{ 0 }; row < dpkSize; ++row)
            {
                SDpkData& dpk = m_dpkList.list[row];
                ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(dpk.index1)));
                ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(dpk.column)));
                if (dpk.type == 1) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("整数")));
                else if (dpk.type == 2) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("小数")));
                else if (dpk.type == 3) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("字符")));
                ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dpk.data));
                ui->tableWidget->item(row, 0)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            // 调整列名
            ui->tableWidget->setHorizontalHeaderLabels(QStringList() << QString::fromLocal8Bit("索引(列1)") << QString::fromLocal8Bit("列号") << QString::fromLocal8Bit("类型") << QString::fromLocal8Bit("数据"));
        }
        else if (m_indexMode == 1)
        {
            for(int&& row{ 0 }; row < dpkSize; ++row)
            {
                SDpkData& dpk = m_dpkList.list[row];
                ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(dpk.index1)));
                ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(dpk.index2)));
                ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(dpk.column)));
                if (dpk.type == 1) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("整数")));
                else if (dpk.type == 2) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("小数")));
                else if (dpk.type == 3) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("字符")));
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(dpk.data));
                ui->tableWidget->item(row, 0)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 1)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 3)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            // 调整列名
            ui->tableWidget->setHorizontalHeaderLabels(QStringList() << QString::fromLocal8Bit("索引(列1)") << QString::fromLocal8Bit("索引(列2)") << QString::fromLocal8Bit("列号") << QString::fromLocal8Bit("类型") << QString::fromLocal8Bit("数据"));
        }
        else if (m_indexMode == 2)
        {
            for(int&& row{ 0 }; row < dpkSize; ++row)
            {
                SDpkData& dpk = m_dpkList.list[row];
                ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(dpk.index1)));
                ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(dpk.index2)));
                ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(dpk.column)));
                if (dpk.type == 1) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("整数")));
                else if (dpk.type == 2) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("小数")));
                else if (dpk.type == 3) ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLocal8Bit("字符")));
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(dpk.data));
                ui->tableWidget->item(row, 0)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 1)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 3)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            // 调整列名
            ui->tableWidget->setHorizontalHeaderLabels(QStringList() << QString::fromLocal8Bit("索引(列1)") << QString::fromLocal8Bit("索引(列3)") << QString::fromLocal8Bit("列号") << QString::fromLocal8Bit("类型") << QString::fromLocal8Bit("数据"));
        }
        else if (m_indexMode == 3)
        {
            for(int&& row{ 0 }; row < dpkSize; ++row)
            {
                SDpkData& dpk = m_dpkList.list[row];
                ui->tableWidget->setItem(row, 0, new QTableWidgetItem(dpk.indexS));
                ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(dpk.column)));
                if (dpk.type == 1) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("整数")));
                else if (dpk.type == 2) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("小数")));
                else if (dpk.type == 3) ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLocal8Bit("字符")));
                ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dpk.data));
                ui->tableWidget->item(row, 0)->setForeground(QColor(255, 0, 0, 255));
                ui->tableWidget->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->tableWidget->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            // 调整列名
            ui->tableWidget->setHorizontalHeaderLabels(QStringList() << QString::fromLocal8Bit("索引(列1)") << QString::fromLocal8Bit("列号") << QString::fromLocal8Bit("类型") << QString::fromLocal8Bit("数据"));
        }

        // 调整列宽
        ui->tableWidget->resizeColumnsToContents();
        for (int &&column{ 0 }, &&columnSize{ ui->tableWidget->columnCount() }; column < columnSize; ++column)
            if (ui->tableWidget->columnWidth(column) > 500) ui->tableWidget->setColumnWidth(column, 500);

        // 调整UI界面
        m_readMode = 2;
        m_bDisableChanged = false;
        ui->menu_import->setEnabled(0);
        ui->menu_save->setEnabled(1);
        ui->menu_lookup->setEnabled(1);
        ui->menu_reset->setEnabled(1);
        ui->menu_toolbar->setEnabled(1);
        ui->Button_indexMode->setEnabled(0);
        ui->Button_delSelect->setEnabled(1);
        ui->Button_delAll->setEnabled(1);
        ui->stacked1->setCurrentIndex(1);
    }
}

// menu 打开dat/dpk文件
void MainWindow::menuOpen()
{
    int result{ 0 };
    FJJDialog* pDialog = new FJJDialog(this, "open", &result);
    pDialog->m_pDatList = &m_datList;
    pDialog->m_pDpkList = &m_dpkList;
    pDialog->m_pstrFileName = &m_strFileName;
    pDialog->m_strGamePath = ui->line_gamePath->text();
    pDialog->exec();
    if (result == -1)  // erorr
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("读取数据出错！"));
        initUI();
        initData();
    }
    else openData(result);
}

// menu 导入dpk文件
void MainWindow::menuImport()
{
    if (m_readMode != 1) return;
    QString strFileName = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("请选择要导入的文件"), nullptr,"DPK File(*.dpk)");
    if (strFileName.isNull()) return;

    QFile inFile(strFileName);
    if (inFile.open(QIODevice::ReadOnly))
    {
        QByteArray&& data = inFile.readAll();
        DPKLIST dpkList;
        int&& ret = CPatch::ReadDpkData(data.data(), data.size(), dpkList);
        inFile.close();
        if (ret) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("读取数据出错！"));
        else
        {
            if (dpkList.mode != m_indexMode) if (QMessageBox::Yes != QMessageBox::warning(this, QString::fromLocal8Bit("警告"),
                                                                                          QString::fromLocal8Bit("dpk文件索引模式(") + QString::number(dpkList.mode)
                                                                                          + QString::fromLocal8Bit(")与当前索引模式(") + QString::number(m_indexMode)
                                                                                          + QString::fromLocal8Bit(")不符合！\n是否需要强制导入？"),
                                                                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) return;
            CPatch::DpkToDat(dpkList, m_datList, m_datpkList, m_indexMode);
            m_bDisableChanged = true;
            for (auto& datpk : m_datpkList)
            {
                ui->tableWidget->item(datpk.row, datpk.column)->setBackground(QColor(0, 255, 0, 50));
                ui->tableWidget->item(datpk.row, datpk.column)->setText(datpk.data);
            }
            m_bDisableChanged = false;
            updateModifyList();
        }
    }
    else QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("读取数据出错！"));
}

// menu 保存文件
void MainWindow::menuSave()
{
    if (m_readMode == 1)
    {
        int&& result{ 0 };
        FJJDialog* pDialog = new FJJDialog(this, "save", &result);
        pDialog->m_pDatList = &m_datList;
        pDialog->m_pDatpkList = &m_datpkList;
        pDialog->m_pstrFileName = &m_strFileName;
        pDialog->m_indexMode = m_indexMode;
        pDialog->exec();
        if (result == -1) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("操作失败！"));
    }
    if (m_readMode == 2)
    {
        QString&& strSavePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("请选择文件导出位置"), m_strFileName + ".dpk", "DPK Files (*.dpk)");
        if (strSavePath.size() == 0) return;
        QFile outFile(strSavePath);
        if (outFile.open(QIODevice::WriteOnly))
        {
            DPKLIST dpkList(m_dpkList);
            for (auto& datpk : m_datpkList) dpkList.list[datpk.row].data = datpk.data;
            QByteArray data;
            if (CPatch::WriteDpkData(dpkList, data)) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("操作失败！"));
            else outFile.write(data);
            outFile.close();
        }
    }
}

// menu 打开查找栏
void MainWindow::menuLookup()
{
    if (!ui->stacked2->isVisible())
    {
        ui->stacked2->setCurrentIndex(0);
        ui->stacked2->setVisible(1);
    }
    else if (ui->stacked2->currentIndex() != 0)
        ui->stacked2->setCurrentIndex(0);
    else ui->stacked2->setVisible(0);
}

// menu 还原全部
void MainWindow::menuReset()
{
    buttonDelAll();
}

// menu 打开工具栏
void MainWindow::menuToolbar()
{
    if (m_readMode == 0 && !ui->stacked2->isVisible())
    {
        ui->stacked1->setCurrentIndex(1);
        ui->stacked2->setCurrentIndex(1);
        ui->stacked2->setVisible(1);
    }
    else if (m_readMode == 0 && ui->stacked2->isVisible())
    {
        ui->stacked2->setVisible(0);
        ui->stacked1->setCurrentIndex(0);
    }
    else if (!ui->stacked2->isVisible())
    {
        ui->stacked2->setCurrentIndex(1);
        ui->stacked2->setVisible(1);
    }
    else if (ui->stacked2->currentIndex() != 1)
        ui->stacked2->setCurrentIndex(1);
    else ui->stacked2->setVisible(0);
}

void MainWindow::getSource(const QPoint &)
{
    if (!ui->label_datName->geometry().contains(this->mapFromGlobal(QCursor::pos()))) return;
    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton = new QAction(QString::fromWCharArray(L"获取开源代码"), this);
    connect(pMenuButton, &QAction::triggered, this, [&] {
        if (QFile::exists("300DATEditorFJJ.zip"))
        {
            if (!QFile::remove("300DATEditorFJJ.zip"))
            {
                QFile::setPermissions("300DATEditorFJJ.zip", QFileDevice::ReadOther | QFileDevice::WriteOther);
                QFile::remove("300DATEditorFJJ.zip");
            }
        }
        if (QFile::copy(":/300DATEditorFJJ.zip", "300DATEditorFJJ.zip"))
            QMessageBox::about(this, QString::fromWCharArray(L"提示"), QString::fromWCharArray(L"开源代码 (300DATEditorFJJ.zip)\n文件已放置在应用程序目录中。"));
        else
            QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"复制文件失败 (300DATEditorFJJ.zip)！\n请手动删除应用程序目录下同名文件！"));
    });
    pMenu->addAction(pMenuButton);
    pMenu->exec(QCursor::pos());

    delete pMenuButton;
    delete pMenu;
}

// table 右键菜单
void MainWindow::tableRightMenu(const QPoint &pos)
{
    QTableWidgetItem* pCurItem = ui->tableWidget->itemAt(pos);
    if (pCurItem == NULL) return;
    QList<QTableWidgetItem*>&& pItemList = ui->tableWidget->selectedItems();
    if (pItemList.size() == 0) return;

    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton = new QAction(QString::fromWCharArray(L"还原编辑"), this);
    connect(pMenuButton, &QAction::triggered, this, [&]{
        m_bDisableChanged = true;
        if (m_readMode == 1)
            for(QTableWidgetItem* &pItem : pItemList)
            {
                int &&row{ ui->tableWidget->row(pItem) }, &&column{ ui->tableWidget->column(pItem) };
                if (m_datList[row][column].type == 0) pItem->setText("");
                else if (m_datList[row][column].type == 1) pItem->setText(QString::number(m_datList[row][column].Int));
                else if (m_datList[row][column].type == 2) pItem->setText(QString::number(m_datList[row][column].Float));
                else if (m_datList[row][column].type == 3) pItem->setText(m_datList[row][column].Str);
                pItem->setBackground(Qt::NoBrush);
                m_datpkList.remove({ row, column });
            }
        else if (m_readMode == 2)
            for(QTableWidgetItem* &pItem : pItemList)
            {
                int &&row{ ui->tableWidget->row(pItem) }, &&column{ ui->tableWidget->column(pItem) };
                if (column + 1 != ui->tableWidget->columnCount()) continue;
                pItem->setText(m_dpkList.list[row].data);
                pItem->setBackground(Qt::NoBrush);
                m_datpkList.remove({ row, column });
            }
        updateModifyList();
        m_bDisableChanged = false;
    });
    pMenu->addAction(pMenuButton);

    pMenu->exec(QCursor::pos());

    delete pMenuButton;
    delete pMenu;
}

// table 单元格发生变化时
void MainWindow::tableChanged(int row, int column)
{
    if (m_bDisableChanged) return;
    m_bDisableChanged = true;
    QTableWidgetItem* pItem = ui->tableWidget->item(row, column);
    if (m_readMode == 1)
    {
        if (m_handTypeList[column] == 1) pItem->setText(QString::number(pItem->text().toInt()));
        else if (m_handTypeList[column] == 2) pItem->setText(QString::number(pItem->text().toFloat()));
        pItem->setBackground(QColor(0, 255, 0, 50));
        m_datpkList.insert({ row, column }, { row, column, m_handTypeList[column], pItem->text() });
        updateModifyList();
    }
    else if (m_readMode == 2)
    {
        if (m_dpkList.list[row].type == 1) pItem->setText(QString::number(pItem->text().toInt()));
        else if (m_dpkList.list[row].type == 2) pItem->setText(QString::number(pItem->text().toFloat()));
        pItem->setBackground(QColor(0, 255, 0, 50));
        m_datpkList.insert({ row, column }, { row, column, m_dpkList.list[row].type, pItem->text() });
        updateModifyList();
    }
    m_bDisableChanged = false;
}

// button 查找
void MainWindow::buttonLookup()
{
    QString&& strLookup = ui->line_lookup->text();
    if (strLookup.size() == 0 || strLookup == " " || strLookup == "  ")
    {
        ui->list_lookup->clear();
        ui->line_lookup->clear();
        return;
    }
    if (strLookup.size() < 2) return;
    ui->list_lookup->clear();
    int &&rowNum{ ui->tableWidget->rowCount() }, &&columnNum{ ui->tableWidget->columnCount() };
    for (int&& row{ 0 }; row < rowNum; ++row)
    {
        for (int&& column{ 0 }; column < columnNum; ++column)
        {
            QString&& strItemText = ui->tableWidget->item(row, column)->text();
            if (strItemText.contains(strLookup, Qt::CaseInsensitive))
                ui->list_lookup->addItem(QString::fromLocal8Bit("行") + QString::number(row + 1) + QString::fromLocal8Bit(", 列") + QString::number(column + 1) + " : " + strItemText);
        }
    }
}

// lookup 双击跳转
void MainWindow::listLookupDoubleClicked(QListWidgetItem *item)
{
    QString&& strItemText = item->text();
    int &&pos1{ strItemText.indexOf(',') }, &&pos2{ strItemText.indexOf(':') };
    ui->tableWidget->setCurrentCell(strItemText.mid(1, pos1 - 1).toInt() - 1, strItemText.mid(pos1 + 3, pos2 - pos1 - 4).toInt() - 1, QItemSelectionModel::ClearAndSelect);
    ui->tableWidget->setFocus();
}

// button 设置索引模式
void MainWindow::buttonIndexMode()
{
    if (m_readMode != 1) return;
    int&& result{ -1 };
    FJJDialog* pDialog = new FJJDialog(this, "index", &result);
    pDialog->exec();
    if (result == -1) return;
    if (result == m_indexMode) return;
    m_bDisableChanged = true;
    if (result == 0)
    {
        if (m_datList[0].size() > 1) for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 1)->setForeground(QColor(0, 0, 0, 255));
        if (m_datList[0].size() > 2) for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 2)->setForeground(QColor(0, 0, 0, 255));
    }
    else if (result == 1)
    {
        if (m_datList[0].size() < 2)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("表格可用列数过少，无法设置索引！"));
            m_bDisableChanged = false;
            return;
        }
        for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 1)->setForeground(QColor(255, 0, 0, 255));
        if (m_datList[0].size() > 2) for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 2)->setForeground(QColor(0, 0, 0, 255));
    }
    else if (result == 2)
    {
        if (m_datList[0].size() < 3)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("表格可用列数过少，无法设置索引！"));
            m_bDisableChanged = false;
            return;
        }
        for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row)
            ui->tableWidget->item(row, 1)->setForeground(QColor(0, 0, 0, 255)), ui->tableWidget->item(row, 2)->setForeground(QColor(255, 0, 0, 255));
    }
    else if (result == 3)
    {
        if (m_handTypeList[0] != 3)
        {
            QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("第1列不是字符类型，无法设置索引！"));
            m_bDisableChanged = false;
            return;
        }
        if (m_datList[0].size() > 1) for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 1)->setForeground(QColor(0, 0, 0, 255));
        if (m_datList[0].size() > 2) for (int &&row{ 0 }, &&rowNum{ m_datList.size() }; row < rowNum; ++row) ui->tableWidget->item(row, 2)->setForeground(QColor(0, 0, 0, 255));
    }
    m_indexMode = result;
    const QString sstrIndexModeList[4] { QString::fromLocal8Bit("[0] 1列整数索引"),  QString::fromLocal8Bit("[1] 1、2列索引"),  QString::fromLocal8Bit("[2] 1、3列索引"), QString::fromLocal8Bit("[3] 1列字符索引") };
    ui->label_indexMode->setText(sstrIndexModeList[m_indexMode]);
    m_bDisableChanged = false;
}

// button 设置文件关联
void MainWindow::buttonRelation()
{
    if (ui->Button_relation->text() == QString::fromLocal8Bit("设置关联"))
    {
        QString strIcoPath = QCoreApplication::applicationFilePath().replace('/', '\\') + ",";
        QString strExePath = "\"" + QCoreApplication::applicationFilePath().replace('/', '\\') + "\" \"%1\"";
        QSettings settingRegClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
        settingRegClasses.setValue("/300DATEditorFJJ.open.dat/Shell/Open/Command/.", strExePath);
        settingRegClasses.setValue("/300DATEditorFJJ.open.dat/.", QString::fromLocal8Bit("DAT数据库"));
        settingRegClasses.setValue("/300DATEditorFJJ.open.dat/DefaultIcon/.", strIcoPath + "1");
        settingRegClasses.setValue("/.dat/.", "300DATEditorFJJ.open.dat");
        settingRegClasses.setValue("/300DATEditorFJJ.open.dpk/Shell/Open/Command/.", strExePath);
        settingRegClasses.setValue("/300DATEditorFJJ.open.dpk/.", QString::fromLocal8Bit("DPK修改表"));
        settingRegClasses.setValue("/300DATEditorFJJ.open.dpk/DefaultIcon/.", strIcoPath + "2");
        settingRegClasses.setValue("/.dpk/.", "300DATEditorFJJ.open.dpk");
        settingRegClasses.sync();
        ui->label_relation->setText(QString::fromLocal8Bit("已关联"));
        ui->Button_relation->setText(QString::fromLocal8Bit("取消关联"));
    }
    else
    {
        QSettings().remove("HKEY_CURRENT_USER\\Software\\Classes\\300DATEditorFJJ.open.dat");
        QSettings().remove("HKEY_CURRENT_USER\\Software\\Classes\\300DATEditorFJJ.open.dpk");
        QSettings("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat).setValue("/.dat/.", "");
        QSettings("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat).setValue("/.dpk/.", "");
        ui->label_relation->setText(QString::fromLocal8Bit("未关联"));
        ui->Button_relation->setText(QString::fromLocal8Bit("设置关联"));
    }
}

// button 自动搜寻游戏路径
void MainWindow::buttonAutoPath()
{
    QString strGamePath;
    // 从注册表搜索
    QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\", QSettings::NativeFormat);
    for(auto& strRegItem : settings.childGroups())
    {
        settings.beginGroup(strRegItem);
        QString strDisplayName = settings.value("DisplayName").toString();
        if((!strDisplayName.isEmpty())&&(strDisplayName.contains("300", Qt::CaseSensitive)))
        {
            QString strPath = settings.value("UninstallString").toString();
            if(strDisplayName.contains("300battle", Qt::CaseSensitive))
            {
                int&& nPos = strPath.lastIndexOf("\\");
                nPos = strPath.lastIndexOf("\\", nPos - 1);
                strGamePath = strPath.mid(1, nPos).append("300Hero").replace("\\", "/");
                break;
            }
            else if(strDisplayName == L"300英雄")
            {
                int&& nPos = strPath.lastIndexOf("\\");
                strGamePath = strPath.mid(1, nPos-1).replace("\\", "/");
                break;
            }
        }
        settings.endGroup();
    }
    // 从特定位置搜索
    if (strGamePath.size() == 0)
        for (QFileInfo my_info : QDir::drives())
        {
            QString strPath(my_info.absoluteFilePath());
            if (QDir().exists(QString(strPath).append("JumpGame/300Hero")))
            {
                strGamePath = strPath + "JumpGame/300Hero";
                break;
            }
            else if (QDir().exists(QString(strPath).append(QString::fromLocal8Bit("Program Files (x86)/Jump/300英雄"))))
            {
                strGamePath = strPath + QString::fromLocal8Bit("Program Files (x86)/Jump/300英雄");
                break;
            }
        }
    if (strGamePath.size() == 0)
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("自动搜索失败！请手动设置游戏路径！"));
        return;
    }
    QSettings settingRegClasses("HKEY_CURRENT_USER\\Software", QSettings::NativeFormat);
    settingRegClasses.setValue("/300DATEditorFJJ/gamepath", strGamePath);
    settingRegClasses.sync();
    ui->line_gamePath->setText(strGamePath);
}

// button 设置游戏路径
void MainWindow::buttonGamePath()
{
    QString strGamePath = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("请设置300英雄游戏路径"));
    if (strGamePath.size() == 0) return;
    QSettings settingRegClasses("HKEY_CURRENT_USER\\Software", QSettings::NativeFormat);
    settingRegClasses.setValue("/300DATEditorFJJ/gamepath", strGamePath);
    settingRegClasses.sync();
    ui->line_gamePath->setText(strGamePath);
}

// button 删除modeify选中项
void MainWindow::buttonDelSelect()
{
    m_bDisableChanged = true;
    if (m_readMode == 1)
    {
        QList<QListWidgetItem*>&& pItemList = ui->list_modify->selectedItems();
        for(auto &&ft{ pItemList.rbegin() }, &&fend{ pItemList.rend() }; ft != fend; ++ft)
        {
            int&& index = ui->list_modify->row(*ft);
            auto&& it = m_datpkList.begin() + index;
            QTableWidgetItem* pTableItem = ui->tableWidget->item(it->row, it->column);
            SDatData& dat = m_datList[it->row][it->column];
            pTableItem->setBackground(Qt::NoBrush);
            if (dat.type == 0) pTableItem->setText("");
            else if (dat.type == 1) pTableItem->setText(QString::number(dat.Int));
            else if (dat.type == 2) pTableItem->setText(QString::number(dat.Float));
            else if (dat.type == 3) pTableItem->setText(dat.Str);
            m_datpkList.erase(it);
        }
        updateModifyList();
    }
    else if (m_readMode == 2)
    {
        QList<QListWidgetItem*>&& pItemList = ui->list_modify->selectedItems();
        for(auto &&ft{ pItemList.rbegin() }, &&fend{ pItemList.rend() }; ft != fend; ++ft)
        {
            int&& index = ui->list_modify->row(*ft);
            auto&& it = m_datpkList.begin() + index;
            ui->tableWidget->item(it->row, it->column)->setBackground(Qt::NoBrush);
            ui->tableWidget->item(it->row, it->column)->setText(m_dpkList.list[it->row].data);
            m_datpkList.erase(it);
        }
        updateModifyList();
    }
    m_bDisableChanged = false;
}

// button 删除modeify全部
void MainWindow::buttonDelAll()
{
    m_bDisableChanged = true;
    if (m_readMode == 1)
    {
        for(auto& datpk : m_datpkList)
        {
            QTableWidgetItem* pItem = ui->tableWidget->item(datpk.row, datpk.column);
            SDatData& dat = m_datList[datpk.row][datpk.column];
            pItem->setBackground(Qt::NoBrush);
            if (dat.type == 0) pItem->setText("");
            else if (dat.type == 1) pItem->setText(QString::number(dat.Int));
            else if (dat.type == 2) pItem->setText(QString::number(dat.Float));
            else if (dat.type == 3) pItem->setText(dat.Str);
        }
        m_datpkList.clear();
        ui->list_modify->clear();
    }
    else if (m_readMode == 2)
    {
        for(auto& datpk : m_datpkList)
        {
            ui->tableWidget->item(datpk.row, datpk.column)->setBackground(Qt::NoBrush);
            ui->tableWidget->item(datpk.row, datpk.column)->setText(m_dpkList.list[datpk.row].data);
        }
        m_datpkList.clear();
        ui->list_modify->clear();
    }
    m_bDisableChanged = false;
}

// modeify 更新已修改列表
void MainWindow::updateModifyList()
{
    ui->list_modify->clear();
    if (m_readMode == 1)
        for (auto& datpk : m_datpkList) ui->list_modify->addItem(QString::fromLocal8Bit("行") + QString::number(datpk.row + 1) + QString::fromLocal8Bit(", 列") + QString::number(datpk.column + 1) + " : " + datpk.data);
    else if (m_readMode == 2)
        for (auto& datpk : m_datpkList) ui->list_modify->addItem(QString::fromLocal8Bit("行") + QString::number(datpk.row + 1) + " : " + datpk.data);
}

// modeify 双击跳转
void MainWindow::listModifyDoubleClicked(QListWidgetItem *item)
{
    int&& index = ui->list_modify->row(item);
    auto&& it = m_datpkList.begin() + index;
    ui->tableWidget->setCurrentCell(it->row, it->column, QItemSelectionModel::ClearAndSelect);
    ui->tableWidget->setFocus();
}
