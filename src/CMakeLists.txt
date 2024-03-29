cmake_minimum_required(VERSION 3.10)

project(Cumple)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenMP)
add_compile_options ( -std=c++17 -O3  -fopenmp -Wall )
if(OPENMP_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 ${OpenMP_CXX_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 ${OpenMP_C_FLAGS}")
else(OPENMP_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 ")
  message (STATUS "OpenMP not found")
endif()


set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})


# find required packages
include(FetchContent)

# ABSL
if (NOT TARGET absl::strings OR NOT TARGET absl::status OR NOT TARGET absl::span)
  message(STATUS "Cumple fetching packages: `absl` targets not found. Attempting to fetch contents...")
  FetchContent_Declare(
    abseil-cpp 
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/3rdParty/abseil-cpp-src
    # GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    # GIT_TAG        origin/master
  )
  FetchContent_MakeAvailable(abseil-cpp)
else()
  message(STATUS "Cumple fetching packages: `absl` targets found.")
endif()

# EIGEN
# Eigen is usually available as a system package, so we use find_package.
if (NOT TARGET Eigen3::Eigen)
  message(STATUS "Cumple fetching packages: `Eigen3` targets not found. Attempting to find package...")
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        origin/master
    #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/3rdParty/eigen3-src
  )
  FetchContent_MakeAvailable(eigen)
  set(EIGEN3_INCLUDE_DIR ${eigen_SOURCE_DIR})
  # find_package(Eigen3 3.3.7 REQUIRED NO_MODULE)
else()
  message(STATUS "Cumple fetching packages: `Eigen3` targets found.")
endif()


# find_package(ZLIB)

add_subdirectory(lib)

set(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/app/AMFPlacer/main.cc)
add_executable(Cumple ${SOURCE_FILES})
add_executable(partitionHyperGraph lib/3rdParty/partitionHyperGraph.cc)

if (ENABLE_STATIC_COMPILE)
    message(STATUS "enable static compilation...")
    set_target_properties(Cumple PROPERTIES
                                LINK_FLAGS "-static -Wl,--whole-archive -rdynamic -lpthread -Wl,--no-whole-archive")
else (ENABLE_STATIC_COMPILE)
    set_target_properties(Cumple PROPERTIES
                                LINK_FLAGS "-Wl,--whole-archive -rdynamic -lpthread -Wl,--no-whole-archive")
endif (ENABLE_STATIC_COMPILE)


include_directories(./lib/
./lib/HiFPlacer/designInfo/ 
./lib/HiFPlacer/deviceInfo/ 
./lib/utils/
./lib/HiFPlacer/placement/placementInfo/
./lib/HiFPlacer/placement/globalPlacement/
./lib/HiFPlacer/placement/legalization/
./lib/HiFPlacer/placement/packing/
./lib/HiFPlacer/placement/placementTiming/
./lib/3rdParty
./lib/HiFPlacer/problemSolvers
${abseil-cpp_SOURCE_DIR} 
${EIGEN3_INCLUDE_DIR}
${ZLIB_INCLUDE_DIRS}
)

link_directories(${CMAKE_SOURCE_DIR}/lib/3rdParty/PaToH/)

target_link_libraries(Cumple GlobalPlacer DesignInfo DeviceInfo PlacementInfo PlacementTiming Packing Legalization ProblemSolvers Utils
                        ${CMAKE_SOURCE_DIR}/lib/3rdParty/PaToH/libpatoh.a 
                        pthread ${ZLIB_LIBRARIES})
target_link_libraries(partitionHyperGraph m ${CMAKE_SOURCE_DIR}/lib/3rdParty/PaToH/libpatoh.a )
