#include <QFileDialog>
#include <QMessageBox>
#include <filesystem>
#include <Windows.h>
#include <ShlObj.h>
#include "erSaveFixer.h"

ERSaveFixer::ERSaveFixer(QWidget* parent)
    : QMainWindow(parent), saveFile_("")
{
    ui.setupUi(this);
    ui.validity->setHidden(true);
    ui.fixButton->setHidden(true);
}

void ERSaveFixer::onChooseFile() {

    // Getting Elden Ring folder in Roaming
    std::wstring roamingPathW;
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        roamingPathW = std::wstring(path);
    }

    std::string roamingPath(roamingPathW.begin(), roamingPathW.end());
    roamingPath += "\\EldenRing\\";
    if (std::filesystem::exists(roamingPath)) {
        std::string steamIdFolder = "";
        for (auto& entry : std::filesystem::directory_iterator(roamingPath)) {
            if (std::filesystem::is_directory(entry)) {
                steamIdFolder = entry.path().string();
                break;
            }
        }
        if (!steamIdFolder.empty()) {
            roamingPath = steamIdFolder;
        }
        else {
            roamingPath = "";
        }
    }


    QString filePath = QFileDialog::getOpenFileName(nullptr,
        "Open File",
        QString(roamingPath.c_str()),
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
