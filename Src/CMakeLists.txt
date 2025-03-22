set(PROJECT_NAME DXRenderer)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "MainWindow.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "DXRenderer.cpp"
    "MainWindow.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(Shader_Files
   "Shaders.hlsl"
)
source_group("Shader Files" Files ${Shader_Files})
set_source_files_properties(Shaders.hlsl PROPERTIES VS_TOOL_OVERRIDE "Text")

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
    ${Shader_Files}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED on
    C_STANDARD 17
    C_STANDARD_REQUIRED on
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Output directory
################################################################################
set(BIN_DIR "${CMAKE_SOURCE_DIR}/Bin")
set(PROJECT_DIR "${CMAKE_SOURCE_DIR}/Project")

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${BIN_DIR}/$<CONFIG>/"
    OUTPUT_DIRECTORY_RELEASE "${BIN_DIR}/$<CONFIG>/"
    RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "../../Bin/$<CONFIG>"
    VS_DEBUGGER_COMMAND ""
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "_CONSOLE;"
    "UNICODE;"
    "_UNICODE"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy
        >
        /permissive-;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:WINDOWS
    )
endif()

################################################################################
# Dependencies
################################################################################


list(APPEND CMAKE_PREFIX_PATH "${PROJECT_DIR}/vcpkg_installed/x64-windows/share/directx-headers")
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_DIR}/vcpkg_installed/x64-windows/share/directx12-agility")

find_package(directx-headers CONFIG REQUIRED PATHS "${PROJECT_DIR}/vcpkg_installed/x64-windows/share/directx-headers" NO_DEFAULT_PATH)
find_package(directx12-agility CONFIG REQUIRED PATHS "${PROJECT_DIR}/vcpkg_installed/x64-windows/share/directx12-agility" NO_DEFAULT_PATH)

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
target_link_libraries(${PROJECT_NAME} PRIVATE d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib) # Probably not using vcpkg libs.

# Output file copying and directory creation.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Dirs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DIR}/Release/D3D12"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DIR}/Debug/D3D12"

    # D3D12 Libs
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_DIR}/vcpkg_installed/x64-windows/bin/D3D12Core.dll" "${BIN_DIR}/Release/D3D12"
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_DIR}/vcpkg_installed/x64-windows/debug/bin/D3D12Core.dll" "${BIN_DIR}/Debug/D3D12"
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_DIR}/vcpkg_installed/x64-windows/debug/bin/d3d12SDKLayers.dll" "${BIN_DIR}/Debug/D3D12"
    
    # Shader(s)
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${Shader_Files}" "${BIN_DIR}/Debug"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${Shader_Files}" "${BIN_DIR}/Release"
    
)


# Example from the vcpkg DX12 
#target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)
# if(TARGET Microsoft::DirectX12-Agility)
#     file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY_DEBUG}/D3D12")
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> ${PROJECT_NAME}/D3D12
#         COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> ${PROJECT_NAME}/D3D12
#        COMMAND_EXPAND_LISTS
#     )
# endif()
