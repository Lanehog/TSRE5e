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
    void getUnsaed(std::vector<int> &unsavedConIds);
    void getUnsaedAct(std::vector<int> &unsavedActIds);
    void findConsistsByEng(int id);
    bool isActivity();
    int getCurrentActivityId();
    EngLib* englib;
    
public slots:
    void itemsSelected(QListWidgetItem* it);
    void conFChan(QString n);
    void conTChan(QString n);
    void routeTChan(QString n);
    void actTChan(QString n);
signals:
    void conListSelected(int id);
    void conListSelected(int aid, int id);
private:
    void fillConListLastQuery();
    void fillConListAct();
    void routeFill();
    int currentConType = 0;
    QListWidget query;
    QLineEdit totalVal;
    QListWidget items;
    QComboBox conType;
    QComboBox conShow;
    QComboBox routeShow;
    QComboBox actShow;
    
    QWidget conTypeList;
    QWidget actTypeList;
};
#endif	/* CONLISTWIDGET_H */

