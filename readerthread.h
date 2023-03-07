#ifndef READERTHREAD_H
#define READERTHREAD_H

#include <QThread>


class ReaderThread : public QThread
{
	Q_OBJECT

public:
	explicit ReaderThread(QObject* parent = nullptr);
	~ReaderThread();

	bool startReader(const QString& portName);
	void stopReader();
	bool setBaudRate(quint32 baudRate);
	static bool validateBaudRate(quint32 baudRate);

signals:
	void error(const QString& s);
	void receive(const QString& s);

private:
	QString	_portName;
	bool	_stop = true;
	quint32	_baudRate = 115200;
	quint32	_waitTimeout = 500;

	void run() override;
};

#endif // READERTHREAD_H
