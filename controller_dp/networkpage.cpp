#include "networkpage.h"
#include "mainwindow.h"
#include "cbitmapbutton.h"
#include <QRect>
#include <QProcess>
#include <QListWidget>
#include "dhintdialog.h"
#include "dlineedit.h"
#include "exconfig.h"
#include <QFile>

NetworkPage::NetworkPage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) 
                        : CSubPage(parent,widget,wndMain), m_bRefreshed(false)
{
    m_iNetworkMask = gGlobalParam.MiscParam.iNetworkMask;

    creatTitle();

    initUi();

    buildTranslation();

}

void NetworkPage::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void NetworkPage::buildTitles()
{
    QStringList stringList;

    stringList << tr("Connectivity");

    setTitles(stringList);

}

void NetworkPage::buildTranslation()
{
    int iLoop;
    m_astrNetName[0] = tr("CAN");
    m_astrNetName[1] = tr("Zigbee");
    m_astrNetName[2] = tr("WIFI");
    
    for(iLoop = 0 ; iLoop < DISPLAY_NETWORK_NUM ; iLoop++)
    {
        m_laName[iLoop]->setText(m_astrNetName[iLoop]);
    }
     
    m_pBtnSave->setTip(tr("Save"),QColor(228, 231, 240),BITMAPBUTTON_TIP_CENTER);

    m_pSSIDLab->setText(tr("SSID:"));
    m_pAddSSIDBtn->setText(tr("Add"));
    m_pRefreshWifiBtn->setText(tr("Refresh"));
    m_pAddCheckBox->setText(tr("Add network"));
	
	m_pGetNetInfoBtn->setText(tr("Information"));
}

void NetworkPage::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}

void NetworkPage::setBackColor()
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

