#ifndef DLEAKALARMDLG_H
#define DLEAKALARMDLG_H

#include <QDialog>

class DDialogTitle;
class QLabel;

class DLeakAlarmDlg : public QDialog
{
    Q_OBJECT
public:
    explicit DLeakAlarmDlg(QWidget *parent = 0);

private:
    void initTitleBar();
    void initUI();

private slots:
    void on_pushButton_Reset();

private:
    DDialogTitle* m_titleBar;
    QLabel *m_pLabel;
    QPushButton *m_pConfirmBtn;
    
};

#endif // DLEAKALARMDLG_H
