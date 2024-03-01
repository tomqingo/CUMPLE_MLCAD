#include "DeviceInfo.h"
#include "readZip.h"
#include "strPrint.h"
#include "stringCheck.h"
#include <algorithm>
#include <assert.h>
#include "xcvu3p.h"


std::string ReduceStringSpace(std::string strorg)
{
    bool flag_findspace = true;
    std::string strout;
    //std::cout<<strorg<<std::endl;
    for(int i=0; i<strorg.size(); i++)
    {
        if(flag_findspace && ((strorg[i] == ' ')||(strorg[i] == '\t')))
            continue;
        strout.push_back(strorg[i]);

        if(strorg[i] != ' ')
            flag_findspace = false;
        else
            flag_findspace = true;

    }
    return strout;
}

bool siteSortCmp(DeviceInfo::DeviceSite *a, DeviceInfo::DeviceSite *b)
{
    const float eps = 1e-3;
    if (a->Y() <= b->Y() + eps && a->Y() >= b->Y() - eps)
        return a->X() < b->X();
    else if (a->Y() < b->Y() - eps)
        return true;
    else if (a->Y() > b->Y() + eps)
        return false;
    return true;
}

DeviceInfo::DeviceInfo(std::map<std::string, std::string> &JSONCfg, std::string _deviceName) : JSONCfg(JSONCfg)
{
    // deviceArchievedTextFileName = std::string(JSONCfg["vivado extracted device information file"]);
    // specialPinOffsetFileName = std::string(JSONCfg["special pin offset info file"]);
    SclFileName = std::string(JSONCfg["designPath"])+"/design.scl";

    // site=> SLICE_X59Y220 tile=> CLEL_R_X36Y220 sitetype=> SLICEL tiletype=> CLE_R centerx=> 37.25 centery=> 220.15
    // BELs=>
    // [SLICE_X59Y220/A5LUT,SLICE_X59Y220/A6LUT,SLICE_X59Y220/AFF,SLICE_X59Y220/AFF2,SLICE_X59Y220/B5LUT,SLICE_X59Y220/B6LUT,SLICE_X59Y220/BFF,SLICE_X59Y220/BFF2,SLICE_X59Y220/C5LUT,SLICE_X59Y220/C6LUT,SLICE_X59Y220/CARRY8,SLICE_X59Y220/CFF,SLICE_X59Y220/CFF2,SLICE_X59Y220/D5LUT,SLICE_X59Y220/D6LUT,SLICE_X59Y220/DFF,SLICE_X59Y220/DFF2,SLICE_X59Y220/E5LUT,SLICE_X59Y220/E6LUT,SLICE_X59Y220/EFF,SLICE_X59Y220/EFF2,SLICE_X59Y220/F5LUT,SLICE_X59Y220/F6LUT,SLICE_X59Y220/F7MUX_AB,SLICE_X59Y220/F7MUX_CD,SLICE_X59Y220/F7MUX_EF,SLICE_X59Y220/F7MUX_GH,SLICE_X59Y220/F8MUX_BOT,SLICE_X59Y220/F8MUX_TOP,SLICE_X59Y220/F9MUX,SLICE_X59Y220/FFF,SLICE_X59Y220/FFF2,SLICE_X59Y220/G5LUT,SLICE_X59Y220/G6LUT,SLICE_X59Y220/GFF,SLICE_X59Y220/GFF2,SLICE_X59Y220/H5LUT,SLICE_X59Y220/H6LUT,SLICE_X59Y220/HFF,SLICE_X59Y220/HFF2]
    // site=> RAMB18_X7Y88 tile=> BRAM_X36Y220 sitetype=> RAMBFIFO18 tiletype=> CLE_R centerx=> 36.75 centery=>
    // 221.96249999999998 BELs=> [RAMB18_X7Y88/RAMBFIFO18] site=> RAMB18_X7Y89 tile=> BRAM_X36Y220 sitetype=> RAMB181
    // tiletype=> CLE_R centerx=> 36.75 centery=> 224.11249999999998 BELs=> [RAMB18_X7Y89/RAMB18E2_U]

    BELTypes.clear();
    BELType2BELs.clear();
    BELs.clear();

    siteTypes.clear();
    siteType2Sites.clear();
    sites.clear();

    tileTypes.clear();
    tileType2Tiles.clear();
    tiles.clear();

    BELType2FalseBELType.clear();
    coord2ClockRegion.clear();

	// ***hard code***
    // if (JSONCfg.find("mergedSharedCellType2sharedCellType") != JSONCfg.end())
    // {
        // loadBELType2FalseBELType(JSONCfg["mergedSharedCellType2sharedCellType"]);
        loadBELType2FalseBELType("");
    // }

    deviceName = _deviceName;

    loadSiteY2SclColumnDict(SclFileName);

    // std::string unzipCmnd = "unzip -p " + deviceArchievedTextFileName;
    // FILEbuf sbuf(popen(unzipCmnd.c_str(), "r"));
    // std::istream infile(&sbuf);
    // // std::ifstream infile(designTextFileName.c_str());

	std::vector<std::string> deviceVec = loadxcvu3p();
	int elemNum = 8;
	int siteNum = deviceVec.size() / elemNum;

    std::string line;
    std::string siteName, tileName, siteType, tileType, strBELs, clockRegionName, fill0, fill1, fill2, fill3, fill4,
        fill5, fill6, fill7;
    float centerX, centerY;

    // while (std::getline(infile, line))
    // {
    //     std::istringstream iss(line);
    //     iss >> fill0 >> siteName >> fill1 >> tileName >> fill7 >> clockRegionName >> fill2 >> siteType >> fill3 >>
    //         tileType >> fill4 >> centerX >> fill5 >> centerY >> fill6 >> strBELs;
    //     assert(fill0 == "site=>");
    //     assert(fill1 == "tile=>");
    //     assert(fill7 == "clockRegionName=>");
    //     assert(fill2 == "sitetype=>");
    //     assert(fill3 == "tiletype=>");
	for (int si = 0; si < siteNum; si ++) {
        fill0 = "site=>"; 
		fill1 = "tile=>"; 
		fill7 = "clockRegionName=>";
		fill2 = "sitetype=>";
		fill3 = "tiletype=>";
		fill4 = "centerx=>";
		fill5 = "centery=>";
		fill6 = "BELs=>";
		siteName         = deviceVec[si*elemNum];
		tileName         = deviceVec[si*elemNum + 1] ;
		clockRegionName  = deviceVec[si*elemNum + 2];
		siteType         = deviceVec[si*elemNum + 3];
        tileType         = deviceVec[si*elemNum + 4]; 
		centerX = std::stof(deviceVec[si*elemNum + 5]);
		centerY = std::stof(deviceVec[si*elemNum + 6]);
		strBELs          = deviceVec[si*elemNum + 7];

		if (strContains(siteName, "SLICE_X"))
			strBELs = "[A5LUT,A6LUT,AFF,AFF2,B5LUT,B6LUT,BFF,BFF2,C5LUT,C6LUT,CARRY8,CFF,CFF2,D5LUT,D6LUT,DFF,DFF2,E5LUT,E6LUT,EFF,EFF2,F5LUT,F6LUT,F7MUX_AB,F7MUX_CD,F7MUX_EF,F7MUX_GH,F8MUX_BOT,F8MUX_TOP,F9MUX,FFF,FFF2,G5LUT,G6LUT,GFF,GFF2,H5LUT,H6LUT,HFF,HFF2]";
		else if (strContains(siteName, "DSP48E2_X"))
			strBELs = "[DSP_ALU,DSP_A_B_DATA,DSP_C_DATA,DSP_MULTIPLIER,DSP_M_DATA,DSP_OUTPUT,DSP_PREADD,DSP_PREADD_DATA]";
		
		// std::cout << siteName << " " << tileName << " " << clockRegionName << " " << siteType << " " << tileType << " " << centerX << " " << centerY << " " << strBELs << std::endl;

        std::string tmpFrom = "X";
        std::string tmpTo = "";
        replaceAll(clockRegionName, tmpFrom, tmpTo);
        tmpFrom = "Y";
        tmpTo = " ";
        replaceAll(clockRegionName, tmpFrom, tmpTo);
        std::vector<std::string> coordNumbers(0);
        strSplit(clockRegionName, coordNumbers, " ");
        assert(coordNumbers.size() == 2);
        int clockRegionX = std::stoi(coordNumbers[0]);
        int clockRegionY = std::stoi(coordNumbers[1]);
        if (clockRegionX + 1 > clockRegionNumX)
            clockRegionNumX = clockRegionX + 1;
        if (clockRegionY + 1 > clockRegionNumY)
            clockRegionNumY = clockRegionY + 1;

        std::pair<int, int> clockRegionCoord(clockRegionX, clockRegionY);

        // print("site=> " + curSite.siteName
        //       + " tile=> " + curSite.tileName
        //       + " clockRegionName=> " + curSite.clockRegionName
        //       + " sitetype=> " + curSite.siteType
        //       + " tiletype=> " + curSite.tileType
        //       + " centerx=> " + str(cx)
        //       + " centery=> " + str(cy)
        //       + " BELs=> " + str(curSite.BELs).replace(" ", "").replace("\'", ""), file=exportfile)

        if (strContains(fill0, "site=>"))
        {
            addTile(tileName, tileType);
            DeviceTile *curTile = name2Tile[tileName];
            addSite(siteName, siteType, centerX, centerY, clockRegionX, clockRegionY, curTile);

            DeviceSite *curSite = name2Site[siteName];

            int siteX = curSite->getSiteX();
            int siteY = curSite->getSiteY();

            int SclX, SclY;
            float locX = curSite->X();
            float locY = curSite->Y();

            if(siteType.find("DSP") != siteType.npos)
            {
                SclX = DSPcolumnlist[siteX];
            }
            else
            {
                if(siteType.find("RAMB") != siteType.npos)
                    SclX = BRAMcolumnlist[siteX];
                else
                {
                    if(siteType.find("SLICE") != siteType.npos)
                        SclX = Slicecolumnlist[siteX];
                    else
                    {
                        if(siteType.find("HPIOB") != siteType.npos)
                            SclX = IOcolumnlist[siteX];
                    }
                }
            }

            if(SclX2realMapX.find(SclX) == SclX2realMapX.end())
                SclX2realMapX[SclX] = locX;

            if(siteType.find("SLICEL") != siteType.npos)
            {
                SclY = siteY;
                SclY2realMapY[SclY] = locY;
            }
            
            if (coord2ClockRegion.find(clockRegionCoord) == coord2ClockRegion.end())
            {
                ClockRegion *newCR = new ClockRegion(curSite);
                coord2ClockRegion[clockRegionCoord] = newCR;
            }
            else
            {
                coord2ClockRegion[clockRegionCoord]->addSite(curSite);
            }
            strBELs = strBELs.substr(1, strBELs.size() - 2); // remove [] in string
            std::vector<std::string> BELnames;
            strSplit(strBELs, BELnames, ",");

			if (strContains(siteName, "SLICE_X") || strContains(siteName, "DSP48E2_X")) {
            	for (std::string BELname : BELnames)
            	{
					std::string fullName = siteName + "/" + BELname;
	                addBEL(fullName, BELname, curSite);
				}
			} else {
            	for (std::string BELname : BELnames)
            	{
            	    std::vector<std::string> splitedName;
            	    strSplit(BELname, splitedName, "/");
            	    addBEL(BELname, splitedName[1], curSite);
            	}
			}
        }
        else
            assert(false && "Parser Error");
    }

    if(verbose)
    {
        std::cout<<"X axis correspondings: ";
        for(auto &SclXRealXpair : SclX2realMapX)
        {
            std::cout<<"("<<SclXRealXpair.first<<" "<<SclXRealXpair.second<<"),";
        }
        std::cout<<std::endl;

        std::cout<<"Y axis correspondings: ";
        for(auto &SclYRealYpair : SclY2realMapY)
        {
            std::cout<<"("<<SclYRealYpair.first<<" "<<SclYRealYpair.second<<"),";
        }
        std::cout<<std::endl;
    }  

    std::sort(sites.begin(), sites.end(), siteSortCmp);

    std::map<std::string, std::vector<DeviceSite *>>::iterator tmpIt;
    for (tmpIt = siteType2Sites.begin(); tmpIt != siteType2Sites.end(); tmpIt++)
    {
        std::sort(tmpIt->second.begin(), tmpIt->second.end(), siteSortCmp);
    }

    // loadPCIEPinOffset(specialPinOffsetFileName);

    print_info("There are " + std::to_string(clockRegionNumY) + "x" + std::to_string(clockRegionNumX) +
                "(YxX) clock regions on the device");

    mapClockRegionToArray();
	IOMap.clear();
	loadIOMap();
    print_status("New Device Info Created.");
}

