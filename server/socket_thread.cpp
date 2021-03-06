#include "socket_thread.h"

#include <QThread>

socket_thread::socket_thread(qint32 socket_id, QObject *parent)
    : QThread(parent)
    , _socket_id(socket_id)
{
}

void socket_thread::write(const QJsonObject& data)
{
    qDebug() << "\nWRITE";
    QJsonDocument doc(data);
    QString res = doc.toJson(QJsonDocument::Compact);
    qDebug() << res.mid(0, 100);
    qint32 start = 0;
    QString temp;
    do
    {
        temp = res.mid(start, 100);
        _soc->write(temp.toUtf8());
        start += 100;
    }
    while (temp != "");
    _soc->write(QString("*").toUtf8());
    _soc->flush();
}

void socket_thread::read_data()
{
    qDebug() << "\nREAD";
    QString data = _soc->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject obj =  doc.object();
    qDebug() << obj;
    if (!obj.contains("type")) return;
    QString type = obj["type"].toString();
    if (type == "animal")  // switch не работает со строками 8(
    {
        qDebug() << "ANIMAL";
        if (!(
            obj.contains("animal speed") &&
            obj.contains("animal endurance") &&
            obj.contains("animal vitality") &&
            obj.contains("animal max vitality") &&
            obj.contains("animal number") &&
            obj.contains("animal level")
            )) return;
        logic_component::Animal animal;
        animal.speed = obj["animal speed"].toInt();
        animal.endurance = obj["animal endurance"].toInt();
        animal.vitality = obj["animal vitality"].toInt();
        animal.max_vitality = obj["animal max vitality"].toInt();
        _logic->add_animal(animal, obj["animal number"].toInt(), obj["animal level"].toInt());
    } else if (type == "grass")
    {
        qDebug() << "GRASS";
        if (!obj.contains("grass size")) return;
        _logic->add_grass(obj["grass size"].toInt());
    } else if (type == "start")
    {
        qDebug() << "NEXT";
        _timer->start(1000);
    } else if (type == "stop")
    {
        qDebug() << "STOP";
        _timer->stop();
    } else if (type == "restart")
    {
        qDebug() << "CLEAR";
        _logic->clear();
    } else if (type == "delete")
    {
        qDebug() << "DELETE";
        if (!obj.contains("num")) return;
        _logic->remove_animal(obj["num"].toInt());
    }
}

void socket_thread::run()
{
    _soc = new QTcpSocket();
    _logic = new logic_component(30, 30);
    _timer = new QTimer();
    if (_soc->setSocketDescriptor(_socket_id))
    {
        connect(_soc, &QTcpSocket::readyRead, this, &socket_thread::read_data, Qt::DirectConnection);
        connect(_logic, &logic_component::notify, this, &socket_thread::write, Qt::DirectConnection);
        connect(_soc, &QTcpSocket::disconnected, this, &socket_thread::quit);
        connect(_timer, &QTimer::timeout, _logic, &logic_component::tik);

        connect(_soc, &QTcpSocket::disconnected, _timer, &QTimer::deleteLater);
        connect(_soc, &QTcpSocket::disconnected, _soc, &QTcpSocket::deleteLater);
        connect(_soc, &QTcpSocket::disconnected, _logic, &logic_component::deleteLater);
        connect(_soc, &QTcpSocket::disconnected, _timer, &QTimer::deleteLater);

        qDebug() << "Connection started";
        _logic->tik();
        exec();
        qDebug() << "Connection ends";
    } else qDebug() << "Something wrong";
    emit is_finished(_socket_id);
}
