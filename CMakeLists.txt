cmake_minimum_required(VERSION 3.15.0)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)
set(SMTG_CREATE_PLUGIN_LINK OFF)


project(Saturator
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.hpp and ${PROJECT_BINARY_DIR}/projectversion.hpp.
    VERSION 1.0.0
    DESCRIPTION "Saturator VST 3 Plug-in"
)

set(vst3sdk_SOURCE_DIR "/Users/viste/dev/vst3sdk_vsrat")
set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")
add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(Saturator
        source/Version.h
        source/PluginIds.hpp
        source/BaseProcessor.hpp
        source/BaseProcessor.cpp
        source/BaseController.hpp
        source/BaseController.cpp
        source/PluginEntry.cpp
        source/MessagesConsts.hpp
        source/utils/BufferArray.cpp
        source/utils/BufferArray.hpp
        source/utils/AudioBuffers.cpp
        source/utils/AudioBuffers.hpp
        source/utils/TransientDetector.hpp
        source/views/CustomCKnobCreator.cpp
        source/views/CustomCKnobCreator.hpp
        source/views/CustomCKnob.cpp
        source/views/CustomCKnob.hpp
        source/views/CustomVST3Editor.cpp
        source/views/CustomVST3Editor.hpp
        source/subcontrollers/ModeSwitchController.cpp
        source/subcontrollers/ModeSwitchController.hpp
        source/subcontrollers/MainKnobController.cpp
        source/subcontrollers/MainKnobController.hpp
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extra/infra")
add_subdirectory(extra/infra)
target_include_directories(Saturator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extra/infra/include)
target_link_libraries(Saturator PRIVATE infra)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extra/q_lib")
add_subdirectory(extra/q_lib)
target_include_directories(Saturator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extra/q_lib/include)
target_link_libraries(Saturator PRIVATE libq)

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(Saturator
        PRIVATE
            resource/layout.uidesc
    )
    target_link_libraries(Saturator
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(Saturator
        RESOURCES
            "resource/layout.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (Saturator
    RESOURCES
        resource/9AD205CEB65E5EFC8CC882A1C02BF720_snapshot.png
        resource/9AD205CEB65E5EFC8CC882A1C02BF720_snapshot_2.0x.png
)

target_link_libraries(Saturator
    PRIVATE
        sdk
)

smtg_target_configure_version_file(Saturator)


target_compile_features(Saturator PRIVATE cxx_std_17)

if(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(Saturator PRIVATE ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()

if(SMTG_MAC)
    smtg_target_set_bundle(Saturator
        BUNDLE_IDENTIFIER com.pprfnk.vsraturatorvst3
        COMPANY_NAME "Paperfunk"
    )
    smtg_target_set_debug_executable(Saturator
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(Saturator PRIVATE
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Saturator)

        smtg_target_set_debug_executable(Saturator
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
