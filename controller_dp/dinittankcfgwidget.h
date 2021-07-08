#ifndef DINITTANKCFGWIDGET_H
#define DINITTANKCFGWIDGET_H

#include <QWidget>

class DLineEdit;
class QLabel;
class QComboBox;
class QValidator;

class DInitTankCfgWidget : public QWidget
{
    Q_OBJECT
public:
    enum TankCfgWidgetType
    {
        ComboBoxType = 0,
        LineEditType,
        WidgetTypeNum
    };

public:
    explicit DInitTankCfgWidget(TankCfgWidgetType type, QWidget *parent = 0);

    void setComboBoxItems(const QStringList &texts);
    void setName(const QString &name);
    void setValue(const QString &strValue);
    void setValueValidator(const QValidator *v);
    void setUnit(const QString &strUnit);
    void setID(int id);
    
    int ComboBoxCurrentIndex() const;
    const QString value() const;
    int id() const;

signals:
    
public slots:


private:
    void initUI(TankCfgWidgetType type);
    void initComboBotTypeWidget();
    void initLineEditTypeWidget();

private:
    int m_id;
    QLabel *m_pNameLabel;
    QComboBox *m_pTankSelectBox;
    DLineEdit *m_pValueEdit;
    QLabel *m_pUnitLabel;

    TankCfgWidgetType m_type;
};

#endif // DINITTANKCFGWIDGET_H
