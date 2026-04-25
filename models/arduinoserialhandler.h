#ifndef ARDUINOSERIALHANDLER_H
#define ARDUINOSERIALHANDLER_H

#include <QString>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QObject>
#include <QTimer>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>

// ================================================================================
// ArduinoSerialHandler - Manages communication between Arduino and Qt/Database
// ================================================================================

class ArduinoSerialHandler : public QObject
{
    Q_OBJECT

private:
    QSerialPort *serialPort;
    QTimer *reconnectTimer;
    QString receivedData;
    bool isConnected;
    const int BAUD_RATE = 9600;
    const int READ_TIMEOUT = 100;  // milliseconds

public:
    /**
     * Constructor
     */
    ArduinoSerialHandler() : isConnected(false) {
        serialPort = new QSerialPort(this);
        reconnectTimer = new QTimer(this);
        
        connect(serialPort, &QSerialPort::readyRead, this, &ArduinoSerialHandler::onDataReceived);
        connect(reconnectTimer, &QTimer::timeout, this, &ArduinoSerialHandler::attemptConnection);
    }

    /**
     * Destructor
     */
    ~ArduinoSerialHandler() {
        if (serialPort->isOpen()) {
            serialPort->close();
        }
    }

    /**
     * Initialize and connect to Arduino
     * Scans available COM ports and attempts connection
     */
    bool initializeConnection() {
        QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
        
        if (availablePorts.isEmpty()) {
            qWarning() << "No serial ports available";
            reconnectTimer->start(3000);  // Try again in 3 seconds
            return false;
        }

        // Try to connect to the first available port
        // You can modify this to search for a specific port if needed
        for (const QSerialPortInfo &portInfo : availablePorts) {
            if (connectToPort(portInfo.portName())) {
                return true;
            }
        }

        // If no port worked, retry after 3 seconds
        reconnectTimer->start(3000);
        return false;
    }

    /**
     * Disconnect from Arduino
     */
    void disconnectArduino() {
        if (serialPort->isOpen()) {
            serialPort->close();
            isConnected = false;
        }
        reconnectTimer->stop();
    }

    /**
     * Check if connected to Arduino
     */
    bool isArduinoConnected() const {
        return isConnected;
    }

private slots:
    /**
     * Handle data received from Arduino
     */
    void onDataReceived() {
        QByteArray data = serialPort->readAll();
        receivedData += QString::fromStdString(data.toStdString());

        // Process complete commands (ended with newline)
        while (receivedData.contains('\n')) {
            int endIndex = receivedData.indexOf('\n');
            QString command = receivedData.left(endIndex).trimmed();
            receivedData = receivedData.mid(endIndex + 1);

            // Process the command
            if (command.startsWith("QUERY:")) {
                QString cin = command.mid(6);  // Extract CIN
                handleCINQuery(cin);
            }
        }
    }

    /**
     * Attempt to connect to Arduino
     */
    void attemptConnection() {
        if (!isConnected) {
            initializeConnection();
        }
    }

private:
    /**
     * Connect to a specific serial port
     */
    bool connectToPort(const QString &portName) {
        serialPort->setPortName(portName);
        serialPort->setBaudRate(BAUD_RATE);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if (serialPort->open(QIODevice::ReadWrite)) {
            isConnected = true;
            reconnectTimer->stop();
            qInfo() << "Connected to Arduino on port:" << portName;
            return true;
        }

        qWarning() << "Failed to open port:" << portName;
        return false;
    }

    /**
     * Handle CIN query from Arduino
     * Query the database for the employee
     */
    void handleCINQuery(const QString &cin) {
        QString response = queryEmployeeByCIN(cin);
        
        if (!response.isEmpty()) {
            sendToArduino(response);
        } else {
            sendToArduino("INVALID");
        }
    }

    /**
     * Query the smartvision database for employee by CIN
     * Returns "VALID:EmployeeName" or empty string if not found
     */
    QString queryEmployeeByCIN(const QString &cin) {
        QSqlQuery query;
        query.prepare("SELECT \"FULL_NAME\" FROM \"Employés\" "
                      "WHERE \"CIN\" = :cin AND \"ACTIVE\" = 'O'");
        query.addBindValue(cin);

        if (query.exec()) {
            if (query.next()) {
                QString fullName = query.value(0).toString();
                return QString("VALID:%1\n").arg(fullName);
            }
        } else {
            qWarning() << "Database query error:" << query.lastError().text();
        }

        return "";
    }

    /**
     * Send response to Arduino via Serial
     */
    void sendToArduino(const QString &message) {
        if (isConnected && serialPort->isOpen()) {
            QByteArray data = message.toUtf8();
            qint64 bytesWritten = serialPort->write(data);
            
            if (bytesWritten != data.size()) {
                qWarning() << "Failed to send complete message to Arduino";
            } else {
                qDebug() << "Sent to Arduino:" << message;
            }
        }
    }

public slots:
    /**
     * Public method to manually send test data to Arduino
     * Useful for debugging
     */
    void sendTestMessage(const QString &message) {
        sendToArduino(message);
    }
};

#endif // ARDUINOSERIALHANDLER_H
