// Kaeru — compact custom Keep Awake duration dialog (WP-08)
// Shared by the main GUI and the tray duration menu. Whole hours/minutes,
// validated, finite, maximum seven days. "Indefinitely" is a separate choice
// and is never produced by this dialog.
#pragma once

#include "core/AwakeDuration.h"

#include <QDialog>

class QDialogButtonBox;
class QLabel;
class QSpinBox;

namespace kaeru {

class CustomDurationDialog final : public QDialog {
    Q_OBJECT
public:
    explicit CustomDurationDialog(QWidget* parent = nullptr,
                                  qint64 initialMs = 90LL * 60 * 1000);

    AwakeDuration selection() const;

private slots:
    void validate();

private:
    QSpinBox* m_hours = nullptr;
    QSpinBox* m_minutes = nullptr;
    QLabel* m_summary = nullptr;
    QLabel* m_error = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};

} // namespace kaeru
