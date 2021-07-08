#include "dleakalarmdlg.h"
#include "mainwindow.h"
#include "exdisplay.h"
#include "ddialogtitle.h"
#include <QLabel>
#include "ccb.h"

DLeakAlarmDlg::DLeakAlarmDlg(QWidget *parent) :
    QDialog(parent)
{
    initUI();
}

void DLeakAlarmDlg::initTitleBar()
{
    this->setFixedSize(360, 222);
    this->setWindowFlags(Qt::FramelessWindowHint);
    m_titleBar = new DDialogTitle(this);
    m_titleBar->move(0, 0);
    m_titleBar->setTitleContent(tr("Alarm"), Qt::white, 16);
}

void DLeakAlarmDlg::initUI()
{
    initTitleBar();
    QString text = tr("Leak or Tank Overflow,all valves are closed,system could run after troubleshooting.");
    m_pLabel = new QLabel(text, this);
    m_pLabel->setWordWrap(true);
    m_pLabel->setAlignment(Qt::AlignCenter);
    QPalette palette = m_pLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    m_pLabel->setPalette(palette);

    m_pConfirmBtn = new QPushButton(tr("Confirm"), this);

    m_pLabel->setGeometry(10, 40, 340, 110);
    m_pConfirmBtn->setGeometry(130, 160, 100, 30);

    QString qss = QString("QDialog{\
                          background-color:rgb(23, 78, 158);\
                         }\
                         QPushButton\
                         {\
                            color:white;\
                            background-color:rgb(14 , 150 , 254);\
                            border-radius:5px;\
                         }\
                         QPushButton:hover \
                         {\
                            color:white;\
                            background-color:rgb(44 , 137 , 255);\
                         }\
                         QPushButtonn:pressed\
                         {\
                            color:white;\
                            background-color:rgb(14 , 135 , 228);\
                            padding-left:3px;\
                            padding-top:3px;\
                         }");

    this->setStyleSheet(qss);
    connect(m_pConfirmBtn, SIGNAL(clicked()), this, SLOT(on_pushButton_Reset()));
}

void DLeakAlarmDlg::on_pushButton_Reset()
{
    CCB *pCcb = CCB::getInstance();

    if(pCcb->getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY) ||  pCcb->getLeakState())
    {
        return;
    }

    pCcb->DispCmdEntry(DISP_CMD_LEAK_RESET,NULL,0);
    close();

    gpMainWnd->setLokupState(false);
}
