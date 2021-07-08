#include "dwarningdlg.h"
#include "ddialogtitle.h"
#include <QLabel>
#include <QPushButton>

DWarningDlg::DWarningDlg(const QString &text, QWidget *parent) : QDialog(parent)
{
    initUI(text);
}

void DWarningDlg::initTitleBar()
{
    this->setFixedSize(360, 222);
    this->setWindowFlags(Qt::FramelessWindowHint);
    m_titleBar = new DDialogTitle(this);
    m_titleBar->move(0, 0);
    m_titleBar->setTitleContent(tr("Warning"), Qt::white, 16);
}

void DWarningDlg::initUI(const QString &text)
{
    initTitleBar();

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
    connect(m_pConfirmBtn, SIGNAL(clicked()), this, SLOT(onConfirmBtnClicked()));
}

void DWarningDlg::onConfirmBtnClicked()
{
    this->close();
}


