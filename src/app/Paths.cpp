#include "Paths.h"

#include <QDir>
#include <QStandardPaths>

namespace kaeru {

static QString base(const char* envVar, QStandardPaths::StandardLocation fallback)
{
    const QString testHome = qEnvironmentVariable("KAERU_TEST_HOME");
    if (!testHome.isEmpty())
        return testHome;
    const QString env = qEnvironmentVariable(envVar);
    if (!env.isEmpty())
        return env;
    return QStandardPaths::writableLocation(fallback);
}

QString Paths::configHome() { return base("XDG_CONFIG_HOME", QStandardPaths::GenericConfigLocation); }
QString Paths::dataHome() { return base("XDG_DATA_HOME", QStandardPaths::GenericDataLocation); }
QString Paths::cacheHome() { return base("XDG_CACHE_HOME", QStandardPaths::GenericCacheLocation); }

QString Paths::runtimeDir()
{
    const QString testHome = qEnvironmentVariable("KAERU_TEST_HOME");
    if (!testHome.isEmpty())
        return testHome + QStringLiteral("/runtime");
    const QString env = qEnvironmentVariable("XDG_RUNTIME_DIR");
    return env.isEmpty() ? dataHome() : env;
}

QString Paths::configDir() { return configHome() + QStringLiteral("/kaeru"); }
QString Paths::dataDir() { return dataHome() + QStringLiteral("/kaeru"); }
QString Paths::cacheDir() { return cacheHome() + QStringLiteral("/kaeru"); }
QString Paths::settingsFile() { return configDir() + QStringLiteral("/kaeru.conf"); }
QString Paths::databaseFile() { return dataDir() + QStringLiteral("/kaeru.db"); }

void Paths::applyTestSandbox(const QString& testHomeRoot)
{
    qputenv("KAERU_TEST_HOME", testHomeRoot.toUtf8());
    qputenv("XDG_CONFIG_HOME", (testHomeRoot + QStringLiteral("/config")).toUtf8());
    qputenv("XDG_DATA_HOME", (testHomeRoot + QStringLiteral("/data")).toUtf8());
    qputenv("XDG_CACHE_HOME", (testHomeRoot + QStringLiteral("/cache")).toUtf8());
}

} // namespace kaeru
