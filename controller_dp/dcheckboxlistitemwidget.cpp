#include "dcheckboxlistitemwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QFile>

DCheckBoxListItemWidget::DCheckBoxListItemWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
    initUI();
}

void DCheckBoxListItemWidget::setTag(const QString &tags)
{
    m_pTagLabel->setText(tags);
}

void DCheckBoxListItemWidget::setCheckBoxState(bool status)
{
    m_pInsCheckBox->setChecked(status);
}

void DCheckBoxListItemWidget::setID(int id)
{
    m_id = id;
}

int DCheckBoxListItemWidget::id()
{
    return m_id;
}

int DCheckBoxListItemWidget::setCheckBoxEnabled(bool bEnabled)
{
    m_pInsCheckBox->setEnabled(bEnabled);
    return 0;
}

Qt::CheckState DCheckBoxListItemWidget::checkBoxState()
{
    return m_pInsCheckBox->checkState();
}

void DCheckBoxListItemWidget::initUI()
{
    m_pTagLabel = new QLabel;
    m_pTagLabel->setFixedWidth(300);
    m_pTagLabel->setStyleSheet("font-size:20pt;color:#000000;font-family:Arial;QFont::Bold");
    m_pInsCheckBox = new QCheckBox;
    m_pInsCheckBox->setEnabled(false);

    QFile qss(":/app/checkbox.qss");
    qss.open(QFile::ReadOnly);
    QString strQss = QLatin1String (qss.readAll());
    qss.close();
    m_pInsCheckBox->setStyleSheet(strQss);

    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addSpacing(10);
    hLayout->addWidget(m_pTagLabel);
    hLayout->addSpacing(60);
    hLayout->addWidget(m_pInsCheckBox);
    hLayout->addStretch();

    this->setLayout(hLayout);

}
