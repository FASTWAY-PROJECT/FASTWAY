// Copyright (c) 2019 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/pivx/coincontrolpivwidget.h"
#include "qt/pivx/forms/ui_coincontrolpivwidget.h"

#include "guiutil.h"
#include <QAction>

CoinControlPivWidget::CoinControlPivWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinControlPivWidget)
{
    ui->setupUi(this);

    // Stylesheet
    this->setStyleSheet(parent->styleSheet());

    // Container

    ui->frameContainer->setProperty("cssClass", "container-dialog");
    ui->layoutAmount->setProperty("cssClass", "container-border-purple");
    ui->layoutAfter->setProperty("cssClass", "container-border-purple");
    ui->layoutBytes->setProperty("cssClass", "container-border-purple");
    ui->layoutChange->setProperty("cssClass", "container-border-purple");
    ui->layoutDust->setProperty("cssClass", "container-border-purple");
    ui->layoutFee->setProperty("cssClass", "container-border-purple");
    ui->layoutQuantity->setProperty("cssClass", "container-border-purple");

    // Title

    ui->labelTitle->setText("Select FAW Outputs to Spend");
    ui->labelTitle->setProperty("cssClass", "text-title-dialog");

    // Label Style

    ui->labelTitleAfter->setProperty("cssClass", "text-main-purple");
    ui->labelTitleAmount->setProperty("cssClass", "text-main-purple");
    ui->labelTitleAmount->setText("FAW");
    ui->labelTitleBytes->setProperty("cssClass", "text-main-purple");
    ui->labelTitleBytes->setProperty("cssClass", "text-main-purple");
    ui->labelTitleChange->setProperty("cssClass", "text-main-purple");
    ui->labelTitleType->setProperty("cssClass", "text-main-purple");
    ui->labelTitleConfirmations->setProperty("cssClass", "text-main-purple");
    ui->labelTitleDenom->setProperty("cssClass", "text-main-purple");
    ui->labelTitleDust->setProperty("cssClass", "text-main-purple");
    ui->labelTitleFee->setProperty("cssClass", "text-main-purple");
    ui->labelTitleId->setProperty("cssClass", "text-main-purple");
    ui->labelTitleQuantity->setProperty("cssClass", "text-main-purple");
    ui->labelTitleQuantity->setText("Quantity");
    ui->labelTitleSpen->setProperty("cssClass", "text-main-purple");
    ui->labelTitleVersion->setProperty("cssClass", "text-main-purple");

    ui->labelValueAfter->setProperty("cssClass", "text-main-purple");
    ui->labelValueAmount->setProperty("cssClass", "text-main-purple");
    ui->labelValueBytes->setProperty("cssClass", "text-main-purple");
    ui->labelValueDust->setProperty("cssClass", "text-main-purple");
    ui->labelValueChange->setProperty("cssClass", "text-main-purple");
    ui->labelValueFee->setProperty("cssClass", "text-main-purple");
    ui->labelValueQuantity->setProperty("cssClass", "text-main-purple");

    // Values
    ui->labelValueAfter->setText("0.00 FAW");
    ui->labelValueAmount->setText("0");
    ui->labelValueBytes->setText("0");
    ui->labelValueDust->setText("No");
    ui->labelValueChange->setText("0.00 FAW");
    ui->labelValueFee->setText("0.00 FAW");
    ui->labelValueQuantity->setText("0");

    QAction* copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction* copyFeeAction = new QAction(tr("Copy fee"), this);
    QAction* copyAfterFeeAction = new QAction(tr("Copy after fee"), this);

    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyCCAmount()));
    connect(copyFeeAction, SIGNAL(triggered()), this, SLOT(copyCCFee()));
    connect(copyAfterFeeAction, SIGNAL(triggered()), this, SLOT(copyCCAfterFee()));

    ui->labelValueAmount->addAction(copyAmountAction);
    ui->labelValueFee->addAction(copyFeeAction);
    ui->labelValueAfter->addAction(copyAfterFeeAction);

    // Buttons
    ui->btnEsc->setText("");
    ui->btnEsc->setProperty("cssClass", "ic-close");

    ui->btnCancel->setProperty("cssClass", "btn-dialog-cancel");
    ui->btnSave->setText("SAVE");
    ui->btnSave->setProperty("cssClass", "btn-primary");

    connect(ui->btnEsc, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));

}

CoinControlPivWidget::~CoinControlPivWidget()
{
    delete ui;
}

// copy label "Amount" to clipboard
void CoinControlPivWidget::copyCCAmount()
{
    GUIUtil::setClipboard(ui->labelValueAmount->text().left(ui->labelValueAmount->text().indexOf(" ")));
}

// copy label "Fee" to clipboard
void CoinControlPivWidget::copyCCFee()
{
    GUIUtil::setClipboard(ui->labelValueFee->text().left(ui->labelValueFee->text().indexOf(" ")).replace("~", ""));
}

// copy label "After fee" to clipboard
void CoinControlPivWidget::copyCCAfterFee()
{
    GUIUtil::setClipboard(ui->labelValueAfter->text().left(ui->labelValueAfter->text().indexOf(" ")).replace("~", ""));
}