void DeviceInfo::mapClockRegionToArray()
{
    clockRegions.clear();
    clockRegions.resize(clockRegionNumY, std::vector<ClockRegion *>(clockRegionNumX, nullptr));
    for (int i = 0; i < clockRegionNumY; i++)
    {
        for (int j = 0; j < clockRegionNumX; j++)
        {
            std::pair<int, int> clockRegionCoord(j, i);
            assert(coord2ClockRegion.find(clockRegionCoord) != coord2ClockRegion.end());
            clockRegions[i][j] = coord2ClockRegion[clockRegionCoord];
        }
    }
    for (int i = 0; i < clockRegionNumY; i++)
    {
        int j = 0;
        clockRegions[i][j]->setLeft(clockRegions[i][j]->getLeft() - boundaryTolerance);
    }
    for (int i = 0; i < clockRegionNumY; i++)
    {
        int j = clockRegionNumX - 1;
        clockRegions[i][j]->setRight(clockRegions[i][j]->getRight() + boundaryTolerance);
    }
    for (int j = 0; j < clockRegionNumX; j++)
    {
        int i = 0;
        clockRegions[i][j]->setBottom(clockRegions[i][j]->getBottom() - boundaryTolerance);
    }
    for (int j = 0; j < clockRegionNumX; j++)
    {
        int i = clockRegionNumY - 1;
        clockRegions[i][j]->setTop(clockRegions[i][j]->getTop() + boundaryTolerance);
    }

    for (int i = 0; i < clockRegionNumY; i++)
    {
        for (int j = 0; j < clockRegionNumX; j++)
        {
            if (j > 0)
            {
                float midX = (clockRegions[i][j - 1]->getRight() + clockRegions[i][j]->getLeft()) / 2;
                clockRegions[i][j]->setLeft(midX);
                clockRegions[i][j - 1]->setRight(midX);
            }
            if (i > 0)
            {
                float midY = (clockRegions[i - 1][j]->getTop() + clockRegions[i][j]->getBottom()) / 2;
                clockRegions[i - 1][j]->setTop(midY);
                clockRegions[i][j]->setBottom(midY);
            }
        }
    }

    clockRegionXBounds.clear();
    clockRegionYBounds.clear();
    for (int j = 0; j < clockRegionNumX; j++)
    {
        clockRegionXBounds.push_back(clockRegions[0][j]->getLeft());
    }
    for (int i = 0; i < clockRegionNumY; i++)
    {
        clockRegionYBounds.push_back(clockRegions[i][0]->getBottom());
    }

    for (int i = 0; i < clockRegionNumY; i++)
    {
        for (int j = 0; j < clockRegionNumX; j++)
        {
            assert(clockRegions[i][j]->getLeft() == clockRegionXBounds[j]);
            assert(clockRegions[i][j]->getBottom() == clockRegionYBounds[i]);
            clockRegions[i][j]->mapSiteToClockColumns();
        }
    }
}

