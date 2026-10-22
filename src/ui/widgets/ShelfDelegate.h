#ifndef SHELFDELEGATE_H
#define SHELFDELEGATE_H

#include <QStyledItemDelegate>

class CoverLoader;

/**
 * Строка «полки» чарта: миниатюра обложки, название, исполнитель, источник
 * и длительность. Обложка подтягивается из CoverLoader (кэш + сеть), поэтому
 * делегат умеет просить её загрузку и перерисовываться, когда она пришла.
 */
class ShelfDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /// Высота одной строки полки (её же использует страница для расчёта высоты).
    static constexpr int RowHeight = 52;

    enum Role {
        ArtistRole = Qt::UserRole,
        DurationRole,
        CoverUrlRole,
        SourceRole,
        UrlRole,
        PreviewUrlRole,
        /// Строка-подсказка («данных нет»): не трек, рисуется иначе и не считается данными.
        HintRole,
    };

    explicit ShelfDelegate(CoverLoader *covers, QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    CoverLoader *m_covers = nullptr;
};

#endif // SHELFDELEGATE_H