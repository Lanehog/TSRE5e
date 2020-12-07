/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include <QDebug>
#include <QFile>
#include <QDir>
#include "Route.h"
#include "TSectionDAT.h"
#include "GLUU.h"
#include "Tile.h"
#include "GLMatrix.h"
#include "TerrainLib.h"
#include "TerrainLibSimple.h"
#include "TerrainLibQt.h"
#include "Game.h"
#include "TrackObj.h"
#include "Path.h"
#include "Terrain.h"
#include "FileFunctions.h"
#include "ParserX.h"
#include "ReadFile.h"
#include "DynTrackObj.h"
#include "PlatformObj.h"
#include "CarSpawnerObj.h"
#include "Flex.h"
#include "ForestObj.h"
#include "Coords.h"
#include "CoordsMkr.h"
#include "CoordsKml.h"
#include "CoordsGpx.h"
#include "CoordsRoutePlaces.h"
#include "SoundList.h"
#include "ActLib.h"
#include "Trk.h"
#include "AboutWindow.h"
#include "TrkWindow.h"
#include "PlatformObj.h"
#include "GroupObj.h"
#include "Undo.h"
#include "Activity.h"
#include "Service.h"
#include "Traffic.h"
#include "Path.h"
#include "Environment.h"
#include "OrtsWeatherChange.h"
#include "GeoCoordinates.h"
#include "Consist.h"
#include "Skydome.h"
#include "TRitem.h"
#include "ActionChooseDialog.h"
#include "ErrorMessagesLib.h"
#include "ErrorMessage.h"
#include "AceLib.h"
#include "Renderer.h"
#include "RenderItem.h"
#include "SpeedPostDAT.h"
#include "SigCfg.h"
#include "TDBClient.h"

Route::Route() {

}

void Route::load(){
    Game::currentRoute = this;
    trkName = Game::trkName;
    routeDir = Game::route;
    
    qDebug() << "# Load Route";
    
    if(!Game::useQuadTree)
        terrainLib = new TerrainLibSimple();
    else
        terrainLib = new TerrainLibQt();
    
    Game::terrainLib = terrainLib;
    
    QFile file(Game::root + "/routes");
    if (!file.exists()){ 
        qDebug() << "Route dir not exist " << file.fileName();
        return;
    }
    file.setFileName(Game::root + "/global");
    if (!file.exists()){ 
        qDebug() << "Global dir not exist " << file.fileName();
        return;
    }

    file.setFileName(Game::root + "/routes/" + Game::route);
    if (!file.exists()) {
        qDebug() << "Route does not exist.";
        if (Game::createNewRoutes) {
            qDebug() << "new Route";
            Route::createNew();
        }
    }

    trk = new Trk();
    trk->load();
    Game::useSuperelevation = trk->tsreSuperelevation;
    
    if(trk->tsreProjection != NULL){
        qDebug() << "TSRE Geo Projection";
        Game::GeoCoordConverter = new GeoTsreCoordinateConverter(trk->tsreProjection);
    } else {
        qDebug() << "MSTS Geo Projection";
        Game::GeoCoordConverter = new GeoMstsCoordinateConverter();
    }
    env = new Environment(Game::root + "/routes/" + Game::route + "/ENVFILES/editor.env");
    Game::routeName = trk->routeName.toLower();
    routeName = Game::routeName;
    qDebug() << Game::routeName;

    this->tsection = new TSectionDAT();
    // Check Track Section Databaase
    if(!checkTrackSectionDatabase())
        return;
    
    if(Game::loadAllWFiles){
        preloadWFiles(Game::gui);
    }

    this->trackDB = new TDB(tsection, false);
    this->trackDB->loadTdb();
    this->roadDB = new TDB(tsection, true);
    this->roadDB->loadTdb();
    Game::trackDB = this->trackDB;
    Game::roadDB = this->roadDB;
    
    loadAddons();

    loadMkrList();
    createMkrPlaces();
    loadServices();
    loadTraffic();
    loadPaths();
    loadActivities();

    soundList = new SoundList();
    soundList->loadSoundSources(Game::root + "/routes/" + Game::route + "/ssource.dat");
    soundList->loadSoundRegions(Game::root + "/routes/" + Game::route + "/ttype.dat");
    Game::soundList = soundList;
    
    Game::terrainLib->loadQuadTree();
    OrtsWeatherChange::LoadList();
    ForestObj::LoadForestList();
    ForestObj::ForestClearDistance = trk->forestClearDistance;
    CarSpawnerObj::LoadCarSpawnerList();

    if(Game::loadAllWFiles){
        preloadWFilesInit();
    }
    
    checkRouteDatabase();
    
    loaded = true;
    
    Vec3::set(placementAutoTranslationOffset, 0, 0, 0);
    Vec3::set(placementAutoRotationOffset, 0, 0, 0);
    
    skydome = new Skydome();

    // Route Merge. 
    if(Game::routeMergeString.length() > 0){
        QStringList args = Game::routeMergeString.split(":");
        if(args.size() == 4){
            float offsetX = args[1].toFloat();
            float offsetY = args[2].toFloat();
            float offsetZ = args[3].toFloat();
            mergeRoute(args[0], offsetX, offsetY, offsetZ);
            setAsCurrentGameRoute();
        }
    }

}

void Route::load(QString name){
    if(!Game::useQuadTree)
        terrainLib = new TerrainLibSimple();
    else
        terrainLib = new TerrainLibQt();
    
    QFile file(Game::root + "/routes");
    if (!file.exists()){ 
        qDebug() << "Route dir not exist " << file.fileName();
        return;
    }
    file.setFileName(Game::root + "/global");
    if (!file.exists()){ 
        qDebug() << "Global dir not exist " << file.fileName();
        return;
    }

    file.setFileName(Game::root + "/routes/" + name);
    if (!file.exists()) {
        qDebug() << "Route does not exist.";
        return;
    }
    Game::route = name;
    Game::checkRoute(Game::route);
    routeDir = Game::route;
    trkName = Game::trkName;
    trk = new Trk();
    trk->load();
    //Game::useSuperelevation = trk->tsreSuperelevation;
    
    
    /*if(trk->tsreProjection != NULL){
        qDebug() << "TSRE Geo Projection";
        Game::GeoCoordConverter = new GeoTsreCoordinateConverter(trk->tsreProjection);
    } else {
        qDebug() << "MSTS Geo Projection";
        Game::GeoCoordConverter = new GeoMstsCoordinateConverter();
    }
    env = new Environment(Game::root + "/routes/" + Game::route + "/ENVFILES/editor.env");*/
    Game::routeName = trk->routeName.toLower();
    routeName = Game::routeName;
    qDebug() << Game::routeName;

    this->tsection = new TSectionDAT();
    
    if(Game::loadAllWFiles){
        preloadWFiles(true);
    }

    this->trackDB = new TDB(tsection, false);
    this->trackDB->loadTdb();
    this->roadDB = new TDB(tsection, true);
    this->roadDB->loadTdb();
    //Game::trackDB = this->trackDB;
    //Game::roadDB = this->roadDB;
    
    //loadAddons();

    //loadMkrList();
    //createMkrPlaces();
    //loadServices();
    //loadTraffic();
    //loadPaths();
    //loadActivities();

    //soundList = new SoundList();
    //soundList->loadSoundSources(Game::root + "/routes/" + Game::route + "/ssource.dat");
    //soundList->loadSoundRegions(Game::root + "/routes/" + Game::route + "/ttype.dat");
    //Game::soundList = soundList;
    
    terrainLib->loadQuadTree();
    //OrtsWeatherChange::LoadList();
    //ForestObj::LoadForestList();
    //ForestObj::ForestClearDistance = trk->forestClearDistance;
    //CarSpawnerObj::LoadCarSpawnerList();

    //if(Game::loadAllWFiles){
    //    preloadWFilesInit();
    //}
    
    //checkRouteDatabase();
    
    loaded = true;
    
    //Vec3::set(placementAutoTranslationOffset, 0, 0, 0);
    //Vec3::set(placementAutoRotationOffset, 0, 0, 0);
    
    //skydome = new Skydome();
    
}

void Route::setAsCurrentGameRoute(){
    Game::route = routeDir;
    Game::routeName = routeName;
    Game::trkName = trkName;
    Game::currentRoute = this;
}

