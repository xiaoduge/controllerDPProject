#ifndef DWARNINGDLG_H
#define DWARNINGDLG_H

#include <QDialog>

class DDialogTitle;
class QLabel;

class DWarningDlg : public QDialog
{
    Q_OBJECT
public:
    explicit DWarningDlg(const QString &text, QWidget *parent = 0);

private:
    void initTitleBar();
    void initUI(const QString &text);

private slots:
    void onConfirmBtnClicked();

private:
    DDialogTitle* m_titleBar;
    QLabel *m_pLabel;
    QPushButton *m_pConfirmBtn;

};

#endif // DWARNINGDLG_H
