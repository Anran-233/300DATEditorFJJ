#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fjjdialog.h"
#include "cpatch.h"

#include <QListWidget>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setArg(int argc, char* argv[]);

private:
    Ui::MainWindow *ui;

    bool m_bDisableChanged = true;      // 是否禁用Changed信号
    int m_readMode = 0;                 // 当前读取文件模式: 0.无; 1.dat; 2.dpk;
    int m_indexMode = 0;                // 索引模式 (默认模式0)
    QString m_strFileName;              // 文件名称
    DATLIST m_datList;                  // dat数据列表
    DPKLIST m_dpkList;                  // dpk数据列表
    DATPKLIST m_datpkList;              // dat和dpk的数据对照表
    QVector<int> m_handTypeList;        // table表格头部类型表

    void initUI();
    void initData();
    void initConnect();
    void openData(int readMode);
    void menuOpen();
    void menuImport();
    void menuSave();
    void menuLookup();
    void menuReset();
    void menuToolbar();
    void getSource(const QPoint &pos);
    void tableRightMenu(const QPoint &pos);
    void tableChanged(int row, int column);
    void buttonLookup();
    void listLookupDoubleClicked(QListWidgetItem *item);
    void buttonIndexMode();
    void buttonRelation();
    void buttonAutoPath();
    void buttonGamePath();
    void buttonDelSelect();
    void buttonDelAll();
    void updateModifyList();
    void listModifyDoubleClicked(QListWidgetItem *item);
};
#endif // MAINWINDOW_H