void Route::mergeRoute(QString route2Name, float offsetX, float offsetY, float offsetZ){
    QProgressDialog *progress = NULL;
    bool gui = true;
    
    Route *route2 = new Route();
    route2->load(route2Name);
    float mOffset[3];
    mOffset[0] = offsetX;// (-4*2048) - 256;
    mOffset[1] = offsetY;//81.47992;
    mOffset[2] = offsetZ;//(-5*2048) + 640;
    
    // Merge TDB
    qDebug() << "## Merge TDB";
    unsigned int trackNodeOffset = 0; 
    unsigned int trackItemOffset = 0;
    unsigned int roadNodeOffset = 0; 
    unsigned int roadItemOffset = 0;
    QHash<unsigned int, unsigned int> fixedSectionIds;
    QHash<unsigned int, unsigned int> fixedShapeIds;
    unsigned int oldTrackNodeCount = this->trackDB->iTRnodes; 
    unsigned int oldRoadNodeCount = this->roadDB->iTRnodes;
    this->trackDB->mergeTDB(route2->trackDB, mOffset, trackNodeOffset, trackItemOffset, fixedSectionIds, fixedShapeIds);
    this->roadDB->mergeTDB(route2->roadDB, mOffset, roadNodeOffset, roadItemOffset, fixedSectionIds, fixedShapeIds);
    
    // Merge world objects
    if(gui){
        progress = new QProgressDialog("Merging World Objects ...", "", 0, route2->tile.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    qDebug() << "## Merge World Objects";
    QHash<int, Tile*> modifiedWorldTiles;
    Tile *t2Tile;
    QVector<int*> trackObjUpdates;
    int pi = 0;
    foreach (Tile* tTile, route2->tile){
        if(progress != NULL){
            progress->setValue((++pi));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        if (tTile == NULL) 
            continue;
        if (tTile->loaded == 1) {
            tTile->updateTrackSectionInfo(fixedShapeIds, fixedSectionIds);
        }
        
        for(int i = 0; i < tTile->jestObiektow; i++){
            WorldObj *wObj = tTile->obiekty[i];
            if(wObj == NULL) continue;
            //if(wObj->isTrackItem()) continue;
            wObj->addTrackItemIdOffset(trackItemOffset, roadItemOffset);
            int x, z, uid, oldx, oldz, olduid;
            oldx = wObj->x;
            oldz = wObj->y;
            olduid = wObj->UiD;
            wObj->position[0] += mOffset[0];
            wObj->position[1] += mOffset[1];
            wObj->position[2] -= mOffset[2];
            //qDebug() << "old tile" << wObj->x << wObj->y;
            while(wObj->position[0] > 1024 || wObj->position[0] < -1024 || wObj->position[2] > 1024 || wObj->position[2] < -1024 ){
                Game::check_coords(wObj->x, wObj->y, wObj->position);
            }
            //qDebug() << "new tile" << wObj->x << wObj->y;
            //qDebug() << "";
            x = wObj->x;
            z = wObj->y;
            
            t2Tile = tile[((x)*10000 + z)];
            if (t2Tile == NULL){
                tile[(x)*10000 + z] = new Tile(x, z);
                t2Tile = tile[((x)*10000 + z)];
                t2Tile->initNew();
            }
            if (modifiedWorldTiles[((x)*10000 + z)] == NULL)
                modifiedWorldTiles[((x)*10000 + z)] = t2Tile;
            //
            t2Tile->placeObject(wObj);
            if(wObj->typeID == wObj->trackobj || wObj->typeID == wObj->dyntrack){
                int *u = new int[6];
                u[0] = oldx;
                u[1] = oldz;
                u[2] = olduid;
                u[3] = x;
                u[4] = z;
                u[5] = wObj->UiD;
                trackObjUpdates.push_back(u);
            }
        }
        
    }
    
    this->trackDB->updateUiDs(trackObjUpdates, oldTrackNodeCount);
    this->roadDB->updateUiDs(trackObjUpdates, oldRoadNodeCount);
    
    if(progress != NULL)
        delete progress;
    
    qDebug() << "## Create MKR Places";
    createMkrPlaces();
    
    // Merge terrain
    if(gui){
        progress = new QProgressDialog("Merging Terrain ...", "", 0, route2->tile.size() + modifiedWorldTiles.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    pi = 0;
    qDebug() << "Load all route2 terrain tiles";
    foreach (Tile* wTile, route2->tile){
        if(progress != NULL){
            progress->setValue((++pi));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
       if (wTile == NULL) 
           continue;
       Terrain *t = route2->terrainLib->getTerrainByXY(wTile->x, wTile->z, true);
       if (t == NULL)
           qDebug() << "FAIL terrain NULL";
       else if (!t->loaded)
           qDebug() << "FAIL terrain not loaded";
       qDebug() << t->mojex << t->mojez;
    }
    
    setAsCurrentGameRoute();
    
    qDebug() << "Fill Terrain data";
    Terrain *tTile;
    foreach (Tile* wTile, modifiedWorldTiles){
        if(progress != NULL){
            progress->setValue((++pi));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        if (wTile == NULL) 
            continue;
        tTile = terrainLib->getTerrainByXY(wTile->x, wTile->z, false);
        if(tTile == NULL){
            terrainLib->saveEmpty(wTile->x, -wTile->z);
            if(!terrainLib->reload(wTile->x, wTile->z)){
                qDebug() << "reload terrain fail";
            }
            tTile = terrainLib->getTerrainByXY(wTile->x, wTile->z, false);
            
        }
        if (!tTile->loaded)
           qDebug() << "FAIL main terrain not loaded";
        
        qDebug() << "fill";
        route2->terrainLib->fillTerrainData(tTile, mOffset);
        
    }
    if(progress != NULL)
        delete progress;
    // Other
    

}

void Route::selectObjectsByXYRange(int mojex, int mojez, int minx, int maxx, int minz, int maxz){
    Tile *tTile = tile[mojex*10000 + mojez];
    if (tTile == NULL)
        return;
    QVector<GameObj*> objs;
    tTile->selectObjectsByXYRange(objs, minx, maxx, minz, maxz);
    this->objectSelected(objs);
}

Route::Route(const Route& orig) {
}

Route::~Route() {
}

void Route::loadAddons(){
    this->ref = new Ref((Game::root + "/routes/" + Game::route + "/" + Game::routeName + ".ref"));
    
    QString dirFile = Game::root + "/routes/" + Game::route + "/addons";
    QDir aDir(dirFile);
    if(!aDir.exists()){
        qDebug() << dirFile;
        qDebug() << "# No Addons";
        return;
    }
        
    aDir.setFilter(QDir::Files);
    aDir.setNameFilters(QStringList()<<"*.ref");
    foreach(QString file, aDir.entryList()){
        //qDebug()<< dirFile + "/" + file;
        this->ref->loadFile(dirFile + "/" + file);
    }

}

void Route::checkRouteDatabase(){
    trackDB->checkDatabase();
    roadDB->checkDatabase();
    
}

bool Route::checkTrackSectionDatabase(){
    if(!this->tsection->dataOutOfSync)
        return true;
    
    qDebug() << "tsection out of sync !!!";
    if(Game::playerMode)
        return true;
    if(!Game::writeEnabled)
        return true;
    if(!Game::writeTDB)
        return true;
    
    // Edit mode. Make an action regarding not synced tsection data
    ActionChooseDialog dialog(4);
    dialog.setWindowTitle("TDB Error");
    dialog.setInfoText("Route Track Section database is out of sync with your Global database.\n"
                       "Choose action:");
    dialog.pushAction("FIX", "Convert route database to current Global now");
    dialog.pushAction("VIEW", "Disable writing to TDB - avoid editing tracks and interactives");
    dialog.pushAction("IGNORE", "Ignore and continue - saving route may destroy your route");
    dialog.pushAction("EXIT", "Quit TSRE now");
    dialog.exec();
    qDebug() << dialog.actionChoosen;
    
    if(dialog.actionChoosen == "FIX"){
        Game::loadAllWFiles = true;
        preloadWFiles(true);
        // load tsection with autofix
        this->tsection = new TSectionDAT(true);
        // update ids inside W files
        foreach (Tile* tTile, tile){
            if (tTile == NULL) continue;
            if (tTile->loaded == 1) {
                tTile->updateTrackSectionInfo(tsection->autoFixedShapeIds, tsection->autoFixedSectionIds);
             }
        }
        ErrorMessage *e = new ErrorMessage(
            ErrorMessage::Type_Info, 
            ErrorMessage::Source_Editor, 
            QString("Route Track Section synced by TSRE. "),
                    "TSRE made automatic conversion of route database to current Global."
                    );
        ErrorMessagesLib::PushErrorMessage(e);
        
        
        return true;
    } 
    ErrorMessage *e = new ErrorMessage(
    ErrorMessage::Type_Error, 
    ErrorMessage::Source_TDB, 
    QString("Route Track Section database is out of sync with your Global database. "),
            "Route Track Section isn't compatibile with your current Global database. \n"
            "Check route installation for custom Global or convert Route using TSRE. \n"
            "Editing route may cause fatal errors. Make sure that writing to TDB is disabled."
        );
    ErrorMessagesLib::PushErrorMessage(e);
    if(dialog.actionChoosen == "VIEW"){
        Game::writeTDB = false;
        return true;
    }
    if(dialog.actionChoosen == "IGNORE"){
        // just do nothing
        return true;
    } 
    if(dialog.actionChoosen == "EXIT"){
        loaded = false;
        return false;
    }

    return true;
}

void Route::activitySelected(Activity* selected){
    currentActivity = selected;
}

Trk *Route::getTrk(){
    return this->trk;
}

int Route::getStartTileX(){
    return this->trk->startTileX;
}

int Route::getStartTileZ(){
    return this->trk->startTileZ;
}

float Route::getStartpX(){
    return this->trk->startpX;
}

float Route::getStartpZ(){
    return this->trk->startpZ;
}

void Route::createMkrPlaces(){
    QString key;
    key = "| Route: Stations";
    mkrList[key] = new CoordsRoutePlaces(trackDB, "stations");
    
    key = "| Route: Sidings";
    mkrList[key] = new CoordsRoutePlaces(trackDB, "sidings");
}

void Route::loadMkrList(){
    //this->mkr = new CoordsMkr(Game::root + "/routes/" + Game::route + "/" + Game::routeName +".mkr");
    QDir dir(Game::root + "/routes/" + Game::route);
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList()){
        if(dirFile.endsWith(".mkr", Qt::CaseInsensitive))
            mkrList[(dirFile).toLower()] = new CoordsMkr(Game::root + "/routes/" + Game::route + "/" + dirFile);
        if(dirFile.endsWith(".kml", Qt::CaseInsensitive))
            mkrList[(dirFile).toLower()] = new CoordsKml(Game::root + "/routes/" + Game::route + "/" + dirFile);
        if(dirFile.endsWith(".gpx", Qt::CaseInsensitive))
            mkrList[(dirFile).toLower()] = new CoordsGpx(Game::root + "/routes/" + Game::route + "/" + dirFile);
    }
    if(mkrList.size() > 0){
        if(mkrList[(Game::routeName+".mkr").toLower()] != NULL){
            if(mkrList[(Game::routeName+".mkr").toLower()]->loaded)
                mkr = mkrList[(Game::routeName+".mkr").toLower()];
            else
                mkr = mkrList.begin().value();
                //mkr = mkrList[(Game::routeName+".mkr").toLower().toStdString()];
        } else {
            mkr = mkrList.begin().value();
        }
    }
}

void Route::setMkrFile(QString name){
    if(mkrList[name] != NULL)
        this->mkr = mkrList[name];
}

void Route::loadActivities(){
    QDir dir(Game::root + "/routes/" + Game::route + "/activities");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.act");
    foreach(QString actfile, dir.entryList()){
        activityId.push_back(ActLib::AddAct(dir.path(), actfile));
    }
    
    //for(int i = 0; i < ActLib::jestact; i++){
    //    ActLib::Act[i]->setRouteContent(&path, &service, &traffic);
    //}

    qDebug() << "activity loaded";
    return;
}

void Route::loadServices(){
    QDir dir(Game::root + "/routes/" + Game::route + "/services");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.srv");
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddService(dir.path(), actfile);
        //service.push_back(ActLib::Services[id]);
    }

    qDebug() << "service loaded";
    return;
}

void Route::loadTraffic(){
    QDir dir(Game::root + "/routes/" + Game::route + "/traffic");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.trf");
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddTraffic(dir.path(), actfile);
        //traffic.push_back(ActLib::Traffics[id]);
    }
    qDebug() << "traffic loaded";
    return;
}

void Route::loadPaths(){
    QDir dir(Game::root + "/routes/" + Game::route + "/paths");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.pat");
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddPath(dir.path(), actfile);
        if(!path.contains(ActLib::Paths[id]))
            path.push_back(ActLib::Paths[id]);
    }

    qDebug() << "paths loaded";
    return;
}

WorldObj* Route::getObj(int x, int z, int id) {
    Tile *tTile;

    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return NULL;
    return tTile->getObj(id);

}

WorldObj* Route::findNearestObj(int x, int z, float* pos){
    Game::check_coords(x, z, pos);
    Tile *tTile;
    //try {
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return NULL;
    return tTile->findNearestObj(pos);
}

void Route::transalteObj(int x, int z, float px, float py, float pz, int uid) {
    Tile *tTile;

    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return;
    tTile->transalteObj(px, py, pz, uid);

}

void Route::updateSim(float *playerT, float deltaTime){
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            if (tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                tTile->updateSim(deltaTime);
            }
        }
    }
    
    if(currentActivity != NULL){
        currentActivity->updateSim(playerT, deltaTime);
    }
}

WorldObj* Route::updateWorldObjData(FileBuffer *data){
    QString sh;
    int x = 0;
    int z = 0;
    WorldObj *nowy = NULL;
    bool objloaded = true;
    
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        qDebug() << sh;
        if (sh == ("x")) {
            x = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("z")) {
            z = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("remove")) {
            objloaded = false;
            ParserX::SkipToken(data);
            continue;
        }
        if ((nowy = WorldObj::createObj(sh)) != NULL) {
            qDebug() << nowy->type;
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                nowy->set(sh, data);
                ParserX::SkipToken(data);
            }
            if(tile[x*10000+z] != NULL){
                tile[x*10000+z]->replaceWorldObj(nowy);
            }
            nowy->loaded = objloaded;
            qDebug() << nowy->loaded;
            //obiekty[jestObiektow++] = nowy;
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
        continue;
    }
    return nowy;
}

