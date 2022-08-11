#include "fjjdialog.h"
#include "ui_fjjdialog.h"

#include "Windows.h"
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QtZlib/zlib.h>

FJJDialog::FJJDialog(QWidget *parent, QString strType, int *pResult) :
    QDialog(parent),
    ui(new Ui::FJJDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::WindowCloseButtonHint | Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose);

    m_pResult = pResult;
    if (strType == "open")
    {
        setMinimumSize(300, 160);
        setMaximumSize(300, 160);
        ui->stacked->setCurrentIndex(0);
        ui->stacked_open->setCurrentIndex(0);
        ui->label_open->clear();
        ui->combo_open->clear();
        connect(ui->Button_openFile, &QPushButton::clicked, this, &FJJDialog::buttonOpenFile);
        connect(ui->Button_openGame, &QPushButton::clicked, this, &FJJDialog::buttonOpenGame);
        connect(ui->Button_open300RB, &QPushButton::clicked, this, &FJJDialog::buttonOpen300RB);
        connect(ui->Button_openCancel, &QPushButton::clicked, this, &FJJDialog::close);
        connect(ui->Button_openListOK, &QPushButton::clicked, this, &FJJDialog::buttonOpenListOK);
        connect(ui->Button_openListCancel, &QPushButton::clicked, this, &FJJDialog::close);
    }
    else if (strType == "save")
    {
        setMinimumSize(300, 160);
        setMaximumSize(300, 160);
        ui->stacked->setCurrentIndex(1);
        connect(ui->Button_saveDAT, &QPushButton::clicked, this, &FJJDialog::buttonSaveDat);
        connect(ui->Button_saveDPK, &QPushButton::clicked, this, &FJJDialog::buttonSaveDpk);
        connect(ui->Button_saveCancel, &QPushButton::clicked, this, &FJJDialog::close);
    }
    else if (strType == "index")
    {
        setMinimumSize(300, 160);
        setMaximumSize(300, 160);
        ui->stacked->setCurrentIndex(2);
        connect(ui->Button_index0, &QPushButton::clicked, this, [&]{ *m_pResult = 0; this->close(); });
        connect(ui->Button_index1, &QPushButton::clicked, this, [&]{ *m_pResult = 1; this->close(); });
        connect(ui->Button_index2, &QPushButton::clicked, this, [&]{ *m_pResult = 2; this->close(); });
        connect(ui->Button_index3, &QPushButton::clicked, this, [&]{ *m_pResult = 3; this->close(); });
        connect(ui->Button_indexCancel, &QPushButton::clicked, this, &FJJDialog::close);
    }
}

FJJDialog::~FJJDialog()
{
    delete ui;
}

// 事件过滤器（屏蔽Esc键事件）
bool FJJDialog::eventFilter(QObject *, QEvent *event)
{
    if ( event->type() == QEvent::KeyPress ||
        event->type() == QEvent::KeyRelease ) {
        if ( ( (QKeyEvent *) event )->key() == Qt::Key_Escape ) {
            return true;
        }
    }
    return false;
}

// open 打开本地文件
void FJJDialog::buttonOpenFile()
{
    QString strFileName = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("请选择要打开的文件"), nullptr,"DAT/DPK File(*.dat *.dpk)");
    if (strFileName.isNull()) return;

    if (strFileName.right(4) == ".dat")
    {
        QFile inFile(strFileName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QByteArray&& data = inFile.readAll();
            int&& ret = CPatch::ReadDatData(data.data(), data.size(), *m_pDatList);
            inFile.close();
            if (!ret) *m_pResult = 1;
            else *m_pResult = -1;
            *m_pstrFileName = strFileName.mid(strFileName.indexOf('/') + 1);
            this->close();
        }
    }
    else if (strFileName.right(4) == ".dpk")
    {
        QFile inFile(strFileName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QByteArray&& data = inFile.readAll();
            int&& ret = CPatch::ReadDpkData(data.data(), data.size(), *m_pDpkList);
            inFile.close();
            if (!ret) *m_pResult = 2;
            else *m_pResult = -1;
            *m_pstrFileName = strFileName.mid(strFileName.indexOf('/') + 1);
            this->close();
        }
    }
}

