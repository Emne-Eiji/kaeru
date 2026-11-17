// Kaeru unit tests — GitHub Releases update check (WP-09)
#include "app/UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using namespace kaeru;

class TestUpdates final : public QObject {
    Q_OBJECT

private slots:
    void versionComparison_data()
    {
        QTest::addColumn<QString>("a");
        QTest::addColumn<QString>("b");
        QTest::addColumn<int>("expected");

        QTest::newRow("equal") << QStringLiteral("0.2.0") << QStringLiteral("0.2.0") << 0;
        QTest::newRow("v-prefix") << QStringLiteral("v0.2.0") << QStringLiteral("0.2.0") << 0;
        QTest::newRow("newer-patch") << QStringLiteral("0.2.1") << QStringLiteral("0.2.0") << 1;
        QTest::newRow("older-patch") << QStringLiteral("0.1.9") << QStringLiteral("0.2.0") << -1;
        QTest::newRow("numeric-minor") << QStringLiteral("0.10.0") << QStringLiteral("0.9.9") << 1;
        QTest::newRow("shorter") << QStringLiteral("1.0") << QStringLiteral("1.0.0") << 0;
        QTest::newRow("prerelease-suffix") << QStringLiteral("0.3.0-beta") << QStringLiteral("0.3.0") << 0;
    }

    void versionComparison()
    {
        QFETCH(QString, a);
        QFETCH(QString, b);
        QFETCH(int, expected);
        QCOMPARE(UpdateChecker::compareVersions(a, b), expected);
    }

    void updateAvailable()
    {
        QJsonObject object;
        object.insert(QStringLiteral("tag_name"), QStringLiteral("v0.3.0"));
        object.insert(QStringLiteral("html_url"),
                      QStringLiteral("https://github.com/Emne-Eiji/kaeru/releases/tag/v0.3.0"));
        object.insert(QStringLiteral("draft"), false);
        object.insert(QStringLiteral("prerelease"), false);
        const UpdateChecker::Result r = UpdateChecker::parseLatestRelease(
            QJsonDocument(object).toJson(), QStringLiteral("0.2.0"));
        QCOMPARE(r.outcome, UpdateChecker::Outcome::UpdateAvailable);
        QCOMPARE(r.latestVersion, QStringLiteral("0.3.0"));
        QCOMPARE(r.currentVersion, QStringLiteral("0.2.0"));
        QVERIFY(r.message.contains(QStringLiteral("update is available")));
        QCOMPARE(r.releaseUrl.host(), QStringLiteral("github.com"));
    }

    void upToDateIsExplicit()
    {
        QJsonObject object;
        object.insert(QStringLiteral("tag_name"), QStringLiteral("0.2.0"));
        object.insert(QStringLiteral("html_url"),
                      QStringLiteral("https://github.com/Emne-Eiji/kaeru/releases/tag/0.2.0"));
        const UpdateChecker::Result r = UpdateChecker::parseLatestRelease(
            QJsonDocument(object).toJson(), QStringLiteral("0.2.0"));
        QCOMPARE(r.outcome, UpdateChecker::Outcome::UpToDate);
        QVERIFY(r.message.contains(QStringLiteral("latest version")));
    }

    void failureNeverClaimsUpToDate()
    {
        const QByteArray badData[] = {
            QByteArray("not json"),
            QByteArray("{}"),
            QByteArray("{\"message\":\"API rate limit exceeded\"}"),
            QByteArray("{\"tag_name\":\"garbage\",\"html_url\":\"https://example.com\"}"),
            QByteArray("{\"tag_name\":\"v9.9.9\",\"html_url\":\"not a url\"}"),
        };
        for (const QByteArray& data : badData) {
            const UpdateChecker::Result r = UpdateChecker::parseLatestRelease(
                data, QStringLiteral("0.2.0"));
            QCOMPARE(r.outcome, UpdateChecker::Outcome::Error);
            QVERIFY(!r.message.contains(QStringLiteral("latest version")));
            QVERIFY(!r.message.isEmpty());
        }
    }

    void prereleaseIsNotApplicable()
    {
        QJsonObject object;
        object.insert(QStringLiteral("tag_name"), QStringLiteral("v0.3.0-beta"));
        object.insert(QStringLiteral("html_url"),
                      QStringLiteral("https://github.com/Emne-Eiji/kaeru/releases/tag/v0.3.0-beta"));
        object.insert(QStringLiteral("prerelease"), true);
        const UpdateChecker::Result r = UpdateChecker::parseLatestRelease(
            QJsonDocument(object).toJson(), QStringLiteral("0.2.0"));
        QCOMPARE(r.outcome, UpdateChecker::Outcome::NoReleases);
        QVERIFY(!r.message.contains(QStringLiteral("latest version")));
    }
};

QTEST_MAIN(TestUpdates)
#include "test_updates.moc"
