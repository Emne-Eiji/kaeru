// Kaeru — Reports page (spec §8.2, delivered with WP-06)
#pragma once
#include "app/Application.h"

#include <QWidget>

class QLabel;
class QPushButton;

namespace kaeru {

class ReportsPage final : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    Application* m_app = nullptr;
    QLabel* m_summary = nullptr;
    QPushButton* m_export = nullptr;
};

} // namespace kaeru
