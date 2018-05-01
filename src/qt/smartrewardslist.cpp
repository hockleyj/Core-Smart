#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif
 
#include "smartrewardslist.h"
#include "ui_smartrewardslist.h"

#include "smartrewards/rewards.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "txmempool.h"
#include "walletmodel.h"
#include "coincontrol.h"
//#include "init.h"
//#include "validation.h" // For minRelayTxFee
#include "wallet/wallet.h"
 
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
 
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QTableWidget>
#include <QTime>

typedef PAIRTYPE(QString,CSmartRewardEntry) rewardEntry_p;

SmartrewardsList::SmartrewardsList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SmartrewardsList),
    model(0)
{
    ui->setupUi(this);
 
   //QMessageBox::information(this,"Hello","test");

   QTableWidget *smartRewardsTable = ui->tableWidget;
   //QTableWidget *smartRewardsTable = new QTableWidget(this);

   smartRewardsTable->setAlternatingRowColors(true);
   smartRewardsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
   smartRewardsTable->setSelectionMode(QAbstractItemView::SingleSelection);
   smartRewardsTable->setSortingEnabled(true);
   smartRewardsTable->setColumnCount(4);
   smartRewardsTable->setShowGrid(false);
   smartRewardsTable->verticalHeader()->hide();

   smartRewardsTable->setColumnWidth(0, 200);
   smartRewardsTable->setColumnWidth(1, 250);
   smartRewardsTable->setColumnWidth(2, 160);
   smartRewardsTable->setColumnWidth(3, 200);

   // Actions
   smartRewardsTable->setContextMenuPolicy(Qt::CustomContextMenu);

   QAction *copyAddressAction = new QAction(tr("Copy address"), this);
   QAction *copyLabelAction = new QAction(tr("Copy label"), this);
   QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
   QAction *copyEligibleAmountAction = new QAction(tr("Copy eligible amount"), this);

   contextMenu = new QMenu(this);
   contextMenu->addAction(copyLabelAction);
   contextMenu->addAction(copyAddressAction);
   contextMenu->addAction(copyAmountAction);
   contextMenu->addAction(copyEligibleAmountAction);

   // Connect actions
   connect(smartRewardsTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
   connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
   connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
   connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
   connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyEligibleAmount()));

}
 
SmartrewardsList::~SmartrewardsList()
{
    delete ui;
}
 
 void SmartrewardsList::setModel(WalletModel *model)
{
    this->model = model;
    if(!model) {
       return;
    }

    ui->stackedWidget->setCurrentIndex(1);

    int nDisplayUnit = model->getOptionsModel()->getDisplayUnit();
 
    std::map<QString, std::vector<COutput> > mapCoins;
    model->listCoins(mapCoins);

//    //Smartrewards snapshot date
//    QDateTime lastSmartrewardsSnapshotDateTimeUtc = QDateTime::currentDateTimeUtc();
//    int currentDay = lastSmartrewardsSnapshotDateTimeUtc.toString("dd").toInt();
//    if(currentDay < SMARTREWARDS_DAY){
//       lastSmartrewardsSnapshotDateTimeUtc = lastSmartrewardsSnapshotDateTimeUtc.addMonths(-1);
//    }
//    int snapshotMonth = lastSmartrewardsSnapshotDateTimeUtc.toString("MM").toInt();
//    int snapshotYear = lastSmartrewardsSnapshotDateTimeUtc.toString("yyyy").toInt();
//    lastSmartrewardsSnapshotDateTimeUtc = QDateTime(QDate(snapshotYear, snapshotMonth, SMARTREWARDS_DAY), QTime(SMARTREWARDS_UTC_HOUR, 0), Qt::UTC);

    std::map<const CScript, rewardEntry_p> rewardList;

    BOOST_FOREACH(const PAIRTYPE(QString, std::vector<COutput>)& coins, mapCoins) {

        QString sWalletAddress = coins.first;
 
        bool added;

        BOOST_FOREACH(const COutput& out, coins.second) {

            CScript pub = out.tx->vout[out.i].scriptPubKey;

            if( rewardList.find(pub) == rewardList.end() ){
                CSmartRewardEntry entry;
                prewards->GetRewardEntry(pub, entry, added);
                rewardEntry_p pairEntry = make_pair(sWalletAddress, entry);
                rewardList.insert(make_pair(pub,pairEntry));
            }
        }
    }

    int nRow = 0;

    BOOST_FOREACH(const PAIRTYPE(const CScript, rewardEntry_p)& reward, rewardList) {

            ui->tableWidget->insertRow(nRow);
            QString sWalletLabel = model->getAddressTableModel()->labelForAddress(reward.second.first);
            if (sWalletLabel.isEmpty())
                sWalletLabel = tr("(no label)");

            ui->tableWidget->setItem(nRow, 0, new QTableWidgetItem(sWalletLabel));
            ui->tableWidget->setItem(nRow, 1, new QTableWidgetItem(reward.second.first));
            ui->tableWidget->setItem(nRow, 2, new QTableWidgetItem(BitcoinUnits::format(nDisplayUnit, reward.second.second.balance)));

            if( reward.second.second.eligible ){
                 ui->tableWidget->setItem(nRow, 3, new QTableWidgetItem(BitcoinUnits::format(nDisplayUnit, reward.second.second.balanceOnStart)));
            }else{
                ui->tableWidget->setItem(nRow, 3, new QTableWidgetItem(BitcoinUnits::format(nDisplayUnit, 0)));
            }

            nRow++;

        }

}
 
 void SmartrewardsList::contextualMenu(const QPoint &point)
 {
     QModelIndex index =  ui->tableWidget->indexAt(point);
     QModelIndexList selection =  ui->tableWidget->selectionModel()->selectedRows(0);
     if (selection.empty())
         return;

     if(index.isValid())
     {
         contextMenu->exec(QCursor::pos());
     }
 }

 void SmartrewardsList::copyLabel()
 {
     GUIUtil::copyEntryData(ui->tableWidget, 0);
 }


 void SmartrewardsList::copyAddress()
 {
     GUIUtil::copyEntryData(ui->tableWidget, 1);
 }


 void SmartrewardsList::copyAmount()
 {
     GUIUtil::copyEntryData(ui->tableWidget, 2);
 }


 void SmartrewardsList::copyEligibleAmount()
 {
     GUIUtil::copyEntryData(ui->tableWidget, 3);
 }