void Route::loadTSectionData(FileBuffer *data){
    tsection->loadRouteUtf16Data(data, false);
    tsection->routeMaxIdx += 2 - tsection->routeMaxIdx % 2;
    tsection->routeShapes++;
    if(Game::ServerMode){
        loadingProgress++;
        load();
    }
}

void Route::loadTrkData(FileBuffer *data){
    trk = new Trk();
    trk->loadUtf16Data(data);
    if(Game::ServerMode){
        loadingProgress++;
        load();
    }
}
    
void Route::loadTdbData(FileBuffer *data, QString type){
    bool road = false;
    if(type == "rdb")
        road = true;
    
    QString sh;
    int x = 0;
    int z = 0;
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        qDebug() << sh;
        if (sh == "trackdb") {
            TDB *t = NULL;
            if(!Game::ServerMode){
                t = new TDB(tsection, road);
            } else {
                t = new TDBClient(tsection, road);
            }
            t->loadUtf16Data(data);
            if(!t->isRoad()){
                t->speedPostDAT = new SpeedPostDAT();
                t->sigCfg = new SigCfg();
                this->trackDB = t;
                Game::trackDB = t;
            } else {
                this->roadDB = t;
                Game::roadDB = t;
            }
            t->loaded = true;
            
            if(Game::ServerMode){
                loadingProgress++;
                load();
            }
            ParserX::SkipToken(data);
            continue;
            
        }
        ParserX::SkipToken(data);
    }
}

void Route::updateTileData(FileBuffer *data){
    QString sh;
    int x = 0;
    int z = 0;
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        qDebug() << sh;
        if (sh == ("x")) {
            x = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("z")) {
            z = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("tr_worldfile")) {
            tile[x*10000+z] = new Tile(x, z, data);
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
        continue;
    }
}

