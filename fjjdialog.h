#ifndef FJJDIALOG_H
#define FJJDIALOG_H

#include "cpatch.h"

#include <QDialog>

namespace Ui {
class FJJDialog;
}

class FJJDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FJJDialog(QWidget *parent, QString strType, int *result);
    ~FJJDialog();

    DATLIST* m_pDatList{ NULL };
    DPKLIST* m_pDpkList{ NULL };
    DATPKLIST* m_pDatpkList{ NULL };
    QString* m_pstrFileName{ NULL };
    QString m_strGamePath;
    int m_indexMode;

private:
    Ui::FJJDialog *ui;
    int* m_pResult;
    JMPLIST m_jmpList[4];
    QHash<QString, QString> m_rbDatList;

    bool eventFilter(QObject *, QEvent *event ) override;
    void buttonOpenFile();
    void buttonOpenGame();
    void buttonOpen300RB();
    void buttonOpenListOK();
    void buttonSaveDat();
    void buttonSaveDpk();
};

#endif // FJJDIALOG_H
