// Kaeru — XDG path resolution with test sandbox override
#pragma once
#include <QString>

namespace kaeru {

class Paths final {
public:
    // Returns the effective directories honoring KAERU_TEST_HOME (tests and
    // integration scripts) so user data is never touched by test runs.
    static QString configHome();    // $XDG_CONFIG_HOME or ~/.config
    static QString dataHome();      // $XDG_DATA_HOME or ~/.local/share
    static QString cacheHome();     // $XDG_CACHE_HOME or ~/.cache
    static QString runtimeDir();    // $XDG_RUNTIME_DIR or data fallback

    static QString configDir();     // <configHome>/kaeru
    static QString dataDir();       // <dataHome>/kaeru
    static QString cacheDir();      // <cacheHome>/kaeru
    static QString settingsFile();
    static QString databaseFile();

    static void applyTestSandbox(const QString& testHomeRoot); // for tests only
};

} // namespace kaeru