void DeviceInfo::ClockRegion::mapSiteToClockColumns()
{
    assert(sites.size() > 0);
    std::sort(sites.begin(), sites.end(), [](DeviceSite *a, DeviceSite *b) -> bool {
        return a->getSiteY() == b->getSiteY() ? (a->getSiteX() < b->getSiteX()) : a->getSiteY() < b->getSiteY();
    });

    assert(sites.size() > 0);
    for (unsigned int i = 0; i < sites.size(); i++)
    {
        if (sites[i]->getName().find("SLICE") != std::string::npos)
        {
            leftSiteX = sites[i]->getSiteX();
            bottomSiteY = sites[i]->getSiteY();
            break;
        }
    }
    for (int i = sites.size() - 1; i >= 0; i--)
    {
        if (sites[i]->getName().find("SLICE") != std::string::npos)
        {
            rightSiteX = sites[i]->getSiteX();
            topSiteY = sites[i]->getSiteY();
            break;
        }
    }

    clockColumns = std::vector<std::vector<ClockColumn>>(
        columnNumY, std::vector<ClockColumn>(rightSiteX - leftSiteX + 1, ClockColumn()));

    int eachLevelY = (topSiteY - bottomSiteY + 1) / columnNumY;
    for (auto curSite : sites)
    {
        if (curSite->getName().find("SLICE") == std::string::npos)
        {
            // currently, BRAM/DSP slices will not lead to clock utilization overflow
            continue;
        }
        int levelY = (curSite->getSiteY() - bottomSiteY) / eachLevelY;
        assert(levelY >= 0);
        assert(levelY < (int)clockColumns.size());
        assert(curSite->getSiteX() - leftSiteX >= 0);
        assert(curSite->getSiteX() - leftSiteX < (int)clockColumns[levelY].size());
        clockColumns[levelY][curSite->getSiteX() - leftSiteX].addSite(curSite);
    }

    assert(clockColumns.size() > 0);
    colHeight = (topY - bottomY) / columnNumY;
    colWidth = (rightX - leftX) / clockColumns[0].size();

    float offsetY = bottomY;
    for (int levelY = 0; levelY < columnNumY; levelY++)
    {
        float offsetX = leftX;
        for (unsigned int colOffset = 0; colOffset < clockColumns[levelY].size(); colOffset++)
        {
            clockColumns[levelY][colOffset].setBoundary(offsetX, offsetX + colWidth, offsetY + colHeight, offsetY);
            offsetX += colWidth;
        }
        offsetY += colHeight;
    }
}

