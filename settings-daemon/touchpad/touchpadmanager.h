#ifndef TOUCHPADMANAGER_H
#define TOUCHPADMANAGER_H

#include <QObject>

class TouchpadManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled CONSTANT)
    Q_PROPERTY(bool tapToClick READ tapToClick WRITE setTapToClick CONSTANT)
    Q_PROPERTY(bool naturalScroll READ naturalScroll WRITE setNaturalScroll CONSTANT)
    Q_PROPERTY(qreal pointerAcceleration READ pointerAcceleration WRITE setPointerAcceleration CONSTANT)

public:
    explicit TouchpadManager(QObject *parent = nullptr);

    bool available() const;

    bool enabled() const;
    void setEnabled(bool enabled);

    bool tapToClick() const;
    void setTapToClick(bool value);

    bool naturalScroll() const;
    void setNaturalScroll(bool naturalScroll);

    qreal pointerAcceleration() const;
    void setPointerAcceleration(qreal value);

private:
    void applyConfig();

    bool m_available = false;
    bool m_enabled = true;
    bool m_tapToClick = false;
    bool m_naturalScroll = false;
    qreal m_pointerAcceleration = 0.0;
};

#endif // TOUCHPADMANAGER_H
