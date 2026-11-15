// Kaeru — theme watcher: light/dark following the desktop (ADR-007)
// Uses the xdg-desktop-portal Settings interface (org.freedesktop.appearance
// color-scheme), a native API; falls back to system default when unavailable.
#pragma once
#include "core/Types.h"

#include <QDBusVariant>
#include <QObject>
#include <QString>

namespace kaeru {

enum class ColorScheme { Unknown, Light, Dark };

class ThemeWatcher final : public QObject {
    Q_OBJECT
public:
    explicit ThemeWatcher(QObject* parent = nullptr);

    ColorScheme scheme() const { return m_scheme; }
    bool available() const { return m_available; }

signals:
    void schemeChanged(ColorScheme scheme);

private slots:
    void onSettingChanged(const QString& namespaceName, const QString& key, const QDBusVariant& value);
    void readInitial();

private:
    void setScheme(quint32 value);
    ColorScheme m_scheme = ColorScheme::Unknown;
    bool m_available = false;
};

} // namespace kaeru