void DeviceInfo::recordClockRelatedCell(float locX, float locY, int regionX, int regionY, int cellId, int netId)
{
    auto clockRegion = clockRegions[regionY][regionX];
    clockRegion->addClockAndCell(netId, cellId, locX, locY);
}

void DeviceInfo::loadPCIEPinOffset(std::string specialPinOffsetFileName)
{

    std::ifstream infile(specialPinOffsetFileName.c_str());

    std::string line;
    std::string refpinname, fill0, fill1, fill2;
    float offsetX, offsetY;

    std::vector<std::string> refPinsName;
    std::map<std::string, float> name2offsetX;
    std::map<std::string, float> name2offsetY;
    refPinsName.clear();
    name2offsetY.clear();
    name2offsetX.clear();

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        iss >> fill0 >> refpinname >> fill1 >> offsetX >> fill2 >> offsetY;
        if (strContains(fill0, "pin=>"))
        {
            refPinsName.push_back(refpinname);
            name2offsetX[refpinname] = offsetX;
            name2offsetY[refpinname] = offsetY;
        }
        else
            assert(false && "Parser Error");
    }

    std::string STR_PCIE_3_1 = "PCIE_3_1";
    for (DeviceSite *PCIESite : getSitesInType(STR_PCIE_3_1))
    {
        DeviceSite::DeviceSitePinInfos *sitePins = new DeviceSite::DeviceSitePinInfos();
        sitePins->refPinsName = refPinsName;
        sitePins->name2offsetX = name2offsetX;
        sitePins->name2offsetY = name2offsetY;
        PCIESite->setSitePinInfos(sitePins);
    }
}

void DeviceInfo::DeviceBEL::setSite(DeviceElement *parentPtr)
{
    DeviceSite *sitePtr = dynamic_cast<DeviceSite *>(parentPtr);
    assert(sitePtr);
    site = sitePtr;
}

void DeviceInfo::DeviceSite::setTile(DeviceElement *parentTilePtr)
{
    DeviceTile *tilePtr = dynamic_cast<DeviceTile *>(parentTilePtr);
    assert(tilePtr);
    parentTile = tilePtr;
}

void DeviceInfo::DeviceSite::setParentSite(DeviceElement *parentSitePtr)
{
}

void DeviceInfo::DeviceSite::addChildSite(DeviceElement *sitePtr)
{
}

void DeviceInfo::DeviceTile::addChildSite(DeviceInfo::DeviceElement *sitePtr)
{
    DeviceSite *childSite = dynamic_cast<DeviceSite *>(sitePtr);
    assert(childSite);
    childrenSites.push_back(childSite);
}

void DeviceInfo::printStat(bool verbose)
{
    print_info("#ExtractedTile= " + std::to_string(tiles.size()));
    print_info("#ExtractedSite= " + std::to_string(sites.size()));
    print_info("#ExtractedBEL= " + std::to_string(BELs.size()));

    std::string existTypes = "";
    for (std::string tmptype : tileTypes)
    {
        existTypes += tmptype + " ";
    }
    print_info("Tile(" + std::to_string(tileTypes.size()) + " types): " + existTypes);
    if (verbose)
    {
        for (std::map<std::string, std::vector<DeviceTile *>>::iterator it = tileType2Tiles.begin();
             it != tileType2Tiles.end(); ++it)
        {
            print_info("#" + it->first + ": " + std::to_string(it->second.size()));
        }
    }

    existTypes = "";
    for (std::string tmptype : siteTypes)
    {
        existTypes += tmptype + " ";
    }
    print_info("Site(" + std::to_string(siteTypes.size()) + " types): " + existTypes);
    if (verbose)
    {
        for (std::map<std::string, std::vector<DeviceSite *>>::iterator it = siteType2Sites.begin();
             it != siteType2Sites.end(); ++it)
        {
            print_info("#" + it->first + ": " + std::to_string(it->second.size()));
        }
    }

    existTypes = "";
    for (std::string tmptype : BELTypes)
    {
        existTypes += tmptype + " ";
    }
    print_info("BEL(" + std::to_string(BELTypes.size()) + " types): " + existTypes);
    if (verbose)
    {
        for (std::map<std::string, std::vector<DeviceBEL *>>::iterator it = BELType2BELs.begin();
             it != BELType2BELs.end(); ++it)
        {
            print_info("#" + it->first + ": " + std::to_string(it->second.size()));
        }
    }
}

