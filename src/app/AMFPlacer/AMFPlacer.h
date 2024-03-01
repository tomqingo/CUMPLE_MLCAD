/**
 * @file AMFPlacer.h
 * @author Tingyuan LIANG (tliang@connect.ust.hk)
 * @brief
 * @version 0.1
 * @date 2021-06-03
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "DesignInfo.h"
#include "DeviceInfo.h"
#include "GlobalPlacer.h"
#include "PlacementTimingOptimizer.h"
#include "ParallelCLBPacker.h"
#include "IncrementalBELPacker.h"
#include "InitialPacker.h"
#include "PlacementInfo.h"
#include "utils/simpleJSON.h"
#include <iostream>
#include <omp.h>

/**
 * @brief AMFPlacer is an analytical mixed-size FPGA placer
 *
 * To enable the performance optimization of application mapping on modern field-programmable gate arrays (FPGAs),
 * certain critical path portions of the designs might be prearranged into many multi-cell macros during synthesis.
 * These movable macros with constraints of shape and resources lead to challenging mixed-size placement for FPGA
 * designs which cannot be addressed by previous works of analytical placers. In this work, we propose AMF-Placer,
 * an open-source analytical mixed-size FPGA placer supporting mixed-size placement on FPGA, with an interface to
 * Xilinx Vivado. To speed up the convergence and improve the quality of the placement, AMF-Placer is equipped with
 * a series of new techniques for wirelength optimization, cell spreading, packing, and legalization. Based on a set
 * of the latest large open-source benchmarks from various domains for Xilinx Ultrascale FPGAs, experimental results
 * indicate that AMF-Placer can improve HPWL by 20.4%-89.3% and reduce runtime by 8.0%-84.2%, compared to the
 * baseline. Furthermore, utilizing the parallelism of the proposed algorithms, with 8 threads, the placement procedure
 * can be accelerated by 2.41x on average.
 *
 */
class AMFPlacer
{
  public:
    /**
     * @brief Construct a new AMFPlacer object according to a given placer configuration file
     *
     * @param JSONFileName
     */
    AMFPlacer(std::string JSONFileName)
    {
        JSON = parseJSONFile(JSONFileName);

        // assert(JSON.find("vivado extracted device information file") != JSON.end());
        // assert(JSON.find("special pin offset info file") != JSON.end());
        // assert(JSON.find("cellType2fixedAmo file") != JSON.end());
        // assert(JSON.find("cellType2sharedCellType file") != JSON.end());
        // assert(JSON.find("sharedCellType2BELtype file") != JSON.end());
        assert(JSON.find("GlobalPlacementIteration") != JSON.end());
        assert(JSON.find("dumpparallelCLBPackerFLAG") != JSON.end());

		init();
    };

	void init() {
        oriTime = std::chrono::steady_clock::now();

        omp_set_num_threads(std::stoi(JSON["jobs"]));
        if (JSON.find("jobs") != JSON.end())
        {
            omp_set_num_threads(std::stoi(JSON["jobs"]));
        }
        else
        {
            omp_set_num_threads(1);
        }

        // load device information
        std::cout << "Iter num: " << JSON["GlobalPlacementIteration"] << std::endl;
		std::cout << "Load input designs: START." << std::endl;
        deviceinfo = new DeviceInfo(JSON, "VCU108");
        deviceinfo->printStat();

        // load design information
        designInfo = new DesignInfo(JSON, deviceinfo);
        designInfo->printStat();
		std::cout << "Load input designs: DONE." << " " << getElapsedTime() << std::endl;
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
    };

