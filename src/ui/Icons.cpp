#include "Icons.h"

#include <QFile>

namespace kaeru {

static const char* stateFile(IconState s)
{
    switch (s) {
    case IconState::Inactive: return ":/icons/kaeru.svg";
    case IconState::Awake: return ":/icons/kaeru-awake.svg";
    case IconState::Focus: return ":/icons/kaeru-focus.svg";
    case IconState::Recording: return ":/icons/kaeru-record.svg";
    case IconState::Paused: return ":/icons/kaeru-paused.svg";
    case IconState::Error: return ":/icons/kaeru-error.svg";
    }
    return ":/icons/kaeru.svg";
}

QString Icons::stateIconName(IconState state)
{
    switch (state) {
    case IconState::Inactive: return QStringLiteral("io.github.kaeru.Kaeru");
    case IconState::Awake: return QStringLiteral("io.github.kaeru.Kaeru.awake");
    case IconState::Focus: return QStringLiteral("io.github.kaeru.Kaeru.focus");
    case IconState::Recording: return QStringLiteral("io.github.kaeru.Kaeru.record");
    case IconState::Paused: return QStringLiteral("io.github.kaeru.Kaeru.paused");
    case IconState::Error: return QStringLiteral("io.github.kaeru.Kaeru.error");
    }
    return QStringLiteral("io.github.kaeru.Kaeru");
}

QIcon Icons::appIcon()
{
    return QIcon(QStringLiteral(":/icons/kaeru.svg"));
}

QIcon Icons::stateIcon(IconState state)
{
    return QIcon(QString::fromUtf8(stateFile(state)));
}

} // namespace kaeru
