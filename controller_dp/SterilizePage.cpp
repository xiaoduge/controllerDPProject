#include "SterilizePage.h"
#include "cbitmapbutton.h"
#include "mainwindow.h"
#include "ToastDlg.h"
#include "exconfig.h"
#include "exdisplay.h"
#include "drunwarningdialog.h"
#include <QProgressBar>
#include <QMessageBox>
#include "dwarningdlg.h"

#define ControlNum 6

#define BACKWIDGET_START_Y      80
#define BACKWIDGET_START_HIATUS 160   //120
#define BACKWIDGET_HEIGHT       120   //80
#define BACKWIDGET_WIDTH        700

QRect   QrectAry[ControlNum] = 
{
    QRect(0,  10,  41 , 37) ,
    QRect(50, 20, 130 , 18) ,
    QRect(190, 20, 340 , 15) ,
    QRect(50, 60, 280 , 15) , //QRect(60, 50, 190 , 15) ,
    QRect(50, 85, 280 , 15) ,  //325->300
    QRect(600, 22, 150 , 15) 
};


static CONFIG_BTN cleanBtn[1] =
{
    {149,340,":/pic/BackNo.png",":/pic/BackNo.png",BITMAPBUTTON_STYLE_PUSH,BITMAPBUTTON_PIC_STYLE_SINGLE,},
};

SterilizePage::SterilizePage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    creatTitle();

    initUi();

    buildTranslation();

}

void SterilizePage::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void SterilizePage::buildTitles()
{
    QStringList stringList;

    stringList << tr("Cleaning & Sanitization");

    setTitles(stringList);

}

void SterilizePage::startCleanProgress(int iType)
{
    m_pCleanProBar->show();
    m_curProgreeValue = 0;
    switch(iType)
    {
    case APP_CLEAN_RO:
        m_pCleanProBar->setRange(0, m_iWashDuration[APP_CLEAN_RO]);
        break;
    case APP_CLEAN_PH:
        m_pCleanProBar->setRange(0, m_iWashDuration[APP_CLEAN_PH]);
        break;
    default:
        break;
    }
    m_pCleanProBar->setValue(m_curProgreeValue);
    m_cleanTimerId = startTimer(1000);
}

void SterilizePage::buildTranslation()
{
    int iLoop;

    m_aSterilize[APP_CLEAN_RO].laName->setText(tr("RO CL Cleaning"));
    m_aSterilize[APP_CLEAN_PH].laName->setText(tr("RO pH Cleaning"));

    for (iLoop = 0; iLoop < (APP_CLEAN_NUM - 1); iLoop++)
    {
        QString strTitle = tr("Last Maintenance:");
        time_t tmInfo ;

        QDateTime lstDateTime;
        QString strLstDateTime;


        tmInfo = gGlobalParam.CleanParam.aCleans[iLoop].lstTime;
        lstDateTime = QDateTime::fromTime_t(tmInfo);
        strLstDateTime = lstDateTime.toString("d/M/yyyy"); //hh:mm:ss
        strTitle = strTitle + " " + strLstDateTime;

        m_aSterilize[iLoop].laMtTime->setText(strTitle);

        if(APP_CLEAN_PH != iLoop)
        {
            strTitle = tr("Next Due Date:");
    
            tmInfo = gGlobalParam.CleanParam.aCleans[iLoop].period;

            lstDateTime = QDateTime::fromTime_t(tmInfo);
            strLstDateTime = lstDateTime.toString("d/M/yyyy"); //hh:mm:ss
            strTitle = strTitle + " " + strLstDateTime;

            m_aSterilize[iLoop].laNextMtTime->setText(strTitle);
        }

        m_aSterilize[iLoop].laPrompt->setText(tr("Install Case First"));
        
        m_aSterilize[iLoop].btnClean->setText(tr("Cleaning"),BITMAPBUTTON_STATE_SEL);
        m_aSterilize[iLoop].btnClean->setText(tr("Clean"),BITMAPBUTTON_STATE_UNSEL);
        m_aSterilize[iLoop].btnClean->setColor(QColor(0x00,0x99,0xfa),BITMAPBUTTON_STATE_SEL);
        m_aSterilize[iLoop].btnClean->setColor(QColor(255,255,255),BITMAPBUTTON_STATE_UNSEL);
        
    }

}

void SterilizePage::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}

void SterilizePage::setBackColor()
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