    AMFPlacer(std::string inputPath, std::string outputPath, std::string GP2Region)
    {
    	JSON["designPath"] = inputPath;
		JSON["logPath"] = outputPath;
        JSON["GP2Region"] = GP2Region;

        JSON["randomseed"]= "100";
		JSON["writePort"] = "false";
    	JSON["GlobalPlacerPrintHPWL"] = "true";
    	JSON["Simulated Annealing restartNum"] = "600";
    	JSON["Simulated Annealing IterNum"] = "300000";
    	// JSON["RandomInitialPlacement" ] = "false";
        JSON["RandomInitialPlacement"] = "true";
        JSON["RegionInitialPlacement"] = "true";
        JSON["AddLUTFFFeedback"] = "true";
        
    	JSON["DrawNetAfterEachIteration"] = "false";
        // adjust the weights of the pseudo nets
    	JSON["PseudoNetWeight"] = "0.0025";
    	JSON["GlobalPlacementIteration"] = "10";
    	JSON["SpreaderInnerIter"] = "1";
    	JSON["clockRegionXNum"] = "5";
    	JSON["clockRegionYNum"] = "8";
    	JSON["clockRegionDSPNum"] = "30";
    	JSON["clockRegionBRAMNum"] = "96";
    	JSON["jobs"] = "8";
    	//JSON["y2xRatio"] = "1.0";
        JSON["y2xRatio"] = "0.4";
    	JSON["ClusterPlacerVerbose"] = "false";
    	JSON["GlobalPlacerVerbose"] = "true";
    	JSON["SpreaderSimpleExpland"] = "false";
    	JSON["drawClusters"] = "false";
    	JSON["MKL"] = "true";
    	JSON["dumpDirectory"] = "./placerDumpData/";
    	JSON["dumpparallelCLBPackerFLAG"] = "false";

		init();
	}

    ~AMFPlacer()
    {
        delete placementInfo;
        delete designInfo;
        delete deviceinfo;
        if (incrementalBELPacker)
            delete incrementalBELPacker;
        if (globalPlacer)
            delete globalPlacer;
        if (initialPacker)
            delete initialPacker;
    }

    /**
     * @brief launch the analytical mixed-size FPGA placement procedure
     *
     */
    void run()
    {
		std::cout << "Macro placement: START." << std::endl;
        // initialize placement information, including how to map cells to BELs
        placementInfo = new PlacementInfo(designInfo, deviceinfo, JSON);
        // checkSiteMap();

        // we have to pack cells in design info into placement units in placement info with packer
        InitialPacker *initialPacker = new InitialPacker(designInfo, deviceinfo, placementInfo, JSON);
        initialPacker->pack();
        placementInfo->resetLUTFFDeterminedOccupation();

        placementInfo->printStat();
        placementInfo->createGridBins(5.0, 5.0);
        // placementInfo->createSiteBinGrid();
        placementInfo->verifyDeviceForDesign();

        placementInfo->buildSimpleTimingGraph();
        PlacementTimingOptimizer *timingOptimizer = new PlacementTimingOptimizer(placementInfo, JSON);

        // go through several glable placement iterations to get initial placement
        globalPlacer = new GlobalPlacer(placementInfo, JSON);
        std::cout<<"Cell Inflation Configure: "<<JSON["Cellinflation"]<<std::endl;

        if (true)
        {
            if (true)
            {
                // designInfo->enhanceFFControlSetNets();
				// placementInfo->outputCongestionMap();
                globalPlacer->clusterPlacement();
                globalPlacer->GlobalPlacement_fixedCLB(1, 0.0002);
                globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) / 3, false, 5,
                                                          true, true);
                print_info("1st GP finished");
				std::cout << "..." << " " << getElapsedTime() << std::endl;
				// placementInfo->outputCongestionMap(1);
                //placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"solution_v1.pl");

                // designInfo->resetNetEnhanceRatio();

                // placementInfo->updateLongPaths();
                placementInfo->createGridBins(2.0, 2.0);
                placementInfo->adjustLUTFFUtilization(-10, true);
                globalPlacer->spreading(-1);

                incrementalBELPacker = new IncrementalBELPacker(designInfo, deviceinfo, placementInfo, JSON);
                incrementalBELPacker->LUTFFPairing(16.0);
                incrementalBELPacker->FFPairing(4.0);
                placementInfo->printStat();
                print_info("Current Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()));

                // timingOptimizer->enhanceNetWeight_LevelBased(10);
                globalPlacer->setPseudoNetWeight(globalPlacer->getPseudoNetWeight() * 0.85);
                globalPlacer->setNeighborDisplacementUpperbound(3.0);

                globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) * 2 / 9, true, 5,
                                                          true, true);

