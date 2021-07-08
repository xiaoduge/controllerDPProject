#include "dinittankcfgwidget.h"
#include "dlineedit.h"
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QValidator>

DInitTankCfgWidget::DInitTankCfgWidget(TankCfgWidgetType type, QWidget *parent) :
    QWidget(parent),
    m_type(type)
{
    initUI(type);
}

void DInitTankCfgWidget::setComboBoxItems(const QStringList &texts)
{
    m_pTankSelectBox->addItems(texts);
}

void DInitTankCfgWidget::setName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void DInitTankCfgWidget::setValue(const QString &strValue)
{
    m_pValueEdit->setText(strValue);
}

void DInitTankCfgWidget::setValueValidator(const QValidator *v)
{
    m_pValueEdit->setValidator(v);
}

void DInitTankCfgWidget::setUnit(const QString &strUnit)
{
    m_pUnitLabel->setText(strUnit);
}

void DInitTankCfgWidget::setID(int id)
{
    m_id = id;
}

int DInitTankCfgWidget::ComboBoxCurrentIndex() const
{
    return m_pTankSelectBox->currentIndex();
}

const QString DInitTankCfgWidget::value() const
{
    return m_pValueEdit->text();
}

int DInitTankCfgWidget::id() const
{
    return m_id;
}

void DInitTankCfgWidget::initUI(TankCfgWidgetType type)
{
    m_pNameLabel = new QLabel;
    m_pNameLabel->setFixedWidth(300);
    m_pNameLabel->setStyleSheet("font-size:20pt;color:#000000;font-family:Arial;QFont::Bold");

    switch(type)
    {
    case ComboBoxType:
        initComboBotTypeWidget();
        break;
    case LineEditType:
        initLineEditTypeWidget();
        break;
    default:
        break;
    }
}

void DInitTankCfgWidget::initComboBotTypeWidget()
{
    m_pTankSelectBox = new QComboBox;
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addSpacing(10);
    hLayout->addWidget(m_pNameLabel);
    hLayout->addSpacing(60);
    hLayout->addWidget(m_pTankSelectBox);
    hLayout->addStretch();
    this->setLayout(hLayout);
}

void DInitTankCfgWidget::initLineEditTypeWidget()
{
    m_pValueEdit = new DLineEdit;
    m_pValueEdit->setFixedWidth(150);
    m_pUnitLabel = new QLabel;
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addSpacing(10);
    hLayout->addWidget(m_pNameLabel);
    hLayout->addSpacing(60);
    hLayout->addWidget(m_pValueEdit);
    hLayout->addSpacing(10);
    hLayout->addWidget(m_pUnitLabel);
    hLayout->addStretch();
    this->setLayout(hLayout);
}
