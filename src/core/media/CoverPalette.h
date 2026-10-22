#ifndef COVERPALETTE_H
#define COVERPALETTE_H

#include <QColor>
#include <QImage>

/**
 * Извлечение акцентной пары цветов из картинки (обложки/обоев) — аналог
 * «Cover Colors» из SoundCloud Desktop: интерфейс перекрашивается под текущий
 * трек, если в настройках включено «брать акцент из обложки».
 *
 * Алгоритм детерминированный и без зависимостей: квантование по 5 битам на
 * канал, отбор по «частота × насыщенность × средняя яркость», затем нормализация
 * до пригодного акцента и тёмного фона.
 */
namespace CoverPalette {

struct Palette {
    QColor accent;      // пригодный для UI акцент (валиден всегда)
    QColor background;  // тёмный/светлый фон под этот акцент
    bool valid = false; // false, если картинка пустая/однотонная — тогда не применяем
};

Palette extract(const QImage &image);

} // namespace CoverPalette

#endif // COVERPALETTE_H