                print_info("Current Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()));
                print_info("2nd GP finished");
				std::cout << "..." << " " << getElapsedTime() << std::endl;
				// placementInfo->outputCongestionMap(2);
                //placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"solution_v2.pl");

                // // pack simple LUT-FF pairs and go through several global placement iterations
                // incrementalBELPacker = new IncrementalBELPacker(designInfo, deviceinfo, placementInfo, JSON);
                // incrementalBELPacker->LUTFFPairing(16.0);
                // incrementalBELPacker->FFPairing(4.0);
                // placementInfo->printStat();
                // print_info("Current Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()));

                // // timingOptimizer->enhanceNetWeight_LevelBased(10);
                // globalPlacer->setPseudoNetWeight(globalPlacer->getPseudoNetWeight() * 0.85);
                // globalPlacer->setNeighborDisplacementUpperbound(3.0);
                // globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) * 2 / 9, true, 5,
                //                                           true, true);
                // print_info("3rd GP finished");
				// // placementInfo->outputCongestionMap(3);
                // //placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"solution_v3.pl");

                // globalPlacer->setNeighborDisplacementUpperbound(2.0);
                // globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) * 2 / 9, true, 5,
                //                                           true, true);
                // print_info("4th GP finished");
				// // placementInfo->outputCongestionMap(4);
                // //placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"solution_v4.pl");

                placementInfo->getDesignInfo()->resetNetEnhanceRatio();

                //globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) / 2, true, 5,
                //                                          true, true);

                globalPlacer->GlobalPlacement_CLBElements(std::stoi(JSON["GlobalPlacementIteration"]) / 2, true, 5,
                                                          true, false);
                print_info("5th GP finished");
				// placementInfo->outputCongestionMap(5);
                //placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"solution_v5.pl");

                for (auto PU : placementInfo->getPlacementUnits())
                {
                    if (PU->isPacked())
                        PU->resetPacked();
                }
                for (auto pair : placementInfo->getPULegalXY().first)
                {
                    if (pair.first->isFixed() && !pair.first->isLocked())
                    {
                        pair.first->setUnfixed();
                    }
                }
            }

            //Remove the final parallelCLBPack

            if(JSON["dumpparallelCLBPackerFLAG"] == "true")
            {
                placementInfo->dumpPlacementUnitInformation(JSON["logPath"] + "/" + "PUInfoBeforeFinalPacking");
                placementInfo->loadPlacementUnitInformation(JSON["logPath"] + "/" + "PUInfoBeforeFinalPacking.gz");
                print_info("Current Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()));
                parallelCLBPacker = new ParallelCLBPacker(designInfo, deviceinfo, placementInfo, JSON, 3, 10, 0.5, 0.5, 6,
                                                        10, 0.1, "first");
                parallelCLBPacker->packCLBs(30, true);
                parallelCLBPacker->setPULocationToPackedSite();
                // timingOptimizer->setEdgesDelay();
                placementInfo->checkClockUtilization(true);
                print_info("Current Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()));
                placementInfo->resetLUTFFDeterminedOccupation();
                parallelCLBPacker->updatePackedMacro(true, true);
                placementInfo->adjustLUTFFUtilization(1, true);
                // placementInfo->dumpCongestion("congestionInfo");

                if (parallelCLBPacker)
                    delete parallelCLBPacker;
                placementInfo->printStat();
            }

            for (auto PU : placementInfo->getPlacementUnits())
            {
                if (PU->isPacked())
                    PU->resetPacked();
            }
            for (auto pair : placementInfo->getPULegalXY().first)
            {
                if (pair.first->isFixed() && !pair.first->isLocked())
                {
                    pair.first->setUnfixed();
                }
            }
        }

        //placementInfo->updateB2BAndGetTotalHPWL();

        if(JSON["dumpparallelCLBPackerFLAG"] == "true")
        {
            placementInfo->dumpPlacementUnitInformation(JSON["logPath"] + "/" + "PUInfoBeforeFinal");            
            placementInfo->loadPlacementUnitInformation(JSON["logPath"] + "/" + "PUInfoBeforeFinal.gz");
            placementInfo->printStat();

            placementInfo->dumpPlacementUnitInformation(JSON["logPath"] + "/" + "PUInfoFinal");
            placementInfo->checkClockUtilization(true);
        }
        
        placementInfo->PlacementLocationOutput(JSON["logPath"] + "/" + JSON["macro_pl"]+"macroplacement.pl", 
			JSON["logPath"] + "/" + JSON["macro_pl"]+"merge.vivado.tcl");
        // print_status("Placement Done");
		std::cout << "Macro placement: DONE." << " " << getElapsedTime() << std::endl;
        std::cout << "Total HPWL = " + std::to_string(placementInfo->updateB2BAndGetTotalHPWL()) << " " << getElapsedTime() << std::endl;

        // auto nowTime = std::chrono::steady_clock::now();
        // auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - oriTime).count();

        return;
    }

