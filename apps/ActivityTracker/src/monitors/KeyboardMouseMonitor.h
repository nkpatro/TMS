#ifndef KEYBOARDMOUSEMONITOR_H
#define KEYBOARDMOUSEMONITOR_H

#include <QObject>

class KeyboardMouseMonitor : public QObject
{
    Q_OBJECT
public:
    explicit KeyboardMouseMonitor(QObject *parent = nullptr);
    virtual ~KeyboardMouseMonitor();

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    int idleTimeThreshold() const;
    void setIdleTimeThreshold(int milliseconds);

    virtual int getIdleTime() const = 0; // Added virtual method to fix override error

    signals:
        void keyboardActivity();
    void mouseActivity(int x, int y, bool clicked);
    void idleTimeExceeded();
    void userReturnedFromIdle();

protected:
    int m_idleTimeThreshold;  // milliseconds
};

#endif // KEYBOARDMOUSEMONITOR_H