// open 从游戏目录的数据包中选择dat文件
void FJJDialog::buttonOpenGame()
{
    if (m_strGamePath.size() < 2)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("游戏路径为空！"));
        return;
    }
    if (!QDir().exists(m_strGamePath))
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("游戏路径不存在！"));
        return;
    }

    // 读取jmp索引表
    const QString strIndexList[4] { "(data0)", "(data1)", "(data2)", "(data3)" };
    const QString strFilePathList[4] { m_strGamePath + "/Data.jmp", m_strGamePath + "/Data1.jmp", m_strGamePath + "/Data2.jmp", m_strGamePath + "/Data3.jmp" };
    for (int&& i{ 0 }; i < 4; ++i)
    {
        QFile inFile(strFilePathList[i]);
        if (inFile.open(QIODevice::ReadOnly))
        {
            if (inFile.size() > 10485760)
            {
                QByteArray&& data = inFile.read(10485760);
                int&& ret = CPatch::ReadJmpDataOfDat(data.data(), data.size(), m_jmpList[i]);
                if (ret) m_jmpList[i].clear();
            }
            inFile.close();
        }
    }
    if ((m_jmpList[0].size() + m_jmpList[1].size() + m_jmpList[2].size() + m_jmpList[3].size()) == 0)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("找不到dat文件！"));
        return;
    }
    ui->label_open->setText(QString::fromLocal8Bit("从游戏目录的数据包中选择dat文件："));
    for (int&& i{ 0 }; i < 4; ++i) for (auto& jmp : m_jmpList[i]) ui->combo_open->addItem(strIndexList[i] + jmp.name);
    ui->stacked_open->setCurrentIndex(1);
}

// open 从300资源浏览器的导出目录中选择dat文件
void FJJDialog::buttonOpen300RB()
{
    // 确认300资源浏览器关联状态
    QSettings settingRegClasses("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    if (settingRegClasses.value(".jmp/.", "errer").toString() != "300ResourceBrowser.open.jmp")
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("300资源浏览器未设置关联！"));
        return;
    }

    // 读取300资源浏览器配置文件
    QSettings settingRegJmp("HKEY_CURRENT_USER\\Software\\Classes\\300ResourceBrowser.open.jmp\\Shell\\Open\\Command\\", QSettings::NativeFormat);
    QString&& strPath = settingRegJmp.value(".", "errer").toString();
    QString strConfigPath = strPath.mid(1, strPath.lastIndexOf('\\')) + "BConfig\\Config.ini";
    if (!QFile::exists(strConfigPath))
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("无法读取300资源浏览器配置文件！"));
        return;
    }

    // 读取dat文件列表
    wchar_t lcExportPath[260], lcAlonePath[260];
    GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 260, strConfigPath.toStdWString().data());
    GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 260, strConfigPath.toStdWString().data());
    QString&& strExportPath = QString::fromWCharArray(lcExportPath);
    QString&& strAlonePath = QString::fromWCharArray(lcAlonePath);
    if (strExportPath.size() > 0)
    {
        for(auto& strFileName : QDir(strExportPath + "/excel").entryList(QStringList() << "*.dat", QDir::Files | QDir::Readable))
        {
            const QString&& strKey = QString::fromLocal8Bit("(路径导出)") + strFileName;
            ui->combo_open->addItem(strKey);
            m_rbDatList.insert(strKey, strExportPath + "/excel/" + strFileName);
        }
    }
    if (strAlonePath.size() > 0)
    {
        for(auto& strFileName : QDir(strAlonePath).entryList(QStringList() << "*.dat", QDir::Files | QDir::Readable))
        {
            const QString&& strKey = QString::fromLocal8Bit("(单独导出)") + strFileName;
            ui->combo_open->addItem(strKey);
            m_rbDatList.insert(strKey, strAlonePath + "/" + strFileName);
        }
    }
    if (m_rbDatList.size() == 0)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("找不到dat文件！"));
        return;
    }
    ui->label_open->setText(QString::fromLocal8Bit("从300资源浏览器的导出目录中选择dat文件："));
    ui->stacked_open->setCurrentIndex(1);
}

