// Kaeru — Diagnostics page (spec §8.2, uiux §8)
// Capability matrix with honest statuses + live backend bindings.
// No private content is ever displayed or logged here.
#pragma once
#include "app/Application.h"

#include <QWidget>

class QLabel;
class QTableWidget;

namespace kaeru {

class DiagnosticsPage final : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    void refreshCapabilities();
    void refreshBindings();

    Application* m_app = nullptr;
    QTableWidget* m_capabilityTable = nullptr;
    QLabel* m_bindings = nullptr;
    QLabel* m_paths = nullptr;
};

} // namespace kaeru
