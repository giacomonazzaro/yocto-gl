add_executable(splinetest
    splinetest.cpp
    ext/flipout/flipout.h ext/flipout/flipout.cpp
)

target_include_directories(yocto PUBLIC ${CMAKE_SOURCE_DIR}/exts/geometry-central/include)
target_include_directories(yocto PUBLIC /usr/local/include/eigen3)

set_target_properties(splinetest  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(splinetest  PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_include_directories(splinetest    PUBLIC ${CMAKE_SOURCE_DIR}/exts)
target_link_libraries(splinetest  yocto geometry-central)