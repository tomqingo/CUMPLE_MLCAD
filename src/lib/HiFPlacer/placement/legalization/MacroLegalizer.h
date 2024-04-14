#ifndef _MACROLEGALIZER
#define _MACROLEGALIZER

#include "DesignInfo.h"
#include "DeviceInfo.h"
#include "MinCostBipartiteMatcher.h"
#include "PlacementInfo.h"
#include "dumpZip.h"
#include "sysInfo.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <omp.h>
#include <semaphore.h>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief MacroLegalizer maps DSP/BRAM/CARRY macros to legal location.
 *
 * (RAMB36E2 will be treated as 1+1 RAMB18E2, one of which is a virtual cell)
 *
 * The legalization procedure will only conduct rough legalization in the early iterations of global placement, and it
 * will conduct exact legalization following rough legalization when the macros are close enough to their potential
 * legal positions.
 */
class MacroLegalizer
{
  public:
    /**
     * @brief Construct a new MacroLegalizer object
     *
     * @param legalizerName the name string of legalizer for log dumping
     * @param placementInfo the PlacementInfo for this placer to handle
     * @param deviceInfo device information
     * @param macroTypesToLegalize a vector of Cell Type string indicating the target types handled by this
     * MacroLegalizer
     * @param JSONCfg  the user-defined placement configuration
     */
    MacroLegalizer(std::string legalizerName, PlacementInfo *placementInfo, DeviceInfo *deviceInfo,
                   std::vector<DesignInfo::DesignCellType> &macroTypesToLegalize,
                   std::map<std::string, std::string> &JSONCfg);
    ~MacroLegalizer()
    {
        if (minCostBipartiteMatcher)
            delete minCostBipartiteMatcher;
    }

    /**
     * @brief conduct legalization and map the PlacementUnit of one of the given types to sites
     *
     * @param exactLegalization true to ensure elements in a macro are consecutive
     * @param directLegalization
     */
    void legalize(bool is_cascade_flag, bool is_region_constr_flag, bool exactLegalization = false, bool directLegalization = false);

    inline float getAverageDisplacementOfExactLegalization()
    {
        if (finalAverageDisplacement > 1000)
            return finalAverageDisplacement;
        float tmpAverageDisplacement = 0.0;
        for (auto PUSitePair : PULevelMatching)
        {
            tmpAverageDisplacement += std::fabs(PUSitePair.first->X() - PUSitePair.second->X()) +
                                      std::fabs(PUSitePair.first->Y() - PUSitePair.second->Y());
        }
        tmpAverageDisplacement /= PULevelMatching.size();

        return tmpAverageDisplacement;
    }

    inline float getAverageDisplacementOfRoughLegalization()
    {
        return roughAverageDisplacement;
    }

    void dumpMatching(bool fixedColumn = false, bool enforce = false);

    void setIntitialParameters(float displacementThr, int candidateNum, int _candidateFactor = -1)
    {
        initialDisplacementThreshold = displacementThr;
        initialMaxNumCandidate = candidateNum;
        if (_candidateFactor > 1)
        {
            candidateFactor = _candidateFactor;
        }
    }

    void resetSitesMapped();

  private:
    std::string legalizerName;
    PlacementInfo *placementInfo;
    DeviceInfo *deviceInfo;
    PlacementInfo::CompatiblePlacementTable *compatiblePlacementTable;
    std::vector<DesignInfo::DesignCellType> macroTypesToLegalize;
    std::vector<PlacementInfo::Location> &cellLoc;
    bool useLookaheadTech;

    std::map<std::string, std::string> &JSONCfg;

    MinCostBipartiteMatcher *minCostBipartiteMatcher = nullptr;
    std::vector<DesignInfo::DesignCell *> macrosToLegalize;
    std::vector<DesignInfo::DesignCell *> initialMacrosToLegalize;
    std::set<PlacementInfo::PlacementUnit *> macroUnitsToLegalizeSet;
    std::map<DesignInfo::DesignCellType, std::vector<DeviceInfo::DeviceSite *>> macroType2Sites;
    std::map<DesignInfo::DesignCell *, std::vector<DeviceInfo::DeviceSite *>> macro2Sites;
    std::map<DesignInfo::DesignCell *, std::vector<DeviceInfo::DeviceSite *> *> macro2SitesInDisplacementThreshold;
    std::map<DeviceInfo::DeviceSite *, int> rightSiteIds;
    std::vector<DeviceInfo::DeviceSite *> siteList;
    std::vector<std::vector<std::pair<int, float>>> adjList;
    std::map<DesignInfo::DesignCell *, std::map<DeviceInfo::DeviceSite *, float>> cell2Site2HPWLIncrease;
    std::map<PlacementInfo::PlacementUnit *, std::map<DeviceInfo::DeviceSite *, float>> PU2Site2HPWLIncrease;

