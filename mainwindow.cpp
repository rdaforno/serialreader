#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtSerialPort/qserialportinfo.h"
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dbt.h>
#endif

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow{parent},
	  _ui{new Ui::MainWindow}
{
	_ui->setupUi(this);
	_ui->saveButton->setEnabled(false);

	_timer.callOnTimeout(this, &MainWindow::clearStatus);
	_timer.setSingleShot(true);

	connect(&_thread, &ReaderThread::receive, this, &MainWindow::appendToLog);
	connect(&_thread, &ReaderThread::error, this, &MainWindow::serialErrorHandler);
	connect(&_tcpServer, &QTcpServer::newConnection, this, &MainWindow::acceptConnection);
	connect(&_tcpClient, &QAbstractSocket::connected, this, &MainWindow::connected);
	connect(&_tcpClient, &QIODevice::readyRead, this, &MainWindow::receiveData);
	connect(&_tcpClient, &QAbstractSocket::errorOccurred, this, &MainWindow::socketErrorHandler);

	updatePortList();

#ifdef Q_OS_WIN
	QApplication::instance()->installNativeEventFilter(this);
#elif defined(Q_OS_LINUX)
	connect(&_watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::updatePortList);
	_watcher.addPath("/dev/");
#endif
}

MainWindow::~MainWindow()
{
	delete _ui;
}

void MainWindow::on_portSelect_currentIndexChanged(int index)
{
	QVariant info = _ui->portSelect->itemData(index);
	_ui->deviceInfo->setText(info.toString());

	if (_ui->portSelect->itemText(index) == "TCP/IP")
	{
		_ui->paramLabel->setText("IP:");
		_ui->paramEdit->setText(_defaultHost);
	}
	else
	{
		_ui->paramLabel->setText("Baudrate:");
		_ui->paramEdit->setText(QString::number(_defaultBaudRate));
	}
}

void MainWindow::on_connectButton_clicked()
{
	if (_conn == Connection::None)
	{
		QString port = _ui->portSelect->currentText();
		if (port == "TCP/IP")
		{
			connectToServer(_ui->paramEdit->text());
		}
		else
		{
			connectToSerial(port);
		}
	}
	else
	{
		disconnect();
	}
}

void MainWindow::on_clearButton_clicked()
{
	_ui->serialLog->clear();
	_ui->saveButton->setEnabled(false);
}

void MainWindow::on_saveButton_clicked()
{
	QString filePath = QFileDialog::getSaveFileName(this,
													QString("Save serial log to file"),
													".",
													tr("Text file( *.txt)"));
	qDebug() << filePath;
	if (filePath.isEmpty())
	{
		return;
	}
	QFile file(filePath);
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream stream(&file);
		stream << _ui->serialLog->toPlainText();
		file.close();
	}
	else
	{
		QMessageBox::warning(this, "Error", "Could not open file for writing");
	}
}

void MainWindow::updatePortList()
{
	_ui->portSelect->clear();

	const auto infos = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo& info : infos)
	{
		QVariant deviceInfo = info.description() + ", " +
							  info.manufacturer() + " (" +
							  info.serialNumber() + ")";
		_ui->portSelect->addItem(info.portName(), deviceInfo);
	}

	_ui->portSelect->addItem("TCP/IP", "");
	_ui->portSelect->setCurrentIndex(-1);
	_ui->paramEdit->setText("");
}

void MainWindow::updateUI()
{
	if (_conn == Connection::None)
	{
		_ui->connectButton->setText("Connect");
		_ui->portSelect->setEnabled(true);
		_ui->paramEdit->setEnabled(true);
		updateStatus("disconnected");
	}
	else
	{
		_ui->portSelect->setEnabled(false);
		_ui->paramEdit->setEnabled(false);
		_ui->connectButton->setText("Disconnect");
	}
}

void MainWindow::connectToSerial(const QString& port)
{
	if (!_thread.setBaudRate(_ui->paramEdit->text().toULong()))
	{
		updateStatus("invalid baudrate");
		return;
	}
	if (!_thread.startReader(port))
	{
		updateStatus("could not connect to port " + port);
		return;
	}
	_conn = Connection::Serial;
	updateUI();
	updateStatus("connected to port " + port);

	startServer();
}

