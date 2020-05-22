#include "mainwindow.h"
#include <QTimer>
#include <QMessageBox>
#include "./ui_mainwindow.h"

class DeviceListItem : QListWidgetItem
{
public:

};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, devices(NULL)
	, connected_device(NULL)
	, timer(NULL)
{
	ui->setupUi(this);
	ui->statusbar->showMessage("Disconnected");
	on_btnReScanDevices_clicked();
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::on_Timer));
}

MainWindow::~MainWindow()
{
	if (connected_device)
		on_btnDisconnect_clicked();
	hid_exit();
	delete ui;
}


void MainWindow::on_btnConnect_clicked()
{
	if (connected_device != NULL) return;
	if (ui->lbDevices->currentItem() == NULL) return;
	QString cur_item = ui->lbDevices->currentItem()->text();
	if (cur_item.isEmpty()) return;
	if (!device_map.contains(cur_item)) return;
	struct hid_device_info *device_info = device_map[cur_item];
	connected_device = hid_open_path(device_info->path);
	if (!connected_device) {
		QMessageBox::critical(this, "Device Error", "Unable to connect to device");
		return;
	}

	hid_set_nonblocking(connected_device, 1);
	timer->start(5);

	ui->statusbar->showMessage("Connected to: " + cur_item);
	ui->btnSendOutputReport->setEnabled(true);
	ui->btnSendFeatureReport->setFlat(true);
	ui->btnGetFeatureReport->setEnabled(true);
	ui->btnConnect->setEnabled(false);
	ui->btnDisconnect->setEnabled(true);
	on_btnClear_clicked();
}

void MainWindow::on_btnDisconnect_clicked()
{
	timer->stop();
	hid_close(connected_device);
	connected_device = NULL;
	ui->statusbar->showMessage("Disconnected");
	ui->btnSendOutputReport->setEnabled(false);
	ui->btnSendFeatureReport->setFlat(false);
	ui->btnGetFeatureReport->setEnabled(false);
	ui->btnConnect->setEnabled(true);
	ui->btnDisconnect->setEnabled(false);
}

void MainWindow::on_btnReScanDevices_clicked()
{
	ui->lbDevices->clear();
	device_map.clear();
	struct hid_device_info *cur_dev;
	hid_free_enumeration(devices);
	devices = hid_enumerate(0x0, 0x0);
	cur_dev = devices;
	while (cur_dev)
	{
		QString s = QString("%1").arg(cur_dev->vendor_id, 4, 16, QChar('0')) + ":" + QString("%1").arg(cur_dev->product_id, 4, 16, QChar('0'));
		if (cur_dev->manufacturer_string != NULL) s.append(" " + QString::fromWCharArray(cur_dev->manufacturer_string));
		if (cur_dev->product_string != NULL) s.append(" " + QString::fromWCharArray(cur_dev->product_string));
		s.append(" (usage: " + QString("%1").arg(cur_dev->usage_page, 4, 16, QChar('0')) + ":" + QString("%1").arg(cur_dev->usage, 4, 16, QChar('0')) + ")");
		ui->lbDevices->addItem(s);
		device_map.insert(s, cur_dev);
		cur_dev = cur_dev->next;
	}
	if (ui->lbDevices->count() > 0) ui->lbDevices->item(0)->setSelected(true);
}

void MainWindow::on_btnSendOutputReport_clicked()
{
	char buf[256];
	size_t data_len, len;
	int textfield_len;

	memset(buf, 0x0, sizeof(buf));
	textfield_len = getLengthFromTextField(ui->tbLengthOutput);
	data_len = getDataFromTextField(ui->tbDataOutput, buf, sizeof(buf));

	if (textfield_len < 0) {
		QMessageBox::critical(this, "Invalid length", "Length field is invalid. Please enter a number in hex, octal, or decimal.");
		return;
	}

	if (textfield_len > sizeof(buf)) {
		QMessageBox::critical(this, "Invalid length", "Length field is too long.");
		return;
	}

	len = (textfield_len) ? textfield_len : data_len;

	int res = hid_write(connected_device, (const unsigned char*)buf, len);
	if (res < 0) {
		QMessageBox::critical(this, "Error Writing", "Could not write to device. Error reported was: " + QString::fromWCharArray(hid_error(connected_device)));
	}
}