    std::set<DesignInfo::DesignCell *> matchedMacroCells;
    std::set<DeviceInfo::DeviceSite *> matchedSites;
    std::vector<std::pair<DesignInfo::DesignCell *, DeviceInfo::DeviceSite *>> cellLevelMatching;
    std::vector<std::pair<PlacementInfo::PlacementUnit *, DeviceInfo::DeviceSite *>> PULevelMatching;

    int DumpMacroLegalizationCnt = 0;

    float displacementThreshold = 30;
    int maxNumCandidate = 30;

    int BRAMColumnNum = -1;
    int DSPColumnNum = -1;
    int CARRYColumnNum = -1;

    int BRAMRowNum = -1;
    int DSPRowNum = -1;
    int CARRYRowNum = -1;

    std::vector<float> BRAMColumnXs;
    std::vector<float> DSPColumnXs;
    std::vector<float> CARRYColumnXs;

    std::vector<std::vector<DeviceInfo::DeviceSite *>> BRAMColumn2Sites;
    std::vector<std::vector<DeviceInfo::DeviceSite *>> DSPColumn2Sites;
    std::vector<std::vector<DeviceInfo::DeviceSite *>> CARRYColumn2Sites;

    std::vector<std::deque<PlacementInfo::PlacementUnit *>> BRAMColumn2PUs;
    std::vector<std::deque<PlacementInfo::PlacementUnit *>> DSPColumn2PUs;
    std::vector<std::deque<PlacementInfo::PlacementUnit *>> CARRYColumn2PUs;

    std::vector<int> BRAMColumnUntilization;
    std::vector<int> DSPColumnUntilization;
    std::vector<int> CARRYColumnUntilization;

    std::map<DesignInfo::DesignCell *, int> BRAMCell2Column;
    std::map<DesignInfo::DesignCell *, int> DSPCell2Column;
    std::map<DesignInfo::DesignCell *, int> CARRYCell2Column;

    std::map<PlacementInfo::PlacementUnit *, std::vector<DeviceInfo::DeviceSite *>> PU2LegalSites;
    std::map<PlacementInfo::PlacementUnit *, float> PU2X;
    std::map<PlacementInfo::PlacementUnit *, float> PU2Y;
    std::map<PlacementInfo::PlacementUnit *, int> PU2SiteX;
    std::map<PlacementInfo::PlacementUnit *, std::vector<int>> PU2Columns;
    std::set<PlacementInfo::PlacementUnit *> BRAMPUs;
    std::set<PlacementInfo::PlacementUnit *> DSPPUs;
    std::set<PlacementInfo::PlacementUnit *> CARRYPUs;

    bool enableBRAMLegalization = false;
    bool enableDSPLegalization = false;
    bool enableCARRYLegalization = false;
    bool verbose = true;
    float y2xRatio = 1.0;

    float finalAverageDisplacement = 10000.0;
    float fixedColumnAverageDisplacement = 10000.0;
    float roughAverageDisplacement = 10000.0;

    float initialDisplacementThreshold = 30;
    //float initialDisplacementThreshold = 1000;
    int initialMaxNumCandidate = 30;
    //int initialMaxNumCandidate = 1000;
    int nJobs = 1;
    int candidateFactor = 5;

