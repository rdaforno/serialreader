#include "readerthread.h"
#include <QSerialPort>


ReaderThread::ReaderThread(QObject* parent) :
	QThread(parent)
{
}

ReaderThread::~ReaderThread()
{
	stopReader();
}

bool ReaderThread::startReader(const QString& portName)
{
	_portName = portName;

	if (!isRunning() && portName.length())
	{
		_stop = false;
		start();
		return true;
	}
	return false;
}

void ReaderThread::stopReader()
{
	_stop = true;
	if (isRunning())
	{
		wait(_waitTimeout * 2);
	}
}

bool ReaderThread::setBaudRate(quint32 baudRate)
{
	if (!validateBaudRate(baudRate))
	{
		return false;
	}
	_baudRate = baudRate;
	return true;
}

void ReaderThread::run()
{
	QSerialPort serial;

	serial.setBaudRate(_baudRate);
	serial.setPortName(_portName);
	if (!serial.open(QIODevice::ReadOnly))
	{
		emit error(tr("Can't open %1, error code %2").arg(_portName).arg(serial.error()));
		return;
	}

	while (!_stop && serial.isOpen())
	{
		if (serial.waitForReadyRead(_waitTimeout))
		{
			QByteArray readData = serial.readAll();
			emit receive(readData);
		}
	}

	if (serial.isOpen())
	{
		serial.close();
	}
	else
	{
		emit error("serial port closed unexpectedly");
	}
}

bool ReaderThread::validateBaudRate(quint32 baudRate)
{
	const quint32 validBaudRates[] = { 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1000000 };

	for (const quint32 br : validBaudRates)
	{
		if (baudRate == br)
		{
			return true;
		}
	}
	return false;
}
