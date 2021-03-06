# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Project's name
project(QuasiMolto)

# Set the output folders where your program will be created
set(CMAKE_BINARY_DIR ${QuasiMolto_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Set links for various folders
set(DRIVER_DIR ${CMAKE_SOURCE_DIR}/driver)
set(TPLS_DIR ${CMAKE_SOURCE_DIR}/TPLs)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)

# The following folder will be included
include_directories("${PROJECT_SOURCE_DIR}")
include_directories(${TPLS_DIR}/yaml-cpp/include)
include_directories(${TPLS_DIR}/armadillo/include)
include_directories(${TPLS_DIR}/superlu/include)
include_directories(${TPLS_DIR}/petsc/include)
include_directories(${TPLS_DIR}/petsc/arch-linux-c-debug/include)

# Link directories
link_directories(${TPLS_DIR}/petsc/arch-linux-c-debug/lib)

# Add subdirectories
add_subdirectory(${DRIVER_DIR})
add_subdirectory(${LIBS_DIR})
add_subdirectory(${TPLS_DIR}/yaml-cpp)
add_subdirectory(${TPLS_DIR}/armadillo)
add_subdirectory(${TPLS_DIR}/superlu)

#find_package(Armadillo REQUIRED)
#include_directories(${ARMADILLO_INCLUDE_DIRS})

add_executable(QuasiMolto 
	       ${PROJECT_SOURCE_DIR}/driver/driver.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/Materials.cpp
	       ${PROJECT_SOURCE_DIR}/libs/MultiGroupQD.cpp
	       ${PROJECT_SOURCE_DIR}/libs/SingleGroupQD.cpp
	       ${PROJECT_SOURCE_DIR}/libs/StartingAngle.cpp
	       ${PROJECT_SOURCE_DIR}/libs/Mesh.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/MultiGroupTransport.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/SingleGroupTransport.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/SimpleCornerBalance.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/QuasidiffusionSolver.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/TransportToQDCoupling.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/MMS.cpp 
	       ${PROJECT_SOURCE_DIR}/libs/Material.cpp
               ${PROJECT_SOURCE_DIR}/libs/SingleGroupDNP.cpp
               ${PROJECT_SOURCE_DIR}/libs/MultiGroupDNP.cpp
               ${PROJECT_SOURCE_DIR}/libs/MultiGroupQDToMultiPhysicsQDCoupling.cpp
               ${PROJECT_SOURCE_DIR}/libs/GreyGroupQD.cpp
               ${PROJECT_SOURCE_DIR}/libs/MultiPhysicsCoupledQD.cpp
               ${PROJECT_SOURCE_DIR}/libs/HeatTransfer.cpp
               ${PROJECT_SOURCE_DIR}/libs/GreyGroupSolver.cpp
               ${PROJECT_SOURCE_DIR}/libs/CollapsedCrossSections.cpp
               ${PROJECT_SOURCE_DIR}/libs/WriteData.cpp
               ${PROJECT_SOURCE_DIR}/libs/MultilevelCoupling.cpp
               ${PROJECT_SOURCE_DIR}/libs/PETScWrapper.cpp
               )

target_link_libraries(
    QuasiMolto
    PUBLIC
    superlu
    ${ARMADILLO_LIBRARIES} 
    petsc
    PRIVATE
    yaml-cpp  # This is the "target" exposed by 
              # yaml-cpp's CMakeLists.txt file
)

# Add compilier flags
SET(CMAKE_CXX_FLAGS "-std=c++11 -Ofast -fopenmp")
SET(CMAKE_CXX_FLAGS_DEBUG "-pg")

# Enable testing
ENABLE_TESTING()
add_subdirectory(${QuasiMolto_SOURCE_DIR}/tests)

