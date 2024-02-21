#include <QFileDialog>
#include <QMessageBox>
#include "erSaveFixer.h"

ERSaveFixer::ERSaveFixer(QWidget* parent)
    : QMainWindow(parent), saveFile_("")
{
    ui.setupUi(this);
    ui.validity->setHidden(true);
    ui.fixButton->setHidden(true);
}

ERSaveFixer::~ERSaveFixer()
{}

void ERSaveFixer::onChooseFile() {
    QString filePath = QFileDialog::getOpenFileName(nullptr,
        "Open File",
        "",
        "All Files (*.sl2)");
    if (!filePath.isEmpty()) {
        ui.saveFileLineEdit->setText(filePath);
        saveFile_ = SaveFile(filePath.toStdString());
        saveFile_.listSlots();
        auto isHashValid = saveFile_.verifyHashes();

        ui.validity->setHidden(false);
        ui.validity->setText(isHashValid ? "Valid" : "Invalid");
        ui.validity->setStyleSheet(
            isHashValid ?
            "color: green" : "color: red"
        );

        ui.fixButton->setHidden(false);
    }
}

void ERSaveFixer::fixSaveHashes() {
    if (!ui.saveFileLineEdit->text().isEmpty()) {
        saveFile_.fixHashes();
        QMessageBox::information(nullptr,
            "Success",
            "Save Hashes repaired successfully.");

        auto isHashValid = saveFile_.verifyHashes();

        ui.validity->setHidden(false);
        ui.validity->setText(isHashValid ? "Valid" : "Invalid");
        ui.validity->setStyleSheet(
            isHashValid ?
            "color: green" : "color: red"
        );

        ui.fixButton->setHidden(false);
    }
}