void MainWindow::on_btnSendFeatureReport_clicked()
{
	char buf[256];
	size_t data_len, len;
	int textfield_len;

	memset(buf, 0x0, sizeof(buf));
	textfield_len = getLengthFromTextField(ui->tbLengthFeatureSend);
	data_len = getDataFromTextField(ui->tbDataFeatureSend, buf, sizeof(buf));

	if (textfield_len < 0) {
		QMessageBox::critical(this, "Invalid length", "Length field is invalid. Please enter a number in hex, octal, or decimal.");
		return;
	}

	if (textfield_len > sizeof(buf)) {
		QMessageBox::critical(this, "Invalid length", "Length field is too long.");
		return;
	}

	len = (textfield_len) ? textfield_len : data_len;

	int res = hid_send_feature_report(connected_device, (const unsigned char*)buf, len);
	if (res < 0) {
		QMessageBox::critical(this, "Error Writing", "Could not send feature report to device. Error reported was: " + QString::fromWCharArray(hid_error(connected_device)));
	}
}

void MainWindow::on_btnGetFeatureReport_clicked()
{
	char buf[256];
	size_t len;

	memset(buf, 0x0, sizeof(buf));
	len = getDataFromTextField(ui->tbDataFeatureGet, buf, sizeof(buf));

	if (len != 1) {
		QMessageBox::critical(this, "Too many numbers", "Enter only a single report number in the text field");
	}

	int res = hid_get_feature_report(connected_device, (unsigned char*)buf, sizeof(buf));
	if (res < 0) {
		QMessageBox::critical(this, "Error Getting Report", "Could not get feature report from device. Error reported was: " + QString::fromWCharArray(hid_error(connected_device)));
	}

	if (res > 0) {
		QString s("Returned Feature Report. %1 bytes:\n");
		s.arg(res);
		for (int i = 0; i < res; i++)
		{
			s += QString("%1").arg((int)buf[i], 2, 16, QChar('0'));
			if ((i + 1) % 4 == 0)
				s += " ";
			if ((i + 1) % 16 == 0)
				s += "\n";
		}
		s += "\n";
		ui->tbInput->appendPlainText(s);
		ui->tbInput->moveCursor(QTextCursor::End);
	}
}

void MainWindow::on_btnClear_clicked()
{
	ui->tbInput->clear();
}

void MainWindow::on_Timer()
{
	unsigned char buf[256];
	int res = hid_read(connected_device, buf, sizeof(buf));

	if (res > 0) {
		QString s("Received %1 bytes:\n");
		s.arg(res);
		for (int i = 0; i < res; i++) {
			s += QString("%1").arg((int)buf[i], 2, 16, QChar('0'));
			if ((i + 1) % 4 == 0)
				s += " ";
			if ((i + 1) % 16 == 0)
				s += "\n";
		}
		s += "\n";
		ui->tbInput->appendPlainText(s);
		ui->tbInput->moveCursor(QTextCursor::End);
	}
	if (res < 0) {
		ui->tbInput->appendPlainText("hid_read() returned error\n");
		ui->tbInput->moveCursor(QTextCursor::End);
	}
}

size_t MainWindow::getDataFromTextField(QLineEdit *tf, char *buf, size_t len)
{
	const char *delim = " ,{}\t\r\n";
	QString data = tf->text();
	const char *d = data.toStdString().c_str();
	size_t i = 0;

	// Copy the string from the GUI.
	size_t sz = strlen(d);
	char *str = new char[sz + 1];
	memcpy(str, d, sz);
	str[sz] = 0;

	// For each token in the string, parse and store in buf[].
	char *token = strtok(str, delim);
	while (token)
	{
		char *endptr;
		long int val = strtol(token, &endptr, 0);
		buf[i++] = val;
		token = strtok(NULL, delim);
	}

	delete str;
	return i;
}

/* getLengthFromTextField()
   Returns length:
	 0: empty text field
	>0: valid length
	-1: invalid length */
int MainWindow::getLengthFromTextField(QLineEdit *tf)
{
	long len = 0;
	QString str = tf->text();
	size_t sz = str.length();

	if (sz > 0) {
		len = str.toLong();
		if (len <= 0) {
			QMessageBox::warning(this, "Invalid length", "Enter a length greater than zero.");
			return -1;
		}
		return len;
		return -1;
	}
	return 0;
}
