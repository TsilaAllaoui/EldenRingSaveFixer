#pragma once

#include <QMainWindow>
#include "ui_erSaveFixer.h"
#include <savefile.h>

class ERSaveFixer : public QMainWindow
{
    Q_OBJECT

public:
    ERSaveFixer(QWidget* parent = nullptr);
    ~ERSaveFixer() = default;

private:
    Ui::ERSaveFixerClass ui;
    SaveFile saveFile_;

public slots:
    void onChooseFile();
    void fixSaveHashes();
};
