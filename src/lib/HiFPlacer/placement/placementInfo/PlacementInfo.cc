/**
 * @file PlacementInfo.cc
 * @author Tingyuan LIANG (tliang@connect.ust.hk)
 * @brief This implementation file contains APIs to initialize and process the information related to FPGA
 * placement (wirelength optimization, cell spreading, legalization, packing)
 * @version 0.1
 * @date 2021-06-03
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "PlacementInfo.h"
#include "readZip.h"
#include "strPrint.h"
#include "stringCheck.h"
#include <iomanip>
#include <assert.h>
#include <cmath>
#include <queue>

PlacementInfo::PlacementInfo(DesignInfo *designInfo, DeviceInfo *deviceInfo,
                             std::map<std::string, std::string> &JSONCfg)
    : designInfo(designInfo), deviceInfo(deviceInfo), JSONCfg(JSONCfg)
{

	// ***hard code***
    // cellType2fixedAmoFileName = std::string(JSONCfg["cellType2fixedAmo file"]);
    // cellType2sharedCellTypeFileName = std::string(JSONCfg["cellType2sharedCellType file"]);
    // sharedCellType2BELtypeFileName = std::string(JSONCfg["sharedCellType2BELtype file"]);

    if (JSONCfg.find("y2xRatio") != JSONCfg.end())
    {
        y2xRatio = std::stof(JSONCfg["y2xRatio"]);
    }

    if (JSONCfg.find("useLookaheadTech") != JSONCfg.end())
    {
        useLookaheadTech = JSONCfg["useLookaheadTech"] == "true";
    }

    print_status("Loading compatiblePlacementTable");
    compatiblePlacementTable = loadCompatiblePlacementTable(cellType2fixedAmoFileName, cellType2sharedCellTypeFileName,
                                                            sharedCellType2BELtypeFileName);
    globalMinX = getMinXFromSites(deviceInfo->getSites());
    globalMinY = getMinYFromSites(deviceInfo->getSites());
    globalMaxX = getMaxXFromSites(deviceInfo->getSites());
    globalMaxY = getMaxYFromSites(deviceInfo->getSites());

    cellId2PlacementUnit.clear();
    placementUnits.clear();
    placementMacros.clear();
    cellInMacros.clear();
    placementNets.clear();
    placementUnpackedCells.clear();

    simplePlacementTimingInfo = new PlacementTimingInfo(designInfo, deviceInfo, JSONCfg);

    print_status("New Placement Info Created.");
}

PlacementInfo::CompatiblePlacementTable::CompatiblePlacementTable(std::string cellType2fixedAmoFileName,
                                                                  std::string cellType2sharedCellTypeFileName,
                                                                  std::string sharedCellType2BELtypeFileName,
                                                                  DesignInfo *designInfo, DeviceInfo *deviceInfo)
    : designInfo(designInfo), deviceInfo(deviceInfo)
{

    realBELTypes.clear();
    realBELTypeName2ID.clear();
    sharedCellBELTypes.clear();
    sharedCellBELTypeName2ID.clear();
    cellType2sharedBELTypeOccupation.clear();
    cellType2sharedBELTypes.clear();
    cellType2sharedBELTypeIDs.clear();
    sharedCellType2SiteType.clear();
    sharedCellType2BELNames.clear();
    

    // get the list of BEL type
    std::set<std::string>::iterator typeIt;
    for (typeIt = deviceInfo->getBELTypes().begin(); typeIt != deviceInfo->getBELTypes().end(); typeIt++)
    {
        realBELTypeName2ID[*typeIt] = realBELTypes.size();
        realBELTypes.push_back(*typeIt);
    }

    if(designInfo->JSONCfg.find("useLookaheadTech") != designInfo->JSONCfg.end())
    {
        useLookaheadTech = designInfo->JSONCfg["useLookaheadTech"] == "true";
    }

    // load cell occupation of BELs
    // std::ifstream infile0(cellType2fixedAmoFileName.c_str());
	std::vector<std::pair<std::string, int>> cellType2fixedAmoVec{
		{"CARRY8", 1},
		{"LUT1", 1},
		{"LUT2", 1},
		{"LUT3", 1},
		{"LUT4", 1},
		{"LUT5", 1},
		{"LUT6", 1},
		{"FDRE", 1},
		{"FDSE", 1},
		{"RAMB36E2", 2},
		{"DSP48E2", 1},
		{"IBUF", 1},
		{"OBUF", 1},
		{"BUFGCE", 1}
	};
    std::string line;

    std::string cellTypeStr;
    int occupationAmo;
    std::string CompatiblePlacementTableLoading = "CompatiblePlacementTableLoading";

    // while (std::getline(infile0, line))
    // {
    //     std::istringstream iss(line);
        // iss >> cellTypeStr >> occupationAmo;
	for (auto &elem : cellType2fixedAmoVec) {
        cellTypeStr = elem.first; occupationAmo = elem.second;
        DesignInfo::DesignCellType cellType =
            designInfo->fromStringToCellType(CompatiblePlacementTableLoading, cellTypeStr);
        cellType2sharedBELTypeOccupation[cellType] = occupationAmo;
    }

    for (auto &tmppair : designInfo->getType2Cells())
    {
        if (cellType2sharedBELTypeOccupation.find(tmppair.first) == cellType2sharedBELTypeOccupation.end())
        {
            std::cout << "cellType: " << tmppair.first
                      << " is not defined with resource demand. Check your compatibale table setting.\n";
        }
        assert(cellType2sharedBELTypeOccupation.find(tmppair.first) != cellType2sharedBELTypeOccupation.end() &&
               "each type should be defined with resource demand.");
    }

    // load cell mapping to BEL shared virtual type
    // std::ifstream infile1(cellType2sharedCellTypeFileName.c_str());
	std::vector<std::pair<std::string, std::string>> cellType2sharedCellTypeVec {
		{"CARRY8", "SLICEL_CARRY8,SLICEM_CARRY8"},
		{"LUT1,LUT2,LUT3,LUT4,LUT5,LUT6", "SLICEL_LUT,SLICEM_LUT"},
		{"FDRE,FDSE", "SLICEL_FF,SLICEM_FF"},
		{"RAMB36E2", "RAMB18E2_L,RAMB18E2_U"},
		{"DSP48E2", "DSP_ALU"},
		{"IBUF", "HPIOB_INPUT,HRIO_INPUT"},
		{"OBUF", "HPIOB_OUTPUT,HRIO_OUTPUT"},
		{"BUFGCE", "BUFCE"}
	};
    std::string cell2BELType;

    std::set<std::string> involvedSharedBELStr;
    involvedSharedBELStr.clear();


    // while (std::getline(infile1, line))
    // {
    //     std::istringstream iss(line);
    //     iss >> cellTypeStr >> cell2BELType;
	for (auto &elem : cellType2sharedCellTypeVec) {
        cellTypeStr = elem.first; cell2BELType = elem.second;
        std::vector<std::string> cellTypeStrVec;
        strSplit(cellTypeStr, cellTypeStrVec, ",");
        std::vector<std::string> BELTypeStrVec;
        strSplit(cell2BELType, BELTypeStrVec, ",");

        for (auto tmpCellTypeStr : cellTypeStrVec)
        {
            DesignInfo::DesignCellType cellType =
                designInfo->fromStringToCellType(CompatiblePlacementTableLoading, tmpCellTypeStr);
            cellType2sharedBELTypes[cellType] = BELTypeStrVec;
        }

        for (auto tmpSharedBELStr : BELTypeStrVec)
        {
            involvedSharedBELStr.insert(tmpSharedBELStr);
            if (sharedCellBELTypeName2ID.find(tmpSharedBELStr) == sharedCellBELTypeName2ID.end())
            {
                sharedCellBELTypeName2ID[tmpSharedBELStr] = sharedCellBELTypes.size();
                sharedCellBELTypes.push_back(tmpSharedBELStr);
            }
        }
        for (auto tmpCellTypeStr : cellTypeStrVec)
        {
            DesignInfo::DesignCellType cellType =
                designInfo->fromStringToCellType(CompatiblePlacementTableLoading, tmpCellTypeStr);
            if (cellType2sharedBELTypeIDs.find(cellType) == cellType2sharedBELTypeIDs.end())
            {
                cellType2sharedBELTypeIDs[cellType] = std::vector<int>();
                for (auto tmpSharedBELStr : BELTypeStrVec)
                    cellType2sharedBELTypeIDs[cellType].push_back(sharedCellBELTypeName2ID[tmpSharedBELStr]);
            }
        }
    }

    // load BEL shared virtual type to real compatible BEL type
    // std::ifstream infile2(sharedCellType2BELtypeFileName.c_str());
	std::vector<std::vector<std::string>> sharedCellType2BELtypeVec {
		{"SLICEL_CARRY8", "SLICEL", "CARRY8"},
		{"SLICEM_CARRY8", "SLICEM", "CARRY8"},
		{"SLICEM_LUT", "SLICEM", "A5LUT,A6LUT,B5LUT,B6LUT,C5LUT,C6LUT,D5LUT,D6LUT,E5LUT,E6LUT,F5LUT,F6LUT,G5LUT,G6LUT,H5LUT,H6LUT"},
		{"SLICEL_LUT", "SLICEL", "A5LUT,A6LUT,B5LUT,B6LUT,C5LUT,C6LUT,D5LUT,D6LUT,E5LUT,E6LUT,F5LUT,F6LUT,G5LUT,G6LUT,H5LUT,H6LUT"},
		{"SLICEL_FF", "SLICEL", "AFF,AFF2,BFF,BFF2,CFF,CFF2,DFF,DFF2,EFF,EFF2,FFF,FFF2,GFF,GFF2,HFF,HFF2"},
		{"SLICEM_FF", "SLICEM", "AFF,AFF2,BFF,BFF2,CFF,CFF2,DFF,DFF2,EFF,EFF2,FFF,FFF2,GFF,GFF2,HFF,HFF2"},
		{"DSP_ALU", "DSP48E2", "DSP_ALU"},
		{"RAMB18E2_L", "RAMBFIFO18", "RAMB18E2_L"},
		{"RAMB18E2_U", "RAMB181", "RAMB18E2_U"},
		{"HPIOB_INPUT", "HPIOB_M", "INBUF"},
		{"HRIO_INPUT", "HPIOB_S", "INBUF"},
		{"HPIOB_OUTPUT", "HPIOB_M", "OUTBUF"},
		{"HRIO_OUTPUT", "HPIOB_S", "OUTBUF"},
		{"BUFCE", "BUFGCE", "BUFCE"}
	};
    std::string compatibleBELTypeStrs, sharedCellTypeStr, siteTypeStr;

    // while (std::getline(infile2, line))
    // {
    //     std::istringstream iss(line);
    //     iss >> sharedCellTypeStr >> siteTypeStr >> compatibleBELTypeStrs;
	for (auto &elem : sharedCellType2BELtypeVec) {
        sharedCellTypeStr = elem[0]; siteTypeStr = elem[1]; compatibleBELTypeStrs = elem[2];
        std::vector<std::string> CompatibleBELStrVec;
        strSplit(compatibleBELTypeStrs, CompatibleBELStrVec, ",");

        sharedCellType2SiteType[sharedCellTypeStr] = siteTypeStr;
        sharedCellType2BELNames[sharedCellTypeStr] = CompatibleBELStrVec;
        print_info("There are " + std::to_string(CompatibleBELStrVec.size()) + " slot(s) in a site for cell type : (" +
                   sharedCellTypeStr + "). They are :" + compatibleBELTypeStrs);

        for(int i=0; i < CompatibleBELStrVec.size(); i++)
        {
            std::string belStr = CompatibleBELStrVec[i];
            BELName2sharedCellType[belStr] = sharedCellTypeStr;
        }
    }

    for (auto it = involvedSharedBELStr.begin(); it != involvedSharedBELStr.end(); it++)
    {
        assert(sharedCellType2SiteType.find(*it) != sharedCellType2SiteType.end());
    }

    print_status("New Compatible Placement Table Loaded.");
}

void PlacementInfo::CompatiblePlacementTable::setBELTypeForCells(DesignInfo *designInfo)
{
    cellId2SharedCellBELTypeID.resize(designInfo->getNumCells());
    cellId2Occupation.resize(designInfo->getNumCells());
    cellId2InfationRatio.resize(designInfo->getNumCells());

    // cellType statistic
    // static const char *DesignCellTypeStr_const[] = {CELLTYPESTRS};
    // std::vector<std::string> DesignCellTypeStr{CELLTYPESTRS};
    // std::vector<int> cellTypeCounts(DesignCellTypeStr.size(), 0);
    // std::cout << "Type num: " << cellTypeCounts.size() << std::endl;
    // for (auto cell : designInfo->getCells())
    // {
    //     cellTypeCounts[cell->getCellType()] ++;
    //     // std::cout << "id: " << cell->getCellId() << " name: " << cell->getName() << " " << " type: " << cell->getCellType() \
    //     //           << " " << DesignCellTypeStr_const[cell->getCellType()] << std::endl; 
    // }
    // for (int i = 0; i < cellTypeCounts.size(); i ++) {
    //     if (cellTypeCounts[i] != 0) {
    //         std::cout << "type id: " << i << " type: " << DesignCellTypeStr_const[i] << " " << " freq: " << cellTypeCounts[i] << std::endl;
    //     }
    // }

    for (auto cell : designInfo->getCells())
    {
        assert(cellId2SharedCellBELTypeID[cell->getCellId()].size() == 0);

        cellId2Occupation[cell->getCellId()] = cellType2sharedBELTypeOccupation[cell->getCellType()];
        cellId2InfationRatio[cell->getCellId()] = 1.0;
        for (auto sharedBELTypeStr : cellType2sharedBELTypes[cell->getCellType()])
        {
            // map some BEL type to the same BEL id tomporarily for cell spreading, e.g., SLICEM_LUT -> SLICEL_LUT
            int sharedBELTypeID = sharedCellBELTypeName2ID[deviceInfo->getBELType2FalseBELType(sharedBELTypeStr)];
            cellId2SharedCellBELTypeID[cell->getCellId()].push_back(sharedBELTypeID);
        }
        static const char *DesignCellTypeStr_const[] = {CELLTYPESTRS};
        if (cellId2SharedCellBELTypeID[cell->getCellId()].size() == 0) {
            std::cout << "id: " << cell->getCellId() << " name: " << cell->getName() << " " << " type: " << cell->getCellType() \
                      << " " << DesignCellTypeStr_const[cell->getCellType()] << " bel: " << cellType2sharedBELTypes[cell->getCellType()].size() << std::endl; 
        }

        // look ahead considerations
        if(useLookaheadTech && cell->getRegionConstrType()!=-1)
        {
            int regionconstrType = cell->getRegionConstrType();
            std::string sharedBELType = cellType2sharedBELTypes[cell->getCellType()][0];
            //int cellOccupy = cellType2sharedBELTypeOccupation[cell->getCellType()];
            DesignInfo::DesignRegionConstr* regionconstr = designInfo->GetRegionalConstraint(regionconstrType);
            regionconstr->addNode(cell, sharedBELType, true);

        }

        assert(cellId2SharedCellBELTypeID[cell->getCellId()].size() != 0);
    }
    defaultCellId2Occupation = cellId2Occupation;

    //Add corresponding bel to the corresponding region
    for(auto site : deviceInfo->getSites())
    {
        float locX = site->X();
        float locY = site->Y();
        float left_right_slack = 0.5;
        float up_down_slack = 1.0;
        std::string sitetype = site->getSiteType();
        if(sitetype.find("RAMB") != sitetype.npos)
            up_down_slack = 5.0;
        else
        {
            if(sitetype.find("DSP") != sitetype.npos)
                up_down_slack = 2.5;
        }


        for(auto regionconstr : designInfo->GetRegionConstraintAll())
        {
            if(regionconstr->IsinRegionConstraint(locX, locY, left_right_slack, up_down_slack))
            {
                for(auto belpertype : site->getBELNumPerType())
                {
                    std::string beltype = belpertype.first;
                    int beltypenum = belpertype.second;
                    std::string sharedbeltype = deviceInfo->getBELType2FalseBELType(BELName2sharedCellType[beltype]);
                    if(sharedbeltype.size()>0)
                    {
                        //if((sharedbeltype.compare("RAMB18E2_L") == 0)&&(site->getSiteY() % 2 != 0))
                        //    continue;
                        regionconstr->addSite(sharedbeltype, beltypenum);
                    }
                }
            }
        }
    }

    // for(auto regionconstr : designInfo->GetRegionConstraintAll())
    // {
    //     std::cout<<"RegionConstrain "<<regionconstr->GetRegionConstrId()<<std::endl;
    //     std::cout<<"BEL Occupy: "<<std::endl;
    //     for(auto &beloccupy : regionconstr->GetBELOccupy())
    //     {
    //         std::cout<<"("<<beloccupy.first<<','<<beloccupy.second<<")"<<std::endl;
    //     }
    //     std::cout<<"BEL Supply: "<<std::endl;
    //     for(auto &belsupply : regionconstr->GetBELSupply())
    //     {
    //         std::cout<<"("<<belsupply.first<<','<<belsupply.second<<")"<<std::endl;
    //     }
    // }

    print_status("set BEL type for each cell");
}

void PlacementInfo::CompatiblePlacementTable::resetCellOccupationToDefault()
{
    cellId2Occupation = defaultCellId2Occupation;
    for (unsigned int i = 0; i < cellId2Occupation.size(); i++)
    {
        cellId2InfationRatio[i] = 1.0;
    }
}

float PlacementInfo::getMinXFromSites(std::vector<DeviceInfo::DeviceSite *> &sites)
{
    float minX = 100000;
    for (auto curSite : sites)
    {
        if (curSite->X() < minX)
            minX = curSite->X();
    }
    return minX;
}

float PlacementInfo::getMinYFromSites(std::vector<DeviceInfo::DeviceSite *> &sites)
{
    float minY = 100000;
    for (auto curSite : sites)
    {
        if (curSite->Y() < minY)
            minY = curSite->Y();
    }
    return minY;
}

float PlacementInfo::getMaxXFromSites(std::vector<DeviceInfo::DeviceSite *> &sites)
{
    float maxX = -100000;
    for (auto curSite : sites)
    {
        if (curSite->X() > maxX)
            maxX = curSite->X();
    }
    return maxX;
}

float PlacementInfo::getMaxYFromSites(std::vector<DeviceInfo::DeviceSite *> &sites)
{
    float maxY = -100000;
    for (auto curSite : sites)
    {
        if (curSite->Y() > maxY)
            maxY = curSite->Y();
    }
    return maxY;
}

void PlacementInfo::printStat(bool verbose)
{
    print_info("globalMinX: " + std::to_string(globalMinX));
    print_info("globalMinY: " + std::to_string(globalMinY));
    print_info("globalMaxX: " + std::to_string(globalMaxX));
    print_info("globalMaxY: " + std::to_string(globalMaxY));
    print_info("Total Placement Unit(s): " + std::to_string(placementUnits.size()));
    print_info("Total Unpacked Placement Unit(s): " + std::to_string(designInfo->getNumCells() - cellInMacros.size()));
    print_info("Total Macro Placement Unit(s): " +
               std::to_string(placementUnits.size() - (designInfo->getNumCells() - cellInMacros.size())));
}

void PlacementInfo::PlacementBinInfo::addSiteIntoBin(DeviceInfo::DeviceSite *curSite)
{
    if (inRange(curSite->X(), curSite->Y()))
    {
        correspondingSites.push_back(curSite);
        capacity += compatiblePlacementTable->sharedCellType2BELNames[sharedCellType].size();
    }
    else
    {
        print_error("leftX: " + std::to_string(leftX) + "rightX: " + std::to_string(rightX) +
                    "topY: " + std::to_string(topY) + "bottomY: " + std::to_string(bottomY));
        print_error("curSite: " + curSite->getName());
        assert(false && "some sites are out of the scope");
    }
}

void PlacementInfo::createGridBins(float _binWidth, float _binHeight)
{
    startX = round(globalMinX) - deviceInfo->getBoundaryTolerance();
    startY = round(globalMinY) - deviceInfo->getBoundaryTolerance();
    endX = round(globalMaxX) + deviceInfo->getBoundaryTolerance();
    endY = round(globalMaxY) + deviceInfo->getBoundaryTolerance();
    eps = 1e-6;
    binWidth = _binWidth;
    binHeight = _binHeight;

    if (SharedBELTypeBinGrid.size())
    {
        for (auto &tmpBinGrid : SharedBELTypeBinGrid)
        {
            for (auto &tmpRow : tmpBinGrid)
                for (auto &curBin : tmpRow)
                {
                    delete curBin;
                }
        }
    }
    if (LUTFFBinGrid.size())
    {
        for (auto &tmpRow : LUTFFBinGrid)
            for (auto &curBin : tmpRow)
            {
                delete curBin;
            }
    }
    if (globalBinGrid.size())
    {
        for (auto &tmpRow : globalBinGrid)
            for (auto &curBin : tmpRow)
            {
                delete curBin;
            }
    }

    SharedBELTypeBinGrid.clear();
    globalBinGrid.clear();

    int i = 0, j = 0;
    float curBottomY, curLeftX;
    for (std::string sharedBELStr : compatiblePlacementTable->sharedCellBELTypes)
    {
        std::vector<std::vector<PlacementBinInfo *>> tmpSharedBELGrid;
        tmpSharedBELGrid.clear();
        for (curBottomY = startY, i = 0; curBottomY < endY - eps; curBottomY += binHeight, i++)
        {
            std::vector<PlacementBinInfo *> tmpBELGridRow;
            tmpBELGridRow.clear();
            for (curLeftX = startX, j = 0; curLeftX < endX - eps; curLeftX += binWidth, j++)
            {
                PlacementBinInfo *newBin = new PlacementBinInfo(sharedBELStr, curLeftX, curLeftX + binWidth, curBottomY,
                                                                curBottomY + binHeight, i, j, compatiblePlacementTable);
                tmpBELGridRow.push_back(newBin);
            }
            tmpSharedBELGrid.push_back(tmpBELGridRow);
        }
        SharedBELTypeBinGrid.push_back(tmpSharedBELGrid);
    }

    // extra binGrid for LUTFF utilization adjustment and a global bin grid for all types of elememnts to evelaute
    // routability
    LUTFFBinGrid.clear();
    for (curBottomY = startY, i = 0; curBottomY < endY - eps; curBottomY += binHeight, i++)
    {
        std::vector<PlacementBinInfo *> tmpBELGridRow;
        tmpBELGridRow.clear();
        std::vector<PlacementBinInfo *> tmpGlobalBELGridRow;
        tmpGlobalBELGridRow.clear();
        for (curLeftX = startX, j = 0; curLeftX < endX - eps; curLeftX += binWidth, j++)
        {
            PlacementBinInfo *newBin =
                new PlacementBinInfo("LUTFF_BECAREFUL", curLeftX, curLeftX + binWidth, curBottomY,
                                     curBottomY + binHeight, i, j, compatiblePlacementTable);
            PlacementBinInfo *globalNewBin =
                new PlacementBinInfo("globalInfo_BECAREFUL", curLeftX, curLeftX + binWidth, curBottomY,
                                     curBottomY + binHeight, i, j, compatiblePlacementTable);
            tmpBELGridRow.push_back(newBin);
            tmpGlobalBELGridRow.push_back(globalNewBin);
        }
        LUTFFBinGrid.push_back(tmpBELGridRow);
        globalBinGrid.push_back(tmpGlobalBELGridRow);
    }
    // a set for grid check
    std::set<DeviceInfo::DeviceSite *> countedSites;
    countedSites.clear();

    for (std::string sharedBELStr : compatiblePlacementTable->sharedCellBELTypes)
    {
        if (compatiblePlacementTable->sharedCellType2SiteType.find(sharedBELStr) ==
            compatiblePlacementTable->sharedCellType2SiteType.end())
        {
            print_error(sharedBELStr + " is not found in sharedCellType2SiteType.");

            for (auto it = compatiblePlacementTable->sharedCellType2SiteType.begin();
                 it != compatiblePlacementTable->sharedCellType2SiteType.end(); it++)
                print_error("sharedCellType2SiteType Key: " + it->first);
        }
        assert(compatiblePlacementTable->sharedCellType2SiteType.find(sharedBELStr) !=
               compatiblePlacementTable->sharedCellType2SiteType.end());
        std::string targetSiteType = compatiblePlacementTable->sharedCellType2SiteType[sharedBELStr];

        std::vector<DeviceInfo::DeviceSite *> &sitesInType = deviceInfo->getSitesInType(targetSiteType);

        // map some BEL type to the same BEL id tomporarily for cell spreading, e.g., SLICEM_LUT -> SLICEL_LUT
        std::string targetSharedBELStr = getBELType2FalseBELType(sharedBELStr);
        int targetSharedBELTypeId = compatiblePlacementTable->getSharedBELTypeId(targetSharedBELStr);
        int actualSharedBELTypeId = compatiblePlacementTable->getSharedBELTypeId(sharedBELStr);

        unsigned int siteIndex = 0;
        for (curBottomY = startY, i = 0; curBottomY < endY - eps; curBottomY += binHeight, i++)
        {
            while (SharedBELTypeBinGrid[targetSharedBELTypeId][i][0]->inRangeY(sitesInType[siteIndex]->Y()))
            {
                for (curLeftX = startX, j = 0; curLeftX < endX - eps; curLeftX += binWidth, j++)
                {
                    while (SharedBELTypeBinGrid[targetSharedBELTypeId][i][j]->inRange(sitesInType[siteIndex]->X(),
                                                                                      sitesInType[siteIndex]->Y()))
                    {
                        SharedBELTypeBinGrid[targetSharedBELTypeId][i][j]->addSiteIntoBin(sitesInType[siteIndex]);
                        globalBinGrid[i][j]->addSiteIntoBin(sitesInType[siteIndex]);
                        if (actualSharedBELTypeId != targetSharedBELTypeId)
                            SharedBELTypeBinGrid[actualSharedBELTypeId][i][j]->addSiteIntoBin(sitesInType[siteIndex]);
                        countedSites.insert(sitesInType[siteIndex]);
                        siteIndex++;
                        if (siteIndex >= sitesInType.size())
                            break;
                    }
                    if (siteIndex >= sitesInType.size())
                        break;
                }
                if (siteIndex >= sitesInType.size())
                    break;
            }
            if (siteIndex >= sitesInType.size())
                break;
        }
    }

    std::set<std::string> siteTypeNotMapped;
    if (countedSites.size() < deviceInfo->getSites().size())
    {
        for (auto site : deviceInfo->getSites())
        {
            if (countedSites.find(site) == countedSites.end() &&
                siteTypeNotMapped.find(site->getSiteType()) == siteTypeNotMapped.end())
            {
                print_warning("Site Type (" + site->getSiteType() + ") is not mapped to bin grid. e.g. [" +
                              site->getName() +
                              "]. It might be not critical if the design will not utilize this kind of sites. Please "
                              "check the compatible table you defined.");
                siteTypeNotMapped.insert(site->getSiteType());
            }
        }
    }
    // assert(countedSites.size()==deviceInfo->getSites().size() && "all sites should be mapped into bins.");

    std::string SLICEL_LUT_STR = "SLICEL_LUT";
    print_status(
        "Bin Grid Size: Y: " +
        std::to_string(SharedBELTypeBinGrid[compatiblePlacementTable->getSharedBELTypeId(SLICEL_LUT_STR)].size()) +
        " X:" +
        std::to_string(SharedBELTypeBinGrid[compatiblePlacementTable->getSharedBELTypeId(SLICEL_LUT_STR)][0].size()));

    print_status("Bin Grid for Density Control Created");
}

void PlacementInfo::reloadNets()
{
    if (!placementNets.empty())
    {
        for (auto net : placementNets)
        {
            delete net;
        }
    }
    placementNets.clear();
    clockNets.clear();
    for (DesignInfo::DesignNet *net : designInfo->getNets())
    {
        PlacementNet *newPNet = new PlacementNet(net, placementNets.size(), cellId2PlacementUnitVec);
        PlacementUnit *PUInNet = nullptr;
        bool isInternalNet = true;
        for (auto tmpPU : newPNet->getUnits())
        {
            if (!PUInNet)
                PUInNet = tmpPU;
            else
            {
                if (PUInNet != tmpPU)
                {
                    isInternalNet = false;
                }
            }
        }
        if (isInternalNet)
        {
            delete newPNet;
            continue;
        }
        placementNets.push_back(newPNet);
        if (newPNet->isGlobalClock())
            clockNets.push_back(newPNet);
    }

    std::vector<std::set<PlacementNet *>> placementUnitId2NetSet;
    placementUnitId2NetSet.clear();
    placementUnitId2NetSet.resize(placementUnits.size(), std::set<PlacementNet *>());
    placementUnitId2Nets.clear();
    placementUnitId2Nets.resize(placementUnits.size(), std::vector<PlacementNet *>());

    for (auto curNet : placementNets)
    {
        for (auto curPU : curNet->getUnits())
        {
            if (placementUnitId2NetSet[curPU->getId()].find(curNet) == placementUnitId2NetSet[curPU->getId()].end())
            {
                placementUnitId2NetSet[curPU->getId()].insert(curNet);
                placementUnitId2Nets[curPU->getId()].push_back(curNet);
            }
        }
    }

    for (auto curPU : placementUnits)
    {
        curPU->setNetsSetPtr(&placementUnitId2Nets[curPU->getId()]);
    }

    PUSetContainingFF.clear();
    PUsContainingFF.clear();

    for (auto curPU : placementUnits)
    {
        if (curPU->hasRegister())
        {
            if (PUSetContainingFF.find(curPU) == PUSetContainingFF.end())
            {
                PUSetContainingFF.insert(curPU);
                PUsContainingFF.push_back(curPU);
            }
        }
    }
    // updateLongPaths();
    print_status("reload placementNets and #register-related PU=" + std::to_string(PUsContainingFF.size()));
}

void PlacementInfo::updateLongPaths()
{
    print_status("updating long paths and #PUsContainingFF=" + std::to_string(PUsContainingFF.size()));
    std::set<PlacementUnit *> visitedPUs;

    longPaths.clear();
    visitedPUs.clear();

    int restartNum = 1000000;
    int pathMexLen = 100;

    for (int restartIter = 0; restartIter < restartNum; restartIter++)
    {
        PlacementUnit *curPU = PUsContainingFF[restartIter % PUsContainingFF.size()];
        // if (visitedPUs.find(curPU) != visitedPUs.end())
        // {
        //     continue;
        // }
        std::vector<PlacementUnit *> curPath(0);
        curPath.push_back(curPU);
        // visitedPUs.insert(curPU);

        for (int pathEleId = 0; pathEleId < pathMexLen; pathEleId++)
        {
            std::vector<PlacementNet *> outputNets;
            outputNets.clear();
            for (auto curNet : *(curPU->getNetsSetPtr()))
            {
                if (curNet->getDriverUnits().size() == 1)
                {
                    if (curNet->getDriverUnits()[0] == curPU && curNet->getUnitsBeDriven().size() > 0)
                    {
                        outputNets.push_back(curNet);
                    }
                }
            }

            PlacementUnit *nextPU = nullptr;
            if (outputNets.size())
            {
                int tryCnt = 40;
                while (tryCnt--)
                {
                    auto selectedNet = outputNets[random() % outputNets.size()];

                    nextPU =
                        selectedNet->getUnitsBeDriven()[random() * random() % selectedNet->getUnitsBeDriven().size()];

                    if (curPU == nextPU || !nextPU->hasLogic() || visitedPUs.find(nextPU) != visitedPUs.end())
                    {
                        nextPU = nullptr;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (nextPU)
            {
                curPath.push_back(nextPU);

                if (nextPU->hasRegister())
                    break;
                curPU = nextPU;
            }
        }
        if (curPath.size() > 5)
        {
            longPaths.push_back(curPath);
            for (unsigned int i = 1; i < curPath.size() - 1; i++)
            {
                visitedPUs.insert(curPath[i]);
            }
        }
    }

    std::sort(longPaths.begin(), longPaths.end(),
              [](const std::vector<PlacementUnit *> &a, const std::vector<PlacementUnit *> &b) -> bool {
                  return a.size() > b.size();
              });

    // longPaths.resize(longPaths.size() / 4);
    print_status("Found #path = " + std::to_string(longPaths.size()));
    print_status("Found longest path with " + std::to_string(longPaths[0].size()) + " PUs");
}

void PlacementInfo::optimizeLongPaths()
{
    return;
    for (auto &path : longPaths)
    {
        float totalWeight = 0;
        float totalX = 0;
        float totalY = 0;
        for (auto tmpPU : path)
        {
            float curWeight = 0;
            if (tmpPU->checkHasBRAM() || tmpPU->checkHasCARRY() || tmpPU->checkHasDSP())
            {
                totalWeight += 20;
                curWeight = 20;
            }
            else
            {
                totalWeight += 1;
                curWeight = 1;
            }
            totalX += tmpPU->X() * curWeight;
            totalY += tmpPU->Y() * curWeight;
        }
        float avgX = totalX / totalWeight;
        float avgY = totalY / totalWeight;
        for (auto tmpPU : path)
        {
            if (tmpPU->checkHasDSP() || tmpPU->checkHasBRAM())
                continue;
            tmpPU->setAnchorLocationAndForgetTheOriginalOne(avgX, avgY);
            enforceLegalizeXYInArea(tmpPU);
        }
    }
    print_warning("Update the locations of PUs in long paths");
}

void PlacementInfo::PlacementNet::drawNet(float generalWeight)
{
    std::vector<std::pair<float, float>> nodexy;
    std::vector<std::pair<int, int>> lines;
    for (unsigned int pinId_net = 0; pinId_net < unitsOfNetPins.size(); pinId_net++)
    {
        auto tmpPU = unitsOfNetPins[pinId_net];
        auto tmpPinOffset = pinOffsetsInUnit[pinId_net];
        float curX = tmpPU->X() + tmpPinOffset.x;
        float curY = tmpPU->Y() + tmpPinOffset.y;
        nodexy.push_back(std::pair<float, float>(curX, curY));
    }
    lines.push_back(std::pair<int, int>(leftPinId_net, rightPinId_net));
    lines.push_back(std::pair<int, int>(bottomPinId_net, bottomPinId_net));

    for (unsigned int pinId_net = 0; pinId_net < unitsOfNetPins.size(); pinId_net++)
    {
        if (pinId_net != leftPinId_net && pinId_net != rightPinId_net)
        {
            lines.push_back(std::pair<int, int>(leftPinId_net, pinId_net));
            lines.push_back(std::pair<int, int>(rightPinId_net, pinId_net));
        }
        if (pinId_net != topPinId_net && pinId_net != bottomPinId_net)
        {
            lines.push_back(std::pair<int, int>(bottomPinId_net, pinId_net));
            lines.push_back(std::pair<int, int>(topPinId_net, pinId_net));
        }
    }
    //paintB2BNodeLine(nodexy, lines);
}

void PlacementInfo::verifyDeviceForDesign()
{
    for (DesignInfo::DesignCell *curCell : designInfo->getCells())
    {
        DesignInfo::DesignCellType cellType = curCell->getCellType();
        assert(compatiblePlacementTable->cellType2sharedBELTypes.find(cellType) !=
                   compatiblePlacementTable->cellType2sharedBELTypes.end() &&
               "all cells in the design should be placable in the device.");
    }
    print_status("Device Information Verified for Design Information");
}

std::ostream &operator<<(std::ostream &os, PlacementInfo::PlacementMacro *curMacro)
{
    // std::vector<DesignInfo::DesignCell *> cells;
    // std::set<DesignInfo::DesignCell *> cellSet;
    // std::map<DesignInfo::DesignCell *, short> cell2VirtualCellId;
    // std::vector<DesignInfo::DesignCell *> VirtualCellId2cell;
    // std::vector<DesignInfo::DesignCellType>
    //     virtualCellTypes; // BELTypeOccupation.size()>=realCells.size() because somtime a cell or a connection net
    //                       // can occupy multiple BEL
    // std::vector<float> offsetX, offsetY; // offsetX.size() == BELTypeOccupation.size()
    // std::vector<fixedPlacementInfo_inMacro> fixedCells;
    // float left, right, top, bottom;
    // PlacementMacroType macroType;
    os << "Macro: " << curMacro->getId() << " " << curMacro->getName() << "\n";

    if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_LUTFFPair)
        os << "  macroType: PlacementMacroType_LUTFFPair\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_FFFFPair)
        os << "  macroType: PlacementMacroType_FFFFPair\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_HALFCLB)
        os << "  macroType: PlacementMacroType_HALFCLB\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_LCLB)
        os << "  macroType: PlacementMacroType_LCLB\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_MCLB)
        os << "  macroType: PlacementMacroType_MCLB\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_CARRY)
        os << "  macroType: PlacementMacroType_CARRY\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_DSP)
        os << "  macroType: PlacementMacroType_DSP\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_BRAM)
        os << "  macroType: PlacementMacroType_BRAM\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_MUX7)
        os << "  macroType: PlacementMacroType_MUX7\n";
    else if (curMacro->getMacroType() == PlacementInfo::PlacementMacro::PlacementMacroType_MUX8)
        os << "  macroType: PlacementMacroType_MUX8\n";
    else
        assert(false && "undefined macro type.");

    os << "  placedAt: " << curMacro->X() << " " << curMacro->Y() << "\n";
    os << "  isPlaced: " << curMacro->isPlaced() << "\n";
    os << "  isFixed: " << curMacro->isFixed() << "\n";
    os << "  isLocked: " << curMacro->isLocked() << "\n";
    os << "  left, right, top, bottom: " << curMacro->getLeftOffset() << " " << curMacro->getRightOffset() << " "
       << curMacro->getTopOffset() << " " << curMacro->getBottomOffset() << "\n";

    os << "  numCell: " << curMacro->getCells().size() << "\n";
    for (unsigned int i = 0; i < curMacro->getCells().size(); i++)
    {
        DesignInfo::DesignCell *curCell = curMacro->getCells()[i];
        os << "    CellId: " << curCell->getCellId() << " targetCellTypeEnumId: " << curMacro->getVirtualCellType(i)
           << " name: " << curCell->getName() << " " << curMacro->getCellOffsetXInMacro(curCell) << " "
           << curMacro->getCellOffsetYInMacro(curCell) << "\n";
    }

    os << "  numFixedCell: " << curMacro->getFixedCellInfoVec().size() << "\n";
    for (unsigned int i = 0; i < curMacro->getFixedCellInfoVec().size(); i++)
    {
        DesignInfo::DesignCell *curCell = curMacro->getFixedCellInfoVec()[i].cell;
        os << "    CellId: " << curCell->getCellId() << " siteName: " << curMacro->getFixedCellInfoVec()[i].siteName
           << " BELName: " << curMacro->getFixedCellInfoVec()[i].BELName << "\n";
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, PlacementInfo::PlacementUnpackedCell *curUnpackedCell)
{
    os << "UnpackedCell: " << curUnpackedCell->getId() << " " << curUnpackedCell->getName() << "\n";
    os << "  cellId: " << curUnpackedCell->getCell()->getCellId() << " name: " << curUnpackedCell->getCell()->getName()
       << "\n";
    os << "  placedAt: " << curUnpackedCell->X() << " " << curUnpackedCell->Y() << "\n";
    os << "  isPlaced: " << curUnpackedCell->isPlaced() << "\n";
    os << "  isFixed: " << curUnpackedCell->isFixed() << "\n";
    os << "  isLocked: " << curUnpackedCell->isLocked() << "\n";
    if (curUnpackedCell->isFixed())
    {
        os << "  fixedAt: siteName: " << curUnpackedCell->getFixedSiteName()
           << " BELName: " << curUnpackedCell->getFixedBELName() << "\n";
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, PlacementInfo::PlacementUnit *curPU)
{
    if (auto curMacro = dynamic_cast<PlacementInfo::PlacementMacro *>(curPU))
        os << curMacro;
    else if (auto curCell = dynamic_cast<PlacementInfo::PlacementUnpackedCell *>(curPU))
        os << curCell;
    else
        assert(false && "placement unit type error.");
    return os;
}

void PlacementInfo::resetElementBinGrid()
{
    for (auto &typeGrid : SharedBELTypeBinGrid)
        for (auto &curRow : typeGrid)
            for (auto curBin : curRow)
                curBin->reset();
}

void PlacementInfo::updateElementBinGrid()
{
    resetElementBinGrid();
    cellId2location.resize(designInfo->getNumCells());

    for (auto curPU : placementUnits)
    {
        if (auto curUnpackedCell = dynamic_cast<PlacementUnpackedCell *>(curPU))
        {
            int binIdX, binIdY;
            float cellX = curUnpackedCell->X();
            float cellY = curUnpackedCell->Y();
            DesignInfo::DesignCell *curCell = curUnpackedCell->getCell();
            cellId2location[curCell->getCellId()].X = cellX;
            cellId2location[curCell->getCellId()].Y = cellY;

            getGridXY(cellX, cellY, binIdX, binIdY);

            float num_cellOccupationBELs = getActualOccupation(curCell);
            assert(num_cellOccupationBELs >= 0);

            bool nonOverflow = false;
            for (int SharedBELID : getPotentialBELTypeIDs(curCell))
            {
                assert(binIdY >= 0);
                assert(binIdX >= 0);
                assert((unsigned int)binIdY < getBinGrid(SharedBELID).size());
                assert((unsigned int)binIdX < getBinGrid(SharedBELID)[binIdY].size());
                assert(getBinGrid(SharedBELID)[binIdY][binIdX]->inRange(cellX, cellY));
                if (getBinGrid(SharedBELID)[binIdY][binIdX]->canAddMore(num_cellOccupationBELs))
                {
                    getBinGrid(SharedBELID)[binIdY][binIdX]->addCell(curCell, num_cellOccupationBELs);
                    setCellBinInfo(curCell->getCellId(), SharedBELID, binIdX, binIdY, num_cellOccupationBELs);
                    nonOverflow = true;
                    break;
                }
            }
            if (!nonOverflow)
            {
                getBinGrid(getPotentialBELTypeIDs(curCell)[0])[binIdY][binIdX]->addCell(curCell,
                                                                                        num_cellOccupationBELs);
                setCellBinInfo(curCell->getCellId(), getPotentialBELTypeIDs(curCell)[0], binIdX, binIdY,
                               num_cellOccupationBELs);
            }
        }
        else if (auto curMacro = dynamic_cast<PlacementMacro *>(curPU))
        {
            for (int vId = 0; vId < curMacro->getNumOfCells(); vId++)
            {
                float offsetX_InMacro, offsetY_InMacro;
                DesignInfo::DesignCellType cellType;
                curMacro->getVirtualCellInfo(vId, offsetX_InMacro, offsetY_InMacro, cellType);
                DesignInfo::DesignCell *curCell = curMacro->getCell(vId);

                int binIdX, binIdY;
                float cellX = curMacro->X() + offsetX_InMacro;
                float cellY = curMacro->Y() + offsetY_InMacro;

                getGridXY(cellX, cellY, binIdX, binIdY);

                float num_cellOccupationBELs = getActualOccupation(curCell);
                assert(num_cellOccupationBELs >= 0);
                bool nonOverflow = false;

                cellId2location[curCell->getCellId()].X = cellX;
                cellId2location[curCell->getCellId()].Y = cellY;

                for (int SharedBELID : getPotentialBELTypeIDs(curCell))
                {
                    assert(binIdY >= 0);
                    assert(binIdX >= 0);
                    assert((unsigned int)binIdY < getBinGrid(SharedBELID).size());
                    assert((unsigned int)binIdX < getBinGrid(SharedBELID)[binIdY].size());
                    assert(getBinGrid(SharedBELID)[binIdY][binIdX]->inRange(cellX, cellY));
                    if (getBinGrid(SharedBELID)[binIdY][binIdX]->canAddMore(num_cellOccupationBELs))
                    {

                        getBinGrid(SharedBELID)[binIdY][binIdX]->addCell(curCell, num_cellOccupationBELs);
                        setCellBinInfo(curCell->getCellId(), SharedBELID, binIdX, binIdY, num_cellOccupationBELs);
                        nonOverflow = true;
                        break;
                    }
                }

                if (!nonOverflow)
                {
                    getBinGrid(getPotentialBELTypeIDs(curCell)[0])[binIdY][binIdX]->addCell(curCell,
                                                                                            num_cellOccupationBELs);
                    setCellBinInfo(curCell->getCellId(), getPotentialBELTypeIDs(curCell)[0], binIdX, binIdY,
                                   num_cellOccupationBELs);
                }
            }
        }
    }
}

void PlacementInfo::adjustLUTFFUtilization_Packablity(float neighborDisplacementUpperbound, bool enfore)
{
    assert(getProgress() > 0.3);
    // based on progress, set the frequency of adjustment
    bool doUpdate = false;

    if (getProgress() > 0.8)
        doUpdate = std::fabs(getProgress() - lastProgressWhenLUTFFUtilAdjust) > 0.03;
    else
        doUpdate = std::fabs(getProgress() - lastProgressWhenLUTFFUtilAdjust) > 0.05;

    if (LUTFFUtilizationAdjusted && !doUpdate && !enfore)
    {
        // we don't need to update LUT/FF utilization so frequently
        return;
    }
    lastProgressWhenLUTFFUtilAdjust = getProgress();

    print_status("PlacementInfo: adjusting LUT/FF utilization based on Packablity");
    for (auto &curRow : LUTFFBinGrid)
        for (auto curBin : curRow)
            curBin->reset();
    // assign LUT/FF to bin grid to find their neighbors easily
    for (auto curPU : placementUnits)
    {
        if (auto curUnpackedCell = dynamic_cast<PlacementUnpackedCell *>(curPU))
        {
            int binIdX, binIdY;
            float cellX = curUnpackedCell->X();
            float cellY = curUnpackedCell->Y();
            DesignInfo::DesignCell *curCell = curUnpackedCell->getCell();

            if (curCell->isLUT() || curCell->isFF())
            {
                getGridXY(cellX, cellY, binIdX, binIdY);
                assert(LUTFFBinGrid[binIdY][binIdX]->inRange(cellX, cellY));
                assert(binIdY >= 0);
                assert(binIdX >= 0);
                assert((unsigned int)binIdY < LUTFFBinGrid.size());
                assert((unsigned int)binIdX < LUTFFBinGrid[binIdY].size());
                LUTFFBinGrid[binIdY][binIdX]->addCell(curCell,
                                                      0); // we just use the bininfo to record LUTFF location
                cellId2location[curCell->getCellId()].X = cellX;
                cellId2location[curCell->getCellId()].Y = cellY;
            }
        }
        else if (auto curMacro = dynamic_cast<PlacementMacro *>(curPU))
        {
            for (int vId = 0; vId < curMacro->getNumOfCells(); vId++)
            {
                float offsetX_InMacro, offsetY_InMacro;
                DesignInfo::DesignCellType cellType;
                curMacro->getVirtualCellInfo(vId, offsetX_InMacro, offsetY_InMacro, cellType);
                DesignInfo::DesignCell *curCell = curMacro->getCell(vId);
                if (curCell->isLUT() || curCell->isFF())
                {
                    int binIdX, binIdY;
                    float cellX = curMacro->X() + offsetX_InMacro;
                    float cellY = curMacro->Y() + offsetY_InMacro;

                    getGridXY(cellX, cellY, binIdX, binIdY);
                    assert(LUTFFBinGrid[binIdY][binIdX]->inRange(cellX, cellY));

                    assert(binIdY >= 0);
                    assert(binIdX >= 0);
                    assert((unsigned int)binIdY < LUTFFBinGrid.size());
                    assert((unsigned int)binIdX < LUTFFBinGrid[binIdY].size());
                    LUTFFBinGrid[binIdY][binIdX]->addCell(curCell,
                                                          0); // we just use the bininfo to record LUTFF location
                    cellId2location[curCell->getCellId()].X = cellX;
                    cellId2location[curCell->getCellId()].Y = cellY;
                }
            }
        }
    }

    unsigned int cellNum = getCells().size();
    std::vector<float> &compatiblePlacementTable_cellId2Occupation = compatiblePlacementTable->getcellId2Occupation();
#pragma omp parallel for schedule(dynamic)
    for (unsigned int cellId = 0; cellId < cellNum; cellId++)
    {
        auto curCell = getCells()[cellId];
        int cellRealId = curCell->getCellId();

        if (curCell->isLUT())
        {
            std::vector<DesignInfo::DesignCell *> *neighborCells =
                findNeiborLUTFFsFromBinGrid(curCell, neighborDisplacementUpperbound);
            assert(neighborCells->size()); // at least it should find itself
            int pairableCnt = 0;
            float tmpRatio = -1;
            int deteminted = getDeterminedOccupation(curCell->getCellId());
            if (deteminted >= 0)
            {
                compatiblePlacementTable_cellId2Occupation[curCell->getCellId()] = deteminted;
            }
            else
            {
                if (curCell->getInputPins().size() <= 5 && !curCell->isLUT6())
                {
                    int totalNumNeighborCells = neighborCells->size();
                    for (auto neighborLUT : *neighborCells)
                    {
                        if (getDeterminedOccupation(neighborLUT->getCellId()) >= 0) // paired LUT
                        {
                            totalNumNeighborCells--;
                            continue;
                        }
                        if (neighborLUT->getInputPins().size() <= 5 && !neighborLUT->isLUT6() && curCell != neighborLUT)
                        {
                            if (getPairPinNum(curCell, neighborLUT) <= 5)
                            {
                                pairableCnt++;
                            }
                        }
                    }
                    assert(totalNumNeighborCells > 0);
                    if (totalNumNeighborCells == 1 || pairableCnt == 0)
                        tmpRatio = 0;
                    else
                        tmpRatio = std::pow((float)pairableCnt / (float)(totalNumNeighborCells - 1), 0.3);
                    assert(tmpRatio <= 1 && tmpRatio >= 0);
                    // compatiblePlacementTable_cellId2Occupation[cellRealId] =
                    //     (compatiblePlacementTable_cellId2Occupation[cellRealId] +
                    //      (float)(1 * tmpRatio + 2 * (1 - tmpRatio))) /
                    //     2;

                    float areaDemand = (float)(1 * tmpRatio + 2 * (1 - tmpRatio));
                    compatiblePlacementTable_cellId2Occupation[cellRealId] = areaDemand * 1.1;
                }
                else
                {
                    compatiblePlacementTable_cellId2Occupation[cellRealId] = 2;
                }
            }
            assert(compatiblePlacementTable_cellId2Occupation[cellRealId] >= 0);
            delete neighborCells;
        }
        else if (curCell->isFF())
        {
            if (!curCell->getControlSetInfo())
            {
                assert(curCell->isVirtualCell());
                compatiblePlacementTable_cellId2Occupation[cellRealId] = 1;
                continue;
            }
            assert(!curCell->isVirtualCell());
            std::vector<DesignInfo::DesignCell *> *neighborCells =
                findNeiborLUTFFsFromBinGrid(curCell, neighborDisplacementUpperbound);
            assert(neighborCells->size()); // at least it should find itself
            std::map<int, int> CSId2Cnt;
            int targetControlSetId = curCell->getControlSetInfo()->getId();
            for (auto neighborFF : *neighborCells)
            {
                if (neighborFF->isVirtualCell())
                    continue;
                if (neighborFF->getControlSetInfo()->getCLK() == curCell->getControlSetInfo()->getCLK() &&
                    neighborFF->getControlSetInfo()->getSR() == curCell->getControlSetInfo()->getSR())
                    CSId2Cnt[neighborFF->getControlSetInfo()->getId()] = 0;
            }
            for (auto neighborFF : *neighborCells)
            {
                if (neighborFF->isVirtualCell())
                {
                    //  CSId2Cnt[targetControlSetId]++; // it can pack with the target cell
                    continue;
                }
                if (neighborFF->getControlSetInfo()->getCLK() == curCell->getControlSetInfo()->getCLK() &&
                    neighborFF->getControlSetInfo()->getSR() == curCell->getControlSetInfo()->getSR())
                    CSId2Cnt[neighborFF->getControlSetInfo()->getId()]++;
            }
            int totalDemand = 0;
            int targetFFNum = CSId2Cnt[targetControlSetId];   // n0
            int targetDemand = (1 + ((targetFFNum - 1) / 4)); // ceil(n0/4)
            for (auto CSIdCntpair : CSId2Cnt)
            {
                totalDemand += (1 + ((CSIdCntpair.second - 1) / 4)); // sum(ceil(ni/4))
            }
            int factor = (1 + ((totalDemand - 1) / 2)); // ceil(sum/2)
            const float alpha = 1.2;
            assert(totalDemand > 0);
            assert(targetFFNum > 0);

            float areaDemand =
                alpha * 8.0 * ((float)targetDemand) / ((float)totalDemand) * (float)factor / (float)(targetFFNum);
            if (areaDemand > 1.5)
                areaDemand *= 1.1;
            // else if (areaDemand > 1.2)
            //     areaDemand *= 1.15;
            // compatiblePlacementTable_cellId2Occupation[cellRealId] =
            //     alpha * 4.0 * ((float)targetDemand) / (float)(targetFFNum);
            compatiblePlacementTable_cellId2Occupation[cellRealId] = areaDemand;
            assert(compatiblePlacementTable_cellId2Occupation[cellRealId] >= 0);
            delete neighborCells;
        }
    }
    print_status("PlacementInfo: adjusted LUT/FF utilization based on Packablity");
}

const void PlacementInfo::outputCongestionMap(int iter) {
	std::cout << "outputCongestion Map starts. iter: " << iter << " Y: " << globalBinGrid.size() << " X: " << globalBinGrid[0].size() << std::endl;
	double minDmd = 100000;
	double maxDmd = 0;
	double dmdTot = 0;
    for (auto &tmpRow : globalBinGrid)
    {
        for (auto &curBin : tmpRow)
        {
            assert(curBin);
			double dmd = curBin->getSwitchDemandForNets();
			std::cout << std::setw(3) << (int) dmd << " ";
			if (dmd > maxDmd)
				maxDmd = dmd;
			if (dmd < minDmd)
				minDmd = dmd;
			dmdTot += dmd;
        }
		std::cout << std::endl;
    }
	dmdTot /= (globalBinGrid.size() * globalBinGrid[0].size());
	std::cout << "max: " << maxDmd << std::endl;
	std::cout << "avg: " << dmdTot << std::endl;
	std::cout << "min: " << minDmd << std::endl;
	std::cout << "outputCongestion Map ends." << std::endl;
};

// refer to RippleFPGA's implementation
void PlacementInfo::adjustLUTFFUtilization_Routability(bool enfore)
{
    print_status("PlacementInfo: adjusting LUT/FF utilization based on Routability");
    for (auto &tmpRow : globalBinGrid)
    {
        for (auto &curBin : tmpRow)
        {
            assert(curBin);
            curBin->reset();
        }
    }

    for (auto curPU : placementUnits)
    {
        if (auto curUnpackedCell = dynamic_cast<PlacementUnpackedCell *>(curPU))
        {
            int binIdX, binIdY;
            float cellX = curUnpackedCell->X();
            float cellY = curUnpackedCell->Y();
            DesignInfo::DesignCell *curCell = curUnpackedCell->getCell();

            //if (curCell->isLUT() || curCell->isFF() || curCell->isBRAM() || curCell->isDSP()) // currently we only resize LUT/FF
            if (curCell->isLUT() || curCell->isFF())
            {
                getGridXY(cellX, cellY, binIdX, binIdY);
                assert(globalBinGrid[binIdY][binIdX]->inRange(cellX, cellY));
                assert(binIdY >= 0);
                assert(binIdX >= 0);
                assert((unsigned int)binIdY < globalBinGrid.size());
                assert((unsigned int)binIdX < globalBinGrid[binIdY].size());
                globalBinGrid[binIdY][binIdX]->addCell(curCell, 0);
            }
        }
        else if (auto curMacro = dynamic_cast<PlacementMacro *>(curPU))
        {
            for (int vId = 0; vId < curMacro->getNumOfCells(); vId++)
            {
                float offsetX_InMacro, offsetY_InMacro;
                DesignInfo::DesignCellType cellType;
                curMacro->getVirtualCellInfo(vId, offsetX_InMacro, offsetY_InMacro, cellType);
                DesignInfo::DesignCell *curCell = curMacro->getCell(vId);
                //if (curCell->isLUT() || curCell->isFF() || curCell->isBRAM() || curCell->isDSP()) // currently we only resize LUT/FF
                if(curCell->isLUT() || curCell->isFF())
                {
                    int binIdX, binIdY;
                    float cellX = curMacro->X() + offsetX_InMacro;
                    float cellY = curMacro->Y() + offsetY_InMacro;

                    getGridXY(cellX, cellY, binIdX, binIdY);
                    assert(globalBinGrid[binIdY][binIdX]->inRange(cellX, cellY));

                    assert(binIdY >= 0);
                    assert(binIdX >= 0);
                    assert((unsigned int)binIdY < globalBinGrid.size());
                    assert((unsigned int)binIdX < globalBinGrid[binIdY].size());
                    globalBinGrid[binIdY][binIdX]->addCell(curCell, 0);
                }
            }
        }
    }

    // calculate the congestion ratio for the bin grid
    for (auto tmpNet : placementNets)
    {
        if (tmpNet->getHPWL(y2xRatio) < eps)
            continue;
        int leftBinX, rightBinX, topBinY, bottomBinY;
        getGridXY(tmpNet->getLeftPinX(), tmpNet->getBottomPinY(), leftBinX, bottomBinY);
        getGridXY(tmpNet->getRightPinX(), tmpNet->getTopPinY(), rightBinX, topBinY);

        assert(bottomBinY >= 0);
        assert(leftBinX >= 0);
        assert((unsigned int)bottomBinY < globalBinGrid.size());
        assert((unsigned int)leftBinX < globalBinGrid[bottomBinY].size());
        assert(topBinY >= 0);
        assert(rightBinX >= 0);
        assert((unsigned int)topBinY < globalBinGrid.size());
        assert((unsigned int)rightBinX < globalBinGrid[topBinY].size());

        float totW = tmpNet->getHPWL(y2xRatio) + 0.5;
        unsigned int nPins = tmpNet->getPinOffsetsInUnit().size();
        if (nPins < 10)
            totW *= 1.06;
        else if (nPins < 20)
            totW *= 1.2;
        else if (nPins < 30)
            totW *= 1.4;
        else if (nPins < 50)
            totW *= 1.6;
        else if (nPins < 100)
            totW *= 1.8;
        else if (nPins < 200)
            totW *= 2.1;
        else
            totW *= 3.0;

        int numGCell = (rightBinX - leftBinX + 1) * (topBinY - bottomBinY + 1) * binHeight * binWidth;
        float indW = totW / numGCell;
        for (int x = leftBinX; x <= rightBinX; ++x)
            for (int y = bottomBinY; y <= topBinY; ++y)
                globalBinGrid[y][x]->increaseSWDemandBy(indW);
    }

    std::vector<float> &compatiblePlacementTable_cellId2InfationRatio =
        compatiblePlacementTable->getcellId2InfationRatio();
    auto LUTTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_LUT6);
    std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &LUTBinGrid = getBinGrid(LUTTypeBELIds[0]);
    auto FFTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_FDRE);
    std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &FFBinGrid = getBinGrid(FFTypeBELIds[0]);
    // auto BRAMTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_RAMB36E2);
    // std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &BRAMBinGrid = getBinGrid(BRAMTypeBELIds[0]);
    // auto DSPTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_DSP48E2);
    // std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &DSPBinGrid = getBinGrid(DSPTypeBELIds[0]);

    for (unsigned int i = 0; i < getCells().size(); i++)
    {
        compatiblePlacementTable_cellId2InfationRatio[i] = std::sqrt(compatiblePlacementTable_cellId2InfationRatio[i]);
    }

    std::vector<PlacementInfo::PlacementBinInfo *> bins;
    for (auto &tmpRow : globalBinGrid)
    {
        for (auto &curBin : tmpRow)
        {
            assert(curBin);
            bins.push_back(curBin);
        }
    }
    std::sort(bins.begin(), bins.end(),
              [](const PlacementInfo::PlacementBinInfo *a, const PlacementInfo::PlacementBinInfo *b) -> bool {
                  return a->getSwitchDemandForNets() > b->getSwitchDemandForNets();
              });

    int binOrderId = 0;
    for (auto &curBin : bins)
    {
        binOrderId++;
        assert(curBin);
        if (curBin->getSwitchDemandForNets() > 90 && binOrderId < bins.size() * 0.3)
        {
            float infateRatio = 2 * (curBin->getSwitchDemandForNets() / 90);
            if (infateRatio > 4)
                infateRatio = 4;
            for (auto tmpCell : curBin->getCells())
            {
                assert(tmpCell->isLUT() || tmpCell->isFF());
                compatiblePlacementTable_cellId2InfationRatio[tmpCell->getCellId()] = infateRatio;
            }
            float LUTSupplyRatio = LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 0.9;
            float FFSupplyRatio = FFBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 0.9;
            // float BRAMSupplyRatio = BRAMBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 0.9;
            // float DSPSupplyRatio = DSPBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 0.9;

            if (LUTSupplyRatio > 0.8)
                LUTBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(LUTSupplyRatio);
            if (FFSupplyRatio > 0.8)
                FFBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(FFSupplyRatio);
            // if (BRAMSupplyRatio > 0.8)
            //     BRAMBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(BRAMSupplyRatio);
            // if (DSPSupplyRatio > 0.8)
            //     DSPBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(DSPSupplyRatio);
        }
        // we don't recover the suppy until next creation of bin grid (i.e., createBinGrid), otherwise, you can
        // uncomment contents below which might slightly increase the congestion but improve HPWL a bit.
        else if (curBin->getSwitchDemandForNets() < 50 || binOrderId > bins.size() * 0.5)
        {
            if (LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() < 0.999)
            {
                LUTBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(1);
                FFBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(1);
                // BRAMBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(1);
                // DSPBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(1);
            }
        }
        else if (curBin->getSwitchDemandForNets() < 65)
        {
            if (LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() < 0.999)
            {
                float LUTSupplyRatio = LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.05;
                if (LUTSupplyRatio > 1)
                    LUTSupplyRatio = 1;
                float FFSupplyRatio = FFBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.05;
                if (FFSupplyRatio > 1)
                    FFSupplyRatio = 1;
                // float BRAMSupplyRatio = BRAMBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.05;
                // if (BRAMSupplyRatio > 1)
                //     BRAMSupplyRatio = 1;
                // float DSPSupplyRatio = DSPBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.05;
                // if (DSPSupplyRatio > 1)
                //     DSPSupplyRatio = 1;                                
                LUTBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(LUTSupplyRatio);
                FFBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(FFSupplyRatio);
                // BRAMBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(BRAMSupplyRatio);
                // DSPBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(DSPSupplyRatio);
            }
        }
        else if (curBin->getSwitchDemandForNets() < 80)
        {
            if (LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() < 0.999)
            {
                float LUTSupplyRatio = LUTBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.025;
                if (LUTSupplyRatio > 1)
                    LUTSupplyRatio = 1;
                float FFSupplyRatio = FFBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.025;
                if (FFSupplyRatio > 1)
                    FFSupplyRatio = 1;
                // float BRAMSupplyRatio = BRAMBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.025;
                // if (BRAMSupplyRatio > 1)
                //     BRAMSupplyRatio = 1;
                // float DSPSupplyRatio = DSPBinGrid[curBin->Y()][curBin->X()]->getRequiredBinShrinkRatio() * 1.025;
                // if (DSPSupplyRatio > 1)
                //     DSPSupplyRatio = 1;
                LUTBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(LUTSupplyRatio);
                FFBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(FFSupplyRatio);
                // BRAMBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(BRAMSupplyRatio);
                // DSPBinGrid[curBin->Y()][curBin->X()]->setRequiredBinShrinkRatio(DSPSupplyRatio);
            }
        }
    }
    print_status("PlacementInfo: adjusted LUT/FF utilization based on Routability");
}

void PlacementInfo::adjustLUTFFUtilization_Clocking()
{
    print_status("PlacementInfo: adjusting FF utilization based on Clock Utilization");

    float infateRatio = 1.1;
    std::vector<float> &compatiblePlacementTable_cellId2InfationRatio =
        compatiblePlacementTable->getcellId2InfationRatio();

    auto &clockRegions = deviceInfo->getClockRegions();

    std::set<int> addedCells;
    addedCells.clear();
    auto &cells = getCells();

    for (int i = deviceInfo->getClockRegionNumY() - 1; i >= 0; i--)
    {
        for (int j = 0; j < deviceInfo->getClockRegionNumX(); j++)
        {
            for (auto &row : clockRegions[i][j]->getClockColumns())
            {
                for (auto &clockColumn : row)
                {
                    if (clockColumn.getClockNum() > 10)
                    {
                        for (auto &pair : clockColumn.getClockNetId2CellIds())
                        {
                            for (auto cellId : pair.second)
                            {
                                assert(cellId < (int)cells.size());
                                if (cells[cellId]->isFF())
                                {
                                    if (addedCells.find(cellId) == addedCells.end())
                                    {
                                        compatiblePlacementTable_cellId2InfationRatio[cellId] *= infateRatio;
                                        addedCells.insert(cellId);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    print_status("PlacementInfo: adjusted LUT/FF utilization based on Clock Utilization");
}

void PlacementInfo::adjustLUTFFUtilization_Routability_Reset()
{
    // sometime, the HPWL increase too much, we need to reset the adjustion
    std::vector<float> &compatiblePlacementTable_cellId2InfationRatio =
        compatiblePlacementTable->getcellId2InfationRatio();
    auto LUTTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_LUT6);
    std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &LUTBinGrid = getBinGrid(LUTTypeBELIds[0]);
    auto FFTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_FDRE);
    std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &FFBinGrid = getBinGrid(FFTypeBELIds[0]);
    // auto BRAMTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_RAMB36E2);
    // std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &BRAMBinGrid = getBinGrid(BRAMTypeBELIds[0]);
    // auto DSPTypeBELIds = getPotentialBELTypeIDs(DesignInfo::CellType_DSP48E2);
    // std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &DSPBinGrid = getBinGrid(DSPTypeBELIds[0]);

    for (unsigned int i = 0; i < getCells().size(); i++)
    {
        compatiblePlacementTable_cellId2InfationRatio[i] = 1.0;
    }
    for (auto &tmpRow : LUTBinGrid)
    {
        for (auto &curBin : tmpRow)
        {
            assert(curBin);
            curBin->setRequiredBinShrinkRatio(1);
        }
    }
    for (auto &tmpRow : FFBinGrid)
    {
        for (auto &curBin : tmpRow)
        {
            assert(curBin);
            curBin->setRequiredBinShrinkRatio(1);
        }
    }
    // for (auto &tmpRow : BRAMBinGrid)
    // {
    //     for (auto &curBin : tmpRow)
    //     {
    //         assert(curBin);
    //         curBin->setRequiredBinShrinkRatio(1);
    //     }
    // }
    // for (auto &tmpRow : DSPBinGrid)
    // {
    //     for (auto &curBin : tmpRow)
    //     {
    //         assert(curBin);
    //         curBin->setRequiredBinShrinkRatio(1);
    //     }
    // }
    
    print_status("PlacementInfo: Routability-oriented area adjustion is reset.");
}

void PlacementInfo::adjustLUTFFUtilization(float neighborDisplacementUpperbound, bool enfore)
{
    if (neighborDisplacementUpperbound > 0)
    {
        print_status("PlacementInfo: adjusting LUT/FF Utilization");
        adjustLUTFFUtilization_Packablity(neighborDisplacementUpperbound, enfore);
        print_status("PlacementInfo: adjusted LUT/FF Utilization");
    }
    else
    {
        compatiblePlacementTable->resetCellOccupationToDefault();
    }
        
    adjustLUTFFUtilization_Routability(enfore);
    // if (getProgress() > 0.8)
    //     adjustLUTFFUtilization_Clocking();
    LUTFFUtilizationAdjusted = true;
}

void PlacementInfo::dumpCongestion(std::string dumpFileName)
{
    dumpFileName = dumpFileName;
    print_status("PlacementInfo: dumping congestion rate to: " + dumpFileName);
    std::vector<std::vector<PlacementInfo::PlacementBinInfo *>> &curBinGrid = globalBinGrid;

    std::ofstream outfile0(dumpFileName.c_str());
    for (auto &row : curBinGrid)
    {
        for (auto curBin : row)
        {
            outfile0 << curBin->getSwitchDemandForNets() << " ";
        }
        outfile0 << "\n";
    }
    outfile0.close();
}

void PlacementInfo::createSiteBinGrid()
{
    // std::vector<std::string> checkSiteNameList{"SLICEL", "SLICEM", "DSP48E2", "RAMBFIFO18"};
    std::vector<std::string> checkSiteNameList{"SLICEL", "SLICEM"};
    siteGridForMacros.clear();

    int i = 0, j = 0;
    float curBottomY, curLeftX;

    for (curBottomY = startX, i = 0; curBottomY < endY - eps; curBottomY += binHeight, i++)
    {
        std::vector<PlacementSiteBinInfo *> tmpBELGridRow;
        tmpBELGridRow.clear();
        for (curLeftX = startX, j = 0; curLeftX < endX - eps; curLeftX += binWidth, j++)
        {
            PlacementSiteBinInfo *newBin =
                new PlacementSiteBinInfo(curLeftX, curLeftX + binWidth, curBottomY, curBottomY + binHeight, i, j);
            tmpBELGridRow.push_back(newBin);
        }
        siteGridForMacros.push_back(tmpBELGridRow);
    }

    for (std::string targetSiteType : checkSiteNameList)
    {
        std::vector<DeviceInfo::DeviceSite *> &sitesInType = deviceInfo->getSitesInType(targetSiteType);

        unsigned int siteIndex = 0;
        for (curBottomY = startY, i = 0; curBottomY < endY - eps; curBottomY += binHeight, i++)
        {
            while (siteGridForMacros[i][0]->inRangeY(sitesInType[siteIndex]->Y()))
            {
                for (curLeftX = startX, j = 0; curLeftX < endX - eps; curLeftX += binWidth, j++)
                {
                    while (siteGridForMacros[i][j]->inRange(sitesInType[siteIndex]->X(), sitesInType[siteIndex]->Y()))
                    {
                        siteGridForMacros[i][j]->addSiteIntoBin(sitesInType[siteIndex]);
                        siteIndex++;
                        if (siteIndex >= sitesInType.size())
                            break;
                    }
                    if (siteIndex >= sitesInType.size())
                        break;
                }
                if (siteIndex >= sitesInType.size())
                    break;
            }
            if (siteIndex >= sitesInType.size())
                break;
        }
    }
}

void PlacementInfo::PlacementSiteBinInfo::addSiteIntoBin(DeviceInfo::DeviceSite *curSite)
{
    if (inRange(curSite->X(), curSite->Y()))
    {
        correspondingSites.push_back(curSite);
        capacity += 1;
    }
    else
    {
        print_error("leftX: " + std::to_string(leftX) + "rightX: " + std::to_string(rightX) +
                    "topY: " + std::to_string(topY) + "bottomY: " + std::to_string(bottomY));
        print_error("curSite: " + curSite->getName());
        assert(false && "some sites are out of the scope");
    }
}

void PlacementInfo::resetSiteBinGrid()
{
    for (auto curRow : siteGridForMacros)
        for (auto curBin : curRow)
            curBin->reset();
}

void PlacementInfo::updateSiteBinGrid()
{
    assert(false && "unimplemented");
    // resetSiteBinGrid();
    // for (auto curMacro : placementMacros)
    // {
    //     for (auto offset_occupation_pair : curMacro->getOccupiedSiteInfo())
    //     {
    //         float offsetX_InMacro = 0, offsetY_InMacro = offset_occupation_pair.first;

    //         int binIdX, binIdY;
    //         float siteX = curMacro->X() + offsetX_InMacro;
    //         float siteY = curMacro->Y() + offsetY_InMacro;

    //         getGridXY(siteX, siteY, binIdX, binIdY);
    //         float siteOccupation = offset_occupation_pair.second;

    //         assert(binIdY >= 0);
    //         assert((unsigned int)binIdY < siteGridForMacros.size());
    //         assert(binIdX >= 0);
    //         assert((unsigned int)binIdX < siteGridForMacros[binIdY].size());
    //         siteGridForMacros[binIdY][binIdX]->addMacroSite(curMacro, siteOccupation);
    //     }
    // }
}

void PlacementInfo::updateCells2PlacementUnits()
{
    cellId2PlacementUnitVec.resize(designInfo->getNumCells());
    cellId2CellBinInfo.resize(designInfo->getNumCells());
    for (int i = 0; i < designInfo->getNumCells(); i++)
    {
        assert(cellId2PlacementUnit[i]);
        cellId2PlacementUnitVec[i] = cellId2PlacementUnit[i];
    }
}

void PlacementInfo::dumpVivadoPlacementTclWithPULegalizationInfo(std::string dumpFile)
{
    print_status("PlacementInfo: dumping placment Tcl commands archieve to: " + dumpFile);
    dumpPlacementUnitLocationCnt++;
    if (dumpFile != "")
    {
        // std::stringstream outfile0;
        // outfile0 << "GlobalPlacerPseudoNetWeight: " << getPseudoNetWeight() << "\n";
        // outfile0 << "GlobalPlacerMacroPseudoNetEnhanceCnt: " << getMacroPseudoNetEnhanceCnt() << "\n";
        // outfile0 << "GlobalPlacerMacroLegalizationWeight: " << getMacroLegalizationWeight() << "\n";
        // for (auto curPU : getPlacementUnits())
        // {
        //     outfile0 << curPU;
        // }
        // for (auto PUSitePair : PU2LegalSites)
        // {
        //     outfile0 << "PULegalization: " << PUSitePair.first->getId() << " ";
        //     std::vector<DeviceInfo::DeviceSite *> tmpSites = PUSitePair.second;
        //     for (unsigned int tmpId = 0; tmpId < tmpSites.size() - 1; tmpId++)
        //         outfile0 << tmpSites[tmpId]->getName() << ",";
        //     outfile0 << tmpSites[tmpSites.size() - 1]->getName() << "\n";
        // }
        // writeStrToGZip(dumpFile, outfile0);
        // print_status("PlacementInfo: dumped placment Tcl commands archieve to " + dumpFile);
    }
}

void PlacementInfo::dumpPlacementUnitInformation(std::string dumpFile)
{
    dumpFile += ".gz";
    print_status("PlacementInfo: dumping PU information archieve to: " + dumpFile);
    dumpPlacementUnitLocationCnt++;
    if (dumpFile != "")
    {
        std::stringstream outfile0;
        outfile0 << "GlobalPlacerPseudoNetWeight: " << getPseudoNetWeight() << "\n";
        outfile0 << "GlobalPlacerMacroPseudoNetEnhanceCnt: " << getMacroPseudoNetEnhanceCnt() << "\n";
        outfile0 << "GlobalPlacerMacroLegalizationWeight: " << getMacroLegalizationWeight() << "\n";
        for (auto curPU : getPlacementUnits())
        {
            outfile0 << curPU;
        }
        for (auto PUSitePair : PU2LegalSites)
        {
            outfile0 << "PULegalization: " << PUSitePair.first->getId() << " ";
            std::vector<DeviceInfo::DeviceSite *> tmpSites = PUSitePair.second;
            for (unsigned int tmpId = 0; tmpId < tmpSites.size() - 1; tmpId++)
                outfile0 << tmpSites[tmpId]->getName() << ",";
            outfile0 << tmpSites[tmpSites.size() - 1]->getName() << "\n";
        }
        writeStrToGZip(dumpFile, outfile0);
        print_status("PlacementInfo: dumped PU information archieve to: " + dumpFile);
    }
}

// TODO: implement a checkpoint mechanism here
void PlacementInfo::loadPlacementUnitInformation(std::string locFile)
{
    print_status("loading PU coordinate archieve from: " + locFile);
    print_warning("Please note that the loaded PU location information should be compatible with the other"
                  "information in the placer! Otherwise, there could be potential errors");
    print_warning(
        "E.g., the initial packing which creates virtual cells should be done. The bin grid for all cell types "
        "should be created.");

    PULegalXY.first.clear();
    PULegalXY.second.clear();

    std::string unzipCmnd = "gzip -c -d " + locFile;
    FILEbuf sbuf(popen(unzipCmnd.c_str(), "r"));
    std::istream infile(&sbuf);
    // std::ifstream infile(designTextFileName.c_str());

    std::string line, PUName, macroTypeStr, cellName, siteName, BELName;
    bool booleanValue;
    std::string fill0, fill1, fill2, fill3, fill4, fill5, fill6;
    unsigned int PUId, numCells, cellId;
    float X, Y;

    for (auto tmpPU : placementUnits)
    {
        delete tmpPU;
    }

    placementUnits.clear();
    fixedPlacementUnits.clear();
    placementMacros.clear();
    cellInMacros.clear();
    placementUnpackedCells.clear();
    cellId2PlacementUnitVec.clear();
    cellId2PlacementUnitVec.resize(getCells().size(), nullptr);
    cellId2PlacementUnit.clear();
    PU2LegalSites.clear();
    LUTFFUtilizationAdjusted = false;

    // outfile0 << "GlobalPlacerPseudoNetWeight: " << getPseudoNetWeight() << "\n";
    std::getline(infile, line);
    std::istringstream iss(line);
    iss >> fill0 >> oriPseudoNetWeight;
    assert(fill0 == "GlobalPlacerPseudoNetWeight:");
    std::getline(infile, line);
    iss = std::istringstream(line);
    iss >> fill0 >> macroPseudoNetEnhanceCnt;
    assert(fill0 == "GlobalPlacerMacroPseudoNetEnhanceCnt:");
    std::getline(infile, line);
    iss = std::istringstream(line);
    iss >> fill0 >> macroLegalizationWeight;
    assert(fill0 == "GlobalPlacerMacroLegalizationWeight:");

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        iss >> fill0 >> PUId >> PUName;
        if (fill0.find("Macro") != std::string::npos)
        {
            // Macro: 8700 design_1_i/xilinx_dma_pcie_ep_0/inst/xdma_0_....
            //   macroType: PlacementMacroType_BRAM
            //   placedAt: 76.75 50.925
            //   isPlaced: 1
            //   isFixed: 1
            //   isLocked: 1
            //   left, right, top, bottom: 0 0 2.5 0
            //   numCell: 2
            //     CellId: 645276 name: design_1_i/xilinx_dma_pcie_ep_0/inst/xdma_0_.... 0 0
            //     CellId: 691036 name: design_1_i/xilinx_dma_pcie_ep_0/inst/xdma_0_....(691036) 0 2.5
            //   numFixedCell: 2
            //     CellId: 645276 siteName: RAMB18_X16Y20 BELName: RAMB180.RAMB18E2_L
            //     CellId: 691036 siteName: RAMB18_X16Y21 BELName: RAMB181.RAMB18E2_U
            assert(placementUnits.size() == PUId && "PlacementUnit should be dumped according to the PU id order.");
            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> macroTypeStr;
            PlacementMacro::PlacementMacroType macroType;
            if (macroTypeStr == "PlacementMacroType_LUTFFPair")
                macroType = PlacementMacro::PlacementMacroType_LUTFFPair;
            else if (macroTypeStr == "PlacementMacroType_FFFFPair")
                macroType = PlacementMacro::PlacementMacroType_FFFFPair;
            else if (macroTypeStr == "PlacementMacroType_HALFCLB")
                macroType = PlacementMacro::PlacementMacroType_HALFCLB;
            else if (macroTypeStr == "PlacementMacroType_LCLB")
                macroType = PlacementMacro::PlacementMacroType_LCLB;
            else if (macroTypeStr == "PlacementMacroType_MCLB")
                macroType = PlacementMacro::PlacementMacroType_MCLB;
            else if (macroTypeStr == "PlacementMacroType_CARRY")
                macroType = PlacementMacro::PlacementMacroType_CARRY;
            else if (macroTypeStr == "PlacementMacroType_DSP")
                macroType = PlacementMacro::PlacementMacroType_DSP;
            else if (macroTypeStr == "PlacementMacroType_BRAM")
                macroType = PlacementMacro::PlacementMacroType_BRAM;
            else if (macroTypeStr == "PlacementMacroType_MUX7")
                macroType = PlacementMacro::PlacementMacroType_MUX7;
            else if (macroTypeStr == "PlacementMacroType_MUX8")
                macroType = PlacementMacro::PlacementMacroType_MUX8;
            else
                assert(false && "undefined macro type.");

            PlacementMacro *tmpPU = new PlacementMacro(PUName, PUId, macroType);
            placementUnits.push_back(tmpPU);
            placementMacros.push_back(tmpPU);

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> X >> Y;
            tmpPU->setAnchorLocationAndForgetTheOriginalOne(X, Y);

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
                tmpPU->setPlaced();

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
            {
                tmpPU->setFixed();
                fixedPlacementUnits.push_back(tmpPU);
            }

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
                tmpPU->setLocked();

            std::getline(infile, line);
            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> numCells;

            int totalWeight = 0;
            while (numCells--)
            {
                std::getline(infile, line);
                iss = std::istringstream(line);
                int virtualCellTypeId;
                iss >> fill0 >> cellId >> fill1 >> virtualCellTypeId >> fill2 >> cellName >> X >> Y;
                assert(cellId < getCells().size());
                assert(getCells()[cellId]->getName() == cellName);
                tmpPU->addCell(getCells()[cellId], static_cast<DesignInfo::DesignCellType>(virtualCellTypeId), X, Y);
                totalWeight +=
                    getCompatiblePlacementTable()
                        ->cellType2sharedBELTypeOccupation[static_cast<DesignInfo::DesignCellType>(virtualCellTypeId)];
                cellId2PlacementUnit[cellId] = tmpPU;
                assert(cellId < cellId2PlacementUnitVec.size());
                cellId2PlacementUnitVec[cellId] = tmpPU;
                cellInMacros.insert(getCells()[cellId]);
            }
            tmpPU->setWeight(totalWeight);

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> numCells;

            while (numCells--)
            {
                std::getline(infile, line);
                iss = std::istringstream(line);
                //     CellId: 691036 siteName: RAMB18_X16Y21 BELName: RAMB181.RAMB18E2_U
                iss >> fill0 >> cellId >> fill1 >> siteName >> fill2 >> BELName;
                assert(cellId < getCells().size());
                tmpPU->addFixedCellInfo(getCells()[cellId], siteName, BELName);
            }
        }
        else if (fill0.find("UnpackedCell") != std::string::npos)
        {
            // UnpackedCell: 321808 design_1_i/xilinx_dma_pcie_ep_0/inst/sys_reset_n_ibuf
            //   cellId: 644125 name: design_1_i/xilinx_dma_pcie_ep_0/inst/sys_reset_n_ibuf
            //   placedAt: 52.75 117.857
            //   isPlaced: 1
            //   isFixed: 1
            //   isLocked: 1
            //   fixedAt: siteName: IOB_X1Y103 BELName: HPIOB.INBUF

            assert(placementUnits.size() == PUId && "PlacementUnit should be dumped according to the PU id order.");
            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> cellId >> fill1 >> cellName;
            assert(cellId < cellId2PlacementUnitVec.size());
            assert(getCells()[cellId]->getName() == cellName);
            PlacementUnpackedCell *tmpPU = new PlacementUnpackedCell(PUName, PUId, getCells()[cellId]);
            tmpPU->setWeight(
                getCompatiblePlacementTable()->cellType2sharedBELTypeOccupation[getCells()[cellId]->getCellType()]);
            placementUnits.push_back(tmpPU);
            placementUnpackedCells.push_back(tmpPU);
            cellId2PlacementUnitVec[cellId] = tmpPU;
            cellId2PlacementUnit[cellId] = tmpPU;

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> X >> Y;
            tmpPU->setAnchorLocationAndForgetTheOriginalOne(X, Y);

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
                tmpPU->setPlaced();

            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
            {
                tmpPU->setFixed();
                fixedPlacementUnits.push_back(tmpPU);
            }

            bool shouldLock = false;
            std::getline(infile, line);
            iss = std::istringstream(line);
            iss >> fill0 >> booleanValue;
            if (booleanValue)
            {
                shouldLock = true;
            }

            if (tmpPU->isFixed())
            {
                std::getline(infile, line);
                iss = std::istringstream(line);
                iss >> fill0 >> fill1 >> siteName >> fill2 >> BELName;
                tmpPU->setLockedAt(siteName, BELName, deviceInfo);
            }

            if (shouldLock) // cannot set locked before other information is loaded
                tmpPU->setLocked();
        }
        else if (fill0.find("PULegalization:") != std::string::npos)
        {
            std::vector<std::string> tmpSiteNames;
            tmpSiteNames.clear();
            strSplit(PUName, tmpSiteNames, ",");
            assert(PUId < placementUnits.size());
            PU2LegalSites[placementUnits[PUId]] = std::vector<DeviceInfo::DeviceSite *>();
            for (auto siteName : tmpSiteNames)
            {
                DeviceInfo::DeviceSite *tmpSite = deviceInfo->getSiteWithName(siteName);
                tmpSite->setMapped(); // so later packing or legalization will bypass this site
                PU2LegalSites[placementUnits[PUId]].push_back(tmpSite);
            }
            PULegalXY.first[placementUnits[PUId]] = PU2LegalSites[placementUnits[PUId]][0]->X();
            PULegalXY.second[placementUnits[PUId]] = PU2LegalSites[placementUnits[PUId]][0]->Y();
        }
        else
        {
            std::cout << "This line : (" << std::string(line) << ") cannot be parsed.\n";
            assert(false);
        }
    }

    setProgress(0.8);
    updateElementBinGrid();
    reloadNets();
}

void PlacementInfo::checkClockUtilization(bool dump)
{
    clockNetCoverages.clear();
    clockRegionUtilization.clear();
    clockRegionUtilization.resize(deviceInfo->getClockRegionNumY(),
                                  std::vector<int>(deviceInfo->getClockRegionNumX(), 0));
    auto &clockRegions = deviceInfo->getClockRegions();

    for (int i = deviceInfo->getClockRegionNumY() - 1; i >= 0; i--)
    {
        for (int j = 0; j < deviceInfo->getClockRegionNumX(); j++)
        {
            clockRegions[i][j]->resetClockUtilizationInfo();
        }
    }

    for (auto curClockNet : clockNets)
    {
        int leftId, rightId, topId, bottomId;
        deviceInfo->getClockRegionByLocation(curClockNet->getLeftPinX(), curClockNet->getBottomPinY(), leftId,
                                             bottomId);
        deviceInfo->getClockRegionByLocation(curClockNet->getRightPinX(), curClockNet->getTopPinY(), rightId, topId);
        assert(leftId >= 0 && leftId < deviceInfo->getClockRegionNumX());
        assert(rightId >= 0 && rightId < deviceInfo->getClockRegionNumX());
        assert(topId >= 0 && topId < deviceInfo->getClockRegionNumY());
        assert(bottomId >= 0 && bottomId < deviceInfo->getClockRegionNumY());
        for (int i = bottomId; i <= topId; i++)
        {
            for (int j = leftId; j <= rightId; j++)
            {
                clockRegionUtilization[i][j] += 1;
            }
        }
    }

    for (auto curClockNet : clockNets)
    {
        auto designNet = curClockNet->getDesignNet();
        for (auto curPin : designNet->getPins())
        {
            auto curCell = curPin->getCell();
            auto loc = cellId2location[curCell->getCellId()];
            int regionX, regionY;
            deviceInfo->getClockRegionByLocation(loc.X, loc.Y, regionX, regionY);
            deviceInfo->recordClockRelatedCell(loc.X, loc.Y, regionX, regionY, curCell->getCellId(),
                                               designNet->getElementIdInType());
        }
    }

    // print_info("Clock region untilization:");
    // for (int i = deviceInfo->getClockRegionNumY() - 1; i >= 0; i--)
    // {
    //     for (int j = 0; j < deviceInfo->getClockRegionNumX(); j++)
    //     {
    //         std::cout << std::left << std::setw(4) << clockRegionUtilization[i][j];
    //     }
    //     std::cout << "\n";
    // }

    // print_info("Clock column untilization:");
    // for (int i = deviceInfo->getClockRegionNumY() - 1; i >= 0; i--)
    // {
    //     for (int j = 0; j < deviceInfo->getClockRegionNumX(); j++)
    //     {
    //         std::cout << std::left << std::setw(4) << deviceInfo->getMaxUtilizationOfClockColumns_InClockRegion(j, i);
    //     }
    //     std::cout << "\n";
    // }
}