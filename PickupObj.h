#ifndef PICKUPOBJ_H
#define	PICKUPOBJ_H

#include "WorldObj.h"
#include <QString>
#include "FileBuffer.h"

class TrackItemObj;

class PickupObj : public WorldObj {
public:
    PickupObj();
    PickupObj(const PickupObj& orig);
    virtual ~PickupObj();
    void load(int x, int y);
    void set(QString sh, FileBuffer* data);
    void save(QTextStream* out);
    bool getBorder(float* border);
    void render(GLUU* gluu, float lod, float posx, float posz, float* playerW, float* target, float fov, int selectionColor);

private:
    int trItemId[4];
    int trItemIdCount = 0;
    int speedRange[2];
    int pickupType[2];
    int pickupAnimData[2];
    int pickupCapacity[2];
    TrackItemObj* pointer3d = NULL;
    float* drawPosition = NULL;
    void renderTritems(GLUU* gluu, int selectionColor);
};

#endif	/* PICKUPOBJ_H */

