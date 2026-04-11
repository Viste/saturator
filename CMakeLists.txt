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
    VERSION 2.2.0
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
        ${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm
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
        ${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

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
)

# Platform-specific GUI sources
if(APPLE)
    target_sources(Saturator PRIVATE
        source/gui/ImGuiPlugView_mac.mm
        source/gui/UpdateChecker_mac.mm
    )
    set_source_files_properties(source/gui/ImGuiPlugView_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
    set_source_files_properties(source/gui/UpdateChecker_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
elseif(WIN32)
    target_sources(Saturator PRIVATE
        source/gui/ImGuiPlugView_win.cpp
        source/gui/UpdateChecker_win.cpp
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
    find_library(GAMECONTROLLER_LIBRARY GameController)
    target_link_libraries(Saturator PRIVATE
        ${COREFOUNDATION_LIBRARY}
        ${IOKIT_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${GAMECONTROLLER_LIBRARY}
    )

    smtg_target_set_bundle(Saturator
        BUNDLE_IDENTIFIER com.pprfnk.vsraturatorvst3
        COMPANY_NAME "Viste"
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
