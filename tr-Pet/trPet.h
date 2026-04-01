#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_trPet.h"

class trPet : public QMainWindow
{
    Q_OBJECT

public:
    trPet(QWidget *parent = nullptr);
    ~trPet();

private:
    Ui::trPetClass ui;
};

