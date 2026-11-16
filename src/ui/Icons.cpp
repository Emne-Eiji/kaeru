// Kaeru — icon access (official frog identity, ADR-008)
// The three frog expressions are the Kaeru visual identity (original artwork
// supplied by Emne, GPL-3.0-or-later with the project; see docs/icon-assets.md).
//
// Embedded UI icons use the supplied PNG variants. Qt 6.2's SVG renderer
// mis-paints the focused expression (black face), so SVG is never used for
// runtime rendering; the installed SVG remains for scalable desktop entries.
#include "Icons.h"

namespace kaeru {

static const char* stateFileStem(IconState s)
{
    switch (s) {
    case IconState::Awake: return "awake";
    case IconState::Focus: return "focused";
    case IconState::Inactive:
    case IconState::Recording:
    case IconState::Paused:
    case IconState::Error:
    default:
        // Recording/paused/error all present as the inactive frog; the
        // tooltip (not the icon) carries the precise state (ADR-008).
        return "sleep";
    }
}

static QIcon frogIcon(IconState state)
{
    const QString stem = QString::fromLatin1(stateFileStem(state));
    QIcon icon;
    for (int px : { 16, 24, 32, 48, 64, 128, 256 }) {
        icon.addFile(QStringLiteral(":/icons/frog/png/%1-%2.png").arg(stem).arg(px),
            QSize(px, px));
    }
    return icon;
}

QString Icons::stateIconName(IconState state)
{
    switch (state) {
    case IconState::Awake: return QStringLiteral("io.github.kaeru.Kaeru.awake");
    case IconState::Focus: return QStringLiteral("io.github.kaeru.Kaeru.focused");
    case IconState::Inactive:
    case IconState::Recording:
    case IconState::Paused:
    case IconState::Error:
    default:
        return QStringLiteral("io.github.kaeru.Kaeru.sleep");
    }
}

QIcon Icons::appIcon()
{
    // Brand/launcher icon: the awake expression (recognizable, eyes open).
    return frogIcon(IconState::Awake);
}

QIcon Icons::stateIcon(IconState state)
{
    return frogIcon(state);
}

} // namespace kaeru
