#pragma once
#include <QMainWindow>

class BaseWindow : public QMainWindow {
  public:
    explicit BaseWindow(QWidget* parent = 0) : QMainWindow(parent) {
    }
    virtual ~BaseWindow() {
    }
};