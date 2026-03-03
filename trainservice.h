#ifndef TRAINSERVICE_H
#define TRAINSERVICE_H

#include <QObject>

class TrainService : public QObject
{
    Q_OBJECT

public:
    explicit TrainService(QObject *parent = nullptr);
};

#endif // TRAINSERVICE_H
