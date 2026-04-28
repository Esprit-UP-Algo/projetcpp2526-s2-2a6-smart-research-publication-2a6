#ifndef ARDUINO_H
#define ARDUINO_H

#include <QByteArray>
#include <QString>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class Arduino
{
public:
    Arduino();
    ~Arduino();

    int connect_arduino();
    // Connect to a specific port name (e.g. "COM3").
    int connect_arduino(const QString& portName);
    int close_arduino();

    void write_to_arduino(const QByteArray& d);
    QByteArray read_from_arduino();

    QSerialPort* getserial();
    QString getarduino_port_name();

private:
    QSerialPort* serial;

    static const quint16 arduino_uno_vendor_id = 9025;
    static const quint16 arduino_uno_product_id = 67;

    QString arduino_port_name;
    bool arduino_is_available;
    QByteArray data;
};

#endif // ARDUINO_H
