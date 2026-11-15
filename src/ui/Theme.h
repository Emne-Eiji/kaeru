// Kaeru — theming: light/dark palettes + QSS (ADR-007)
#pragma once
#include "platform/ThemeWatcher.h"

#include <QObject>
#include <QString>

namespace kaeru {

class Theme final : public QObject {
    Q_OBJECT
public:
    static Theme& instance();

    // Applies the theme for the given preference: "system"|"light"|"dark".
    void apply(const QString& preference, ColorScheme systemScheme);

    bool isDark() const { return m_dark; }

signals:
    void changed(bool dark);

private:
    Theme() = default;
    void setDark(bool dark);
    QString styleSheet(bool dark) const;

    bool m_dark = false;
};

} // namespace kaeru
