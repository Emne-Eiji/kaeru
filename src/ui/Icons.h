// Kaeru — icon access (original SVG assets, GPL-3.0 licensed with project)
#pragma once
#include <QIcon>

namespace kaeru {

enum class IconState : quint8 { Inactive, Awake, Focus, Recording, Paused, Error };

class Icons final {
public:
    // Loads from embedded resources; never third-party assets.
    static QIcon appIcon();
    static QIcon stateIcon(IconState state);
    static QString stateIconName(IconState state);
};

} // namespace kaeru