void NetworkPage::initUi()
{
    int iLoop;

    m_strQss4Chk = m_wndMain->getQss4Chk();

    setBackColor();

    for(iLoop = 0 ; iLoop < DISPLAY_NETWORK_NUM ; iLoop++)
    {
        m_pBackWidget[iLoop] = new QWidget(m_widget);

        QPalette pal(m_pBackWidget[iLoop]->palette());

        pal.setColor(QPalette::Background, Qt::gray);

        m_pBackWidget[iLoop]->setAutoFillBackground(true);
        m_pBackWidget[iLoop]->setPalette(pal);

//        m_pBackWidget[iLoop]->setGeometry(QRect(124 , 160 + 70 * iLoop , 530 ,60));
        m_pBackWidget[iLoop]->setGeometry(QRect(124 , 100 + 80 * (iLoop - 1) , 530 ,60));

        m_laName[iLoop]      = new QLabel(m_pBackWidget[iLoop]);
        m_laName[iLoop]->setPixmap(NULL);
        m_laName[iLoop]->setGeometry(QRect(25, 30 , 180 , 20));
        m_laName[iLoop]->show();

        m_chkSwitchs[iLoop] = new QCheckBox(m_pBackWidget[iLoop]);

        m_chkSwitchs[iLoop]->setGeometry(QRect(480 , 9 ,40,40));

        m_chkSwitchs[iLoop]->setStyleSheet(m_strQss4Chk);

        m_chkSwitchs[iLoop]->show();

        if (m_iNetworkMask & (1 << iLoop))
        {
            m_chkSwitchs[iLoop]->setChecked(true);
        }
        else
        {
            m_chkSwitchs[iLoop]->setChecked(false);
        }

        connect(m_chkSwitchs[iLoop], SIGNAL(stateChanged(int)), this, SLOT(on_checkBox_changeState(int)));

        //2019.3.14 add
        if(DISPLAY_NETWORK_CAN == iLoop)
        {
            m_pBackWidget[iLoop]->hide();
        }
		if(gAdditionalCfgParam.machineInfo.iMachineFlow >= 500)
		{
		    if(DISPLAY_NETWORK_ZIGBEE == iLoop)
        	{
            	m_pBackWidget[iLoop]->hide();
        	}
			
		}
    }

    //add for wifi config
    m_pWifiConfigWidget = new QWidget(m_widget);
    QPalette pal(m_pWifiConfigWidget->palette());
    pal.setColor(QPalette::Background, Qt::gray);
    m_pWifiConfigWidget->setAutoFillBackground(true);
    m_pWifiConfigWidget->setPalette(pal);
    m_pWifiConfigWidget->setGeometry(QRect(124 , 245 , 530 ,242));

    m_pWifiMsgListWidget = new QListWidget(m_pWifiConfigWidget);
    m_pWifiMsgListWidget->setGeometry(5, 5, 520, 200);

    m_pRefreshWifiBtn = new QPushButton(m_pWifiConfigWidget);
    m_pRefreshWifiBtn->setGeometry(210, 208, 100, 30);

	m_pGetNetInfoBtn = new QPushButton(m_pWifiConfigWidget);
	m_pGetNetInfoBtn->setGeometry(50, 208, 100, 30);

    m_pAddCheckBox = new QCheckBox(m_pWifiConfigWidget);
    m_pAddCheckBox->setGeometry(390, 208, 120, 30);
    connect(m_pAddCheckBox, SIGNAL(stateChanged(int)), this, SLOT(on_addCheckBox_stateChanged(int)));

    QString strQss4Chk = m_wndMain->getQss4Chk();
    m_pAddCheckBox->setStyleSheet(strQss4Chk);

    m_pWifiSSIDAddWidget = new QWidget(m_widget);
    pal = m_pWifiSSIDAddWidget->palette();
    pal.setColor(QPalette::Background, Qt::gray);
    m_pWifiSSIDAddWidget->setAutoFillBackground(true);
    m_pWifiSSIDAddWidget->setPalette(pal);
    m_pWifiSSIDAddWidget->setGeometry(QRect(124 , 495, 530 ,50));

    m_pSSIDLab = new QLabel(m_pWifiSSIDAddWidget);
    m_pSSIDLab->setGeometry(QRect(5, 10 , 50 , 30));

    m_pSSIDEdit = new DLineEdit(m_pWifiSSIDAddWidget);
    m_pSSIDEdit->setGeometry(QRect(60, 10 , 190 , 30));

    m_pAddSSIDBtn = new QPushButton(m_pWifiSSIDAddWidget);
    m_pAddSSIDBtn->setGeometry(400, 10, 100, 30);

    connect(m_pAddSSIDBtn, SIGNAL(clicked()), this, SLOT(on_addSSIDBtn_clicked()));
    connect(m_pRefreshWifiBtn, SIGNAL(clicked()), this, SLOT(on_wifiRefreshBtn_clicked()));
	
    connect(m_pWifiMsgListWidget, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(on_wifiListWidget_itemClicked(QListWidgetItem*)));

	connect(m_pGetNetInfoBtn, SIGNAL(clicked()), this, SLOT(on_getNetInfoBtn_clicked()));

    m_pProcess = new QProcess(this);
    connect(m_pProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_refreshWifiMsg()));

	m_pGetInfoProcess = new QProcess(this);
	connect(m_pGetInfoProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_getNetInfo()));
	
    if(!(Qt::Checked == m_chkSwitchs[DISPLAY_NETWORK_WIFI]->checkState()))
    {
        m_pWifiConfigWidget->hide();
        m_pWifiSSIDAddWidget->hide();
    }

    if(!(Qt::Checked == m_pAddCheckBox->checkState()))
    {
        m_pWifiSSIDAddWidget->hide();
    }

    //end
    m_pBtnSave = new CBitmapButton(m_widget,BITMAPBUTTON_STYLE_PUSH,BITMAPBUTTON_PIC_STYLE_NORMAL,NETWORKPAGE_BTN_SAVE);
    
    m_pBtnSave->setButtonPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_NORMAL]);
    
    m_pBtnSave->setPressPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_ACTIVE]);
    
    m_pBtnSave->setGeometry(700,560,m_pBtnSave->getPicWidth(),m_pBtnSave->getPicHeight());
    
    m_pBtnSave->setStyleSheet("background-color:transparent");
    
    connect(m_pBtnSave, SIGNAL(clicked(int)), this, SLOT(on_btn_clicked(int)));
    
    m_pBtnSave->show();

}

void NetworkPage::save()
{
    if (m_iNetworkMask != gGlobalParam.MiscParam.iNetworkMask)
    {
       DISP_MISC_SETTING_STRU  MiscParam = gGlobalParam.MiscParam;  
   
       MiscParam.iNetworkMask = m_iNetworkMask;

       MainSaveMiscParam(gGlobalParam.iMachineType,MiscParam);

       MainUpdateSpecificParam(NOT_PARAM_MISC_PARAM);
       
    }
    
    m_wndMain->MainWriteLoginOperationInfo2Db(SETPAGE_SYSTEM_NETWORK);
    
    show(false);
    m_parent->show(true);

}

void NetworkPage::on_btn_clicked(int index)
{

   switch(index)
   {
   case NETWORKPAGE_BTN_SAVE:
       save();
       break;
   default:
       break;
   }
   
   m_wndMain->prepareKeyStroke();
}

void NetworkPage::on_checkBox_changeState(int state)
{
    (void)state;
    int iLoop;
    
    QCheckBox *pChkBox = (QCheckBox *)this->sender();

    if (!pChkBox)
    {
        return ;
    }
    
    //int tmp = (Qt::Checked == pChkBox->checkState()) ? 1 : 0;


    for(iLoop = 0 ; iLoop < DISPLAY_NETWORK_NUM ; iLoop++)
    {
       if ((Qt::Checked == m_chkSwitchs[iLoop]->checkState()))
       {
           m_iNetworkMask |= 1 << iLoop;
       }
       else
       {
           m_iNetworkMask &= ~(1 << iLoop);
       }
    }

    if(Qt::Checked == m_chkSwitchs[DISPLAY_NETWORK_WIFI]->checkState())
    {
        m_pWifiConfigWidget->show();
    }
    else
    {
        m_pWifiConfigWidget->hide();
        m_pWifiSSIDAddWidget->hide();
    }
}

