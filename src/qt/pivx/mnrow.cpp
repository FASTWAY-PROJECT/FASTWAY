// Copyright (c) 2019 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/pivx/mnrow.h"
#include "qt/pivx/forms/ui_mnrow.h"
#include "qt/pivx/qtutils.h"

MNRow::MNRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MNRow)
{
    ui->setupUi(this);
    setCssProperty(ui->labelName, "text-list-title1");
    setCssProperty(ui->labelAddress, "text-list-caption-medium");
    setCssProperty(ui->labelIPAddress, "text-list-body2");
    setCssProperty(ui->labelDate, "text-list-caption-medium");
    ui->lblDivisory->setStyleSheet("background-color:#bababa;");
}

void MNRow::updateView(QString ip_address, QString label, QString address, QString status, bool wasCollateralAccepted){
    ui->labelName->setText(label);
    ui->labelAddress->setText(address);
    ui->labelIPAddress->setText(ip_address);
    ui->labelDate->setText("Status: " + status);
    if (!wasCollateralAccepted){
        ui->labelDate->setText("Status: Collateral tx not found");
    } else {
        ui->labelDate->setText("Status: " + status);
    }
}

MNRow::~MNRow(){
    delete ui;
}
