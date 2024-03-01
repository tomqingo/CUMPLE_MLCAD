/**
 * @file main.cc
 * @author Tingyuan Liang (tliang@connect.ust.hk)
 * @brief AMF-Placer Main file
 * @version 0.1
 * @date 2021-05-31
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "AMFPlacer.h"

int main(int argc, const char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <path to input folder> <path output ouput folder>" << std::endl;
        return 1;
    }

	if (argc == 4) {
    	auto placer = new AMFPlacer(argv[1], argv[2], argv[3]);
    	placer->run();
    	delete placer;
	} 
	else {
		// the third argument for json
		assert (argc == 5);
    	auto placer = new AMFPlacer(argv[2]);
    	placer->run();
    	delete placer;
	}

    return 0;
}