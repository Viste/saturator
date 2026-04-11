#include "../source/dsp/InstrumentMode.hpp"
#include "../source/dsp/DrumMode.hpp"
#include "../source/dsp/VocalMode.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>

static constexpr int SR = 44100;
static constexpr int BLOCK = 512;

// генерация тестового сигнала: синус 440Hz
static void generateSine(float* buf, int n, float freq = 440.0f, float amp = 0.5f) {
    for (int i = 0; i < n; ++i) {
        buf[i] = amp * std::sin(2.0f * 3.14159265f * freq * i / SR);
    }
}

// генерация сигнала с транзиентами (имитация ударных)
static int transientPhase = 0;
static void generateTransient(float* buf, int n) {
    for (int i = 0; i < n; ++i) {
        int pos = transientPhase++;
        float bg = 0.05f * std::sin(2.0f * 3.14159265f * 200.0f * pos / SR);
        if (pos % 441 == 0) {
            buf[i] = 0.4f + bg;
        } else {
            float decay = std::exp(-static_cast<float>(pos % 441) / 44.0f);
            buf[i] = 0.4f * decay * std::sin(2.0f * 3.14159265f * 300.0f * pos / SR) + bg;
        }
    }
}

// RMS блока
static float rms(const float* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / n);
}

// максимальная разница двух буферов
static float maxDiff(const float* a, const float* b, int n) {
    float mx = 0.0f;
    for (int i = 0; i < n; ++i) {
        float d = std::abs(a[i] - b[i]);
        if (d > mx) mx = d;
    }
    return mx;
}

struct TestResult {
    const char* mode;
    const char* param;
    float diff;
    bool pass;
};

static std::vector<TestResult> results;

enum SignalType { SINE, TRANSIENT };

// рендерим N блоков
static void renderBlocks(dsp::SaturationMode* mode, float* in, float* out, int blocks, SignalType sig = SINE) {
    float* inPtrs[2] = { in, in };
    float* outPtrs[2] = { out, out };
    for (int b = 0; b < blocks; ++b) {
        if (sig == TRANSIENT) generateTransient(in, BLOCK);
        else generateSine(in, BLOCK);
        mode->process(inPtrs, outPtrs, 1, BLOCK);
    }
}

static void testParam(const char* modeName, const char* paramName,
                       dsp::SaturationMode* mode,
                       auto setupDefault, auto setupChanged,
                       SignalType sig = SINE) {
    float inBuf[BLOCK], outDefault[BLOCK], outChanged[BLOCK];

    transientPhase = 0;
    mode->reset();
    mode->prepare(SR, BLOCK);
    setupDefault();
    renderBlocks(mode, inBuf, outDefault, 5, sig);
    if (sig == TRANSIENT) generateTransient(inBuf, BLOCK);
    else generateSine(inBuf, BLOCK);
    float* inPtrs[2] = { inBuf, inBuf };
    float* outPtrs[2] = { outDefault, outDefault };
    mode->process(inPtrs, outPtrs, 1, BLOCK);

    transientPhase = 0;
    mode->reset();
    mode->prepare(SR, BLOCK);
    setupChanged();
    renderBlocks(mode, inBuf, outChanged, 5, sig);
    if (sig == TRANSIENT) generateTransient(inBuf, BLOCK);
    else generateSine(inBuf, BLOCK);
    float* outPtrs2[2] = { outChanged, outChanged };
    mode->process(inPtrs, outPtrs2, 1, BLOCK);

    float diff = maxDiff(outDefault, outChanged, BLOCK);
    bool pass = diff > 0.001f;
    results.push_back({modeName, paramName, diff, pass});
}

