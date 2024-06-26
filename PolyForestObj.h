/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef POLYFORESTOBJ_H
#define	POLYFORESTOBJ_H

#include "WorldObj.h"
#include <QString>
#include <QVector>

class OglObj;
//class Ref::RefItem;

class PolyForestObj : public WorldObj{
public:
    /*struct Shape{
        int iloscv;
        QOpenGLBuffer VBO;
        QOpenGLVertexArrayObject VAO;
    };*/
    struct PolyForestList{
        QString name;
        QString texture;
        float scaleRangeX = 0;
        float scaleRangeZ = 0;
        float treeSizeX = 0;
        float treeSizeZ = 0;
    };
    static float ForestClearDistance;
    static QVector<PolyForestList> polyForestList;
    QString treeTexture = "";
    QStringList points;
    float scaleRangeX = 0;
    float scaleRangeZ = 0;
    float areaX = 0;
    float areaZ = 0;
    float treeSizeX = 0;
    float treeSizeZ = 0;
    float population = 0;
    PolyForestObj();
    PolyForestObj(const PolyForestObj& o);
    WorldObj* clone();
    bool allowNew();
    void load(int x, int y);
    void set(int sh, FileBuffer* data);
    void set(QString sh, long long int val);
    void set(QString sh, float val);
    void set(QString sh, QString val);
    void set(QString sh, FileBuffer* data);
    Ref::RefItem* getRefInfo();
    void save(QTextStream* out);
    void deleteVBO();
    void translate(float px, float py, float pz);
    void rotate(float x, float y, float z);
    void resize(float x, float y, float z);
    int getDefaultDetailLevel();
    void render(GLUU* gluu, float lod, float posx, float posz, float* playerW, float* target, float fov, int selectionColor, int renderMode);
    static void LoadPolyForestList();
    static int GetListIdByTexture(QString texture);
    virtual ~PolyForestObj();
private:
    void drawShape();
    bool getBoxPoints(QVector<float>& points);
    int tex;
    bool init;
    OglObj shape;
    QString * texturePath = NULL;
};

#endif	/* POLYFORESTOBJ_H */

