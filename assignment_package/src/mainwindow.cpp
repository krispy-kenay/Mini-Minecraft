#include "mainwindow.h"
#include <QScreen>
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"
#include "utils.h"
#include <QResizeEvent>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), cHelp()
{
    ui->setupUi(this);
    ui->mygl->hide();
    ui->mygl->setFocus();
    this->playerInfoWindow.show();
    playerInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width() * 0.75, 0));

    connect(ui->mygl, SIGNAL(sig_sendPlayerPos(QString)), &playerInfoWindow, SLOT(slot_setPosText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerVel(QString)), &playerInfoWindow, SLOT(slot_setVelText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerAcc(QString)), &playerInfoWindow, SLOT(slot_setAccText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerLook(QString)), &playerInfoWindow, SLOT(slot_setLookText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerChunk(QString)), &playerInfoWindow, SLOT(slot_setChunkText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerTerrainZone(QString)), &playerInfoWindow, SLOT(slot_setZoneText(QString)));

    // Main Menu Buttons
    connect(ui->Exit, &QPushButton::clicked, this, &QApplication::exit);
    connect(ui->CreateWorld, &QPushButton::clicked, this, &MainWindow::createNewSave);
    connect(ui->LoadWorld, &QPushButton::clicked, this, &MainWindow::loadExistingSave);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createNewSave() {
    QString folder = QFileDialog::getSaveFileName(this, "Select World Folder", getCurrentPath(), "Directory (*.dir)");
    if (!folder.isEmpty()) {
        std::string worldFolder = folder.toStdString();
        ui->stackedWidget->setCurrentIndex(1);
        ui->mygl->setFolderLocation(worldFolder);
        ui->mygl->show();
        ui->mygl->startGame();
    }
}

void MainWindow::loadExistingSave() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select World Folder");
    if (!folder.isEmpty()) {
        std::string worldFolder = folder.toStdString();
        ui->stackedWidget->setCurrentIndex(1);
        ui->mygl->setFolderLocation(worldFolder);
        ui->mygl->show();
        ui->mygl->startGame();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionSave_triggered() {
    // Make sure that all threads have finished processing before saving
    // (otherwise we may save zones that are still being generated)
    QThreadPool::globalInstance()->waitForDone();
    ui->mygl->stopGame();
    ui->mygl->triggerSave();
    // Wait for the save to complete before starting the game loop again
    // (otherwise we may modify a world that is being saved)
    QThreadPool::globalInstance()->waitForDone();
    ui->mygl->startGame();
}

void MainWindow::on_actionCamera_Controls_triggered()
{
    cHelp.show();
}
