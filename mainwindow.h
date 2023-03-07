#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QAbstractNativeEventFilter>
#include <QTcpServer>
#include <QTcpSocket>
#ifdef Q_OS_LINUX
#include <QFileSystemWatcher>
#endif
#include "readerthread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public QAbstractNativeEventFilter
{
	Q_OBJECT

	enum class Connection
	{
		None,
		Serial,
		Socket
	};

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr*) override;

public slots:
	void acceptConnection();
	void connected();
	void disconnected();
	void receiveData();
	void serialErrorHandler(const QString& err);
	void socketErrorHandler(QAbstractSocket::SocketError err);

private slots:
	void on_portSelect_currentIndexChanged(int index);
	void on_connectButton_clicked();
	void on_clearButton_clicked();
	void on_saveButton_clicked();

private:
	Ui::MainWindow*	_ui;
	ReaderThread	_thread;
	QTimer			_timer;
	QTcpServer		_tcpServer;
	QTcpSocket*		_tcpServerConn = nullptr;
	QTcpSocket		_tcpClient;
	Connection      _conn = Connection::None;
	const quint16	_serverPort = 12345;
	const QString   _defaultHost = "localhost";
	const quint32   _defaultBaudRate = 115200;

#ifdef Q_OS_LINUX
	QFileSystemWatcher _watcher;
#endif

	void updatePortList();
	void updateUI();
	void connectToSerial(const QString& port);
	void disconnect();
	void appendToLog(const QString& s);
	void updateStatus(const QString& s);
	void clearStatus();
	void startServer();
	void stopServer();
	void connectToServer(const QString& ip);
	void sendData(const QString& s);
};
#endif // MAINWINDOW_H
