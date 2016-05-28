#ifndef CONSIST_H
#define	CONSIST_H

#include <QString>
#include <unordered_map>

class Eng;
class TextObj;

class Consist {
public:
    static std::unordered_map<int, TextObj*> txtNumbers;
    static int lastTxtNumbersColor;
    int lastTxtColor;
    static TextObj * txtEngineE;
    static TextObj * txtEngineD;
    static TextObj * txtEngineS;
    static TextObj * txtEngineF;
    static TextObj * txtEngineW;
    static TextObj * txtEngineT;
    struct EngItem{
        bool flip = false;
        int uid = 0;
        int eng = -1;
        float pos = 0;
        float conLength = 0;
        int type = 0;
        TextObj * txt = NULL;
        QString epath;
        QString ename;
    };
    Consist();
    virtual ~Consist();
    QString name;
    QString path;
    QString pathid;
    QString conName;
    QString displayName;
    QString showName;
    unsigned int serial = 0;
    float maxVelocity[2];
    int nextWagonUID = 0;
    float durability = 0;
    float conLength = 0;
    float mass = 0;
    float emass = 0;
    int loaded = -1;
    bool kierunek = false;
    int ref = 0;
    int posInit = false;
    int selectedIdx = -1;
    float textColor[3];
    std::vector<EngItem> engItems;
    Consist(QString p, QString n);
    Consist(QString src, QString p, QString n);
    void load();
    void save();
    void select(int idx);
    void appendEngItem(int id);
    void appendEngItem(int id, int pos = 2);
    void deteleSelected();
    void flipSelected();
    void moveLeftSelected();
    void moveRightSelected();
    bool isNewConsist();
    bool isBroken();
    bool isUnSaved();
    void setNewConsistFlag();
    void setFileName(QString n);
    void setDisplayName(QString n);
    void reverse();
    void setTextColor(float *bgColor);
    void setDurability(float val);
    void render(int selectionColor = 0);
    void render(int aktwx, int aktwz, int selectionColor);
private:
    void initPos();
    bool newConsist = false;
    bool modified = false;
    bool defaultValue = false;
};

#endif	/* CONSIST_H */

