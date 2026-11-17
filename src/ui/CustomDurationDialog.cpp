#include "CustomDurationDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace kaeru {

CustomDurationDialog::CustomDurationDialog(QWidget* parent, qint64 initialMs)
    : QDialog(parent)
{
    setWindowTitle(tr("Custom Keep Awake duration"));
    setModal(true);
    setMinimumWidth(380);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 16);
    root->setSpacing(12);

    auto* intro = new QLabel(
        tr("Choose a finite duration. Indefinitely is available separately."),
        this);
    intro->setObjectName(QStringLiteral("Subtle"));
    intro->setWordWrap(true);
    root->addWidget(intro);

    if (initialMs < kMinCustomAwakeMs || initialMs > kMaxCustomAwakeMs
        || initialMs % (60LL * 1000) != 0)
        initialMs = 90LL * 60 * 1000;

    m_hours = new QSpinBox(this);
    m_hours->setRange(0, 168);
    m_hours->setSuffix(tr(" hours"));
    m_hours->setAccelerated(true);
    m_hours->setAccessibleName(tr("Custom duration hours"));
    m_minutes = new QSpinBox(this);
    m_minutes->setRange(0, 59);
    m_minutes->setSuffix(tr(" minutes"));
    m_minutes->setAccelerated(true);
    m_minutes->setAccessibleName(tr("Custom duration minutes"));

    m_hours->setValue(int(initialMs / (60LL * 60 * 1000)));
    m_minutes->setValue(int((initialMs / (60LL * 1000)) % 60));

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->addRow(tr("Hours"), m_hours);
    form->addRow(tr("Minutes"), m_minutes);
    root->addLayout(form);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("TimerLabelSmall"));
    m_summary->setWordWrap(true);
    root->addWidget(m_summary);

    m_error = new QLabel(this);
    m_error->setObjectName(QStringLiteral("WarningLabel"));
    m_error->setWordWrap(true);
    root->addWidget(m_error);

    auto* note = new QLabel(tr("Maximum 7 days (168 hours)."), this);
    note->setObjectName(QStringLiteral("Subtle"));
    root->addWidget(note);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Start"));
    m_buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("PrimaryButton"));
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_hours, &QSpinBox::valueChanged, this, &CustomDurationDialog::validate);
    connect(m_minutes, &QSpinBox::valueChanged, this, &CustomDurationDialog::validate);

    validate();
}

void CustomDurationDialog::validate()
{
    const qint64 hours = m_hours->value();
    const qint64 minutes = m_minutes->value();
    qint64 ms = 0;
    QString error;
    const bool ok = validateCustomAwakeDuration(hours, minutes, &ms, &error);
    m_error->setText(ok ? QString() : error);
    m_error->setVisible(!ok);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);
    m_summary->setText(ok
            ? tr("Keep awake for %1.").arg(formatAwakeDuration(ms))
            : tr("Enter a duration between 1 minute and 7 days."));
}

AwakeDuration CustomDurationDialog::selection() const
{
    AwakeDuration d;
    if (!AwakeDuration::custom(m_hours->value(), m_minutes->value(), &d))
        return AwakeDuration::standard(AwakeDuration::Kind::Hour1);
    return d;
}

} // namespace kaeru
