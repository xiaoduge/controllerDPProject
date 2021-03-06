#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "LockupDlg.h"
#include "mainwindow.h"
#include "exdisplay.h"
#include "ccb.h"

LockupDlg::LockupDlg(QWidget *parent) :
    QDialog(parent)
{
    setFixedSize(300, 200);

    m_pBtnReset  = new QPushButton;
    m_pBtnReset->setText("Reset");

    m_pLUserNameL = new QLabel;
    m_pLUserNameL->setObjectName(QString::fromUtf8("Alarm"));
    m_pLUserNameL->setText("Leak Alarm");

    m_pMainLayout   =  new QGridLayout(this);
    m_pTopLayout    =  new QGridLayout; //QGridLayout
    m_pBottomLayout =  new QHBoxLayout;

    m_pTopLayout->addWidget(m_pLUserNameL,1,1);

    m_pBottomLayout->addWidget(m_pBtnReset);

    m_pMainLayout->addLayout(m_pTopLayout,0,0);
    m_pMainLayout->addLayout(m_pBottomLayout,1,0,1,2);

    setLayout(m_pMainLayout);

    connect(m_pBtnReset,SIGNAL(clicked()),this,SLOT(on_pushButton_Reset()));
}

LockupDlg::~LockupDlg()
{
    if (m_pBtnReset)     delete m_pBtnReset;
    if (m_pLUserNameL)   delete m_pLUserNameL;
    if (m_pTopLayout)    delete m_pTopLayout;
    if (m_pBottomLayout) delete m_pBottomLayout;
    
    if (m_pMainLayout) delete m_pMainLayout;

}

void LockupDlg::on_pushButton_Reset()
{
    CCB *pCcb = CCB::getInstance();
	if(pCcb->getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY) || pCcb->getLeakState())
	{
		return;
	}
	
    pCcb->DispCmdEntry(DISP_CMD_LEAK_RESET,NULL,0);
    close();

    gpMainWnd->setLokupState(false);
}

bool LockupDlg::eventFilter(QObject *watched, QEvent *event)
{

    return QDialog::eventFilter(watched,event);
}


