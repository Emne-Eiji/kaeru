// Kaeru — GitHub Releases update check (WP-09)
//
// Checks the official repository's public Releases endpoint and compares the
// latest published release tag with the installed version. This milestone
// only reports availability and links to the release page; it never downloads
// or replaces packages. No credentials or tokens are used.
#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkReply;

namespace kaeru {

class UpdateChecker final : public QObject {
    Q_OBJECT
public:
    enum class Outcome {
        UpdateAvailable,
        UpToDate,
        NoReleases,
        Error,
    };

    struct Result {
        Outcome outcome = Outcome::Error;
        QString currentVersion;
        QString latestVersion;
        QUrl releaseUrl;
        QString message;   // user-facing, honest status
        QString detail;    // technical detail for tooltips/logs (no secrets)
    };

    explicit UpdateChecker(QObject* parent = nullptr);

    void check(const QString& currentVersion);

    // Testable pure helpers.
    static int compareVersions(const QString& a, const QString& b);
    static Result parseLatestRelease(const QByteArray& json,
                                     const QString& currentVersion);

signals:
    void finished(const kaeru::UpdateChecker::Result& result);

private:
    QNetworkAccessManager m_network;
    QNetworkReply* m_reply = nullptr;
};

} // namespace kaeru