int main() {
    printf("=== Saturator Parameter Test ===\n\n");

    // ======= INSTRUMENT MODE =======
    {
        dsp::InstrumentMode mode;
        auto defaults = [&]() {
            auto& p = mode.params();
            p.saturation = 0.5f; p.inputGain = 1.0f; p.outputGain = 1.0f;
            p.dryWet = 1.0f; p.lowSat = 1.0f; p.midSat = 1.0f; p.highSat = 1.0f;
            p.lowMidFreq = 200.0f; p.midHighFreq = 2500.0f; p.character = 0.5f;
            p.osFactor = dsp::Oversampler::kNone;
        };

        testParam("Instrument", "saturation", &mode, defaults, [&]() {
            defaults(); mode.params().saturation = 0.9f;
        });
        testParam("Instrument", "dryWet (mix)", &mode, defaults, [&]() {
            defaults(); mode.params().dryWet = 0.2f;
        });
        testParam("Instrument", "inputGain", &mode, defaults, [&]() {
            defaults(); mode.params().inputGain = 2.0f;
        });
        testParam("Instrument", "outputGain", &mode, defaults, [&]() {
            defaults(); mode.params().outputGain = 0.5f;
        });
        testParam("Instrument", "lowSat", &mode, defaults, [&]() {
            defaults(); mode.params().lowSat = 0.1f;
        });
        testParam("Instrument", "midSat", &mode, defaults, [&]() {
            defaults(); mode.params().midSat = 0.1f;
        });
        testParam("Instrument", "highSat", &mode, defaults, [&]() {
            defaults(); mode.params().highSat = 0.1f;
        });
        testParam("Instrument", "lowMidFreq", &mode, defaults, [&]() {
            defaults(); mode.params().lowMidFreq = 800.0f;
        });
        testParam("Instrument", "midHighFreq", &mode, defaults, [&]() {
            defaults(); mode.params().midHighFreq = 6000.0f;
        });
        testParam("Instrument", "character", &mode, defaults, [&]() {
            defaults(); mode.params().character = 0.0f;
        });
    }

    // ======= DRUM MODE =======
    {
        dsp::DrumMode mode;
        auto defaults = [&]() {
            auto& p = mode.params();
            p.saturation = 0.5f; p.inputGain = 1.0f; p.outputGain = 1.0f;
            p.dryWet = 1.0f; p.transientSensitivity = 0.5f;
            p.attackMs = 3.0f; p.sustainSat = 1.0f; p.punch = 0.5f;
            p.osFactor = dsp::Oversampler::kNone;
        };

        testParam("Drums", "saturation", &mode, defaults, [&]() {
            defaults(); mode.params().saturation = 0.9f;
        }, TRANSIENT);
        testParam("Drums", "dryWet (mix)", &mode, defaults, [&]() {
            defaults(); mode.params().dryWet = 0.2f;
        }, TRANSIENT);
        testParam("Drums", "transientSens", &mode, defaults, [&]() {
            defaults(); mode.params().transientSensitivity = 1.0f;
        }, TRANSIENT);
        testParam("Drums", "attackMs", &mode, defaults, [&]() {
            defaults(); mode.params().attackMs = 10.0f;
        }, TRANSIENT);
        testParam("Drums", "sustainSat", &mode, defaults, [&]() {
            defaults(); mode.params().sustainSat = 0.1f;
        }, TRANSIENT);
        testParam("Drums", "punch", &mode, defaults, [&]() {
            defaults(); mode.params().punch = 0.0f;
        }, TRANSIENT);
    }

    // ======= VOCAL MODE =======
    {
        dsp::VocalMode mode;
        auto defaults = [&]() {
            auto& p = mode.params();
            p.saturation = 0.5f; p.inputGain = 1.0f; p.outputGain = 1.0f;
            p.dryWet = 1.0f; p.tapeBias = 0.5f;
            p.wowAmount = 0.3f; p.flutterAmount = 0.2f;
            p.hissLevel = 0.5f; p.headCutoff = 12000.0f; p.tapeSpeed = 0.5f;
            p.osFactor = dsp::Oversampler::kNone;
        };

        testParam("Vocal", "saturation", &mode, defaults, [&]() {
            defaults(); mode.params().saturation = 0.9f;
        });
        testParam("Vocal", "dryWet (mix)", &mode, defaults, [&]() {
            defaults(); mode.params().dryWet = 0.2f;
        });
        testParam("Vocal", "tapeBias", &mode, defaults, [&]() {
            defaults(); mode.params().tapeBias = 1.0f;
        });
        testParam("Vocal", "wowAmount", &mode, defaults, [&]() {
            defaults(); mode.params().wowAmount = 1.0f;
        });
        testParam("Vocal", "flutterAmount", &mode, defaults, [&]() {
            defaults(); mode.params().flutterAmount = 1.0f;
        });
        testParam("Vocal", "hissLevel", &mode, defaults, [&]() {
            defaults(); mode.params().hissLevel = 1.0f;
        });
        testParam("Vocal", "headCutoff", &mode, defaults, [&]() {
            defaults(); mode.params().headCutoff = 3000.0f;
        });
        testParam("Vocal", "tapeSpeed", &mode, defaults, [&]() {
            defaults(); mode.params().tapeSpeed = 0.0f;
        });
    }

    int passed = 0, failed = 0;
    for (auto& r : results) {
        printf("[%s] %-14s %-16s maxDiff=%.6f\n",
               r.pass ? "PASS" : "FAIL", r.mode, r.param, r.diff);
        if (r.pass) ++passed; else ++failed;
    }

    printf("\n%d passed, %d failed out of %d tests\n", passed, failed, (int)results.size());
    return failed > 0 ? 1 : 0;
}
