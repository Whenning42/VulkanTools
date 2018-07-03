#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>

//#include "command_buffer.h"
#include "command_buffer_view.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

 //   void AddCommandBuffer(const VkVizCommandBuffer& buffer);

private:
    Ui::MainWindow *ui;
    CommandBufferView command_buffer_view_;
};

#endif // MAINWINDOW_H