void DeviceInfo::addBEL(std::string &BELName, std::string &BELType, DeviceSite *parent)
{
    assert(name2BEL.find(BELName) == name2BEL.end());

    DeviceBEL *newBEL = new DeviceBEL(BELName, BELType, parent, parent->getChildrenSites().size());
    BELs.push_back(newBEL);
    name2BEL[BELName] = newBEL;

    addBELTypes(newBEL->getBELType());

    if (BELType2BELs.find(newBEL->getBELType()) == BELType2BELs.end())
        BELType2BELs[newBEL->getBELType()] = std::vector<DeviceBEL *>();
    BELType2BELs[newBEL->getBELType()].push_back(newBEL);

    parent->addChildBEL(newBEL);
}

void DeviceInfo::addSite(std::string &siteName, std::string &siteType, float locx, float locy, int clockRegionX,
                         int clockRegionY, DeviceTile *parentTile)
{
    assert(name2Site.find(siteName) == name2Site.end());

    DeviceSite *newSite = new DeviceSite(siteName, siteType, parentTile, locx, locy, clockRegionX, clockRegionY,
                                         parentTile->getChildrenSites().size());
    sites.push_back(newSite);
    name2Site[siteName] = newSite;

    addSiteTypes(newSite->getSiteType());

    if (siteType2Sites.find(newSite->getSiteType()) == siteType2Sites.end())
        siteType2Sites[newSite->getSiteType()] = std::vector<DeviceSite *>();
    siteType2Sites[newSite->getSiteType()].push_back(newSite);

    parentTile->addChildSite(newSite);
}

void DeviceInfo::addTile(std::string &tileName, std::string &tileType)
{
    if (name2Tile.find(tileName) == name2Tile.end())
    {
        DeviceTile *tile = new DeviceTile(tileName, tileType, this, tiles.size());
        tiles.push_back(tile);
        name2Tile[tileName] = tile;

        addTileTypes(tile->getTileType());

        if (tileType2Tiles.find(tile->getTileType()) == tileType2Tiles.end())
            tileType2Tiles[tile->getTileType()] = std::vector<DeviceTile *>();
        tileType2Tiles[tile->getTileType()].push_back(tile);
    }
}

void DeviceInfo::loadBELType2FalseBELType(std::string curFileName)
{
    std::string line;
    // std::ifstream infile(curFileName.c_str());
	std::vector<std::pair<std::string, std::string>> vec {
		{"SLICEM_CARRY8", "SLICEL_CARRY8"},
		{"SLICEM_LUT", "SLICEL_LUT"},
		{"SLICEM_FF", "SLICEL_FF"},
		{"RAMB18E2_U", "RAMB18E2_L"},
	};
    BELType2FalseBELType.clear();
    // while (std::getline(infile, line))
    // {
        // std::vector<std::string> BELTypePair;
        // strSplit(line, BELTypePair, " ");
        // BELType2FalseBELType[BELTypePair[0]] = BELTypePair[1];
	for (auto & elem : vec) {
        BELType2FalseBELType[elem.first] = elem.second;
        print_info("mapping BELType [" + elem.first + "] to [" + BELType2FalseBELType[elem.first] +
                   "] when creating PlacementBins");
    }
}

