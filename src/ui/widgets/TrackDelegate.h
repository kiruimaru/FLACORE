#ifndef TRACKDELEGATE_H
#define TRACKDELEGATE_H

#include <QStyledItemDelegate>

/**
 * Строка трека: название, исполнитель, длительность, индикатор «играет сейчас»,
 * аккуратное скруглённое выделение и подсветка по наведению.
 */
class TrackDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum Role {
        ArtistRole = Qt::UserRole,
        DurationRole,
        AlbumRole,
        PathRole,
    };

    explicit TrackDelegate(QObject *parent = nullptr);

    /// Номер строки, которая играет сейчас (-1 — ничего).
    void setPlayingRow(int row);
    int playingRow() const { return m_playing_row; }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    int m_playing_row = -1;
};

#endif // TRACKDELEGATE_H