    void getMacrosToLegalize(bool is_cascade_flag, bool is_region_constr_flag);
    void findMacroType2AvailableSites();
    void resolveOverflowColumns();
    void findPossibleLegalLocation(bool fixedColumn = false);
    void mapMacrosToColumns(bool directLegalization);
    int findCorrespondingColumn(float curX, std::vector<float> &Xs);
    void createBipartiteGraph(bool is_fixed_column_legalize);
    void roughlyLegalize(bool is_region_constr_flag);
    void fixedColumnLegalize(bool directLegalization);
    void updatePUMatchingLocation(bool isRoughLegalization = true, bool updateDisplacement = true);
    void finalLegalizeBasedOnDP();
    float DPForMinHPWL(int colNum, std::vector<std::vector<DeviceInfo::DeviceSite *>> &Column2Sites,
                       std::vector<std::deque<PlacementInfo::PlacementUnit *>> &Column2PUs);
    void updateMatchingAndUnmatchedMacroCells(int iter = 0);
    void spreadMacros(int columnNum, std::vector<int> &columnUntilization,
                      std::vector<std::vector<DeviceInfo::DeviceSite *>> &column2Sites,
                      std::vector<std::deque<PlacementInfo::PlacementUnit *>> &column2PUs,
                      std::map<DesignInfo::DesignCell *, int> &cell2Column, float budgeRatio = 1);
    int findIdMaxWithRecurence(int minId, int maxId, std::vector<int> &ids);
    void setSitesMapped();

    void findMacroCell2SitesInDistance()
    {
        macro2SitesInDisplacementThreshold.clear();

        int macrosNum = macrosToLegalize.size();

        for (int i = 0; i < macrosNum; i++)
        {
            macro2SitesInDisplacementThreshold[macrosToLegalize[i]] = nullptr;
        }

//#pragma omp parallel for
        for (int i = 0; i < macrosNum; i++)
        {
            DesignInfo::DesignCell *curCell = macrosToLegalize[i];
            //std::cout<<curCell->getName()<<std::endl;
            macro2SitesInDisplacementThreshold[curCell] = placementInfo->findNeiborSiteFromBinGrid(
                curCell, cellLoc[curCell->getCellId()].X, cellLoc[curCell->getCellId()].Y, displacementThreshold,
                candidateFactor * maxNumCandidate);
            //std::cout<<"macro2SitesInDisplacementThreshold"<<macro2SitesInDisplacementThreshold[curCell]->size()<<std::endl;

            /*
            std::cout<<curCell->getName()<<' '<<curCell->getRegionConstrType()<<std::endl;
            for(auto regionconstr : placementInfo->getDesignInfo()->GetRegionConstraintAll())
            {
                std::cout<<"RegionConstrain "<<regionconstr->GetRegionConstrId()<<std::endl;
                std::cout<<"BEL Occupy: "<<std::endl;
                for(auto &beloccupy : regionconstr->GetBELOccupy())
                {
                    std::cout<<"("<<beloccupy.first<<','<<beloccupy.second<<")"<<std::endl;
                }
                std::cout<<"BEL Supply: "<<std::endl;
                for(auto &belsupply : regionconstr->GetBELSupply())
                {
                    std::cout<<"("<<belsupply.first<<','<<belsupply.second<<")"<<std::endl;
                }
            }
            */
        }
    }

    void resetMacroCell2SitesInDistance()
    {
        int macrosNum = macrosToLegalize.size();
        for (int i = 0; i < macrosNum; i++)
        {
            DesignInfo::DesignCell *curCell = macrosToLegalize[i];
            delete macro2SitesInDisplacementThreshold[curCell];
        }
    }

    inline void resetSettings(bool is_cascade_flag)
    {
        displacementThreshold = initialDisplacementThreshold;
        maxNumCandidate = initialMaxNumCandidate;
        matchedMacroCells.clear();
        matchedSites.clear();
        cellLevelMatching.clear();
        PULevelMatching.clear();

        //For the constrained regions containing the cascade macros
        if(is_cascade_flag)
        {
            for(auto regionconstr : placementInfo->getDesignInfo()->GetRegionConstraintAll())
            {
                regionconstr->resetNodeCol();
            }
            
            /*
            for(auto regionconstr : placementInfo->getDesignInfo()->GetRegionConstraintAll())
            {
                std::cout<<"RegionConstrain "<<regionconstr->GetRegionConstrId()<<std::endl;
                std::cout<<"BEL Occupy: "<<std::endl;
                for(auto &beloccupy : regionconstr->GetBELOccupy())
                {
                    std::cout<<"("<<beloccupy.first<<','<<beloccupy.second<<")"<<std::endl;
                }
                std::cout<<"BEL Supply: "<<std::endl;
                for(auto &belsupply : regionconstr->GetBELSupply())
                {
                    std::cout<<"("<<belsupply.first<<','<<belsupply.second<<")"<<std::endl;
                }
            }
            */
            
        }
    }

