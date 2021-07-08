#include "displaypage.h"

#include "titlebar.h"

#include "mainwindow.h"

#include <QPainter>

#include <QScrollBar>

#include <QListWidgetItem>

#include <QRect>

#include "notify.h"

DisplayPage::DisplayPage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    creatTitle();

    initUi();

    buildTranslation();
}

void DisplayPage::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void DisplayPage::buildTitles()
{
    QStringList stringList;

    stringList << tr("LCD");

    setTitles(stringList);

}

void DisplayPage::buildTranslation()
{
    int iLoop;

    m_DispNames[0] = tr("Brightness");
    m_DispNames[1] = tr("Energy-saving");

    for( iLoop = 0 ; iLoop < 2 ; iLoop++)
    {
        laName[iLoop]->setText(m_DispNames[iLoop]);
    }  
    
    m_pBtnSave->setTip(tr("Save"),QColor(228, 231, 240),BITMAPBUTTON_TIP_CENTER);
}

void DisplayPage::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}



void DisplayPage::Set_Back_Color()
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


void DisplayPage::initUi()
{
    int iLoop = 0;
    
    QFile qss(":/app/slider.qss");
    qss.open(QFile::ReadOnly);
    QString qsss = QLatin1String (qss.readAll());
    qss.close();

    Set_Back_Color();

    m_iEnergySave = gGlobalParam.MiscParam.iEnerySave;
    m_iBrightness = gGlobalParam.MiscParam.iBrightness;

    for( iLoop = 0 ; iLoop < 2 ; iLoop++)
    {
        m_pBackWidget[iLoop] = new QWidget(m_widget);

        QPalette pal(m_pBackWidget[iLoop]->palette());

        pal.setColor(QPalette::Background, Qt::white);

        m_pBackWidget[iLoop]->setAutoFillBackground(true);
        m_pBackWidget[iLoop]->setPalette(pal);

        m_pBackWidget[iLoop]->setGeometry(QRect(134 , 190 + 70 * iLoop , 530 ,60));

        laName[iLoop]      = new QLabel(m_pBackWidget[iLoop]);
        laName[iLoop]->setPixmap(NULL);
        laName[iLoop]->setGeometry(QRect(25, 15 , 100 , 20));
        laName[iLoop]->setAlignment(Qt::AlignLeft);
        laName[iLoop]->show();
    }


    pSlider[0] = new QSlider(m_pBackWidget[0]);
    pSlider[0]->setOrientation(Qt::Horizontal);
    pSlider[0]->setStyleSheet(qsss);
    pSlider[0]->setGeometry(QRect(130 , 10 ,400,40));
    pSlider[0]->setMinimum(0);
    pSlider[0]->setMaximum(255);
    pSlider[0]->setValue(m_iBrightness);

    connect(pSlider[0], SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));

    m_pBtnEnergySave = new CBitmapButton(m_pBackWidget[1],BITMAPBUTTON_STYLE_SWITCH , BITMAPBUTTON_PIC_STYLE_NORMAL , DISPLAY_BTN_ENERGY_SAVE);

    m_pBtnEnergySave->setButtonPicture(gpGlobalPixmaps[GLOBAL_BMP_SWITCH_OFF]);

    m_pBtnEnergySave->setPressPicture(gpGlobalPixmaps[GLOBAL_BMP_SWITCH_ON]);

    m_pBtnEnergySave->setGeometry(QRect(450 , 15 ,m_pBtnEnergySave->width(),m_pBtnEnergySave->height()));

    m_pBtnEnergySave->setStyleSheet("background-color:transparent");

    connect(m_pBtnEnergySave, SIGNAL(clicked(int)), this, SLOT(on_btn_clicked(int)));
    
    m_pBtnSave = new CBitmapButton(m_widget,BITMAPBUTTON_STYLE_PUSH,BITMAPBUTTON_PIC_STYLE_NORMAL,DISPLAY_BTN_SAVE);
    
    m_pBtnSave->setButtonPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_NORMAL]);
    
    m_pBtnSave->setPressPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_ACTIVE]);
    
    m_pBtnSave->setGeometry(700,560,m_pBtnSave->getPicWidth(),m_pBtnSave->getPicHeight());
    
    m_pBtnSave->setStyleSheet("background-color:transparent");

    connect(m_pBtnSave, SIGNAL(clicked(int)), this, SLOT(on_btn_clicked(int)));
    
    m_pBtnSave->show();

}

void DisplayPage::setValue(int value)
{
    qDebug()<< this << value;

    if (value < 50)
    {
        return;
    }
    
    m_iBrightness = value;
    
    Write_sys_int(PWMLCD_FILE,m_iBrightness);
}

void DisplayPage::save()
{
    DISP_MISC_SETTING_STRU  MiscParam = gGlobalParam.MiscParam;  

    MiscParam.iBrightness = m_iBrightness;
    MiscParam.iEnerySave  = m_iEnergySave;

    MainSaveMiscParam(gGlobalParam.iMachineType,MiscParam);
    
    MainUpdateSpecificParam(NOT_PARAM_MISC_PARAM);
    
    m_wndMain->MainWriteLoginOperationInfo2Db(SETPAGE_SYSTEM_DISPLAY);

    show(false);
    m_parent->show(true);

}

void DisplayPage::on_btn_clicked(int id)
{
   switch(id)
   {
   case DISPLAY_BTN_ENERGY_SAVE:
       {
           m_iEnergySave = BITMAPBUTTON_STATE_UNSEL == m_pBtnEnergySave->getState() ? 0 : 1;
       }
       break;
   case DISPLAY_BTN_SAVE:
       {
           save();
       }
       break;
   }
   
   m_wndMain->prepareKeyStroke();
}

