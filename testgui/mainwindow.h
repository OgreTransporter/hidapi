#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QLineEdit>
#include <hidapi.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnect_clicked();
    void on_btnDisconnect_clicked();
    void on_btnReScanDevices_clicked();
    void on_btnSendOutputReport_clicked();
    void on_btnSendFeatureReport_clicked();
    void on_btnGetFeatureReport_clicked();
    void on_btnClear_clicked();

    void on_Timer();

private:
    size_t getDataFromTextField(QLineEdit *tf, char *buf, size_t len);
    int getLengthFromTextField(QLineEdit *tf);

private:
    Ui::MainWindow *ui;
    struct hid_device_info *devices;
    hid_device *connected_device;
    QMap<QString, struct hid_device_info*> device_map;
    QTimer* timer;
};
#endif // MAINWINDOW_H
