# Saturator

VST3 плагин сатурации. Три режима обработки + мягкий клиппер(скоро).

## Режимы

- **Инструмент** — многополосная ламповая сатурация (НЧ/СЧ/ВЧ + тембр)
- **Ударные** — раздельная обработка атаки и тела (чувствительность, удар, сустейн)
- **Вокал/Лента** — эмуляция ленточного магнитофона (гистерезис, wow/flutter, фильтр головки)

## Фичи

- Drive + Mix (dry/wet)
- Мягкий клиппер (вкл/выкл + amount)
- Оверсемплинг —/2x/4x
- Осциллограмма вход/выход
- Спектроанализатор
- RU/EN интерфейс

## Сборка

```bash
git clone --recursive <repo>
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)   # Linux
make -j$(sysctl -n hw.ncpu)   # macOS
```

Результат: `build/VST3/Release/Saturator.vst3`

## Структура

```
source/
├── BaseProcessor        — аудио-процессинг
├── BaseController       — параметры и состояние
├── PluginIds.hpp        — ID параметров
├── dsp/                 — DSP модули (waveshapers, 3 режима, oversampler)
└── gui/                 — интерфейс (Dear ImGui + OpenGL)
```

