add_library(driver driver.cpp)

# Add compilier flags
SET(CMAKE_CXX_FLAGS "-std=c++11 -Ofast -fopenmp")
SET(CMAKE_CXX_FLAGS_DEBUG "-pg")