void Route::preloadWFiles(bool gui){
    Tile *tTile;
    
    QString path = Game::root + "/routes/" + Game::route + "/world";
    QDir dir(path);
    //qDebug() << path;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.w");
    
    if (!dir.exists()) {
        qDebug() << "route W dir not exist - aborting";
        return;
    }
        
    int WX = 0, WZ = 0;
    unsigned long long timeNow = QDateTime::currentMSecsSinceEpoch();
    QProgressDialog *progress = NULL;
    if(gui){
        progress = new QProgressDialog("Loading All World Files ...", "", 0, dir.entryList().size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    
    int i = 0;
    foreach(QString wfile, dir.entryList()){
        if(wfile.length() != 17){
            qDebug() << "# W File undefined name " << wfile;
        }
        QStringRef wxString(&wfile, 1, 7);
        QStringRef wzString(&wfile, 8, 7);
        WX = wxString.toInt();
        WZ = -wzString.toInt();
        tTile = tile[(WX)*10000 + WZ];

        if (tTile == NULL){
            qDebug() << wxString << wzString << "-" << WX << WZ;
            tile[(WX)*10000 + WZ] = new Tile(WX, WZ);
        }
        if(progress != NULL){
            progress->setValue((++i));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }

    qDebug() << "#W Files preloaded: " << (QDateTime::currentMSecsSinceEpoch() - timeNow)/1000<< "s";
    delete progress;
    
}

// Use this function to init W files if loaded before route data.
void Route::preloadWFilesInit(){
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->loadInit();
        }
    }
}

void Route::pushRenderItems(float * playerT, float* playerW, float* target, float playerRot, float fov, int renderMode) {
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    if(renderMode == Game::currentRenderer->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }
    
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            
            if(Game::autoNewTiles)
                if (i == 0 && j == 0)
                    if (tTile->loaded == -2) {
                        Route::newTile((int)playerT[0] + i, (int)playerT[1] + j);
                        tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
                    }
            if (tTile->loaded == 1) {
                Game::currentRenderer->mvPushMatrix();
                Mat4::translate(Game::currentRenderer->mvMatrix, Game::currentRenderer->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->pushRenderItems(playerT, playerW, target, fov, renderMode);
                Game::currentRenderer->mvPopMatrix();
            }
        }
    }
    
    /*if (renderMode == gluu->RENDER_DEFAULT) {
        if(Game::viewTrackDbLines)
            trackDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines)
            trackDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewTrackDbLines)
            roadDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines)
            roadDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewMarkers)
            if(this->mkr != NULL)
                this->mkr->render(gluu, playerT, playerW, playerRot);
    }
    if(Game::renderTrItems){
        trackDB->renderItems(gluu, playerT, playerRot, renderMode);
        roadDB->renderItems(gluu, playerT, playerRot, renderMode);
    }
    
    if(currentActivity != NULL){
        currentActivity->render(gluu, playerT, playerRot, renderMode);
    }
    
    for(int i = 0; i < path.size(); i++){
        if(path[i]->isSelected())
            path[i]->render(gluu, playerT, renderMode);
    }*/

    Game::ignoreLoadLimits = false;
}

void Route::render(GLUU *gluu, float * playerT, float* playerW, float* target, float playerRot, float fov, int renderMode) {
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    if(renderMode == gluu->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }

    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            
            if(Game::autoNewTiles)
                if (i == 0 && j == 0)
                    if (tTile->loaded == -2) {
                        Route::newTile((int)playerT[0] + i, (int)playerT[1] + j);
                        tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
                    }
            if (tTile->loaded == 1) {
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->render(playerT, playerW, target, fov, renderMode);
                gluu->mvPopMatrix();
            }
        }
    }
    if (renderMode == gluu->RENDER_DEFAULT) {
        if(Game::viewTrackDbLines && trackDB != NULL)
            trackDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines && trackDB != NULL)
            trackDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewTrackDbLines && roadDB != NULL)
            roadDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines && roadDB != NULL)
            roadDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewMarkers)
            if(this->mkr != NULL)
                this->mkr->render(gluu, playerT, playerW, playerRot);
    }
    if(Game::renderTrItems){
        trackDB->renderItems(gluu, playerT, playerRot, renderMode);
        roadDB->renderItems(gluu, playerT, playerRot, renderMode);
    }
    
    if(currentActivity != NULL){
        currentActivity->render(gluu, playerT, playerRot, renderMode);
    }
    
    for(int i = 0; i < path.size(); i++){
        if(path[i]->isSelected())
            path[i]->render(gluu, playerT, renderMode);
    }
    //trackDB->renderItems(gluu, playerT, playerRot);
    /*
    for (var key in this.tile){
       if(this.tile[key] === undefined) continue;
       if(this.tile[key] === null) continue;
       //console.log(this.tile[key].inUse);
       if(!this.tile[key].inUse){
           //console.log("a"+this.tile[key]);
           this.tile[key] = undefined;
       } else {
           this.tile[key].inUse = false;
       }
    }*/
    Game::ignoreLoadLimits = false;
}

void Route::renderShadowMap(GLUU *gluu, float * playerT, float* playerW, float* target, float playerRot, float fov, bool selection) {
    if(!loaded) return;
    
    int mintile = -1;
    int maxtile = 1;
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->render(playerT, playerW, target, fov, GLUU::RENDER_SHADOWMAP);
                gluu->mvPopMatrix();
            }
        }
    }
    if(currentActivity != NULL)
        currentActivity->render(gluu, playerT, playerRot, 0);
}

void Route::setTerrainTextureToObj(int x, int y, float *pos, Brush* brush, WorldObj* obj){
    if(obj == NULL)
        obj = Route::findNearestObj(x, y, pos);
    if(obj == NULL)
        return;
    if(obj->typeID == WorldObj::trackobj){
        setTerrainTextureToTrack(x, y, pos, brush, 0);
        return;
    }
    if(!obj->hasLinePoints())
        return;

    float* punkty = new float[10000];
    float* ptr = punkty;
    obj->getLinePoints(ptr);
    int length = ptr - punkty;
    qDebug() << "lo "<<length;
    Game::terrainLib->setTextureToTrackObj(brush, punkty, length, obj->x, obj->y);
    delete[] punkty;
}

void Route::setTerrainTextureToTrack(int x, int y, float* pos, Brush* brush, int mode){
    Game::check_coords(x, y, pos);
    float playerT[2];
    playerT[0] = x;
    playerT[1] = y;
    float tp[3];
    Vec3::copy(tp,pos);
    int ok1, ok2;
    QVector<float> punkty;
    punkty.reserve(10000);
    if(placementAutoTargetType == 0) {
        this->trackDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    } else if(placementAutoTargetType == 1) {
        this->roadDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    } else if(placementAutoTargetType == 2) {
        bool road = false;
        ok1 = this->trackDB->findNearestPositionOnTDB(playerT, tp, NULL, NULL);
        ok2 = this->roadDB->findNearestPositionOnTDB(playerT, tp, NULL, NULL);
        if(ok2 >= 0)
            if(ok1 < 0 || ok2 < ok1){
                road = true;
        }
        if(road)
            this->roadDB->getVectorSectionPoints(x, y, pos, punkty, mode);
        else
            this->trackDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    }
    int length = punkty.length();
    qDebug() << "l "<<length;
    Game::terrainLib->setTextureToTrackObj(brush, punkty.data(), length, x, y);
}

void Route::setTerrainToTrackObj(WorldObj* obj, Brush* brush){
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            setTerrainToTrackObj(gobj->objects[i], brush);
        }
        return;
    }
    
    if(obj->type == "trackobj" || obj->type == "dyntrack" ){
        //TrackObj* tobj = (TrackObj*)obj;
        //TrackObj* track = (TrackObj*)obj;
        QVector<float> punkty;
        punkty.reserve(10000);
        if(this->tsection->isRoadShape(obj->sectionIdx))
            this->roadDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        else
            this->trackDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        int length = punkty.length();
        qDebug() << "l "<<length;
        if(length == 0){
            if(obj->sectionIdx >= 0){
                float matrix[16];
                for(int i = 0; i < tsection->shape[obj->sectionIdx]->path[0].n; i++){
                    memcpy(matrix, obj->matrix, sizeof(float)*16);
                    int sidx = tsection->shape[obj->sectionIdx]->path[0].sect[i];
                    tsection->sekcja[sidx]->getPoints(punkty, matrix);
                }
            } else {
                //obj->getLinePoints(ptr);
            }
            length = punkty.length();
            qDebug() << "l "<<length;
        }
        float offset = 0;//-0.3;
        if(length > 0)
            Game::terrainLib->setTerrainToTrackObj(brush, punkty.data(), length, obj->x, obj->y, obj->matrix, offset);
    } else if(obj->hasLinePoints()) {
        float* punkty = new float[10000];
        float* ptr = punkty;
        obj->getLinePoints(ptr);
        int length = ptr - punkty;
        qDebug() << "l "<<length;
        Game::terrainLib->setTerrainToTrackObj(brush, punkty, length, obj->x, obj->y, obj->matrix);
        delete[] punkty;
    }
}

