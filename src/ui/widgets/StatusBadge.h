// Kaeru — status badge: icon + word + shape (never color-only, spec §8.5)
#pragma once
#include <QWidget>

class QLabel;

namespace kaeru {

class StatusBadge final : public QWidget {
    Q_OBJECT
public:
    explicit StatusBadge(QWidget* parent = nullptr);

    void setStatus(const QString& word, const QString& colorRole, bool filled);

    // colorRole: "neutral" | "ok" | "warn" | "bad" | "info" — paired with a
    // distinct glyph so state never depends on color alone.
    QString accessibleText() const;

private:
    void updateUi();
    QLabel* m_dot = nullptr;
    QLabel* m_text = nullptr;
    QString m_word;
    QString m_role;
    bool m_filled = false;
};

} // namespace kaeru
