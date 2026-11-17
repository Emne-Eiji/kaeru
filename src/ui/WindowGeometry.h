// Kaeru — safe window geometry restore (WP-09)
// Pure helper so the main window never reopens entirely outside the visible
// screen after a monitor change, resolution change or saved position.
#pragma once

#include <QRect>
#include <QSize>

namespace kaeru {

// Fits `saved` into `available`. Invalid saved geometry produces a centered
// `fallback`. The minimum size is honored only as far as the screen allows.
inline QRect fitWindowToAvailable(const QRect& saved, const QRect& available,
                                  const QSize& minimum, const QSize& fallback)
{
    const QRect avail = available.isValid() ? available
                                            : QRect(0, 0, 1280, 800);
    const int minWidth = qMin(qMax(1, minimum.width()), avail.width());
    const int minHeight = qMin(qMax(1, minimum.height()), avail.height());
    const QSize wanted = saved.isValid() ? saved.size() : fallback;
    const int width = qBound(minWidth, wanted.width(), avail.width());
    const int height = qBound(minHeight, wanted.height(), avail.height());

    int x = saved.isValid() ? saved.x()
                            : avail.x() + (avail.width() - width) / 2;
    int y = saved.isValid() ? saved.y()
                            : avail.y() + (avail.height() - height) / 2;

    // Keep the whole frame inside the chosen screen. This also handles a
    // saved position on a monitor that no longer exists.
    x = qBound(avail.left(), x, avail.right() - width + 1);
    y = qBound(avail.top(), y, avail.bottom() - height + 1);
    return QRect(x, y, width, height);
}

} // namespace kaeru
