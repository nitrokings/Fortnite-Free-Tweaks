#include "app/pages.h"

#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUuid>
#include <QVBoxLayout>

#include <string>

#include "app/helper_client.h"
#include "core/gate.h"
#include "core/paths.h"
#include "core/plan.h"

namespace vnm {
namespace {

QLabel* makeLabel(const QString& text, const char* objectName) {
    auto* label = new QLabel(text);
    label->setObjectName(QString::fromLatin1(objectName));
    label->setWordWrap(true);
    return label;
}

QString riskPhrase() {
    return QString::fromUtf8(kRiskAckPhrase.data(), static_cast<int>(kRiskAckPhrase.size()));
}

}  // namespace

GameModePage::GameModePage(QWidget* parent)
    : QWidget(parent),
      journal_(current_journal_file()),
      tweak_(std::make_unique<GameModeTweak>(registry_, journal_)),
      sessionId_(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 20);
    cardLayout->setSpacing(10);

    cardLayout->addWidget(makeLabel(QStringLiteral("Game Mode"), "cardTitle"));
    cardLayout->addWidget(makeLabel(
        QStringLiteral(
            "Windows Game Mode gives the foreground game scheduling priority and reduces "
            "background interference. VNM TWEAKS writes AutoGameModeEnabled under "
            "HKEY_CURRENT_USER, records the previous value, and can put it back exactly."),
        "cardBody"));

    stateLabel_ = makeLabel(QStringLiteral("Checking..."), "stateValue");
    cardLayout->addWidget(stateLabel_);
    detailLabel_ = makeLabel(QString(), "cardBody");
    cardLayout->addWidget(detailLabel_);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    onButton_ = new QPushButton(QStringLiteral("Turn Game Mode on"));
    onButton_->setObjectName(QStringLiteral("primary"));
    offButton_ = new QPushButton(QStringLiteral("Turn Game Mode off"));
    undoButton_ = new QPushButton(QStringLiteral("Undo last change"));
    undoButton_->setEnabled(false);
    buttons->addWidget(onButton_);
    buttons->addWidget(offButton_);
    buttons->addWidget(undoButton_);
    buttons->addStretch(1);
    cardLayout->addLayout(buttons);

    gateLabel_ = makeLabel(
        QStringLiteral(
            "No change is made without a restore point. Every change is written to the journal "
            "first and can be undone on its own."),
        "gate");
    cardLayout->addWidget(gateLabel_);

    layout->addWidget(card);
    layout->addWidget(makeLabel(
        QStringLiteral(
            "Release 0.1 contains this one tweak on purpose. It exercises the full pipeline: "
            "detect, restore-point gate, apply, journal, undo. The remaining Windows Performance "
            "items arrive in release 0.2."),
        "notice"));
    layout->addStretch(1);

    connect(onButton_, &QPushButton::clicked, this, [this] { applyValue(true); });
    connect(offButton_, &QPushButton::clicked, this, [this] { applyValue(false); });
    connect(undoButton_, &QPushButton::clicked, this, [this] { undoLast(); });

    refresh();
}

void GameModePage::setStatus(const QString& text) { gateLabel_->setText(text); }

void GameModePage::refresh() {
    const DetectResult detection = tweak_->detect();
    switch (detection.state) {
        case TweakState::On:
            stateLabel_->setText(QStringLiteral("On"));
            break;
        case TweakState::Off:
            stateLabel_->setText(QStringLiteral("Off"));
            break;
        case TweakState::Unknown:
            stateLabel_->setText(QStringLiteral("Unknown"));
            break;
    }
    detailLabel_->setText(QString::fromStdString(detection.detail));
    onButton_->setEnabled(detection.state != TweakState::On);
    offButton_->setEnabled(detection.state != TweakState::Off);
    undoButton_->setEnabled(!lastJournalId_.isEmpty());
}

bool GameModePage::ensureRestorePoint(QString* restorePointOut) {
    const HelperResult attempt = run_helper_elevated({QStringLiteral("--ensure-restore-point")});
    if (attempt.ok) {
        setStatus(QStringLiteral("Restore point: ") + attempt.detail);
        if (restorePointOut != nullptr) {
            *restorePointOut = attempt.detail;
        }
        return true;
    }

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Restore point required"));
    box.setIcon(QMessageBox::Warning);
    box.setText(QStringLiteral("VNM TWEAKS will not change anything without a restore point."));
    box.setInformativeText(
        attempt.detail +
        QStringLiteral(
            "\n\nWindows allows one restore point per day, and System Protection is often "
            "switched off on prebuilt machines. Both can be resolved here."));
    QPushButton* retry = box.addButton(QStringLiteral("Try again"), QMessageBox::AcceptRole);
    QPushButton* enable =
        box.addButton(QStringLiteral("Enable System Protection and retry"), QMessageBox::ActionRole);
    QPushButton* skip = box.addButton(QStringLiteral("Continue without a restore point..."),
                                      QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == retry) {
        return ensureRestorePoint(restorePointOut);
    }
    if (box.clickedButton() == enable) {
        const HelperResult enabling = run_helper_elevated({QStringLiteral("--enable-protection")});
        if (!enabling.ok) {
            QMessageBox::warning(this, QStringLiteral("System Protection"), enabling.detail);
            return false;
        }
        return ensureRestorePoint(restorePointOut);
    }
    if (box.clickedButton() == skip) {
        bool accepted = false;
        const QString typed = QInputDialog::getText(
            this, QStringLiteral("Risk acknowledgement"),
            QStringLiteral("Type %1 to continue without a restore point. This is recorded in the "
                           "journal.")
                .arg(riskPhrase()),
            QLineEdit::Normal, QString(), &accepted);
        if (!accepted) {
            return false;
        }
        if (!verify_ack(typed.toStdString())) {
            QMessageBox::warning(this, QStringLiteral("Not acknowledged"),
                                 QStringLiteral("The phrase did not match, so nothing was changed."));
            return false;
        }
        const HelperResult logged = run_helper({QStringLiteral("--log-ack"), typed});
        if (!logged.ok) {
            QMessageBox::warning(this, QStringLiteral("Acknowledgement"),
                                 logged.detail.isEmpty() ? logged.raw : logged.detail);
            return false;
        }
        setStatus(QStringLiteral("No restore point: the risk acknowledgement was recorded."));
        if (restorePointOut != nullptr) {
            *restorePointOut = QStringLiteral("none (acknowledged)");
        }
        return true;
    }
    return false;
}

