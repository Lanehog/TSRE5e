#ifndef CONLISTWIDGET_H
#define	CONLISTWIDGET_H

#include <QWidget>
#include <QtWidgets>

class EngLib;

class ConListWidget : public QWidget {
    Q_OBJECT
public:
    ConListWidget();
    virtual ~ConListWidget();
    void fillConList();
    void fillConList(QString n);
    void newConsist();
    EngLib* englib;
    
public slots:
    void itemsSelected(QListWidgetItem* it);
    void filterSelected(QString n);
        
signals:
    void conListSelected(int id);
        
private:
    QListWidget items;
    QComboBox conType;
};
#endif	/* CONLISTWIDGET_H */