void SterilizePage::leaveSubPage()
{
    CSubPage::leaveSubPage();
}

void SterilizePage::createControl()
{
    
    QPixmap lbimage;
    lbimage.load(":/pic/BackNo.png");

    QPixmap indimage;
    indimage.load(":/pic/install_cleaning_casette_frame.png");

    QFont ft12 = *m_wndMain->getFont(GLOBAL_FONT_12);
    //ft.setPointSize(12);
    //ui.label_4->setFont(ft);
    
    QPalette paLightblue;
    paLightblue.setColor(QPalette::WindowText,QColor(0x00,0x99,0xfa));

    QPalette paLightgray;
    paLightgray.setColor(QPalette::WindowText,QColor(100,139,116));
     
    //ui.label_4->setPalette(paLightblue);        
    QRect rectTmp;
    
    for(int iLoop = 0 ; iLoop < (APP_CLEAN_NUM - 1); iLoop++)
    {
        m_aSterilize[iLoop].BackWidget = new QWidget(m_widget);

        QPalette pal(m_aSterilize[iLoop].BackWidget->palette());
        pal.setColor(QPalette::Background, QColor(255,255,255));

        m_aSterilize[iLoop].BackWidget->setAutoFillBackground(true);
        m_aSterilize[iLoop].BackWidget->setPalette(pal);

        m_aSterilize[iLoop].BackWidget->setGeometry(QRect(50 , BACKWIDGET_START_Y + BACKWIDGET_START_HIATUS * iLoop, BACKWIDGET_WIDTH ,BACKWIDGET_HEIGHT));

        m_aSterilize[iLoop].laBackNo     = new QLabel(m_aSterilize[iLoop].BackWidget);
        m_aSterilize[iLoop].laBackNo->setGeometry(QrectAry[0]);
        m_aSterilize[iLoop].laBackNo->setPixmap(lbimage);
        m_aSterilize[iLoop].laBackNo->show();

        m_aSterilize[iLoop].laName       = new QLabel(m_aSterilize[iLoop].BackWidget);
        m_aSterilize[iLoop].laName->setGeometry(QrectAry[1]);
        m_aSterilize[iLoop].laName->show();
        
        m_aSterilize[iLoop].laPromptFrame     = new QLabel(m_aSterilize[iLoop].BackWidget);
        rectTmp = QrectAry[2];
        rectTmp.setWidth(indimage.width());
        rectTmp.setHeight(indimage.height());
        m_aSterilize[iLoop].laPromptFrame->setGeometry(rectTmp);
//        m_aSterilize[iLoop].laPromptFrame->setPixmap(indimage);
        m_aSterilize[iLoop].laPromptFrame->show();
        m_aSterilize[iLoop].laPromptFrame->setStyleSheet("background-color:transparent");
 
        m_aSterilize[iLoop].laPrompt     = new QLabel(m_aSterilize[iLoop].BackWidget);

        m_aSterilize[iLoop].laPrompt->adjustSize();
        m_aSterilize[iLoop].laPrompt->setGeometry(QRect(190, 15, 340, 15*2));
        m_aSterilize[iLoop].laPrompt->setWordWrap(true); //
        m_aSterilize[iLoop].laPrompt->setAlignment(Qt::AlignHCenter);

        if( 500 <= gAdditionalCfgParam.machineInfo.iMachineFlow)
        {
            m_aSterilize[iLoop].laPrompt->hide();
        }
        else
        {
            m_aSterilize[iLoop].laPrompt->show();
        }
        
        m_aSterilize[iLoop].laPrompt->setFont(ft12);
        m_aSterilize[iLoop].laPrompt->setPalette(paLightblue);
        m_aSterilize[iLoop].laPrompt->setStyleSheet("background-color:transparent");


        m_aSterilize[iLoop].laMtTime     = new QLabel(m_aSterilize[iLoop].BackWidget);
        m_aSterilize[iLoop].laMtTime->setGeometry(QrectAry[3]);
        m_aSterilize[iLoop].laMtTime->show();
        m_aSterilize[iLoop].laMtTime->setStyleSheet("background-color:transparent");
        m_aSterilize[iLoop].laMtTime->setPalette(paLightgray);
        
        m_aSterilize[iLoop].laNextMtTime = new QLabel(m_aSterilize[iLoop].BackWidget);
        m_aSterilize[iLoop].laNextMtTime->setGeometry(QrectAry[4]);
        m_aSterilize[iLoop].laNextMtTime->show();
        m_aSterilize[iLoop].laNextMtTime->setStyleSheet("background-color:transparent");
        m_aSterilize[iLoop].laNextMtTime->setPalette(paLightgray);

        m_aSterilize[iLoop].btnClean = new CBitmapButton(m_aSterilize[iLoop].BackWidget,BITMAPBUTTON_STYLE_SWITCH,BITMAPBUTTON_PIC_STYLE_SINGLE,iLoop);
        m_aSterilize[iLoop].btnClean->setButtonPicture(m_wndMain->getBitmap(GLOBAL_BMP_PAGE_NAVI_NORMAL));
        m_aSterilize[iLoop].btnClean->setGeometry(600,20,m_aSterilize[iLoop].btnClean->getPicWidth(),m_aSterilize[iLoop].btnClean->getPicHeight());
        m_aSterilize[iLoop].btnClean->setStyleSheet("background-color:transparent");
        m_aSterilize[iLoop].btnClean->show();

        connect(m_aSterilize[iLoop].btnClean, SIGNAL(clicked(int)), this, SLOT(on_btn_clicked(int)));
       
    }

    // NOTE: 新增加进度条
    m_iWashDuration[APP_CLEAN_RO] = gROWashDuration[ROClWash_FirstStep] + gROWashDuration[ROClWash_SecondStep];
    m_iWashDuration[APP_CLEAN_RO] /= 1000;  //单位秒
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        m_iWashDuration[APP_CLEAN_PH] = gROWashDuration[ROPHWash_FirstStep_L] + gROWashDuration[ROPHWash_SecondStep_L] 
                                       + gROWashDuration[ROPHWash_ThirdStep_L];
        break;
    default:
        m_iWashDuration[APP_CLEAN_PH] = gROWashDuration[ROPHWash_FirstStep] + gROWashDuration[ROPHWash_SecondStep] 
                                       + gROWashDuration[ROPHWash_ThirdStep] + gROWashDuration[ROPHWash_FourthStep];
        break;
    }
    m_iWashDuration[APP_CLEAN_PH] /= 1000; //单位秒

    m_pCleanProBar = new QProgressBar(m_widget);
    m_pCleanProBar->setGeometry(100, 420, 600, 40);
    m_pCleanProBar->hide();
}

