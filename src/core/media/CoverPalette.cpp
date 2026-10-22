#include "CoverPalette.h"

#include <QHash>
#include <QList>

#include <algorithm>

namespace {

struct Bucket {
    int count = 0;
    double saturationSum = 0.0;
    double valueSum = 0.0;
    QColor reference;
};

/// Приводит цвет к «UI-акценту»: насыщенность в разумных пределах, яркость — тоже.
QColor normalizeAccent(const QColor &raw)
{
    QColor hsv = raw.toHsv();
    int hue = hsv.hsvHue();
    if (hue < 0)
        hue = 0;

    const int saturation = qBound(90, int(hsv.hsvSaturation() * 1.35), 210);
    const int value = qBound(150, int(hsv.value() * 1.15), 245);

    QColor out = QColor::fromHsv(hue, saturation, value);
    out.setAlpha(255);
    return out;
}

} // namespace

namespace CoverPalette {

Palette extract(const QImage &image)
{
    Palette palette;
    if (image.isNull())
        return palette;

    const QImage scaled = image.convertToFormat(QImage::Format_ARGB32)
                              .scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QHash<int, Bucket> buckets;
    for (int y = 0; y < scaled.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(scaled.constScanLine(y));
        for (int x = 0; x < scaled.width(); ++x) {
            const QRgb pixel = line[x];
            if (qAlpha(pixel) < 160)
                continue;

            QColor color = QColor::fromRgb(pixel);
            const int hsvValue = color.value();
            if (hsvValue < 18 || hsvValue > 246) // почти чёрное / почти белое
                continue;

            const QColor hsv = color.toHsv();
            const int key = ((qRed(pixel) >> 3) << 10) | ((qGreen(pixel) >> 3) << 5)
                            | (qBlue(pixel) >> 3);

            Bucket &bucket = buckets[key];
            ++bucket.count;
            bucket.saturationSum += hsv.hsvSaturationF();
            bucket.valueSum += hsv.valueF();
            bucket.reference = color;
        }
    }

    if (buckets.isEmpty()) {
        // Картинка однотонная или полностью тёмная: берём средний цвет как есть.
        const QColor average = QColor::fromRgb(scaled.pixel(0, 0));
        if (!average.isValid())
            return palette;
        palette.accent = normalizeAccent(average);
        palette.background = QColor::fromHsv(palette.accent.hsvHue(), 60, 16);
        palette.valid = true;
        return palette;
    }

    double bestScore = -1.0;
    QColor best;
    for (auto it = buckets.cbegin(); it != buckets.cend(); ++it) {
        const Bucket &bucket = it.value();
        const double saturation = bucket.saturationSum / bucket.count;
        const double value = bucket.valueSum / bucket.count;
        // Цвет должен быть достаточно контрастным, чтобы читаться на панелях.
        const double score = double(bucket.count) * (0.25 + saturation) * (0.35 + value);
        if (score > bestScore) {
            bestScore = score;
            best = bucket.reference;
        }
    }

    if (!best.isValid())
        return palette;

    palette.accent = normalizeAccent(best);
    const QColor accentHsv = palette.accent.toHsv();
    palette.background = QColor::fromHsv(accentHsv.hsvHue(), qBound(40, accentHsv.hsvSaturation() / 3, 90), 13);
    palette.valid = true;
    return palette;
}

} // namespace CoverPalette