void GameModePage::applyValue(bool enable) {
    onButton_->setEnabled(false);
    offButton_->setEnabled(false);

    QString restorePoint;
    if (!ensureRestorePoint(&restorePoint)) {
        refresh();
        return;
    }

    ChangePlan plan = tweak_->preview(enable);
    plan.session = sessionId_.toStdString();
    plan.restorePoint = restorePoint.toStdString();

    const std::string serialized = serialize_plan(plan);
    const QString planPath =
        QDir(QDir::tempPath())
            .filePath(QStringLiteral("vnm-plan-") + QUuid::createUuid().toString(QUuid::Id128) +
                      QStringLiteral(".json"));

    QFile file(planPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Plan file"),
                             QStringLiteral("Could not write the plan file."));
        refresh();
        return;
    }
    file.write(serialized.c_str(), static_cast<qint64>(serialized.size()));
    file.close();

    const HelperResult result = run_helper({QStringLiteral("--apply"), planPath});
    QFile::remove(planPath);

    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("Change refused"),
                             result.detail.isEmpty() ? result.raw : result.detail);
        refresh();
        return;
    }

    lastJournalId_ = result.journalId;
    setStatus(QStringLiteral("Applied and journaled. Entry: ") + result.journalId);
    refresh();
}

void GameModePage::undoLast() {
    if (lastJournalId_.isEmpty()) {
        return;
    }
    const HelperResult result = run_helper({QStringLiteral("--undo"), lastJournalId_});
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("Undo failed"),
                             result.detail.isEmpty() ? result.raw : result.detail);
        return;
    }
    setStatus(QStringLiteral("Undone. The journal keeps both entries."));
    lastJournalId_ = result.journalId;
    refresh();
}

JournalPage::JournalPage(QWidget* parent)
    : QWidget(parent), journal_(current_journal_file()) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    summary_ = makeLabel(QStringLiteral("Journal"), "cardBody");
    layout->addWidget(summary_);

    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({QStringLiteral("Time"), QStringLiteral("Item"),
                                      QStringLiteral("Before"), QStringLiteral("After"),
                                      QStringLiteral("Journal id")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table_, 1);

    auto* buttons = new QHBoxLayout();
    undoButton_ = new QPushButton(QStringLiteral("Undo selected change"));
    buttons->addWidget(undoButton_);
    buttons->addStretch(1);
    layout->addLayout(buttons);

    connect(undoButton_, &QPushButton::clicked, this, [this] { undoSelected(); });

    refresh();
}

void JournalPage::refresh() {
    std::string error;
    const auto entries = journal_.load(&error);
    table_->setRowCount(0);
    table_->setRowCount(static_cast<int>(entries.size()));

    int row = 0;
    for (auto it = entries.rbegin(); it != entries.rend(); ++it, ++row) {
        table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(it->timestamp)));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(it->title)));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(it->before)));
        table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(it->after)));
        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(it->id)));
    }
    table_->resizeColumnsToContents();

    QString text = QStringLiteral("Journal file: ") + QString::fromStdString(journal_.path().string());
    if (!error.empty()) {
        text += QStringLiteral("  (problem: ") + QString::fromStdString(error) + QStringLiteral(")");
    }
    summary_->setText(text + QStringLiteral("  -  ") + QString::number(entries.size()) +
                      QStringLiteral(" entries"));
    undoButton_->setEnabled(!entries.empty());
}

void JournalPage::undoSelected() {
    const int row = table_->currentRow();
    if (row < 0) {
        return;
    }
    QTableWidgetItem* idItem = table_->item(row, 4);
    if (idItem == nullptr) {
        return;
    }
    const HelperResult result = run_helper({QStringLiteral("--undo"), idItem->text()});
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("Undo failed"),
                             result.detail.isEmpty() ? result.raw : result.detail);
        return;
    }
    refresh();
}

QWidget* makePlaceholder(const QString& title, const QString& text) {
    Q_UNUSED(title);
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 20);
    cardLayout->addWidget(makeLabel(QStringLiteral("Not built yet"), "cardTitle"));
    cardLayout->addWidget(makeLabel(text, "cardBody"));
    layout->addWidget(card);
    layout->addStretch(1);
    return page;
}

}  // namespace vnm
