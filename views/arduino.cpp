#include "arduino.h"

#include <QDebug>

Arduino::Arduino()
    : serial(new QSerialPort),
      arduino_is_available(false)
{
}

Arduino::~Arduino()
{
    if (serial->isOpen()) {
        serial->close();
    }
    delete serial;
    serial = nullptr;
}

int Arduino::connect_arduino()
{
    arduino_is_available = false;
    arduino_port_name.clear();

    for (const QSerialPortInfo& serial_port_info : QSerialPortInfo::availablePorts()) {
        if (serial_port_info.hasVendorIdentifier() && serial_port_info.hasProductIdentifier()) {
            if (serial_port_info.vendorIdentifier() == arduino_uno_vendor_id &&
                serial_port_info.productIdentifier() == arduino_uno_product_id) {
                arduino_is_available = true;
                arduino_port_name = serial_port_info.portName();
                break;
            }
        }
    }

    qDebug() << "arduino_port_name is:" << arduino_port_name;

    if (arduino_is_available) {
        serial->setPortName(arduino_port_name);
        if (serial->open(QSerialPort::ReadWrite)) {
            serial->setBaudRate(QSerialPort::Baud9600);
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);
            return 0;
        }
        qWarning() << "[ARDUINO] Failed to open" << arduino_port_name
                   << "error=" << serial->error()
                   << "details=" << serial->errorString();
        return 1;
    }

    return -1;
}

int Arduino::connect_arduino(const QString& portName)
{
    arduino_is_available = false;
    arduino_port_name.clear();

    for (const QSerialPortInfo& serial_port_info : QSerialPortInfo::availablePorts()) {
        if (serial_port_info.portName() == portName) {
            arduino_is_available = true;
            arduino_port_name = serial_port_info.portName();
            break;
        }
    }

    qDebug() << "arduino_port_name (by name) is:" << arduino_port_name;

    if (arduino_is_available) {
        serial->setPortName(arduino_port_name);
        if (serial->open(QSerialPort::ReadWrite)) {
            serial->setBaudRate(QSerialPort::Baud9600);
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);
            return 0;
        }
        qWarning() << "[ARDUINO] Failed to open" << arduino_port_name
                   << "error=" << serial->error()
                   << "details=" << serial->errorString();
        return 1;
    }

    return -1;
}

int Arduino::close_arduino()
{
    if (serial->isOpen()) {
        serial->close();
        return 0;
    }

    return 1;
}

QByteArray Arduino::read_from_arduino()
{
    if (serial->isReadable()) {
        data = serial->readAll();
        return data;
    }

    return QByteArray();
}

void Arduino::write_to_arduino(const QByteArray& d)
{
    if (serial->isWritable()) {
        serial->write(d);
    } else {
        qDebug() << "Couldn't write to serial!";
    }
}

QSerialPort* Arduino::getserial()
{
    return serial;
}

QString Arduino::getarduino_port_name()
{
    return arduino_port_name;
}