void MainWindow::disconnect()
{
	switch (_conn)
	{
	case Connection::Serial:
	{
		_thread.stopReader();
		_conn = Connection::None;	// must be before stopping the server
		stopServer();
		updateUI();
		break;
	}
	case Connection::Socket:
	{
		_tcpClient.disconnectFromHost();
		_conn = Connection::None;
		updateUI();
		break;
	}
	default:
		break;
	}
}

void MainWindow::serialErrorHandler(const QString& err)
{
	disconnect();
	updateStatus(err);
}

void MainWindow::appendToLog(const QString& s)
{
	// remark: append() does not work because it adds an additional newline
	_ui->serialLog->moveCursor(QTextCursor::End);
	_ui->serialLog->insertPlainText(s);
	_ui->serialLog->moveCursor(QTextCursor::End);

	if (!_ui->saveButton->isEnabled())
	{
		_ui->saveButton->setEnabled(true);
	}

	sendData(s);
}

void MainWindow::updateStatus(const QString& s)
{
	_ui->debugText->setText(s);
	_timer.start(4000);
}

void MainWindow::clearStatus()
{
	_ui->debugText->setText("");
}

bool MainWindow::nativeEventFilter(const QByteArray& eventType, void* message, qintptr*)
{
#ifdef Q_OS_WIN
	MSG* msg	  = static_cast<MSG*>(message);
	UINT type	  = msg->message;
	WPARAM wParam = msg->wParam;

	if (type == WM_DEVICECHANGE)
	{
		if (wParam == DBT_DEVICEARRIVAL)
		{
			updateStatus("device detected");
		}
		else if (wParam == DBT_DEVICEREMOVECOMPLETE)
		{
			updateStatus("device removed");
		}
		updatePortList();
	}
#endif
	return false;
}

void MainWindow::startServer()
{
	if (_tcpServer.isListening())
	{
		return;
	}
	if (!_tcpServer.listen(QHostAddress::AnyIPv4, _serverPort))
	{
		updateStatus("server could not be started");
	}
}

void MainWindow::stopServer()
{
	if (_tcpServer.isListening())
	{
		_tcpServer.close();
		updateStatus("server stopped");
	}
	if (_tcpServerConn)
	{
		_tcpServerConn->close();
		_tcpServerConn->deleteLater();
		_tcpServerConn = nullptr;
	}
}

void MainWindow::socketErrorHandler(QAbstractSocket::SocketError err)
{
	if (_conn == Connection::Socket)
	{
		disconnect();
	}
	updateStatus(QString("socket error %1").arg(err));
}

void MainWindow::sendData(const QString& s)
{
	if (_tcpServerConn)
	{
		_tcpServerConn->write(s.toUtf8());
	}
}

void MainWindow::acceptConnection()
{
	if (_tcpServerConn)
	{
		updateStatus("can't accept further connections");
		return;
	}
	_tcpServerConn = _tcpServer.nextPendingConnection();
	if (!_tcpServerConn)
	{
		updateStatus("invalid connection");
		return;
	}

	connect(_tcpServerConn, &QAbstractSocket::errorOccurred, this, &MainWindow::socketErrorHandler);
	connect(_tcpServerConn, &QTcpSocket::disconnected, this, &MainWindow::disconnected);

	updateStatus("client connected");
	_tcpServer.close();
}

void MainWindow::disconnected()
{
	if (_tcpServerConn)
	{
		 _tcpServerConn->deleteLater();
		 _tcpServerConn = nullptr;
	}
	updateStatus("client disconnected");

	if (_conn == Connection::Serial)
	{
		startServer();
	}
}

void MainWindow::connected()
{
	_conn = Connection::Socket;
	updateUI();
	updateStatus("connection established");
}

void MainWindow::connectToServer(const QString& ip)
{
	_tcpClient.connectToHost(ip, _serverPort,  QIODeviceBase::ReadOnly);
	updateStatus("connecting to " + ip + "...");
}

void MainWindow::receiveData()
{
	if (_tcpClient.bytesAvailable())
	{
		appendToLog(QString::fromUtf8(_tcpClient.readAll()));
	}
}
