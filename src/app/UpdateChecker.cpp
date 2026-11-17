#include "UpdateChecker.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <algorithm>

namespace kaeru {

namespace {

const char* kReleasesUrl =
    "https://api.github.com/repos/Emne-Eiji/kaeru/releases/latest";

QString noReleasesMessage()
{
    return QCoreApplication::translate("UpdateChecker",
        "No published release is available yet. Update status is unknown.");
}

QString errorMessage()
{
    return QCoreApplication::translate("UpdateChecker",
        "Unable to check for updates. Please try again later.");
}

bool versionParts(const QString& version, QList<int>* parts)
{
    QString s = version.trimmed();
    if (s.startsWith(QLatin1Char('v')) || s.startsWith(QLatin1Char('V')))
        s.remove(0, 1);
    const int dash = s.indexOf(QLatin1Char('-'));
    const int plus = s.indexOf(QLatin1Char('+'));
    int cut = -1;
    if (dash >= 0 && plus >= 0)
        cut = qMin(dash, plus);
    else if (dash >= 0)
        cut = dash;
    else if (plus >= 0)
        cut = plus;
    if (cut >= 0)
        s = s.left(cut);
    if (s.isEmpty())
        return false;
    const QStringList pieces = s.split(QLatin1Char('.'));
    for (const QString& piece : pieces) {
        if (piece.isEmpty())
            return false;
        bool ok = false;
        const int value = piece.toInt(&ok);
        if (!ok || value < 0)
            return false;
        parts->append(value);
    }
    return !parts->isEmpty();
}

} // namespace

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
{
}

int UpdateChecker::compareVersions(const QString& a, const QString& b)
{
    QList<int> pa;
    QList<int> pb;
    if (!versionParts(a, &pa) || !versionParts(b, &pb))
        return 0;
    const int count = std::max(pa.size(), pb.size());
    for (int i = 0; i < count; ++i) {
        const int va = i < pa.size() ? pa.at(i) : 0;
        const int vb = i < pb.size() ? pb.at(i) : 0;
        if (va < vb)
            return -1;
        if (va > vb)
            return 1;
    }
    return 0;
}

UpdateChecker::Result UpdateChecker::parseLatestRelease(const QByteArray& json,
                                                        const QString& currentVersion)
{
    Result result;
    result.currentVersion = currentVersion;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.outcome = Outcome::Error;
        result.message = errorMessage();
        result.detail = QStringLiteral("Malformed GitHub response: %1")
                            .arg(parseError.errorString());
        return result;
    }

    const QJsonObject object = doc.object();
    if (object.value(QStringLiteral("draft")).toBool()
        || object.value(QStringLiteral("prerelease")).toBool()) {
        result.outcome = Outcome::NoReleases;
        result.message = noReleasesMessage();
        result.detail = QStringLiteral("The latest release is a draft or prerelease.");
        return result;
    }

    const QString tag = object.value(QStringLiteral("tag_name")).toString().trimmed();
    QString latest = tag;
    if (latest.startsWith(QLatin1Char('v')) || latest.startsWith(QLatin1Char('V')))
        latest.remove(0, 1);
    QList<int> ignored;
    if (tag.isEmpty() || !versionParts(tag, &ignored)) {
        result.outcome = Outcome::Error;
        result.message = errorMessage();
        result.detail = QStringLiteral("The release response has no usable version tag.");
        return result;
    }

    const QUrl url(object.value(QStringLiteral("html_url")).toString());
    if (!url.isValid()
        || (url.scheme() != QStringLiteral("https")
            && url.scheme() != QStringLiteral("http"))) {
        result.outcome = Outcome::Error;
        result.message = errorMessage();
        result.detail = QStringLiteral("The release response has no usable release URL.");
        return result;
    }

    result.latestVersion = latest;
    result.releaseUrl = url;

    QList<int> currentParts;
    if (!versionParts(currentVersion, &currentParts)) {
        result.outcome = Outcome::Error;
        result.message = errorMessage();
        result.detail = QStringLiteral("The installed version could not be compared.");
        return result;
    }

    if (compareVersions(latest, currentVersion) > 0) {
        result.outcome = Outcome::UpdateAvailable;
        result.message = tr("An update is available: %1 (you have %2).")
                             .arg(latest, currentVersion);
    } else {
        result.outcome = Outcome::UpToDate;
        result.message = tr("You are using the latest version (%1).")
                             .arg(currentVersion);
    }
    return result;
}

void UpdateChecker::check(const QString& currentVersion)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(kReleasesUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Kaeru/%1").arg(currentVersion));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_network.get(request);
    m_reply->setProperty("currentVersion", currentVersion);

    // Qt 6.2 has no request transfer timeout; abort an unresponsive check.
    QTimer::singleShot(15000, m_reply, [reply = m_reply] {
        if (reply && reply->isRunning())
            reply->abort();
    });

    connect(m_reply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_reply;
        if (!reply)
            return;
        m_reply = nullptr;
        reply->deleteLater();

        const QString current = reply->property("currentVersion").toString();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        Result result;
        result.currentVersion = current;

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            result.outcome = Outcome::Error;
            result.message = errorMessage();
            result.detail = tr("The update check timed out.");
        } else if (reply->error() != QNetworkReply::NoError && status == 0) {
            result.outcome = Outcome::Error;
            result.message = errorMessage();
            result.detail = reply->errorString();
        } else if (status == 404) {
            result.outcome = Outcome::NoReleases;
            result.message = noReleasesMessage();
            result.detail = tr("GitHub returned 404: no published release.");
        } else if (status == 403 || status == 429) {
            result.outcome = Outcome::Error;
            result.message = errorMessage();
            result.detail = tr("GitHub's public API is rate-limited or refused the request (HTTP %1).")
                                .arg(status);
        } else if (status != 200) {
            result.outcome = Outcome::Error;
            result.message = errorMessage();
            result.detail = tr("GitHub returned HTTP %1.").arg(status);
        } else {
            result = parseLatestRelease(reply->readAll(), current);
        }

        emit finished(result);
    });
}

} // namespace kaeru
