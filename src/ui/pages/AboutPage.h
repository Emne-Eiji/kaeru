// Kaeru — About page (WP-09): official identity, honest project facts and
// a read-only GitHub Releases update check. No Quit action lives here.
#pragma once

#include "app/Application.h"
#include "app/UpdateChecker.h"

#include <QWidget>

class QLabel;
class QPushButton;

namespace kaeru {

class AboutPage final : public QWidget {
    Q_OBJECT
public:
    explicit AboutPage(Application* app, QWidget* parent = nullptr);

private slots:
    void checkForUpdates();
    void onUpdateCheckFinished(const kaeru::UpdateChecker::Result& result);

private:
    QWidget* sectionCard(const QString& title);

    Application* m_app = nullptr;
    UpdateChecker* m_updates = nullptr;
    QPushButton* m_checkButton = nullptr;
    QLabel* m_updateStatus = nullptr;
    QLabel* m_releaseLink = nullptr;
};

} // namespace kaeru