ActivityObject* Route::getActivityObject(int id){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity->getObjectById(id);
    return NULL;
}

Consist* Route::getActivityConsist(int id){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity->getServiceConsistById(id);
}

Activity* Route::getCurrentActivity(){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity;
}

float Route::getDistantTerrainYOffset(){
    return trk->distantTerrainYOffset;
}

WorldObj* Route::placeObject(int x, int z, float* p) {
    float* q = new float[4];
    Quat::fill((float*)q);
    return placeObject(x, z, p, (float*) q, 0, ref->selected);
}

WorldObj* Route::placeObject(int x, int z, float* p, float* q, float elev) {
    return placeObject(x, z, p, q, elev, ref->selected);
}

WorldObj* Route::placeObject(int x, int z, float* p, float* q, float elev, Ref::RefItem* r) {
    if(r == NULL) 
        return NULL;
    Game::check_coords(x, z, p);

    // pozycja wzgledem TDB:
    int itemTrackType = WorldObj::isTrackObj(r->type);
    
    if(itemTrackType != 0){
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
    }
    
    float* tpos = NULL;
    if(placementStickToTarget){
            tpos = new float[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, p);
            Quat::copy(tq, q);
            Vec3::copy(tp2, p);
            Quat::copy(tq2, q);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, tpos);
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, tpos);
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, tpos);
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, tpos);
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                Quat::copy(q, tq);
                if(!snapableOnlyRotation){
                    Vec3::copy(p, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    if(itemTrackType == 1){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->trackDB->findNearestPositionOnTDB(playerT, p, q, tpos);
        if(ok < 0) return NULL;
        x = playerT[0];
        z = playerT[1];
    }
    if(itemTrackType == 2){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->roadDB->findNearestPositionOnTDB(playerT, p, q, tpos);
        if(ok < 0) return NULL;
        x = playerT[0];
        z = playerT[1];
    } 
    if(itemTrackType == 3){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->roadDB->findNearestPositionOnTDB(playerT, p, q, tpos);
        if(ok < 0) return NULL;
        float* buffer;
        int len;
        this->roadDB->getVectorSectionLine(buffer, len, playerT[0], playerT[1], tpos[0]);
        qDebug() << "len "<<len;
        bool ok1 = this->trackDB->getSegmentIntersectionPositionOnTDB(playerT, buffer, len, p, q, tpos);
        if(!ok1) return NULL;
        x = playerT[0];
        z = playerT[1];
        //return NULL;
    }

    Tile *tTile = requestTile(x, z);
    if(tTile == NULL) return NULL;
    if(tTile->loaded != 1) return NULL;
    
    int snapableSide = -1;
    if(placementStickToTarget && placementAutoTargetType == 3){
        snapableSide = tTile->getNearestSnapablePosition(p, q);
    }
        
    float endp[5];
    memset(endp, 0, sizeof(endp));
    endp[3] = 1;
    float firstPos[3];
    if ((r->type == "trackobj" || r->type == "dyntrack" )) {
        if(r->type == "dyntrack"){
            this->roadDB->setDefaultEnd(0);
            this->trackDB->setDefaultEnd(0);
        }
        qDebug() <<"1: "<< x <<" "<<z<<" "<<p[0]<<" "<<p[1]<<" "<<p[2]; 
        int oldx = x;
        int oldz = z;
        Vec3::copy(firstPos, p);
        if(this->tsection->isRoadShape(r->value))
            this->roadDB->findPosition(x, z, p, q, endp, r->value);
        else
            this->trackDB->findPosition(x, z, p, q, endp, r->value);
        Game::check_coords(x, z, p);
        firstPos[0] -= (x-oldx)*2048;
        firstPos[2] -= (z-oldz)*2048;
        qDebug() <<"2: "<< x <<" "<<z<<" "<<p[0]<<" "<<p[1]<<" "<<p[2]; 
        tTile = requestTile(x, z);
        if(tTile == NULL) return NULL;
        if(tTile->loaded != 1) return NULL;
    }

    WorldObj* nowy = tTile->placeObject(p, q, r, tpos);

    if ((r->type == "trackobj" || r->type == "dyntrack" )&& nowy != NULL) {
        if(nowy->endp == 0) nowy->endp = new float[5];
        memcpy(nowy->endp, endp, sizeof(float)*5);
        Vec3::copy(nowy->firstPosition,firstPos);
    }   
    nowy->snapped(snapableSide);
    if(nowy->typeID == nowy->sstatic){
        moveWorldObjToTile(nowy->x, nowy->y, nowy);
    }

    if(elev !=0)
        nowy->rotate(elev, 0, 0);

    Undo::PushWorldObjPlaced(nowy); 
    return nowy;
}

float* Route::getPointerPosition(float* out, int &x, int &z, float* pos){
    if(out == NULL)
        return NULL;
    
    Vec3::copy(out, pos);
    
    if(placementStickToTarget){
            float ttpos[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, pos);
            Vec3::copy(tp2, pos);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, ttpos);
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                if(!snapableOnlyRotation){
                    Vec3::copy(out, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    return out;
}

void Route::dragWorldObject(WorldObj* obj, int x, int z, float* pos){
    if(obj->typeObj != WorldObj::worldobj)
        return;
    
    float tpos[3];
    Vec3::copy(tpos, pos);
    Game::check_coords(x, z, tpos);
    Tile *tTile = requestTile(x, z);
    if(tTile == NULL) return;
    if(tTile->loaded != 1) return;
    int snapableSide = -1;
    float q[4];
    Quat::copy(q, obj->qDirection);
    
    if(placementStickToTarget){
            float ttpos[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, pos);
            Quat::copy(tq, q);
            Vec3::copy(tp2, pos);
            Quat::copy(tq2, q);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, ttpos);
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                Quat::copy(q, tq);
                if(!snapableOnlyRotation){
                    Vec3::copy(tpos, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    
    
    if(obj->isTrackItem() || obj->typeID == obj->groupobject || obj->typeID == obj->ruler ){
        obj->setPosition(x, z, tpos);
        obj->setMartix();
        return;
    }
    
    if (obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack)
        if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
            obj->setPosition(x, z, pos);
            obj->setMartix();
            return;
    }
    
    if(placementStickToTarget && placementAutoTargetType == 3){
        snapableSide = tTile->getNearestSnapablePosition(tpos, q, obj->UiD);
    }

    if ((obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack )) {
        if(this->tsection->isRoadShape(obj->sectionIdx)){
            this->roadDB->setDefaultEnd(0);
            this->roadDB->findPosition(x, z, tpos, q, obj->endp, obj->sectionIdx);
        }else{
            this->trackDB->setDefaultEnd(0);
            this->trackDB->findPosition(x, z, tpos, q, obj->endp, obj->sectionIdx);
        }
        tTile = requestTile(x, z);
        if(tTile == NULL) return;
        if(tTile->loaded != 1) return;
    }

    obj->setPosition(tpos);
    Vec3::copy(obj->firstPosition, obj->position);
    obj->setQdirection(q);
    obj->snapped(snapableSide);
    moveWorldObjToTile(x, z, obj);
    obj->setMartix();
}

TRitem *Route::getTrackItem(int TID, int UID){
    if(TID == 0)
        return trackDB->trackItems[UID];
    if(TID == 1)
        return roadDB->trackItems[UID];
    return NULL;
}

void Route::deleteTrackItem(TRitem * item){
    if(item == NULL)
        return;
    unsigned int TID = item->tdbId;
    unsigned int UID = item->trItemId;
    if(TID == 0)
        return trackDB->deleteTrItem(UID);
    if(TID == 1)
        return roadDB->deleteTrItem(UID);
    return;
}

void Route::actPickNewEventLocation(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->pickNewEventLocation(tpos);
    }
}

void Route::actNewLooseConsist(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->newLooseConsist(tpos);
    }
}

void Route::actNewFailedSignal(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    
    
}

void Route::actNewNewSpeedZone(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->newSpeedZone(tpos);
    }
}

Tile * Route::requestTile(int x, int z, bool allowNew){
    Tile *tTile = tile[((x)*10000 + z)];
    if (tTile == NULL){
        tile[(x)*10000 + z] = new Tile(x, z);
        tTile = tile[(x)*10000 + z];
    }

    if(!allowNew)
        return tTile;

    if (tTile->loaded == -2) {
        if (Game::terrainLib->isLoaded(x, z)) {
            tTile->initNew();
        } else {
            return NULL;
        }
    }
    return tTile;
}

void Route::linkSignal(int x, int z, float* p, WorldObj* obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID != obj->signal)
        return;
    SignalObj* sobj = (SignalObj*)obj;
    float *tpos = new float[3];
    float* playerT = Vec2::fromValues(x, z);
    int ok = this->trackDB->findNearestPositionOnTDB(playerT, p, NULL, tpos);
    if(ok < 0) return;
    sobj->linkSignal(tpos[0], tpos[1]);
}