void SterilizePage::updateRunInfo(int index,bool bRun)
{
    Q_UNUSED(bRun);
    m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);

    m_pCleanProBar->hide();
    killTimer(m_cleanTimerId);
}

void SterilizePage::initUi()
{
    setBackColor();

    createControl();
}

void SterilizePage::update()
{
   m_wndMain->setWorkMode(APP_WORK_MODE_CLEAN);
   buildTranslation();
}

void SterilizePage::fade()
{
    m_wndMain->setWorkMode(APP_WORK_MODE_NORMAL);
}

void SterilizePage::updateRfidInfo(int iRfId)
{
   
   /* check clean package */
   if (!m_wndMain->getRfidState(iRfId))
   {
   
       ToastDlg::makeToast(tr("Detergent Casette Removed!"));
       return; 
   }

   ToastDlg::makeToast(tr("Detergent Casette Inserted!"));

   return; 
}

void SterilizePage::on_btn_clicked(int index)
{
    int state = m_aSterilize[index].btnClean->getState();

    switch(index)
    {
    case APP_CLEAN_RO:
    case APP_CLEAN_PH:
    //case APP_CLEAN_SYSTEM:
        {
            DISPHANDLE hdl = DISP_INVALID_HANDLE;
            if (BITMAPBUTTON_STATE_SEL == state)
            {
                //开启RO清洗前判断当前状态是否可以进行RO清洗
                bool bError = false;
                if (m_wndMain->m_pCcb->DispGetROWashFlag())
                {           
                    DWarningDlg dlg(tr("RO cleaning is in process. Please put system to Operate after the cleaning."));
                    dlg.exec();
                    bError = true;
                }
                if(m_wndMain->m_pCcb->getKeyState() & (1 << APP_EXE_DIN_RF_KEY))
                {
                    DWarningDlg dlg(tr("Pretreatment backwash is in progress. System will start automatically afterwords."));
                    dlg.exec();
                    bError = true;
                }
                if(m_wndMain->m_pCcb->getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY))
                {
                    DWarningDlg dlg(tr("Water tank overflow or system leakage detected. Please fix the issue and re-start the starem."));
                    dlg.exec();
                    bError = true;
                }
                if( m_wndMain->m_pCcb->getLeakState())
                {
                    DWarningDlg dlg(tr("system leakage detected. Please fix the issue and re-start the starem."));
                    dlg.exec();
                    bError = true;
                }
                if (!m_wndMain->getActiveExeBrds())
                {
                    DWarningDlg dlg(tr("System communication is abnormal. Please restart the device or contact a service engineer."));
                    dlg.exec();
                    bError = true;
                }
                if(bError)
                {
                    m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);
                    return ;
                }
                
                switch(gGlobalParam.iMachineType)
                {
                case MACHINE_L_EDI_LOOP:
                case MACHINE_L_RO_LOOP:
                    if(gAdditionalCfgParam.machineInfo.iMachineFlow < 500)
                    {
                        QMessageBox::StandardButton rb = QMessageBox::question(NULL, tr("Clean"), tr("Please Insert Detergent Casette!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No); 
                        if(rb != QMessageBox::Yes) 
                        { 
                            m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);
                            return ;
                        }
                    }
                    break;
                default:
                    QMessageBox::StandardButton rb = QMessageBox::question(NULL, tr("Clean"), tr("Please Insert Detergent Casette!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No); 
                    if(rb != QMessageBox::Yes) 
                    { 
                        m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);
                        return ;
                    }
                    break;
                }
    
                /* check clean package */
                if (!(gGlobalParam.MiscParam.ulMisFlags & (1 << DISP_SM_RFID_Authorization)))
                {
                    int iRet = m_wndMain->getActiveRfidBrds4Cleaning();

                    if (iRet <= 0)
                    {
                        switch(-iRet)
                        {
                        case DISP_PRE_PACK:
                             QMessageBox::about(NULL, tr("About"), tr("No Pre-PACK detected!"));
                             break;
                        case DISP_AC_PACK:
                             QMessageBox::about(NULL, tr("About"), tr("No AC-PACK detected!"));
                             break;
                        case (DISP_P_PACK | (1 << 16)):
                             QMessageBox::about(NULL, tr("About"), tr("No Clean-PACK detected!"));
                             break;
                        case DISP_U_PACK:
                             QMessageBox::about(NULL, tr("About"), tr("No U-PACK detected!"));
                             break;
                        case DISP_AT_PACK:
                             QMessageBox::about(NULL, tr("About"), tr("No AT-PACK detected!"));
                             break;
                        case DISP_H_PACK:
                             QMessageBox::about(NULL, tr("About"), tr("No H-PACK detected!"));
                             break;
                        }

                        m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);
                        return; 
                    }
                }
            }
            if(state == BITMAPBUTTON_STATE_SEL)
            {
                hdl = m_wndMain->startClean(index, true);
            }
            else
            {
                DRunWarningDialog runDlg(tr("Residual chlorine and pH wash agents left may damage the system. Are you sure to abort?"));
                if(QDialog::Accepted != runDlg.exec())
                {
                    m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_SEL);
                    return;
                }
                hdl = m_wndMain->startClean(index, false);
            }
            //hdl = m_wndMain->startClean(index,(state == BITMAPBUTTON_STATE_SEL));

            gGlobalParam.CleanParam.aCleans[index].lstTime = QDateTime::currentDateTime().toTime_t();
            gGlobalParam.CleanParam.aCleans[index].period = QDateTime::currentDateTime().addDays(gGlobalParam.CMParam.aulCms[DISP_ROC12LIFEDAY]).toTime_t();

            MainSaveCleanParam(gGlobalParam.iMachineType, gGlobalParam.CleanParam);

            if (DISP_INVALID_HANDLE == hdl)
            {
                m_aSterilize[index].btnClean->setState(BITMAPBUTTON_STATE_UNSEL);
            }
            else
            {
                startCleanProgress(index);
            }
        }
        break;
    default:
        break;
    }
    m_wndMain->prepareKeyStroke();

}

void SterilizePage::timerEvent(QTimerEvent *event)
{
    if(m_wndMain->m_pCcb->DispGetRoWashPauseFlag())
    {
        return;
    }
    if(event->timerId() == m_cleanTimerId)
    {
        ++m_curProgreeValue;
        m_pCleanProBar->setValue(m_curProgreeValue);
    }
}

