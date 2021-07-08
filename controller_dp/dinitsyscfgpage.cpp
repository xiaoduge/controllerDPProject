#include "dinitsyscfgpage.h"
#include "mainwindow.h"
#include "dcheckboxlistitemwidget.h"
#include "dinittankcfgwidget.h"
#include <QMouseEvent>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QListWidget>

DInitSyscfgpage::DInitSyscfgpage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    creatTitle();
    initUi();
    buildTranslation();
    this->hideTitleBar();
}

void DInitSyscfgpage::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void DInitSyscfgpage::buildTitles()
{
    QStringList stringList;

    stringList << tr("System Config");

    setTitles(stringList);

}

void DInitSyscfgpage::buildTranslation()
{
    m_pExLbTitle->setText(tr("Configuration"));

    for(int i = 0; i < m_cfgMap.size(); ++i)
    {
        int code = m_cfgMap.value(i);
        QListWidgetItem *item = m_pListWidget->item(i);
        DCheckBoxListItemWidget* itemWidget = qobject_cast<DCheckBoxListItemWidget*>(m_pListWidget->itemWidget(item));
        switch(code)
        {
        case DISP_SM_Printer:
            itemWidget->setTag(tr("Printer"));
            break;
        case DISP_SM_SW_PUMP:
            itemWidget->setTag(tr("Feed PUMP"));
            break;
        case DISP_SM_TankUV:
            itemWidget->setTag(tr("Tank UV"));
            break;
        case DISP_SM_TubeUV:
            itemWidget->setTag(tr("Loop UV"));
            break;
        case DISP_SM_TubeDI:
            itemWidget->setTag(tr("Loop DI"));
            break;
        case DISP_SM_HaveTubeFilter:
            itemWidget->setTag(tr("Loop Filter"));
            break;
        default:
            break;
        }

    }

    m_pExNextBtn->setText(tr("Next"));
    m_pExBackBtn->setText(tr("Back"));
}

void DInitSyscfgpage::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}

void DInitSyscfgpage::setBackColor()
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

void DInitSyscfgpage::createControl()
{
    QColor colors[] = {QColor(200,200,188),QColor(228, 231, 240)};

    m_pListWidget = new QListWidget(m_widget);

    m_pListWidget->setStyleSheet("background-color:transparent");
    m_pListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_pListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pListWidget->setFrameShape(QListWidget::NoFrame);

    m_pListWidget->setGeometry(QRect(50, 135, 640, 300));

    for(int iLoop = 0; iLoop < m_cfgMap.size(); ++iLoop)
    {
        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(QSize(600 , 40));
        item->setBackground(colors[iLoop % 2]);

        DCheckBoxListItemWidget* itemWidget = new DCheckBoxListItemWidget;
        itemWidget->setCheckBoxEnabled(true);
        itemWidget->setID(m_cfgMap.value(iLoop));

        m_pListWidget->addItem(item);
        m_pListWidget->setItemWidget(item, itemWidget);
    }

    m_pExNextBtn = new QPushButton(m_widget);
    m_pExBackBtn = new QPushButton(m_widget);

    connect(m_pExNextBtn, SIGNAL(clicked()), this, SLOT(on_ExNextBtn_clicked()));
    connect(m_pExBackBtn, SIGNAL(clicked()), this, SLOT(on_ExBackBtn_clicked()));

    m_pExBackBtn->move(200, 470);
    m_pExNextBtn->move(500, 470);
}

void DInitSyscfgpage::initUi()
{
    setBackColor();
    createHeader();
    initCfgMap();
    createControl();
}

void DInitSyscfgpage::update()
{
    connectData();
}

void DInitSyscfgpage::createHeader()
{
    m_pExLbTitle = new QLabel(m_widget);
    m_pExLbTitle->setGeometry(QRect(50, 80, 220, 28));
    m_pExLbTitle->setStyleSheet(" font-size:24pt;color:#000000;font-family:Arial;QFont::Bold");
}

void DInitSyscfgpage::mousePressEvent(QMouseEvent *e)
{
    if (!m_lstFlag)
    {
        m_lstX = e->x();
        m_lstY = e->y();
        m_curX = e->x();
        m_curY = e->y();
        m_lstFlag = 1;
    }
}