    int getMarcroCellNum(PlacementInfo::PlacementUnit *tmpMacroUnit);

    inline void swapPU(PlacementInfo::PlacementUnit **A, PlacementInfo::PlacementUnit **B)
    {
        PlacementInfo::PlacementUnit *C = *A;
        *A = *B;
        *B = C;
    }

    void sortPUsByPU2Y(std::deque<PlacementInfo::PlacementUnit *> &PUs);

    void sortSitesBySiteY(std::vector<DeviceInfo::DeviceSite *> &sites);

    inline float getDisplacement(PlacementInfo::Location &macroLoc, DeviceInfo::DeviceSite *curSite)
    {
        return std::fabs(macroLoc.X - curSite->X()) + y2xRatio * std::fabs(macroLoc.Y - curSite->Y());
    }

    inline float getDisplacement(PlacementInfo::PlacementUnit *curPU, DeviceInfo::DeviceSite *curSite)
    {
        return std::fabs(curPU->X() - curSite->X()) + y2xRatio * std::fabs(curPU->Y() - curSite->Y());
    }

    inline float getHPWLChange(DesignInfo::DesignCell *curCell, DeviceInfo::DeviceSite *curSite, bool useCache = true)
    {
        if (useCache)
        {
            assert(cell2Site2HPWLIncrease.find(curCell) != cell2Site2HPWLIncrease.end());
            if (cell2Site2HPWLIncrease[curCell].find(curSite) != cell2Site2HPWLIncrease[curCell].end())
            {
                return cell2Site2HPWLIncrease[curCell][curSite];
            }
        }
        float oriHPWL = 0.0;
        float newHPWL = 0.0;
        auto tmpPU = placementInfo->getPlacementUnitByCell(curCell);
        float PUX = 0.0, PUY = 0.0;
        auto nets = placementInfo->getPlacementUnitId2Nets()[tmpPU->getId()];
        float numCellsInMacro = 1.0;
        if (dynamic_cast<PlacementInfo::PlacementUnpackedCell *>(tmpPU))
        {
            PUX = curSite->X();
            PUY = curSite->Y();
        }
        else if (PlacementInfo::PlacementMacro *tmpMacro = dynamic_cast<PlacementInfo::PlacementMacro *>(tmpPU))
        {

            PUX = curSite->X() - tmpMacro->getCellOffsetXInMacro(curCell);
            PUY = curSite->Y() - tmpMacro->getCellOffsetYInMacro(curCell);
            numCellsInMacro = tmpMacro->getCells().size();
        }

        for (auto curNet : nets)
        {
            if (curNet->getDesignNet()->getPins().size() > 1000) // it could be clock
                continue;
            oriHPWL += curNet->getHPWL(y2xRatio);
            newHPWL += curNet->getNewHPWLByTrying(tmpPU, PUX, PUY, y2xRatio);
        }
        if (useCache)
        {
            cell2Site2HPWLIncrease[curCell][curSite] = (newHPWL - oriHPWL) / numCellsInMacro;
        }
        return (newHPWL - oriHPWL) / numCellsInMacro;
        // return std::fabs(macroLoc.X - curSite->X()) + std::fabs(macroLoc.Y - curSite->Y());
        // placementInfo->getPlacementUnitByCell(curCell);
    }

    inline float getHPWLChange(PlacementInfo::PlacementUnit *tmpPU, DeviceInfo::DeviceSite *curSite)
    {
        float oriHPWL = 0.0;
        float newHPWL = 0.0;
        float PUX = 0.0, PUY = 0.0;
        auto nets = placementInfo->getPlacementUnitId2Nets()[tmpPU->getId()];

        PUX = curSite->X();
        PUY = curSite->Y();

        for (auto curNet : nets)
        {
            if (curNet->getDesignNet()->getPins().size() > 1000) // it could be clock
                continue;
            oriHPWL += curNet->getHPWL(y2xRatio);
            newHPWL += curNet->getNewHPWLByTrying(tmpPU, PUX, PUY, y2xRatio);
        }

        return (newHPWL - oriHPWL);
    }

    void resetHPWLChangeCache();

