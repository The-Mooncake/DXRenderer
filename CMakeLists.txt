cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(DXRenderer CXX)

################################################################################
# Get External packages
################################################################################

include(FetchContent)

FetchContent_Declare(
  TBB
  GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
  GIT_TAG        "v2020.3.3"
  #GIT_TAG        "2020_U3"
  #GIT_TAG       "v2022.0.0" 
)


FetchContent_Declare(
  usd
  GIT_REPOSITORY https://github.com/PixarAnimationStudios/OpenUSD.git
  GIT_TAG        "v25.02" 
)


################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################

# If using prebuilt usd from source.
set(PXR_INSTALL_LOCATION "${BIN_DIR}/Debug/")

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################

# PXR Flags
set(OPENVDB_BUILD_PYTHON_MODULE OFF)  
set(PXR_ENABLE_PYTHON_SUPPORT OFF)  
set(PXR_BUILD_IMAGING OFF) 
set(PXR_BUILD_USD_IMAGING OFF)  

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Project/_deps/tbb-src/cmake")
# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/Project/_deps/tbb-src/cmake/templates")
# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/Project/_deps/tbb-src/cmake/")
# set(TBB_DIR "${CMAKE_SOURCE_DIR}/Project/_deps/tbb-src/cmake/templates")

#FetchContent_MakeAvailable(TBB)
#set(TBB_ROOT_DIR "${CMAKE_SOURCE_DIR}/Project/_deps/tbb-src")

# include("C:/Users/olive/Documents/DXRenderer/Project/_deps/tbb-src/cmake/TBBBuild.cmake")
# tbb_build(TBB_ROOT "C:/Users/olive/Documents/DXRenderer/Project/_deps/tbb-src" CONFIG_DIR TBB_DIR)

include(CMakePrintHelpers)
# cmake_print_variables(PROJECT_SOURCE_DIR)
# cmake_print_variables(TBB_DIR)
# cmake_print_variables(CMAKE_MODULE_PATH)
# cmake_print_variables(CMAKE_PREFIX_PATH)

# tbb_build(TBB_ROOT "C:/Users/olive/Documents/DXRenderer/Project/_deps/tbb-src" CONFIG_DIR TBB_DIR)
#find_package(TBB REQUIRED)
message(STATUS "Populating USD")
FetchContent_Populate(usd)
#FetchContent_MakeAvailable(usd)

include(FindPythonInterp)
message(STATUS "Building USD")
set(py_cmd 
  "C:\\Users\\olive\\Documents\\DXRenderer\\Project\\_deps\\usd-src\\build_scripts\\build_usd.py" 
  "--build-variant=debug" 
  "--no-python"
  "--no-docs"
  "--no-embree"
  "--no-python"
  "--no-debug-python"
  "--no-draco"
  "--no-prman"
  "--no-materialx"
  "--no-alembic"
  "--no-hdf5"
  "--no-opencolorio"
  "--no-openimageio"
  "--no-usdview"
  "--no-openvdb"
  "--no-ptex"
  "--no-imaging"
  "C:\\Users\\olive\\Documents\\DXRenderer\\Project\\_deps\\usd-build")
execute_process(COMMAND  ${PYTHON_EXECUTABLE} ${py_cmd} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} RESULT_VARIABLE py_result )
message(STATUS "Python result: ${py_result}")

#file(RENAME "C:/Users/olive/Documents/DXRenderer/Project/_deps/usd-src/pxr/pxrConfig.cmake.in" "C:/Users/olive/Documents/DXRenderer/Project/_deps/usd-src/pxr/pxrConfig.cmake" )

#add_subdirectory("Project/_deps/usd-src" SYSTEM)

add_subdirectory(Src)

