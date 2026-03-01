#include "BaseProcessor.hpp"
#include "BaseController.hpp"
#include "PluginIds.hpp"
#include "Version.h"
#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "Saturator"

BEGIN_FACTORY_DEF ("dev-vlab",
                   "https://dev-vlab.ru/",
                   "mailto:viste02@gmail.com")

        DEF_CLASS2 (INLINE_UID_FROM_FUID(kSaturatorProcessorUID),
                    PClassInfo::kManyInstances,
                    kVstAudioEffectClass,
                    stringPluginName,
                    Vst::kDistributable,
                    SaturatorVST3Category,
                    FULL_VERSION_STR,
                    kVstVersionString,
                    BaseProcessor::createInstance)

        DEF_CLASS2 (INLINE_UID_FROM_FUID(kSaturatorControllerUID),
                    PClassInfo::kManyInstances,
                    kVstComponentControllerClass,
                    stringPluginName "Controller",
                    0,
                    "",
                    FULL_VERSION_STR,
                    kVstVersionString,
                    BaseController::createInstance)

END_FACTORY
