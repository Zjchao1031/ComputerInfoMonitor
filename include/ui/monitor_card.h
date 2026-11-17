#pragma once

#include <QFrame>

class QLabel;
class QMouseEvent;
class QVBoxLayout;

namespace monitor
{

class MonitorCard final : public QFrame
{
    Q_OBJECT

public:
    explicit MonitorCard(const QString& title, QWidget* parent = nullptr);
    ~MonitorCard() override = default;

    void setHardwareName(const QString& name);
    void setInteractive(bool interactive);
    [[nodiscard]] QVBoxLayout* bodyLayout() const noexcept;

signals:
    void activated();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QLabel* m_hardwareNameLabel{nullptr};
    QVBoxLayout* m_bodyLayout{nullptr};
};

} // namespace monitor

