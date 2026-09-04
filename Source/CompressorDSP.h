#pragma once
#include <cmath>
#include <algorithm>

/**
    FET76Compressor
    ----------------
    Эмуляция FET-компрессора в стиле 1176 (feedback-топология).

    Ключевые отличия от "обычного" VCA-компрессора:
      1. Нет отдельного threshold — компрессия начинается от внутреннего
         опорного уровня (REFERENCE_DB), а Input Gain "толкает" сигнал
         в компрессию.
      2. Детектор уровня работает по УЖЕ сжатому сигналу (feedback),
         а не по входному (feedforward) — это даёт более "живой",
         менее предсказуемый характер сжатия.
      3. Фиксированные ratio-ступени, включая режим "All Buttons In"
         (все ratio разом = экстремальное сжатие с искажением).
      4. Лёгкая FET-сатурация (tanh) масштабируется величиной
         гейн-редакции — чем сильнее давим, тем больше "песка".
*/
class FET76Compressor
{
public:
    enum class Ratio { R4_1, R8_1, R12_1, R20_1, AllButtonsIn };

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        fs = sampleRate;
        envelopeDb = REFERENCE_DB; // старт без сжатия
        updateTimeConstants();
    }

    void setInputGainDb  (float db) { inputGainDb  = db; }
    void setOutputGainDb (float db) { outputGainDb = db; }
    void setRatio (Ratio r)         { ratio = r; }

    // attackMs: 0.02–0.8 мс, releaseMs: 50–1100 мс (реальные диапазоны 1176)
    void setAttackMs  (float ms) { attackMs  = std::clamp (ms, 0.02f, 0.8f);   updateTimeConstants(); }
    void setReleaseMs (float ms) { releaseMs = std::clamp (ms, 50.0f, 1100.0f); updateTimeConstants(); }

    void setMix (float m) { mix = std::clamp (m, 0.0f, 1.0f); } // parallel compression

    // Обрабатывает один сэмпл (моно-звено; для стерео — по звену на канал,
    // либо суммируй детектор по каналам для linked stereo — см. PluginProcessor)
    float processSample (float x)
    {
        const float dry = x;

        // 1. Input trim
        float driven = x * dbToLin (inputGainDb);

        // 2. Текущий коэффициент усиления (из ПРЕДЫДУЩЕГО состояния —
        //    это и есть feedback-топология: применяем гейн-редакцию
        //    ДО пересчёта детектора по результату)
        float grLin = dbToLin (-currentGainReductionDb);
        float compressed = driven * grLin;

        // 3. FET-сатурация, пропорциональная глубине сжатия
        float satAmount = std::clamp (currentGainReductionDb / 20.0f, 0.0f, 1.0f) * 0.35f;
        if (satAmount > 0.0001f)
            compressed = softClipFET (compressed, satAmount);

        // 4. Детектор уровня питается от СЖАТОГО сигнала (feedback)
        float levelDb = linToDb (std::abs (compressed) + 1.0e-9f);

        // 5. Envelope follower (разные коэффициенты на attack/release)
        float coeff = (levelDb > envelopeDb) ? attackCoeff : releaseCoeff;
        envelopeDb += (levelDb - envelopeDb) * coeff;

        // 6. Считаем требуемую гейн-редакцию для СЛЕДУЮЩЕГО сэмпла
        currentGainReductionDb = computeGainReduction (envelopeDb);

        // 7. Output trim
        float wet = compressed * dbToLin (outputGainDb);

        return dry * (1.0f - mix) + wet * mix;
    }

    float getGainReductionDb() const { return currentGainReductionDb; }

private:
    static constexpr float REFERENCE_DB = -18.0f; // внутренний "0" компрессии
    static constexpr float KNEE_DB = 3.0f;         // мягкое колено вокруг порога

    double fs = 44100.0;
    float inputGainDb = 0.0f, outputGainDb = 0.0f;
    float attackMs = 0.3f, releaseMs = 250.0f;
    float mix = 1.0f;
    Ratio ratio = Ratio::R4_1;

    float envelopeDb = REFERENCE_DB;
    float currentGainReductionDb = 0.0f;
    float attackCoeff = 0.0f, releaseCoeff = 0.0f;

    void updateTimeConstants()
    {
        attackCoeff  = 1.0f - std::exp (-1.0f / (0.001f * attackMs  * (float) fs));
        releaseCoeff = 1.0f - std::exp (-1.0f / (0.001f * releaseMs * (float) fs));
    }

    float getRatioValue() const
    {
        switch (ratio)
        {
            case Ratio::R4_1:         return 4.0f;
            case Ratio::R8_1:         return 8.0f;
            case Ratio::R12_1:        return 12.0f;
            case Ratio::R20_1:        return 20.0f;
            case Ratio::AllButtonsIn: return 100.0f; // экстремальное сжатие
        }
        return 4.0f;
    }

    // Мягкое колено: ниже порога — 0, в колене — плавный переход,
    // выше — линейный наклон по ratio
    float computeGainReduction (float levelDb) const
    {
        float r = getRatioValue();
        float overshoot = levelDb - REFERENCE_DB;

        if (overshoot <= -KNEE_DB)
            return 0.0f;

        if (overshoot >= KNEE_DB)
            return overshoot * (1.0f - 1.0f / r);

        // квадратичное колено
        float t = (overshoot + KNEE_DB) / (2.0f * KNEE_DB);
        float kneeGain = t * t * (1.0f - 1.0f / r) * KNEE_DB;
        return std::max (0.0f, kneeGain);
    }

    static float softClipFET (float x, float amount)
    {
        // смешиваем чистый сигнал с tanh-сатурацией по amount
        float driven = std::tanh (x * (1.0f + amount * 4.0f));
        return x * (1.0f - amount) + driven * amount;
    }

    static float dbToLin (float db) { return std::pow (10.0f, db / 20.0f); }
    static float linToDb (float lin) { return 20.0f * std::log10 (lin); }
};