    inline float getHPWLChange(PlacementInfo::PlacementUnit *tmpPU, float PUX, float PUY)
    {

        float oriHPWL = 0.0;
        float newHPWL = 0.0;
        auto nets = placementInfo->getPlacementUnitId2Nets()[tmpPU->getId()];

        for (auto curNet : nets)
        {
            if (curNet->getDesignNet()->getPins().size() > 1000) // it could be clock
                continue;
            oriHPWL += curNet->getHPWL(y2xRatio);
            newHPWL += curNet->getNewHPWLByTrying(tmpPU, PUX, PUY, y2xRatio);
        }

        return (newHPWL - oriHPWL);
    }

    inline void swapSitePtr(DeviceInfo::DeviceSite **siteA, DeviceInfo::DeviceSite **siteB)
    {
        DeviceInfo::DeviceSite *tmp = *siteA;
        *siteA = *siteB;
        *siteB = tmp;
    }

    inline int sortPartition(DesignInfo::DesignCell *curCell, std::vector<DeviceInfo::DeviceSite *> &sites, int low,
                             int high, PlacementInfo::Location &macroLoc)
    {
        int pivot, index, i;
        index = low;
        pivot = high;
        for (i = low; i < high; i++)
        {
            // finding index of pivot.
            // if (a[i] < a[pivot])
            // if (getDisplacement(macroLoc, sites[i]) < getDisplacement(macroLoc, sites[pivot]))
            if (getHPWLChange(curCell, sites[i]) < getHPWLChange(curCell, sites[pivot]))
            {
                swapSitePtr(&sites[i], &sites[index]);
                index++;
            }
        }
        swapSitePtr(&sites[pivot], &sites[index]);
        return index;
    }
    inline int RandomPivotPartition(DesignInfo::DesignCell *curCell, std::vector<DeviceInfo::DeviceSite *> &sites,
                                    int low, int high, PlacementInfo::Location &macroLoc)
    {
        // Random selection of pivot.
        int pvt;
        pvt = (high + low) / 2;
        // pvt = low + n % (high - low + 1); // Randomizing the pivot value from sub-array.
        swapSitePtr(&sites[high], &sites[pvt]);
        return sortPartition(curCell, sites, low, high, macroLoc);
    }
    void quick_sort_WLChange(DesignInfo::DesignCell *curCell, std::vector<DeviceInfo::DeviceSite *> &sites, int p,
                             int q, PlacementInfo::Location &macroLoc)
    {
        // recursively sort the list
        int pindex;
        if (p < q)
        {
            pindex = RandomPivotPartition(curCell, sites, p, q, macroLoc); // randomly choose pivot
            // Recursively implementing QuickSort.
            quick_sort_WLChange(curCell, sites, p, pindex - 1, macroLoc);
            quick_sort_WLChange(curCell, sites, pindex + 1, q, macroLoc);
        }
    }

    inline void swapPUs(PlacementInfo::PlacementUnit **PUA, PlacementInfo::PlacementUnit **PUB)
    {
        PlacementInfo::PlacementUnit *tmp = *PUA;
        *PUA = *PUB;
        *PUB = tmp;
    }

    inline int sortPartition(std::vector<PlacementInfo::PlacementUnit *> &PUs, int low, int high)
    {
        int pivot, index, i;
        index = low;
        pivot = high;
        for (i = low; i < high; i++)
        {
            // finding index of pivot.
            // if (a[i] < a[pivot])
            // if (getDisplacement(macroLoc, PUs[i]) < getDisplacement(macroLoc, PUs[pivot]))
            if (PUs[i]->X() < PUs[pivot]->X())
            {
                swapPUs(&PUs[i], &PUs[index]);
                index++;
            }
        }
        swapPUs(&PUs[pivot], &PUs[index]);
        return index;
    }
    inline int RandomPivotPartition(std::vector<PlacementInfo::PlacementUnit *> &PUs, int low, int high)
    {
        // Random selection of pivot.
        int pvt, n;
        n = random();
        pvt = low + n % (high - low + 1); // Randomizing the pivot value from sub-array.
        swapPUs(&PUs[high], &PUs[pvt]);
        return sortPartition(PUs, low, high);
    }
    void quick_sort_locX(std::vector<PlacementInfo::PlacementUnit *> &PUs, int p, int q)
    {
        // recursively sort the list
        int pindex;
        if (p < q)
        {
            pindex = RandomPivotPartition(PUs, p, q); // randomly choose pivot
            // Recursively implementing QuickSort.
            quick_sort_locX(PUs, p, pindex - 1);
            quick_sort_locX(PUs, pindex + 1, q);
        }
    }
};

#endif