	std::string getElapsedTime() {
    	auto nowTime = std::chrono::steady_clock::now();
    	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - oriTime).count();
		return "Elapsed time: " + std::to_string(millis / 1000) + " s";
	}

    void checkSiteMap() {
        std::vector<std::vector<DeviceInfo::DeviceSite *>> BRAMColumn2Sites;
        std::map<DesignInfo::DesignCellType, std::vector<DeviceInfo::DeviceSite *>> macroType2Sites;

        std::vector<float> BRAMColumnXs;
        int BRAMColumnNum = -1;
        int BRAMRowNum = -1;
            
        PlacementInfo::CompatiblePlacementTable *compatiblePlacementTable = placementInfo->getCompatiblePlacementTable();
        DeviceInfo* deviceInfo = placementInfo->getDeviceInfo();

        macroType2Sites.clear();
        DesignInfo::DesignCellType curCellType = DesignInfo::CellType_RAMB36E2;
        macroType2Sites[curCellType] = std::vector<DeviceInfo::DeviceSite *>(0);
        for (int SharedBELID : placementInfo->getPotentialBELTypeIDs(curCellType))
        {
            std::string targetSiteType =
                compatiblePlacementTable
                    ->sharedCellType2SiteType[compatiblePlacementTable->sharedCellBELTypes[SharedBELID]];
            std::vector<DeviceInfo::DeviceSite *> &sitesInType = deviceInfo->getSitesInType(targetSiteType);
            for (auto curSite : sitesInType)
            {
                if (!curSite->isOccupied() && !curSite->isMapped())
                {
                    // if (matchedSites.find(curSite) == matchedSites.end())
                    // {
                    macroType2Sites[curCellType].push_back(curSite);
                    // }
                }
            }
        }

        for (auto curSite : macroType2Sites[curCellType])
        {
            if (curSite->getSiteY() + 1 > BRAMRowNum)
                BRAMRowNum = curSite->getSiteY() + 1;
            if (curSite->getSiteX() + 1 > BRAMColumnNum)
                BRAMColumnNum = curSite->getSiteX() + 1;
        }

        BRAMColumnXs.resize(BRAMColumnNum, -1.0);
        BRAMColumn2Sites.clear();
        BRAMColumn2Sites.resize(BRAMColumnNum, std::vector<DeviceInfo::DeviceSite *>(0));
        for (auto curSite : macroType2Sites[curCellType])
        {
            if (curSite->getSiteY() + 1 > BRAMRowNum)
                BRAMRowNum = curSite->getSiteY() + 1;
            if (curSite->getSiteX() + 1 > BRAMColumnNum)
                BRAMColumnNum = curSite->getSiteX() + 1;
            BRAMColumnXs[curSite->getSiteX()] = curSite->X();
            BRAMColumn2Sites[curSite->getSiteX()].push_back(curSite);
        }

        auto& column2Sites = BRAMColumn2Sites;
        std::cout << "BRAM Num columns: " << column2Sites.size() << " " << BRAMColumnNum << std::endl;
        for (int i = 0; i < BRAMColumnNum; i ++) {
            auto& sites = column2Sites[i];
            std::cout << "    " << sites.size() << " x: " << BRAMColumnXs[i] << " "; 
            for (auto& site : sites) {
                std::cout << "(" << site->getSiteX() << ", " << site->getSiteY() << ")";
            }
            std::cout << std::endl;
        }
    }

  private:
    /**
     * @brief information related to the device (BELs, Sites, Tiles, Clock Regions)
     *
     */
    DeviceInfo *deviceinfo = nullptr;

    /**
     * @brief information related to the design (cells, pins and nets)
     *
     */
    DesignInfo *designInfo = nullptr;

    /**
     * @brief inforamtion related to placement (locations, interconnections, status, constraints, legalization)
     *
     */
    PlacementInfo *placementInfo = nullptr;

    /**
     * @brief initially packing for macro extraction based on pre-defined rules
     *
     */
    InitialPacker *initialPacker = nullptr;

    /**
     * @brief incremental pairing of some FFs and LUTs into small macros
     *
     */
    IncrementalBELPacker *incrementalBELPacker = nullptr;

    /**
     * @brief global placer acconting for initial placement, quadratic placement, cell spreading and macro legalization.
     *
     */
    GlobalPlacer *globalPlacer = nullptr;

    /**
     * @brief final packing of instances into CLB sites
     *
     */
    ParallelCLBPacker *parallelCLBPacker = nullptr;

    /**
     * @brief the user-defined settings of placement
     *
     */
    std::map<std::string, std::string> JSON;
};