void DInitSyscfgpage::mouseMoveEvent(QMouseEvent *e)
{
    if (0 == e->x()
        && 0 == e->y())
    {
       return;
    }

    m_curX = e->x();
    m_curY = e->y();
}

void DInitSyscfgpage::mouseReleaseEvent(QMouseEvent *e)
{
    if (abs(m_curX - m_lstX) >= PAGE_X_DIMENSION
        && abs(m_curY - m_lstY) <= PAGE_Y_DIMENSION)
    {
        save();
        m_wndMain->naviInitPage(Ex_Init_Syscfg, m_curX - m_lstX > 0 ? 1 : 0);
    }
    m_lstFlag = 0;
}


void DInitSyscfgpage::connectData()
{
    for(int i = 0; i < m_cfgMap.size(); ++i)
    {
        int code = m_cfgMap.value(i);
        QListWidgetItem *item = m_pListWidget->item(i);
        DCheckBoxListItemWidget* itemWidget = qobject_cast<DCheckBoxListItemWidget*>(m_pListWidget->itemWidget(item));
        switch (code)
        {
        case DISP_SM_SW_PUMP:
            if(gGlobalParam.MiscParam.ulMisFlags & (1 << code))
            {
                itemWidget->setCheckBoxState(true);
            }
            else
            {
                itemWidget->setCheckBoxState(false);
            }
            break;
        default:
            if(gGlobalParam.SubModSetting.ulFlags & (1 << code))
            {
                itemWidget->setCheckBoxState(true);
            }
            else
            {
                itemWidget->setCheckBoxState(false);
            }
            break;
        }
    }
}

void DInitSyscfgpage::save()
{
    DISP_SUB_MODULE_SETTING_STRU  smParam = gGlobalParam.SubModSetting;
    DISP_MISC_SETTING_STRU        miscParam = gGlobalParam.MiscParam;

    for(int i = 0; i < m_cfgMap.size(); ++i)
    {
        int code = m_cfgMap.value(i);
        QListWidgetItem *item = m_pListWidget->item(i);
        DCheckBoxListItemWidget* itemWidget = qobject_cast<DCheckBoxListItemWidget*>(m_pListWidget->itemWidget(item));
        switch (code)
        {
        case DISP_SM_SW_PUMP:
            if(Qt::Checked == itemWidget->checkBoxState())
            {
                miscParam.ulMisFlags |= 1 << code;
            }
            else
            {
                miscParam.ulMisFlags &= ~(1 << code);
            } 
            break;
        default:
            if(Qt::Checked == itemWidget->checkBoxState())
            {
                smParam.ulFlags |= 1 << code;
            }
            else
            {
                smParam.ulFlags &= ~(1 << code);
            }
            break;
        }

    }

    MainSaveSubModuleSetting(gGlobalParam.iMachineType, smParam);
    MainSaveMiscParam(gGlobalParam.iMachineType, miscParam);

    MainUpdateGlobalParam();
}

void DInitSyscfgpage::on_ExNextBtn_clicked()
{
    save();
    m_wndMain->naviInitPage(Ex_Init_Syscfg,  0);
    m_wndMain->prepareKeyStroke();
}

void DInitSyscfgpage::on_ExBackBtn_clicked()
{
    m_wndMain->naviInitPage(Ex_Init_Syscfg, 1);
    m_wndMain->prepareKeyStroke();
}

void DInitSyscfgpage::initCfgMap()
{
    int ikey = 0;
    m_cfgMap.insert(ikey, DISP_SM_Printer);
    
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_ADAPT:
        break;
    default:
        if(gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB2))
        {
            ++ikey;
            m_cfgMap.insert(ikey, DISP_SM_TankUV);
        }
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        ++ikey;
        m_cfgMap.insert(ikey, DISP_SM_SW_PUMP);
        ++ikey;
        m_cfgMap.insert(ikey, DISP_SM_TubeUV);
        ++ikey;
        m_cfgMap.insert(ikey, DISP_SM_TubeDI);
        ++ikey;
        m_cfgMap.insert(ikey, DISP_SM_HaveTubeFilter);
        break;
    default:
        break;
    }
}





