CMake Warning (dev) at /usr/local/cmake/cmake-3.17.3-Linux-x86_64/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (GOOGLEHASH)
  does not match the name of the calling package (GoogleHash).  This can lead
  to problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  lib/3rdParty/eigen3-src/cmake/FindGoogleHash.cmake:21 (find_package_handle_standard_args)
  lib/3rdParty/eigen3-src/unsupported/test/CMakeLists.txt:15 (find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Could NOT find GOOGLEHASH (missing: GOOGLEHASH_INCLUDES GOOGLEHASH_COMPILE) 
CMake Warning (dev) at /usr/local/cmake/cmake-3.17.3-Linux-x86_64/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (ADOLC) does
  not match the name of the calling package (Adolc).  This can lead to
  problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  lib/3rdParty/eigen3-src/cmake/FindAdolc.cmake:17 (find_package_handle_standard_args)
  lib/3rdParty/eigen3-src/unsupported/test/CMakeLists.txt:25 (find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Could NOT find ADOLC (missing: ADOLC_INCLUDES ADOLC_LIBRARIES) 
-- Could NOT find MPFR (missing: MPFR_INCLUDES MPFR_LIBRARIES MPFR_VERSION_OK) (Required is at least version "2.3.0")
-- Could NOT find GMP (missing: GMP_INCLUDES GMP_LIBRARIES) 
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29.1") 
-- Could NOT find FFTW (missing: FFTW_INCLUDES FFTW_LIBRARIES) 
-- Found CUDA: /usr/local/cuda-11.4 (found suitable version "11.4", minimum required is "7.0") 
CMake Error: File /data/ssd/qluo/AMF-Placer-ICCAD-2021/src/lib/3rdParty/eigen3-src/scripts/buildtests.in does not exist.
CMake Error at lib/3rdParty/eigen3-src/scripts/CMakeLists.txt:2 (configure_file):
  configure_file Problem configuring file


CMake Warning (dev) at /usr/local/cmake/cmake-3.17.3-Linux-x86_64/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (CHOLMOD)
  does not match the name of the calling package (Cholmod).  This can lead to
  problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  lib/3rdParty/eigen3-src/cmake/FindCholmod.cmake:86 (find_package_handle_standard_args)
  lib/3rdParty/eigen3-src/bench/spbench/CMakeLists.txt:16 (find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Could NOT find CHOLMOD (missing: CHOLMOD_INCLUDES CHOLMOD_LIBRARIES) 
CMake Warning (dev) at /usr/local/cmake/cmake-3.17.3-Linux-x86_64/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (UMFPACK)
  does not match the name of the calling package (Umfpack).  This can lead to
  problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  lib/3rdParty/eigen3-src/cmake/FindUmfpack.cmake:50 (find_package_handle_standard_args)
  lib/3rdParty/eigen3-src/bench/spbench/CMakeLists.txt:24 (find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Could NOT find UMFPACK (missing: UMFPACK_INCLUDES UMFPACK_LIBRARIES) 
-- Could NOT find KLU (missing: KLU_INCLUDES KLU_LIBRARIES) 
CMake Warning (dev) at /usr/local/cmake/cmake-3.17.3-Linux-x86_64/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (SUPERLU)
  does not match the name of the calling package (SuperLU).  This can lead to
  problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  lib/3rdParty/eigen3-src/cmake/FindSuperLU.cmake:93 (find_package_handle_standard_args)
  lib/3rdParty/eigen3-src/bench/spbench/CMakeLists.txt:39 (find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Could NOT find SUPERLU (missing: SUPERLU_INCLUDES SUPERLU_LIBRARIES SUPERLU_VERSION_OK) 
-- A version of Pastix has been found but pastix_nompi.h does not exist in the include directory. Because Eigen tests require a version without MPI, we disable the Pastix backend.
-- ************************************************************
-- ***    Eigen's unit tests configuration summary          ***
-- ************************************************************
-- 
-- Build type:        Release
-- Build site:        proj53
-- Build string:      linux-4.15.0-158-generic-g++-10.2.0-sse2-64bit
-- Enabled backends:  Boost.Multiprecision, 
-- Disabled backends: Cholmod,  UmfPack,  KLU,  SuperLU,  PaStiX,  METIS,  SPQR,  GoogleHash,  Adolc,  MPFR C++,  fftw,  OpenGL, 
-- Default order:     Column-major
-- Maximal matrix/vector size: 320
-- SSE2:              Using architecture defaults
-- SSE3:              Using architecture defaults
-- SSSE3:             Using architecture defaults
-- SSE4.1:            Using architecture defaults
-- SSE4.2:            Using architecture defaults
-- AVX:               Using architecture defaults
-- AVX2:              Using architecture defaults
-- FMA:               Using architecture defaults
-- AVX512:            Using architecture defaults
-- AVX512DQ:          Using architecture defaults
-- Altivec:           Using architecture defaults
-- VSX:               Using architecture defaults
-- MIPS MSA:          Using architecture defaults
-- ARM NEON:          Using architecture defaults
-- ARMv8 NEON:        Using architecture defaults
-- S390X ZVECTOR:     Using architecture defaults
-- C++11:             OFF
-- SYCL:              OFF
-- CUDA:              OFF
-- HIP:               OFF
-- 
CXX:               /data/ssd/hcli/usr/local/bin/g++
 CXX_VERSION:       g++ (GCC) 10.2.0
 CXX_FLAGS:          -O3 -fopenmp
 Sparse lib flags:   

-- ************************************************************
-- 
-- Configured Eigen 3.3.90
-- 
-- Available targets (use: make TARGET):
-- ---------+--------------------------------------------------------------
-- Target   |   Description
-- ---------+--------------------------------------------------------------
-- install  | Install Eigen. Headers will be installed to:
--          |     <CMAKE_INSTALL_PREFIX>/<INCLUDE_INSTALL_DIR>
--          |   Using the following values:
--          |     CMAKE_INSTALL_PREFIX: /data/ssd/qluo/AMF-Placer-ICCAD-2021/src
--          |     INCLUDE_INSTALL_DIR:  include/eigen3
--          |   Change the install location of Eigen headers using:
--          |     cmake . -DCMAKE_INSTALL_PREFIX=yourprefix
--          |   Or:
--          |     cmake . -DINCLUDE_INSTALL_DIR=yourdir
-- doc      | Generate the API documentation, requires Doxygen & LaTeX
-- check    | Build and run the unit-tests. Read this page:
--          |   http://eigen.tuxfamily.org/index.php?title=Tests
-- blas     | Build BLAS library (not the same thing as Eigen)
-- uninstall| Remove files installed by the install target
-- ---------+--------------------------------------------------------------
-- 
-- Found ZLIB: /usr/lib/x86_64-linux-gnu/libz.so (found version "1.2.11") 
-- Configuring incomplete, errors occurred!
See also "/data/ssd/qluo/AMF-Placer-ICCAD-2021/build/CMakeFiles/CMakeOutput.log".
make: *** No targets specified and no makefile found.  Stop.