float *fromtwovectors(float* out, float* u, float* v){
    //float m = sqrt(2.f + 2.f * Vec3::dot(u, v));
    float w[3];
    /*Vec3::cross((float*)w, u, v);
    Vec3::scale((float*)w, (float*)w, (1.f / m));
    out[0] = 0.5f * m;
    out[1] = w[0];
    out[2] = w[1];
    out[3] = w[2];*/
    float cos_theta = Vec3::dot(u, v);
    float angle = acos(cos_theta);
    Vec3::cross(w, u, v);
    Vec3::normalize(w, w);
    Quat::setAxisAngle(out, w, angle);
    return out;
}

WorldObj* Route::autoPlaceObject(int x, int z, float* p, int mode) {
    if(ref->selected == NULL) return NULL;
    Game::check_coords(x, z, p);
    
    autoPlacementLastPlaced.clear();
    
    TDB * tdb = NULL;
    if(placementAutoTargetType == 0)
        tdb = this->trackDB;
    else if(placementAutoTargetType == 1)
        tdb = this->roadDB;
    else if(placementAutoTargetType == 2)
        tdb = this->trackDB;
    else
        return NULL;
    
    // pozycja wzgledem TDB:
    float* tpos = new float[3];
    float* playerT = Vec2::fromValues(x, z);
    int ok = tdb->findNearestPositionOnTDB(playerT, p, NULL, tpos);
    if(ok < 0) return NULL;
    
    x = playerT[0];
    z = playerT[1];
    int trackNodeIdx = tpos[0];
    int length = tdb->getVectorSectionLength(trackNodeIdx);
    int metry = 0;
    float drawPosition1[7];
    float drawPosition2[7];
    float xyz[3];
    float *quat = Quat::create();
    float *vec1 = Vec3::create();
    vec1[2] = -1.0;
    float *vec2 = Vec3::create();
    float step = placementAutoLength;
    float startPos = 0;
    float endPos = length;
    float rot = 0;
    if(mode == 1){
        startPos = tpos[1];
    }
    if(mode == 2){
        startPos = 0;
        endPos = tpos[1];
        rot = M_PI;
    }    
    float i1, i2;
    for(float i = startPos; i < endPos; i+=step ){
        if(mode == 2){
           i1 = endPos - i;
           i2 = i1-step;
           if(i2 < 0)
                i2 = 0 + 0.1;
        } else {
            i1 = i;
            i2 = i1+step;
            if(i2 > length)
                i2 = length - 0.1;
        }
        if(!tdb->getDrawPositionOnTrNode((float*)drawPosition1, trackNodeIdx, i1))
            return NULL;
        if(!tdb->getDrawPositionOnTrNode((float*)drawPosition2, trackNodeIdx, i2))
            return NULL;
        x = drawPosition1[5];
        z = -drawPosition1[6];
        
        /*x = currentPosition[5];
        z = -currentPosition[6];
        //vec1[0] = currentPosition[0];
        //vec1[1] = currentPosition[1];
        //vec1[2] = -currentPosition[2];
        vec2[0] = currentPosition1[0] - (currentPosition[0]-(currentPosition1[5]-currentPosition[5])*2048);
        vec2[1] = currentPosition1[1] - currentPosition[1];
        vec2[2] = -currentPosition1[2] + (currentPosition[2]-(currentPosition1[6]-currentPosition[6])*2048);
        Vec3::normalize(vec2, vec2);
        //Vec3::normalize(vec1, vec1);
        if(placementAutoTwoPointRot){
            fromtwovectors(quat, vec1, vec2);
            Quat::rotateY(quat, quat, rot);
        }else {
            Quat::fill(quat);
            Quat::rotateY(quat, quat, currentPosition[3]);
            Quat::rotateX(quat, quat, -currentPosition[4]);
        }
        */
        drawPosition2[0] += 2048*(drawPosition2[5]-drawPosition1[5]);
        drawPosition2[2] += 2048*(drawPosition2[6]-drawPosition1[6]);
        float dlugosc = Vec3::distance(drawPosition1, drawPosition2);

        int someval = (((drawPosition2[2]-drawPosition1[2])+0.00001f)/fabs((drawPosition2[2]-drawPosition1[2])+0.00001f));
        float rotY = ((float)someval+1.0)*(M_PI/2)+(float)(atan((drawPosition1[0]-drawPosition2[0])/(drawPosition1[2]-drawPosition2[2]))); 
        float sinv = (drawPosition1[1]-drawPosition2[1])/(dlugosc);
        if(sinv > 1.0f)
            sinv = 1.0f;
        if(sinv < -1.0f)
            sinv = -1.0f;
        float rotX = -(float)asin(sinv); 

        if(placementAutoTwoPointRot){
            Quat::fill(quat);
            Quat::rotateY(quat, quat, -rotY+M_PI);
            Quat::rotateX(quat, quat, rotX);
        }else {
            Quat::fill(quat);
            Quat::rotateY(quat, quat, drawPosition1[3]+rot);
            Quat::rotateX(quat, quat, -drawPosition1[4]);
        }
        
        float offset[3];
        Vec3::copy(offset, placementAutoTranslationOffset);
        float offsetq[4];
        Quat::fill(offsetq);
        Quat::rotateY(offsetq,offsetq,(placementAutoRotationOffset[1]*M_PI)/180);
        Quat::rotateX(offsetq,offsetq,(placementAutoRotationOffset[0]*M_PI)/180);
        
        Vec3::transformQuat(offset, offset, quat);
        Quat::multiply(quat, quat, offsetq);
        
        xyz[0] = drawPosition1[0] + offset[0];
        xyz[1] = drawPosition1[1] + offset[1];
        xyz[2] = -drawPosition1[2] + offset[2];      
        
        autoPlacementLastPlaced.push_back(placeObject(x, z, (float*) xyz, quat, 0, ref->selected));
    }

    return NULL;
    
}

void Route::fillWorldObjectsByTrackItemIds(QHash<int,QVector<WorldObj*>> &objects, int tdbId){
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->fillWorldObjectsByTrackItemIds(objects, tdbId);
        }
    }
}

void Route::fillWorldObjectsByTrackItemId(QVector<WorldObj*> &objects, int tdbId, int id){
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->fillWorldObjectsByTrackItemId(objects, tdbId, id);
        }
    }
}

void Route::findSimilar(WorldObj* obj, GroupObj* group, float *playerT, int tileRadius){
    if(obj->typeID == WorldObj::groupobject)
        return;
    int mintile = -tileRadius;
    int maxtile = tileRadius;
    
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            if (tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                tTile->findSimilar(obj, group);
            }
        }
    }
}

void Route::autoPlacementDeleteLast(){
    for(int i = 0; i < autoPlacementLastPlaced.length(); i++){
        deleteObj(autoPlacementLastPlaced[i]);
    }
    autoPlacementLastPlaced.clear();
}


