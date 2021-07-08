#include "dsysmsgpage.h"
#include "exconfig.h"
#include "mainwindow.h"
#include <QListWidgetItem>

DSysMsgPage::DSysMsgPage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    creatTitle();
    initUi();
    buildTranslation();
}

void DSysMsgPage::creatTitle()
{
    CSubPage::creatTitle();
    buildTitles();
    selectTitle(0);
}

void DSysMsgPage::buildTitles()
{
    QStringList stringList;
    stringList << tr("Information");
    setTitles(stringList);

}

void DSysMsgPage::buildTranslation()
{
    m_ExMsgName[SYSMS_PAGE_LB_TITLE] = m_wndMain->machineName();

    m_ExMsgName[SYSMS_PAGE_LB_1] = tr("Catalog No.")       + " :  %1";
    m_ExMsgName[SYSMS_PAGE_LB_2] = tr("Serial No.")        + " :  %1";
    m_ExMsgName[SYSMS_PAGE_LB_3] = tr("Production Date")   + " :  %1";
    m_ExMsgName[SYSMS_PAGE_LB_4] = tr("Installation Date") + " :  %1";
    m_ExMsgName[SYSMS_PAGE_LB_5] = tr("Software Version")  + " :  %1";

    setTitleMsg();
    updateNameMsg();
}

void DSysMsgPage::switchLanguage()
{
    buildTranslation();
    buildTitles();
    selectTitle(titleIndex());
}

void DSysMsgPage::setBackColor()
{
    QSize size(width(),height());
    QImage image_bg = QImage(size, QImage::Format_ARGB32);
    QPainter p(&image_bg);
    p.fillRect(image_bg.rect(), QColor(228, 231, 240));
    QPalette pal(m_widget->palette());
    pal.setBrush(m_widget->backgroundRole(),QBrush(image_bg));
    m_widget->setAutoFillBackground(true);
    m_widget->setPalette(pal);
}

void DSysMsgPage::setTitleMsg()
{
    m_pExmsgLabel[SYSMS_PAGE_LB_TITLE]->setText(m_ExMsgName[SYSMS_PAGE_LB_TITLE]);
}

void DSysMsgPage::setNameMsg(int iIdx,QString strInfo)
{
    QString tmpMsg;

    switch(iIdx)
    {
    case SYSMS_PAGE_LB_1:
        tmpMsg = m_ExMsgName[SYSMS_PAGE_LB_1].arg(strInfo);
        m_pExmsgLabel[SYSMS_PAGE_LB_1]->setText(tmpMsg);
        break;
    case SYSMS_PAGE_LB_2:
        tmpMsg = m_ExMsgName[SYSMS_PAGE_LB_2].arg(strInfo);
        m_pExmsgLabel[SYSMS_PAGE_LB_2]->setText(tmpMsg);
        break;
    case SYSMS_PAGE_LB_3:
        tmpMsg = m_ExMsgName[SYSMS_PAGE_LB_3].arg(strInfo);
        m_pExmsgLabel[SYSMS_PAGE_LB_3]->setText(tmpMsg);
        break;
    case SYSMS_PAGE_LB_4:
        tmpMsg = m_ExMsgName[SYSMS_PAGE_LB_4].arg(strInfo);
        m_pExmsgLabel[SYSMS_PAGE_LB_4]->setText(tmpMsg);
        break;
    case SYSMS_PAGE_LB_5:
        tmpMsg = m_ExMsgName[SYSMS_PAGE_LB_5].arg(strInfo);
        m_pExmsgLabel[SYSMS_PAGE_LB_5]->setText(tmpMsg);
        break;
    default:
        break;
    }
}

void DSysMsgPage::updateNameMsg()
{
    setNameMsg(SYSMS_PAGE_LB_1, gAdditionalCfgParam.productInfo.strCatalogNo);
    setNameMsg(SYSMS_PAGE_LB_2, gAdditionalCfgParam.productInfo.strSerialNo);
    setNameMsg(SYSMS_PAGE_LB_3, gAdditionalCfgParam.productInfo.strProductDate);
    setNameMsg(SYSMS_PAGE_LB_4, gAdditionalCfgParam.productInfo.strInstallDate);
    setNameMsg(SYSMS_PAGE_LB_5, gApp->applicationVersion());
}

void DSysMsgPage::initUi()
{
    QFont fontTitle("Arial", 24, QFont::Bold, true);
    QFont fontMsg("Arial", 20, 0);

    setBackColor();

    QHBoxLayout* hLayout = new QHBoxLayout;
    QVBoxLayout* vLayout = new QVBoxLayout;
    QVBoxLayout* msgLayout = new QVBoxLayout;
    hLayout->setAlignment(Qt::AlignHCenter);
    vLayout->setAlignment(Qt::AlignCenter);
    msgLayout->setAlignment(Qt::AlignCenter);

    m_pExmsgLabel[SYSMS_PAGE_LB_TITLE] = new QLabel;
    m_pExmsgLabel[SYSMS_PAGE_LB_TITLE]->setFont(fontTitle);

	m_pNullLabel = new QLabel(" ");
    m_pNullLabel->setFont(QFont("Arial", 12, 0));

    hLayout->addWidget(m_pExmsgLabel[SYSMS_PAGE_LB_TITLE]); //title;

    for(int iLoop = 1 ; iLoop < SYSMS_PAGE_LB_NUM; iLoop++)
    {
        m_pExmsgLabel[iLoop] = new QLabel;
        m_pExmsgLabel[iLoop]->setFont(fontMsg);
        vLayout->addWidget(m_pExmsgLabel[iLoop]);
    }
	
    msgLayout->addLayout(hLayout);
	msgLayout->addWidget(m_pNullLabel);
    msgLayout->addLayout(vLayout);

    m_widget->setLayout(msgLayout);

    QMargins margins = msgLayout->contentsMargins();
    int bottom = margins.bottom();
    margins.setBottom(bottom*4);
    msgLayout->setContentsMargins(margins);
}

void DSysMsgPage::update()
{
    buildTranslation();
}
