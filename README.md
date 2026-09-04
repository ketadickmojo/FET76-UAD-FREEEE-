# FET76 — цифровой FET-компрессор в стиле 1176

Стартовый проект VST3-плагина на JUCE (C++). Реализована feedback-топология
компрессии, 4 фиксированных ratio + "All Buttons In", быстрые attack/release,
лёгкая FET-сатурация, зависящая от глубины сжатия.

## Что нужно установить

1. **CMake** >= 3.22 — https://cmake.org/download/
2. **Компилятор**:
   - Windows: Visual Studio 2022 (Desktop C++ workload)
   - macOS: Xcode + командные инструменты (`xcode-select --install`)
   - Linux: gcc/clang + пакеты для JUCE (alsa, freetype, X11 dev-заголовки)
3. Git (для автоматической загрузки JUCE через FetchContent)

## Сборка

```bash
cd fet76-compressor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

При первой сборке CMake сам склонирует JUCE (включая нужные части VST3 SDK —
отдельно скачивать SDK от Steinberg не нужно).

Готовый `.vst3` появится в:
- Windows: `build/FET76_artefacts/Release/VST3/FET76.vst3`
- macOS: `build/FET76_artefacts/Release/VST3/FET76.vst3`
- Linux: `build/FET76_artefacts/Release/VST3/FET76.vst3`

Благодаря `COPY_PLUGIN_AFTER_BUILD TRUE` JUCE попробует сам скопировать
плагин в системную VST3-папку.

## Структура

```
Source/
  CompressorDSP.h      — ядро DSP (без зависимостей от JUCE, легко тестировать)
  PluginProcessor.h/.cpp — параметры, обработка аудио
  PluginEditor.h/.cpp     — простой GUI (роторные слайдеры + VU гейн-редакции)
```

## Что стоит доработать дальше

- **Sidechain HPF** — в реальном 1176 нет, но многие цифровые ремейки
  добавляют фильтр детектора, чтобы бас не "душил" компрессию
  (легко добавить `juce::dsp::IIR::Filter` перед детектором в `CompressorDSP`).
- **Стерео-линковка детектора** — сейчас каналы независимы (compL/compR
  считают гейн-редукцию раздельно); для линкованного стерео нужно брать
  max(|L|, |R|) и подавать в оба детектора одно значение.
- **Более точная кривая "All Buttons In"** — сейчас это просто ratio=100.
  Настоящий эффект — смешение всех 4 ratio-цепей одновременно,
  что даёт характерную нелинейную "рванину". Можно смоделировать,
  реально просуммировав 4 параллельных вычисления gainReduction.
- **Oversampling** (`juce::dsp::Oversampling`) — сатурация tanh создаёт
  алиасинг на высоких частотах, апсемплинг x2/x4 перед сатурацией уберёт артефакты.
- **Program-dependent release** — в реальном 1176 время release плавает
  в зависимости от материала; можно эмулировать модуляцией releaseCoeff
  от среднего уровня сигнала.

## Тестирование звучания

Собери и загрузи в DAW (Reaper/Ableton/FL Studio — все видят VST3 из папки
`C:\Program Files\Common Files\VST3` на Windows или `~/Library/Audio/Plug-Ins/VST3`
на macOS). Погоняй на вокале и ударных — там характер 1176 проявляется ярче всего.
