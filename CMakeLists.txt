cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)
set(SMTG_CREATE_PLUGIN_LINK OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)

project(Saturator
    VERSION 2.3.3
    DESCRIPTION "Saturator VST 3 Plug-in"
)

# --- VST3 SDK ---
if(NOT DEFINED vst3sdk_SOURCE_DIR)
    set(vst3sdk_SOURCE_DIR "/Users/viste/dev/vst3sdk_vsrat")
endif()
add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

# --- Dear ImGui ---
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.6
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

if(APPLE)
    list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
    )
elseif(WIN32)
    list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_features(imgui PRIVATE cxx_std_17)

if(APPLE)
    target_link_libraries(imgui PUBLIC "-framework AppKit" "-framework Metal")
    set_source_files_properties(
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource/visuals")
set(FONT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource/fonts")
set(EMBEDDED_ASSETS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets.h")
file(GLOB ASSET_FILES "${ASSET_DIR}/*.png" "${FONT_DIR}/*.ttf")

set(EMBED_CONTENT "// Auto-generated. Do not edit.\n#pragma once\nnamespace embedded {\n")
foreach(ASSET_PATH ${ASSET_FILES})
    get_filename_component(ASSET_NAME ${ASSET_PATH} NAME)
    string(REGEX REPLACE "\\.(png|ttf)$" "" VAR_NAME ${ASSET_NAME})
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" VAR_NAME ${VAR_NAME})

    file(READ "${ASSET_PATH}" HEX_CONTENT HEX)
    string(LENGTH "${HEX_CONTENT}" HEX_LEN)
    math(EXPR BYTE_LEN "${HEX_LEN} / 2")
    string(REGEX REPLACE "(..)" "0x\\1," HEX_LIST "${HEX_CONTENT}")

    set(EMBED_CONTENT "${EMBED_CONTENT}// ${ASSET_NAME}\ninline constexpr unsigned char ${VAR_NAME}_data[] = { ${HEX_LIST} };\ninline constexpr unsigned long ${VAR_NAME}_size = ${BYTE_LEN};\n\n")
endforeach()
set(EMBED_CONTENT "${EMBED_CONTENT}} // namespace embedded\n")
file(WRITE "${EMBEDDED_ASSETS_HEADER}" "${EMBED_CONTENT}")
message(STATUS "Embedded ${ASSET_FILES} into ${EMBEDDED_ASSETS_HEADER}")

# --- Plugin Sources ---
smtg_add_vst3plugin(Saturator
    source/Version.h
    source/PluginIds.hpp
    source/BaseProcessor.hpp
    source/BaseProcessor.cpp
    source/BaseController.hpp
    source/BaseController.cpp
    source/PluginEntry.cpp
    source/MessagesConsts.hpp

    # DSP
    source/dsp/Oversampler.hpp
    source/dsp/Oversampler.cpp
    source/dsp/MultibandSplitter.hpp
    source/dsp/MultibandSplitter.cpp
    source/dsp/Waveshapers.hpp
    source/dsp/SaturationMode.hpp
    source/dsp/InstrumentMode.hpp
    source/dsp/InstrumentMode.cpp
    source/dsp/DrumMode.hpp
    source/dsp/DrumMode.cpp
    source/dsp/VocalMode.hpp
    source/dsp/VocalMode.cpp
    source/dsp/MeteringData.hpp

    # GUI
    source/gui/ImGuiPlugView.hpp
    source/gui/ImGuiPlugView.cpp
    source/gui/SaturatorUI.hpp
    source/gui/SaturatorUI.cpp
    source/gui/Widgets.hpp
    source/gui/Widgets.cpp
    source/gui/UIState.hpp
    source/gui/Locale.hpp
    source/gui/Locale.cpp
    source/gui/UpdateChecker.hpp
    source/gui/TextureManager.hpp
    source/gui/TextureManager.cpp
    source/gui/Fonts.hpp
    source/gui/Fonts.cpp
)
target_include_directories(Saturator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extra
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Platform-specific GUI sources
if(APPLE)
    target_sources(Saturator PRIVATE
        source/gui/ImGuiPlugView_mac.mm
        source/gui/UpdateChecker_mac.mm
        source/gui/TextureManager_mac.mm
    )
    set_source_files_properties(source/gui/ImGuiPlugView_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
    set_source_files_properties(source/gui/UpdateChecker_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
    set_source_files_properties(source/gui/TextureManager_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
elseif(WIN32)
    target_sources(Saturator PRIVATE
        source/gui/ImGuiPlugView_win.cpp
        source/gui/UpdateChecker_win.cpp
        source/gui/TextureManager_win.cpp
    )
endif()

# --- Dependencies ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extra/infra")
add_subdirectory(extra/infra)
target_include_directories(Saturator SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extra/infra/include)
target_link_libraries(Saturator PRIVATE infra)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extra/q_lib")
add_subdirectory(extra/q_lib)
target_include_directories(Saturator SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extra/q_lib/include)
target_link_libraries(Saturator PRIVATE libq)

target_link_libraries(Saturator PRIVATE sdk imgui)
target_compile_features(Saturator PRIVATE cxx_std_23)

# --- Snapshots ---
smtg_target_add_plugin_snapshots(Saturator
    RESOURCES
        resource/9AD205CEB65E5EFC8CC882A1C02BF720_snapshot.png
        resource/9AD205CEB65E5EFC8CC882A1C02BF720_snapshot_2.0x.png
)

smtg_target_configure_version_file(Saturator)

# --- Platform-specific ---
if(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(Saturator PRIVATE
        ${COREFOUNDATION_LIBRARY}
        ${IOKIT_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )

    smtg_target_set_bundle(Saturator
        BUNDLE_IDENTIFIER com.vlab.saturatorvst3
        COMPANY_NAME "Viste"
    )
    set_target_properties(Saturator PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resource/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "Saturator"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.vlab.saturatorvst3"
        MACOSX_BUNDLE_BUNDLE_VERSION "2.3.3"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "2.3.3"
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo" "BNDL????")
    add_custom_command(TARGET Saturator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo"
                "$<TARGET_BUNDLE_DIR:Saturator>/Contents/PkgInfo"
        VERBATIM
    )
    add_custom_command(TARGET Saturator POST_BUILD
        COMMAND codesign --force --sign - --timestamp=none
                "$<TARGET_BUNDLE_DIR:Saturator>"
        VERBATIM
    )
    smtg_target_set_debug_executable(Saturator
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(Saturator PRIVATE resource/win32resource.rc)
    target_link_libraries(Saturator PRIVATE opengl32 winhttp)

    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Saturator)
        smtg_target_set_debug_executable(Saturator
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif()

option(SATURATOR_BUILD_UI_PREVIEW "Build offscreen UI preview tool (macOS)" OFF)
if(APPLE AND SATURATOR_BUILD_UI_PREVIEW)
    add_executable(ui_preview
        tools/ui_preview.mm
        source/BaseController.cpp
        source/gui/ImGuiPlugView.cpp
        source/gui/ImGuiPlugView_mac.mm
        source/gui/SaturatorUI.cpp
        source/gui/Widgets.cpp
        source/gui/Locale.cpp
        source/gui/Fonts.cpp
        source/gui/TextureManager.cpp
        source/gui/TextureManager_mac.mm
        source/gui/UpdateChecker_mac.mm
    )
    set_source_files_properties(
        tools/ui_preview.mm
        source/gui/TextureManager_mac.mm
        source/gui/UpdateChecker_mac.mm
        PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
    set_source_files_properties(source/gui/ImGuiPlugView_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
    target_include_directories(ui_preview PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}/extra
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(ui_preview PRIVATE
        sdk imgui
        ${COREFOUNDATION_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        "-framework ImageIO"
        "-framework CoreGraphics"
        "-framework CoreServices"
    )
    target_compile_features(ui_preview PRIVATE cxx_std_23)
endif()
