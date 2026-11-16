// Kaeru — icon access (official frog identity, ADR-008)
// The three frog expressions are the Kaeru visual identity (original artwork
// supplied by Emne, GPL-3.0-or-later with the project; see docs/icon-assets.md).
#include "Icons.h"

#include <QFile>

namespace kaeru {

static const char* stateFile(IconState s)
{
    switch (s) {
    case IconState::Awake: return ":/icons/frog/awake.svg";
    case IconState::Focus: return ":/icons/frog/focused.svg";
    case IconState::Inactive:
    case IconState::Recording:
    case IconState::Paused:
    case IconState::Error:
    default:
        // Recording/paused/error all present as the inactive frog; the
        // tooltip (not the icon) carries the precise state (ADR-008).
        return ":/icons/frog/sleep.svg";
    }
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
    return QIcon(QStringLiteral(":/icons/frog/awake.svg"));
}

QIcon Icons::stateIcon(IconState state)
{
    return QIcon(QString::fromUtf8(stateFile(state)));
}

} // namespace kaeru