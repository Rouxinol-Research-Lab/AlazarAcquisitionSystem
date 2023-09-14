cmake_minimum_required(VERSION 3.0)


project(acquisitionProject)

link_directories(".")
include_directories(".")

add_compile_definitions(MEASURER_EXPORTS)
add_library(measurer SHARED measurer.cpp measurer.hpp)
target_link_libraries(measurer visa64)

add_compile_definitions(ALAZAR_MEASURER_EXPORTS)
add_library(AlazarMeasurer SHARED AlazarMeasurer.cpp AlazarMeasurer.hpp)
target_link_libraries(AlazarMeasurer ATSApi)
target_link_libraries(AlazarMeasurer visa64)


add_executable(acquisition main.cpp)
target_link_libraries(acquisition measurer )
target_link_libraries(acquisition AlazarMeasurer)