void Route::replaceWorldObjPointer(WorldObj* o, WorldObj* n){
    if(o->typeObj != WorldObj::worldobj)
        return;
    if(n->typeObj != WorldObj::worldobj)
        return;
    int x = o->x;
    int z = o->y;
    
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return;
    
    for(int i = 0; i < tTile->jestObiektow; i++){
        if(tTile->obiekty[i] == NULL) continue;
        if(tTile->obiekty[i]->UiD == o->UiD){
            tTile->obiekty[i] = n;
            emit objectSelected((GameObj*)n);
            return;
        }
    }
}

WorldObj* Route::makeFlexTrack(int x, int z, float* p) {
    float qe[4];
    qe[0] = 0;
    qe[1] = 0;
    qe[2] = 0;
    qe[3] = 1;
    this->trackDB->findNearestNode(x, z, p,(float*) &qe);
    float* dyntrackData[10];
    bool success = Flex::NewFlex(x, z, p, (float*)qe, (float*)dyntrackData);
    if(!success) return NULL;
    
    Ref::RefItem r;
    r.type = "dyntrack";
    r.value = -1;
    qDebug() << "1";
    qe[0] = 0;
    qe[1] = 0;
    qe[2] = 0;
    qe[3] = 1;
    DynTrackObj* track = (DynTrackObj*)placeObject(x, z, p, (float*)&qe, 0, &r);
    if(track != NULL){
        qDebug() << "2";
        QString sh = "dyntrackdata";
        track->set(sh, (float*)dyntrackData);
    }
    return track;
}

void Route::addToTDB(WorldObj* obj) {
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    int x = obj->x;//post[0];
    int z = obj->y;//post[1];
    float p[3];
    //p[0] = pos[0];
    //p[1] = pos[1];
    //p[2] = pos[2];
    p[0] = obj->position[0];
    p[1] = obj->position[1];
    p[2] = obj->position[2];
    //Game::check_coords(x, z, (float*) &p);
    float q[4];
    //q[0] = obj->tRotation[0]; //track->qDirection[0];
    //q[1] = obj->tRotation[1]; //qDirection[1];
    //q[2] = 0; //track->qDirection[2];
    //q[3] = 1; //track->qDirection[3];
    q[0] = obj->qDirection[0];
    q[1] = obj->qDirection[1];
    q[2] = obj->qDirection[2];
    q[3] = obj->qDirection[3];
    
    if (obj->type == "trackobj") {
        TrackObj* track = (TrackObj*) obj;
        //this->trackDB->placeTrack(x, z, p, q, r, nowy->UiD);
        //float scale = (float) sqrt(track->qDirection[0] * track->qDirection[0] + track->qDirection[1] * track->qDirection[1] + track->qDirection[2] * track->qDirection[2]);
        //float elevation = ((track->qDirection[0] + 0.0000001f) / fabs(scale + 0.0000001f))*(float) -acos(track->qDirection[3])*2;
        //float elevation = -3.14/16.0;
        //q[0] = elevation;
        if(this->tsection->isRoadShape(track->sectionIdx))
            this->roadDB->placeTrack(x, z, (float*) &p, (float*) &q, track->sectionIdx, obj->UiD);
        else
            this->trackDB->placeTrack(x, z, (float*) &p, (float*) &q, track->sectionIdx, obj->UiD, &track->jNodePosn);
        //obj->setPosition(p);
        //obj->setQdirection(q);
        //obj->setMartix();
        //track->setJNodePosN();
    } else if(obj->type == "dyntrack"){
        Undo::Clear();
        DynTrackObj* dynTrack = (DynTrackObj*) obj;
        if(dynTrack->sectionIdx == -1){
            this->trackDB->fillDynTrack(dynTrack);
        }
        this->trackDB->placeTrack(x, z, (float*) &p, (float*) &q, dynTrack->sectionIdx, obj->UiD);
        obj->setPosition(p);
        obj->setQdirection(q);
        obj->setMartix();
    } 
}

void Route::setTDB(TDB* tdb, bool road){
    if(tdb == NULL)
        return;
    if(road){
        delete this->roadDB;
        this->roadDB = tdb;
        Game::roadDB = tdb;
    } else {
        delete this->trackDB;
        this->trackDB = tdb;
        Game::trackDB = tdb;
    }
}

void Route::toggleToTDB(WorldObj* obj) {
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            toggleToTDB(gobj->objects[i]);
        }
        return;
    }
    
    if (obj->type != "trackobj" && obj->type != "dyntrack") {
            return;
    }
    if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
        removeTrackFromTDB(obj);
        return;
    }
    addToTDB(obj);
}

void Route::addToTDBIfNotExist(WorldObj* obj) {
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            addToTDBIfNotExist(gobj->objects[i]);
        }
        return;
    }
    
    if (obj->type != "trackobj" && obj->type != "dyntrack") {
            return;
    }
    if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
        return;
    }
    
    Undo::StateBegin();
    Undo::PushTrackDB(trackDB, false);
    Undo::PushTrackDB(roadDB, true);
    Undo::StateEnd();
    
    addToTDB(obj);
}

void Route::newPositionTDB(WorldObj* obj) {
    if(obj->typeObj != WorldObj::worldobj)
        return;
    int x = obj->x;//post[0];
    int z = obj->y;//post[1];
    float p[3]; 
    p[0] = obj->firstPosition[0];
    p[1] = obj->firstPosition[1];
    p[2] = obj->firstPosition[2];
    Game::check_coords(x, z, (float*) &p);

    if (obj->type == "trackobj") {
        float q[4];
        q[0] = 0;
        q[1] = 0;
        q[2] = 0;
        q[3] = 1;
        TrackObj* track = (TrackObj*) obj;
        //this->trackDB->placeTrack(x, z, p, q, r, nowy->UiD);
        if(this->tsection->isRoadShape(track->sectionIdx))
            this->roadDB->findPosition(x, z, (float*) &p, (float*) &q, track->endp, track->sectionIdx);
        else
            this->trackDB->findPosition(x, z, (float*) &p, (float*) &q, track->endp, track->sectionIdx);

        //Vec3::copy(obj->position, p);
        obj->setPosition(p);
        obj->setQdirection(q);
        obj->setMartix();
        
        moveWorldObjToTile(x, z, obj);
    }
}

void Route::moveWorldObjToTile(int x, int z, WorldObj* obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    //qDebug() << "new tile" << obj->x <<" "<< obj->y<<" "<< obj->position[0]<<" "<< -obj->position[2];
    float oldPos[3];
    int xx = x, zz = z;
    Vec3::copy(oldPos, obj->position);
    Game::check_coords(xx, zz, oldPos);
    if(xx == obj->x && zz == obj->y)
        return;
    Vec3::copy(obj->position, oldPos);
    x = xx;
    z = zz;
    
    qDebug() << "obj outside tile border !!!";
    //qDebug() << "new tile" << x <<" "<< z;
    qDebug() << "new tile" << xx <<" "<< zz <<" "<< obj->position[0]<<" "<< -obj->position[2];

    
    Undo::Clear();
    
    Tile *tTile = tile[((obj->x)*10000 + obj->y)];
    tTile->deleteObject(obj);
    
    tTile = requestTile(x, z);
    if(tTile == NULL) return;
    if(tTile->loaded != 1) return;
    
    if (tTile->loaded == 1) {
        obj->firstPosition[0] -= (x-obj->x)*2048;
        obj->firstPosition[2] -= (z-obj->y)*2048;
        obj->placedAtPosition[0] = obj->position[0];
        obj->placedAtPosition[2] = obj->position[2];
        tTile->placeObject(obj);
    }
    qDebug() << "--" << obj->x <<" "<< obj->y<<" "<< obj->position[0]<<" "<< -obj->position[2];
}

