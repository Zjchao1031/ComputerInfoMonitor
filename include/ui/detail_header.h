#pragma once

#include <QWidget>

class QLabel;

namespace monitor
{

class DetailHeader final : public QWidget
{
    Q_OBJECT

public:
    explicit DetailHeader(QWidget* parent = nullptr);
    ~DetailHeader() override = default;

    void setTitle(const QString& title);

signals:
    void backRequested();

private:
    QLabel* m_titleLabel{nullptr};
};

} // namespace monitor