void DeviceInfo::loadIOMap() 
{
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 0), "IOB_X0Y25"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 1), "IOB_X0Y12"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 2), "IOB_X0Y0"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 3), "IOB_X0Y1"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 4), "IOB_X0Y2"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 5), "IOB_X0Y3"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 6), "IOB_X0Y4"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 7), "IOB_X0Y5"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 8), "IOB_X0Y6"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 9), "IOB_X0Y7"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 10), "IOB_X0Y8"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 11), "IOB_X0Y9"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 12), "IOB_X0Y10"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 13), "IOB_X0Y11"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 14), "IOB_X0Y13"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 15), "IOB_X0Y14"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 16), "IOB_X0Y15"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 17), "IOB_X0Y16"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 18), "IOB_X0Y17"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 19), "IOB_X0Y18"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 20), "IOB_X0Y19"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 21), "IOB_X0Y20"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 22), "IOB_X0Y21"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 23), "IOB_X0Y22"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 24), "IOB_X0Y23"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 0, 25), "IOB_X0Y24"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 0), "IOB_X0Y51"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 1), "IOB_X0Y38"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 2), "IOB_X0Y26"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 3), "IOB_X0Y27"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 4), "IOB_X0Y28"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 5), "IOB_X0Y29"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 6), "IOB_X0Y30"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 7), "IOB_X0Y31"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 8), "IOB_X0Y32"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 9), "IOB_X0Y33"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 10), "IOB_X0Y34"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 11), "IOB_X0Y35"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 12), "IOB_X0Y36"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 13), "IOB_X0Y37"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 14), "IOB_X0Y39"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 15), "IOB_X0Y40"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 16), "IOB_X0Y41"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 17), "IOB_X0Y42"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 18), "IOB_X0Y43"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 19), "IOB_X0Y44"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 20), "IOB_X0Y45"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 21), "IOB_X0Y46"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 22), "IOB_X0Y47"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 23), "IOB_X0Y48"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 24), "IOB_X0Y49"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 30, 25), "IOB_X0Y50"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 0), "IOB_X0Y77"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 1), "IOB_X0Y64"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 2), "IOB_X0Y52"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 3), "IOB_X0Y53"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 4), "IOB_X0Y54"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 5), "IOB_X0Y55"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 6), "IOB_X0Y56"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 7), "IOB_X0Y57"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 8), "IOB_X0Y58"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 9), "IOB_X0Y59"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 10), "IOB_X0Y60"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 11), "IOB_X0Y61"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 12), "IOB_X0Y62"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 13), "IOB_X0Y63"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 14), "IOB_X0Y65"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 15), "IOB_X0Y66"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 16), "IOB_X0Y67"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 17), "IOB_X0Y68"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 18), "IOB_X0Y69"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 19), "IOB_X0Y70"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 20), "IOB_X0Y71"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 21), "IOB_X0Y72"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 22), "IOB_X0Y73"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 23), "IOB_X0Y74"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 24), "IOB_X0Y75"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 60, 25), "IOB_X0Y76"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 0), "IOB_X0Y103"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 1), "IOB_X0Y90"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 2), "IOB_X0Y78"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 3), "IOB_X0Y79"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 4), "IOB_X0Y80"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 5), "IOB_X0Y81"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 6), "IOB_X0Y82"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 7), "IOB_X0Y83"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 8), "IOB_X0Y84"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 9), "IOB_X0Y85"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 10), "IOB_X0Y86"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 11), "IOB_X0Y87"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 12), "IOB_X0Y88"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 13), "IOB_X0Y89"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 14), "IOB_X0Y91"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 15), "IOB_X0Y92"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 16), "IOB_X0Y93"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 17), "IOB_X0Y94"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 18), "IOB_X0Y95"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 19), "IOB_X0Y96"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 20), "IOB_X0Y97"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 21), "IOB_X0Y98"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 22), "IOB_X0Y99"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 23), "IOB_X0Y100"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 24), "IOB_X0Y101"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 90, 25), "IOB_X0Y102"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 0), "IOB_X0Y129"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 1), "IOB_X0Y116"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 2), "IOB_X0Y104"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 3), "IOB_X0Y105"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 4), "IOB_X0Y106"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 5), "IOB_X0Y107"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 6), "IOB_X0Y108"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 7), "IOB_X0Y109"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 8), "IOB_X0Y110"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 9), "IOB_X0Y111"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 10), "IOB_X0Y112"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 11), "IOB_X0Y113"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 12), "IOB_X0Y114"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 13), "IOB_X0Y115"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 14), "IOB_X0Y117"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 15), "IOB_X0Y118"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 16), "IOB_X0Y119"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 17), "IOB_X0Y120"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 18), "IOB_X0Y121"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 19), "IOB_X0Y122"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 20), "IOB_X0Y123"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 21), "IOB_X0Y124"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 22), "IOB_X0Y125"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 23), "IOB_X0Y126"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 24), "IOB_X0Y127"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 120, 25), "IOB_X0Y128"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 0), "IOB_X0Y155"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 1), "IOB_X0Y142"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 2), "IOB_X0Y130"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 3), "IOB_X0Y131"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 4), "IOB_X0Y132"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 5), "IOB_X0Y133"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 6), "IOB_X0Y134"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 7), "IOB_X0Y135"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 8), "IOB_X0Y136"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 9), "IOB_X0Y137"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 10), "IOB_X0Y138"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 11), "IOB_X0Y139"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 12), "IOB_X0Y140"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 13), "IOB_X0Y141"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 14), "IOB_X0Y143"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 15), "IOB_X0Y144"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 16), "IOB_X0Y145"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 17), "IOB_X0Y146"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 18), "IOB_X0Y147"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 19), "IOB_X0Y148"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 20), "IOB_X0Y149"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 21), "IOB_X0Y150"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 22), "IOB_X0Y151"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 23), "IOB_X0Y152"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 24), "IOB_X0Y153"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 150, 25), "IOB_X0Y154"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 0), "IOB_X0Y181"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 1), "IOB_X0Y168"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 2), "IOB_X0Y156"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 3), "IOB_X0Y157"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 4), "IOB_X0Y158"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 5), "IOB_X0Y159"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 6), "IOB_X0Y160"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 7), "IOB_X0Y161"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 8), "IOB_X0Y162"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 9), "IOB_X0Y163"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 10), "IOB_X0Y164"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 11), "IOB_X0Y165"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 12), "IOB_X0Y166"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 13), "IOB_X0Y167"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 14), "IOB_X0Y169"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 15), "IOB_X0Y170"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 16), "IOB_X0Y171"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 17), "IOB_X0Y172"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 18), "IOB_X0Y173"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 19), "IOB_X0Y174"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 20), "IOB_X0Y175"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 21), "IOB_X0Y176"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 22), "IOB_X0Y177"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 23), "IOB_X0Y178"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 24), "IOB_X0Y179"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 180, 25), "IOB_X0Y180"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 0), "IOB_X0Y207"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 1), "IOB_X0Y194"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 2), "IOB_X0Y182"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 3), "IOB_X0Y183"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 4), "IOB_X0Y184"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 5), "IOB_X0Y185"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 6), "IOB_X0Y186"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 7), "IOB_X0Y187"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 8), "IOB_X0Y188"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 9), "IOB_X0Y189"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 10), "IOB_X0Y190"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 11), "IOB_X0Y191"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 12), "IOB_X0Y192"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 13), "IOB_X0Y193"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 14), "IOB_X0Y195"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 15), "IOB_X0Y196"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 16), "IOB_X0Y197"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 17), "IOB_X0Y198"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 18), "IOB_X0Y199"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 19), "IOB_X0Y200"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 20), "IOB_X0Y201"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 21), "IOB_X0Y202"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 22), "IOB_X0Y203"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 23), "IOB_X0Y204"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 24), "IOB_X0Y205"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 210, 25), "IOB_X0Y206"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 0), "IOB_X0Y233"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 1), "IOB_X0Y220"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 2), "IOB_X0Y208"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 3), "IOB_X0Y209"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 4), "IOB_X0Y210"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 5), "IOB_X0Y211"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 6), "IOB_X0Y212"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 7), "IOB_X0Y213"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 8), "IOB_X0Y214"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 9), "IOB_X0Y215"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 10), "IOB_X0Y216"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 11), "IOB_X0Y217"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 12), "IOB_X0Y218"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 13), "IOB_X0Y219"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 14), "IOB_X0Y221"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 15), "IOB_X0Y222"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 16), "IOB_X0Y223"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 17), "IOB_X0Y224"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 18), "IOB_X0Y225"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 19), "IOB_X0Y226"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 20), "IOB_X0Y227"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 21), "IOB_X0Y228"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 22), "IOB_X0Y229"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 23), "IOB_X0Y230"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 24), "IOB_X0Y231"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 240, 25), "IOB_X0Y232"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 0), "IOB_X0Y259"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 1), "IOB_X0Y246"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 2), "IOB_X0Y234"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 3), "IOB_X0Y235"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 4), "IOB_X0Y236"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 5), "IOB_X0Y237"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 6), "IOB_X0Y238"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 7), "IOB_X0Y239"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 8), "IOB_X0Y240"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 9), "IOB_X0Y241"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 10), "IOB_X0Y242"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 11), "IOB_X0Y243"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 12), "IOB_X0Y244"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 13), "IOB_X0Y245"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 14), "IOB_X0Y247"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 15), "IOB_X0Y248"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 16), "IOB_X0Y249"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 17), "IOB_X0Y250"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 18), "IOB_X0Y251"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 19), "IOB_X0Y252"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 20), "IOB_X0Y253"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 21), "IOB_X0Y254"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 22), "IOB_X0Y255"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 23), "IOB_X0Y256"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 24), "IOB_X0Y257"));
	IOMap.emplace(std::make_pair(std::make_tuple(68, 270, 25), "IOB_X0Y258"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 0), "IOB_X1Y25"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 1), "IOB_X1Y12"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 2), "IOB_X1Y0"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 3), "IOB_X1Y1"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 4), "IOB_X1Y2"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 5), "IOB_X1Y3"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 6), "IOB_X1Y4"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 7), "IOB_X1Y5"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 8), "IOB_X1Y6"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 9), "IOB_X1Y7"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 10), "IOB_X1Y8"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 11), "IOB_X1Y9"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 12), "IOB_X1Y10"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 13), "IOB_X1Y11"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 14), "IOB_X1Y13"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 15), "IOB_X1Y14"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 16), "IOB_X1Y15"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 17), "IOB_X1Y16"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 18), "IOB_X1Y17"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 19), "IOB_X1Y18"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 20), "IOB_X1Y19"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 21), "IOB_X1Y20"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 22), "IOB_X1Y21"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 23), "IOB_X1Y22"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 24), "IOB_X1Y23"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 0, 25), "IOB_X1Y24"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 0), "IOB_X1Y51"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 1), "IOB_X1Y38"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 2), "IOB_X1Y26"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 3), "IOB_X1Y27"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 4), "IOB_X1Y28"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 5), "IOB_X1Y29"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 6), "IOB_X1Y30"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 7), "IOB_X1Y31"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 8), "IOB_X1Y32"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 9), "IOB_X1Y33"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 10), "IOB_X1Y34"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 11), "IOB_X1Y35"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 12), "IOB_X1Y36"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 13), "IOB_X1Y37"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 14), "IOB_X1Y39"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 15), "IOB_X1Y40"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 16), "IOB_X1Y41"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 17), "IOB_X1Y42"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 18), "IOB_X1Y43"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 19), "IOB_X1Y44"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 20), "IOB_X1Y45"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 21), "IOB_X1Y46"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 22), "IOB_X1Y47"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 23), "IOB_X1Y48"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 24), "IOB_X1Y49"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 30, 25), "IOB_X1Y50"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 0), "IOB_X1Y77"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 1), "IOB_X1Y64"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 2), "IOB_X1Y52"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 3), "IOB_X1Y53"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 4), "IOB_X1Y54"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 5), "IOB_X1Y55"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 6), "IOB_X1Y56"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 7), "IOB_X1Y57"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 8), "IOB_X1Y58"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 9), "IOB_X1Y59"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 10), "IOB_X1Y60"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 11), "IOB_X1Y61"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 12), "IOB_X1Y62"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 13), "IOB_X1Y63"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 14), "IOB_X1Y65"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 15), "IOB_X1Y66"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 16), "IOB_X1Y67"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 17), "IOB_X1Y68"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 18), "IOB_X1Y69"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 19), "IOB_X1Y70"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 20), "IOB_X1Y71"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 21), "IOB_X1Y72"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 22), "IOB_X1Y73"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 23), "IOB_X1Y74"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 24), "IOB_X1Y75"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 60, 25), "IOB_X1Y76"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 0), "IOB_X1Y103"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 1), "IOB_X1Y90"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 2), "IOB_X1Y78"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 3), "IOB_X1Y79"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 4), "IOB_X1Y80"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 5), "IOB_X1Y81"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 6), "IOB_X1Y82"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 7), "IOB_X1Y83"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 8), "IOB_X1Y84"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 9), "IOB_X1Y85"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 10), "IOB_X1Y86"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 11), "IOB_X1Y87"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 12), "IOB_X1Y88"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 13), "IOB_X1Y89"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 14), "IOB_X1Y91"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 15), "IOB_X1Y92"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 16), "IOB_X1Y93"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 17), "IOB_X1Y94"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 18), "IOB_X1Y95"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 19), "IOB_X1Y96"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 20), "IOB_X1Y97"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 21), "IOB_X1Y98"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 22), "IOB_X1Y99"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 23), "IOB_X1Y100"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 24), "IOB_X1Y101"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 90, 25), "IOB_X1Y102"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 0), "IOB_X1Y129"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 1), "IOB_X1Y116"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 2), "IOB_X1Y104"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 3), "IOB_X1Y105"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 4), "IOB_X1Y106"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 5), "IOB_X1Y107"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 6), "IOB_X1Y108"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 7), "IOB_X1Y109"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 8), "IOB_X1Y110"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 9), "IOB_X1Y111"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 10), "IOB_X1Y112"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 11), "IOB_X1Y113"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 12), "IOB_X1Y114"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 13), "IOB_X1Y115"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 14), "IOB_X1Y117"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 15), "IOB_X1Y118"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 16), "IOB_X1Y119"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 17), "IOB_X1Y120"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 18), "IOB_X1Y121"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 19), "IOB_X1Y122"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 20), "IOB_X1Y123"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 21), "IOB_X1Y124"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 22), "IOB_X1Y125"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 23), "IOB_X1Y126"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 24), "IOB_X1Y127"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 120, 25), "IOB_X1Y128"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 0), "IOB_X1Y155"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 1), "IOB_X1Y142"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 2), "IOB_X1Y130"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 3), "IOB_X1Y131"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 4), "IOB_X1Y132"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 5), "IOB_X1Y133"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 6), "IOB_X1Y134"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 7), "IOB_X1Y135"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 8), "IOB_X1Y136"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 9), "IOB_X1Y137"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 10), "IOB_X1Y138"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 11), "IOB_X1Y139"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 12), "IOB_X1Y140"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 13), "IOB_X1Y141"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 14), "IOB_X1Y143"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 15), "IOB_X1Y144"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 16), "IOB_X1Y145"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 17), "IOB_X1Y146"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 18), "IOB_X1Y147"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 19), "IOB_X1Y148"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 20), "IOB_X1Y149"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 21), "IOB_X1Y150"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 22), "IOB_X1Y151"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 23), "IOB_X1Y152"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 24), "IOB_X1Y153"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 150, 25), "IOB_X1Y154"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 0), "IOB_X1Y181"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 1), "IOB_X1Y168"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 2), "IOB_X1Y156"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 3), "IOB_X1Y157"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 4), "IOB_X1Y158"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 5), "IOB_X1Y159"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 6), "IOB_X1Y160"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 7), "IOB_X1Y161"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 8), "IOB_X1Y162"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 9), "IOB_X1Y163"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 10), "IOB_X1Y164"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 11), "IOB_X1Y165"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 12), "IOB_X1Y166"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 13), "IOB_X1Y167"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 14), "IOB_X1Y169"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 15), "IOB_X1Y170"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 16), "IOB_X1Y171"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 17), "IOB_X1Y172"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 18), "IOB_X1Y173"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 19), "IOB_X1Y174"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 20), "IOB_X1Y175"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 21), "IOB_X1Y176"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 22), "IOB_X1Y177"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 23), "IOB_X1Y178"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 24), "IOB_X1Y179"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 180, 25), "IOB_X1Y180"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 0), "IOB_X1Y207"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 1), "IOB_X1Y194"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 2), "IOB_X1Y182"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 3), "IOB_X1Y183"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 4), "IOB_X1Y184"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 5), "IOB_X1Y185"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 6), "IOB_X1Y186"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 7), "IOB_X1Y187"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 8), "IOB_X1Y188"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 9), "IOB_X1Y189"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 10), "IOB_X1Y190"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 11), "IOB_X1Y191"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 12), "IOB_X1Y192"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 13), "IOB_X1Y193"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 14), "IOB_X1Y195"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 15), "IOB_X1Y196"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 16), "IOB_X1Y197"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 17), "IOB_X1Y198"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 18), "IOB_X1Y199"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 19), "IOB_X1Y200"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 20), "IOB_X1Y201"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 21), "IOB_X1Y202"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 22), "IOB_X1Y203"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 23), "IOB_X1Y204"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 24), "IOB_X1Y205"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 210, 25), "IOB_X1Y206"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 0), "IOB_X1Y233"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 1), "IOB_X1Y220"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 2), "IOB_X1Y208"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 3), "IOB_X1Y209"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 4), "IOB_X1Y210"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 5), "IOB_X1Y211"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 6), "IOB_X1Y212"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 7), "IOB_X1Y213"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 8), "IOB_X1Y214"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 9), "IOB_X1Y215"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 10), "IOB_X1Y216"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 11), "IOB_X1Y217"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 12), "IOB_X1Y218"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 13), "IOB_X1Y219"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 14), "IOB_X1Y221"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 15), "IOB_X1Y222"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 16), "IOB_X1Y223"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 17), "IOB_X1Y224"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 18), "IOB_X1Y225"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 19), "IOB_X1Y226"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 20), "IOB_X1Y227"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 21), "IOB_X1Y228"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 22), "IOB_X1Y229"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 23), "IOB_X1Y230"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 24), "IOB_X1Y231"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 240, 25), "IOB_X1Y232"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 0), "IOB_X1Y259"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 1), "IOB_X1Y246"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 2), "IOB_X1Y234"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 3), "IOB_X1Y235"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 4), "IOB_X1Y236"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 5), "IOB_X1Y237"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 6), "IOB_X1Y238"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 7), "IOB_X1Y239"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 8), "IOB_X1Y240"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 9), "IOB_X1Y241"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 10), "IOB_X1Y242"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 11), "IOB_X1Y243"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 12), "IOB_X1Y244"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 13), "IOB_X1Y245"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 14), "IOB_X1Y247"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 15), "IOB_X1Y248"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 16), "IOB_X1Y249"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 17), "IOB_X1Y250"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 18), "IOB_X1Y251"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 19), "IOB_X1Y252"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 20), "IOB_X1Y253"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 21), "IOB_X1Y254"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 22), "IOB_X1Y255"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 23), "IOB_X1Y256"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 24), "IOB_X1Y257"));
	IOMap.emplace(std::make_pair(std::make_tuple(138, 270, 25), "IOB_X1Y258"));
}