void Route::deleteTDBTree(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        this->roadDB->deleteTree(obj->x, obj->y, obj->UiD);
        this->trackDB->deleteTree(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::fixTDBVectorElevation(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        //this->roadDB->fixTDBVectorElevation(obj->x, obj->y, obj->UiD);
        this->trackDB->fixTDBVectorElevation(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::deleteTDBVector(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        this->roadDB->deleteVectorSection(obj->x, obj->y, obj->UiD);
        this->trackDB->deleteVectorSection(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::undoPlaceObj(int x, int y, int UiD){
    Tile *tTile;
    tTile = tile[((x)*10000 + y)];
    if (tTile == NULL)
        return;
    
    for(int i = 0; i < tTile->jestObiektow; i++){
        if(tTile->obiekty[i] == NULL) continue;
        if(tTile->obiekty[i]->UiD == UiD){
            tTile->obiekty[i]->loaded = false;
            tTile->obiekty[i]->modified = false;
            tTile->obiekty[i]->UiD = -1;
            emit sendMsg("unselect");
            return;
        }
    }
}

void Route::deleteObj(WorldObj* obj) {
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            deleteObj(gobj->objects[i]);
        }
        return;
    }
    
    Undo::PushWorldObjRemoved(obj);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
        removeTrackFromTDB(obj);
        if(Game::leaveTrackShapeAfterDelete)
            return;
    }
    
    obj->loaded = false;
    obj->setModified();
    if (obj->isTrackItem()) {
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
        obj->deleteTrItems();
    }
    Tile *tTile;
    tTile = tile[((obj->x)*10000 + obj->y)];
    if (tTile != NULL)
        tTile->jestHiddenObj++;
}

void Route::removeTrackFromTDB(WorldObj* obj) {
    if(obj->typeObj != WorldObj::worldobj)
        return;
    bool ok;
    ok = this->roadDB->removeTrackFromTDB(obj->x, obj->y, obj->UiD);
    ok |= this->trackDB->removeTrackFromTDB(obj->x, obj->y, obj->UiD);
    if(ok)
        obj->removedFromTDB();
}

int Route::getTileObjCount(int x, int z) {
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return 0;
    return tTile->jestObiektow;
}

int Route::getTileHiddenObjCount(int x, int z) {
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return 0;
    return tTile->jestHiddenObj;
}

void Route::getUnsavedInfo(QVector<QString> &items){
    if (!Game::writeEnabled) return;
    
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1 && tTile->isModified()) {
            items.push_back("[W] "+QString::number(tTile->x)+" "+QString::number(-tTile->z));
        }
    }
    Game::terrainLib->getUnsavedInfo(items);
    if(this->trk->isModified())
        items.push_back("[S] Route Settings - TRK File");
    
    ActLib::GetUnsavedInfo(items);
    
    /*foreach(Service *s, service){
        if(s == NULL)
            continue;
        if(s->isModified())
            items.push_back("[S] "+s->name);
    }
    foreach(Path *p, path){
        if(p == NULL)
            continue;
        if(p->isModified())
            items.push_back("[P] "+p->name);
    }*/
    //this->trackDB->save();
    //this->roadDB->save();
}


void Route::save() {
    if (!Game::writeEnabled) return;
    qDebug() << "save";
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1 && tTile->isModified()) {
            tTile->save();
            tTile->setModified(false);
        }
    }
    Game::terrainLib->save();
    this->trackDB->save();
    this->roadDB->save();
    this->trk->save();
    ActLib::SaveAll();
    /*foreach(Service *s, service){
        if(s == NULL)
            continue;
        if(s->isModified())
            s->save();
    }
    foreach(Path *p, path){
        if(p == NULL)
            continue;
        if(p->isModified())
            p->save();
    }*/
}

void Route::createNewPaths() {
    if (!Game::writeEnabled) return;
    Path::CreatePaths(this->trackDB);
}

QMap<QString, Coords*> Route::getMkrList(){
    return this->mkrList;
}

void Route::nextDefaultEnd(){
    this->trackDB->nextDefaultEnd();
    this->roadDB->nextDefaultEnd();
}

void Route::flipObject(WorldObj *obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack ){
        nextDefaultEnd();
        newPositionTDB(obj);                
    } else {
        obj->flip();
    }
                
}

void Route::paintHeightMap(Brush* brush, int x, int z, float* p){
    Game::ignoreLoadLimits = true;
    QSet<Terrain*> modifiedTiles = Game::terrainLib->paintHeightMap(brush, x, z, p);
    Tile *ttile;

    QSet<int> tileIds;
    foreach (Terrain *value, modifiedTiles){
        value->getWTileIds(tileIds);
    }
    foreach (int value, tileIds){
        ttile = tile[value];
        if(ttile != NULL){
            ttile->updateTerrainObjects();
        }
    }
}

void Route::createNew() {
    if (!Game::writeEnabled) return;

    QString path;

    path = Game::root + "/routes/" + Game::route;
    if (QDir(path).exists()) {
        qDebug() << "route folder exist - aborting";
        return;
    }
    QDir().mkdir(path);
    QDir().mkdir(path + "/envfiles");
    QDir().mkdir(path + "/envfiles/textures");
    QDir().mkdir(path + "/paths");
    QDir().mkdir(path + "/shapes");
    QDir().mkdir(path + "/sound");
    QDir().mkdir(path + "/textures");
    QDir().mkdir(path + "/terrtex");
    QDir().mkdir(path + "/tiles");
    QDir().mkdir(path + "/td");
    QDir().mkdir(path + "/world");

    int x = Game::newRouteX;
    int z = Game::newRouteZ;
    
    Trk * newTrk = new Trk();
    newTrk->idName = Game::route;
    newTrk->routeName = Game::route;
    newTrk->displayName = Game::route;
    newTrk->startTileX = Game::newRouteX;
    newTrk->startTileZ = Game::newRouteZ;
    showTrkEditr(newTrk);
    newTrk->save();
    
    TDB::saveEmpty(false);
    TDB::saveEmpty(true);
    Game::terrainLib->createNewRouteTerrain(x, z);
    Tile::saveEmpty(x, z);
    //Terrain::saveEmpty(x, z);

    QString templateDir = "templateroute_0.6/";
    QString res = QString("tsre_assets/templateroute_0.6/");//+templateDir;
    path += "/";

    QFile::copy(res + "sigcfg.dat", path + "sigcfg.dat");
    QFile::copy(res + "sigscr.dat", path + "sigscr.dat");
    QFile::copy(res + "ttype.dat", path + "ttype.dat");
    QFile::copy(res + "template.ref", path + Game::route + ".ref");
    QFile::copy(res + "carspawn.dat", path + "carspawn.dat");
    QFile::copy(res + "deer.haz", path + "deer.haz");
    QFile::copy(res + "forests.dat", path + "forests.dat");
    QFile::copy(res + "speedpost.dat", path + "speedpost.dat");
    QFile::copy(res + "spotter.haz", path + "spotter.haz");
    QFile::copy(res + "ssource.dat", path + "ssource.dat");
    QFile::copy(res + "telepole.dat", path + "telepole.dat");

    FileFunctions::copyFiles(res + "envfiles", path + "envfiles");
    FileFunctions::copyFiles(res + "envfiles/textures", path + "envfiles/textures");
    FileFunctions::copyFiles(res + "shapes", path + "shapes");
    FileFunctions::copyFiles(res + "sound", path + "sound");
    FileFunctions::copyFiles(res + "terrtex", path + "terrtex");
    FileFunctions::copyFiles(res + "textures", path + "textures");
    
    Texture *graphicTexture = new Texture(200,150,24);
    AceLib::save(path + "graphic.ace", graphicTexture);
}

void Route::reloadTile(int x, int z) {
    tile[x * 10000 + z] = new Tile(x, z);
    return;
}

int Route::newTile(int x, int z, bool forced) {
    if (!Game::writeEnabled) return 0;
    
    if (tile[x*10000 + z] == NULL)
        tile[x*10000 + z] = new Tile(x, z);
    
    if(!forced)
        if (tile[x*10000 + z]->loaded == 1)
            return 1;
            
    Tile::saveEmpty(x, -z);
    //Terrain::saveEmpty(x, -z);
    Game::terrainLib->saveEmpty(x, -z);
    Game::terrainLib->reload(x, z);
    reloadTile(x, z);

    if(Game::autoGeoTerrain){
        float pos[3];
        Vec3::set(pos, 0, 0, 0);
        Game::terrainLib->setHeightFromGeo(x, z, (float*)&pos);
    }
    
    return 2;
}

void Route::showTrkEditr(Trk * val){
    TrkWindow trkWindow;
    if(val == NULL)
        val = this->trk;
    trkWindow.trk = val;
    trkWindow.exec();
}