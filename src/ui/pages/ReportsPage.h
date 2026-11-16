// Kaeru — Reports page (spec §8.2, §6.4, WP-06)
// Daily totals (local days) + project breakdown + focus time; date and
// project filters; CSV export with summary dialog.
#pragma once
#include "app/Application.h"

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

namespace kaeru {

class ReportsPage final : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onExport();

private:
    void rebuildProjectFilter();
    bool currentRange(QDate* from, QDate* to) const;

    Application* m_app = nullptr;
    QDateEdit* m_from = nullptr;
    QDateEdit* m_to = nullptr;
    QComboBox* m_project = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_summary = nullptr;
    QPushButton* m_export = nullptr;
};

} // namespace kaeru