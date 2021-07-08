#ifndef DCHECKBOXLISTITEMWIDGET_H
#define DCHECKBOXLISTITEMWIDGET_H

#include <QWidget>

class QLabel;
class QCheckBox;

class DCheckBoxListItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DCheckBoxListItemWidget(QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());

    int id();
    int setCheckBoxEnabled(bool bEnabled);
    Qt::CheckState checkBoxState();
signals:

public slots:
    void setTag(const QString& tags);
    void setCheckBoxState(bool status);
    void setID(int id);

private:
    void initUI();

private:
    QLabel* m_pTagLabel;
    QCheckBox* m_pInsCheckBox;

    int m_id;
};

#endif // DCONSUMABLEINSTALLWIDGET_H
