#pragma once

#include "model/monitor_snapshot.h"

#include <QListView>

class QEvent;
class QHideEvent;

namespace monitor
{

class CpuSensorModel;
class SensorRowDelegate;

class CpuSensorTree final : public QListView
{
    Q_OBJECT

public:
    explicit CpuSensorTree(QWidget* parent = nullptr);
    ~CpuSensorTree() override = default;

    void setGroups(const QVector<SensorGroup>& groups);

signals:
    void scrollingChanged(bool scrolling);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void beginScrolling();
    void endScrolling();

    CpuSensorModel* m_model{nullptr};
    SensorRowDelegate* m_delegate{nullptr};
    qreal m_lastMouseY{0.0};
    qreal m_pendingDragPixels{0.0};
    bool m_leftButtonPressed{false};
    bool m_scrolling{false};
};

} // namespace monitor