// open 确定按钮
void FJJDialog::buttonOpenListOK()
{
    QString&& strDatResult = ui->combo_open->currentText();

    // 从游戏目录的数据包中选择dat文件
    const QString strIndexList[4] { "(data0)", "(data1)", "(data2)", "(data3)" };
    const QString strFilePathList[4] { m_strGamePath + "/Data.jmp", m_strGamePath + "/Data1.jmp", m_strGamePath + "/Data2.jmp", m_strGamePath + "/Data3.jmp" };
    for (int&& i{ 0 }; i < 4; ++i)
    {
        if (strDatResult.left(7) != strIndexList[i]) continue;
        SJmpIndex& jmp = m_jmpList[i][strDatResult.mid(7)];
        QFile inFile(strFilePathList[i]);
        if (inFile.open(QIODevice::ReadOnly))
        {
            inFile.seek(jmp.index);
            QByteArray&& compData = inFile.read(jmp.compLen);
            inFile.close();
            unsigned char* pFileData = new unsigned char[jmp.fileLen];
            unsigned long nFileLen = (unsigned long)jmp.fileLen;
            if (Z_OK == z_uncompress(pFileData, &nFileLen, (unsigned char*)compData.data(), (unsigned long)compData.size()))
            {
                if (!CPatch::ReadDatData((char*)pFileData, (int)nFileLen, *m_pDatList))
                {
                    delete [] pFileData;
                    *m_pResult = 1;
                    *m_pstrFileName = strDatResult.mid(7);
                    this->close();
                    return;
                }
            }
            delete [] pFileData;
        }
        *m_pResult = -1;
        this->close();
        return;
    }

    // 从300资源浏览器的导出目录中选择dat文件
    QFile inFile(m_rbDatList[ui->combo_open->currentText()]);
    if (inFile.open(QIODevice::ReadOnly))
    {
        QByteArray&& data = inFile.readAll();
        int&& ret = CPatch::ReadDatData(data.data(), data.size(), *m_pDatList);
        inFile.close();
        if (!ret) *m_pResult = 1;
        else *m_pResult = -1;
        *m_pstrFileName = ui->combo_open->currentText().mid(ui->combo_open->currentText().indexOf(')') + 1);
    }
    else *m_pResult = -1;
    this->close();
}

// save 保存为dat文件
void FJJDialog::buttonSaveDat()
{
    QString&& strSavePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("请选择文件保存位置"), *m_pstrFileName + ".dat", "DAT Files (*.dat)");
    if (strSavePath.size() == 0) return;
    QFile outFile(strSavePath);
    if (outFile.open(QIODevice::WriteOnly))
    {
        DATLIST datList(*m_pDatList);
        for (auto& datpk : *m_pDatpkList)
        {
            SDatData& dat = datList[datpk.row][datpk.column];
            dat.type = datpk.type;
            if (datpk.type == 1) dat.Int = datpk.data.toLongLong();
            else if (datpk.type == 2) dat.Float = datpk.data.toFloat();
            else if (datpk.type == 3) dat.Str = datpk.data;
        }
        QByteArray data;
        if (CPatch::WriteDatData(datList, data)) *m_pResult = -1;
        else *m_pResult = 1;
        outFile.write(data);
        outFile.close();
    }
}

// save 导出为dpk文件
void FJJDialog::buttonSaveDpk()
{
    QString&& strSavePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("请选择文件导出位置"), *m_pstrFileName + ".dpk", "DPK Files (*.dpk)");
    if (strSavePath.size() == 0) return;
    QFile outFile(strSavePath);
    if (outFile.open(QIODevice::WriteOnly))
    {
        DPKLIST dpkList;
        CPatch::DatToDpk(*m_pDatList, dpkList, *m_pDatpkList, m_indexMode);
        QByteArray data;
        if (CPatch::WriteDpkData(dpkList, data)) *m_pResult = -1;
        else *m_pResult = 1;
        outFile.write(data);
        outFile.close();
    }
}
