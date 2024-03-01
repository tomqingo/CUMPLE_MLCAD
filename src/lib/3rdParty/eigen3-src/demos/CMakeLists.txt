project(EigenDemos)

add_custom_target(demos)

if(NOT EIGEN_TEST_NOQT)
  find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
  #if(QT5_FOUND)
  add_subdirectory(mandelbrot)
  add_subdirectory(opengl)
  #else()
  #  message(STATUS "Qt5 not found, so disabling the mandelbrot and opengl demos")
  #endif()
endif()