void NetworkPage::on_addSSIDBtn_clicked()
{
    QString strSSID = QString("ESSID:\"%1\"").arg(m_pSSIDEdit->text());
    m_pWifiMsgListWidget->addItem(strSSID);
}

void NetworkPage::on_wifiRefreshBtn_clicked()
{
    m_pProcess->start("iwlist wlan0 scanning");
    m_pWifiMsgListWidget->clear();
    m_pWifiMsgListWidget->addItem("Searching");
}

void NetworkPage::on_getNetInfoBtn_clicked()
{
	m_pGetInfoProcess->start("ifconfig");
    m_pWifiMsgListWidget->clear();
    m_pWifiMsgListWidget->addItem("Searching");
}

void NetworkPage::on_refreshWifiMsg()
{
    QString strAll = m_pProcess->readAllStandardOutput();

    QRegExp ssidRx("ESSID:\"([^\"]*)\"");
    QStringList ssidList;
    int pos = 0;
    while((pos = ssidRx.indexIn(strAll, pos)) != -1)
    {
        QString strSSID = ssidRx.cap(0);
        ssidList << strSSID;
        pos += ssidRx.matchedLength();
    }

    m_pWifiMsgListWidget->clear();
    QSet<QString> ssidSet = ssidList.toSet();
    QSet<QString>::const_iterator it;
    for(it = ssidSet.begin(); it != ssidSet.end(); ++it)
    {
        m_pWifiMsgListWidget->addItem(*it);
    }
	m_bRefreshed = true;
}

void NetworkPage::on_getNetInfo()
{
    m_pWifiMsgListWidget->clear();

    //???????????????????????????
    QString strFileName(WIFICONFIG_FILE);
    if(strFileName.isEmpty())
    {
        return;
    }

    QFile sourceFile(strFileName);
    if(!sourceFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Open wifi config file failed: 1";
        return;
    }
    QString strWifiCfg = sourceFile.readAll();
    QRegExp rxSSID("ssid=\"([^\"]*)\"");

    int iPos = rxSSID.indexIn(strWifiCfg);
    if(iPos > -1)
    {
        m_pWifiMsgListWidget->addItem(rxSSID.cap(0));

        if(sourceFile.isOpen())
        {
            sourceFile.close();
        }
    }
    else
    {
        m_pWifiMsgListWidget->addItem(tr("No internet connection"));
        if(sourceFile.isOpen())
        {
            sourceFile.close();
        }
        return;
    }
    //??????IP??????
    QString strAll = m_pGetInfoProcess->readAllStandardOutput();
	int index = strAll.indexOf("wlan0");
	QString strDest = strAll.mid(index);

    QRegExp rxIP("inet addr:[0-9]{1,3}[\.][0-9]{1,3}[\.][0-9]{1,3}[\.][0-9]{1,3}");
    int pos = rxIP.indexIn(strDest);
    if(pos > -1)
    {
        m_pWifiMsgListWidget->addItem(rxIP.cap(0));
    }
    else
    {
        m_pWifiMsgListWidget->addItem(tr("No internet connection"));
    }
	m_bRefreshed = false;
}

void NetworkPage::on_wifiListWidget_itemClicked(QListWidgetItem *item)
{
    //
    QString strName = item->text().remove("ESSID:").remove("\"");
    if(m_bRefreshed)
    {
    	m_wndMain->showWifiConfigDlg(strName);
    }
}

void NetworkPage::on_addCheckBox_stateChanged(int state)
{
    if(state == Qt::Checked)
    {
        m_pSSIDEdit->clear();
        m_pWifiSSIDAddWidget->show();
    }
    else
    {
        m_pWifiSSIDAddWidget->hide();
    }
}

void NetworkPage::leaveSubPage()
{    
    CSubPage::leaveSubPage();
}

void NetworkPage::update()
{
    m_pAddCheckBox->setChecked(false);

    if(gGlobalParam.MiscParam.iNetworkMask & 1 << DISPLAY_NETWORK_ZIGBEE)
    {
        m_chkSwitchs[DISPLAY_NETWORK_ZIGBEE]->setChecked(true);
    }
    else
    {
        m_chkSwitchs[DISPLAY_NETWORK_ZIGBEE]->setChecked(false);
    }

    if(gGlobalParam.MiscParam.iNetworkMask & 1 << DISPLAY_NETWORK_WIFI)
    {
        m_chkSwitchs[DISPLAY_NETWORK_WIFI]->setChecked(true);
        m_pWifiConfigWidget->show();

    }
    else
    {
        m_chkSwitchs[DISPLAY_NETWORK_WIFI]->setChecked(false);
        m_pWifiConfigWidget->hide();
        m_pWifiSSIDAddWidget->hide();
    }
}


