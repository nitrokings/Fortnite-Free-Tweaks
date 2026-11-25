#pragma once

#include <QString>
#include <QWidget>

#include <memory>

#include "core/gamemode.h"
#include "core/journal.h"
#include "platform/win/registry_win.h"

class QLabel;
class QPushButton;
class QTableWidget;

namespace vnm {

// Windows Performance, release 0.1: one real, journaled, undoable tweak.
class GameModePage : public QWidget {
public:
    explicit GameModePage(QWidget* parent = nullptr);
    void refresh();

private:
    void applyValue(bool enable);
    void undoLast();
    bool ensureRestorePoint(QString* restorePointOut);
    void setStatus(const QString& text);

    WinRegistry registry_;
    Journal journal_;
    std::unique_ptr<GameModeTweak> tweak_;

    QLabel* stateLabel_ = nullptr;
    QLabel* detailLabel_ = nullptr;
    QLabel* gateLabel_ = nullptr;
    QPushButton* onButton_ = nullptr;
    QPushButton* offButton_ = nullptr;
    QPushButton* undoButton_ = nullptr;

    QString sessionId_;
    QString lastJournalId_;
};

// Read-only view of the journal, with undo for a selected entry.
class JournalPage : public QWidget {
public:
    explicit JournalPage(QWidget* parent = nullptr);
    void refresh();

private:
    void undoSelected();

    Journal journal_;
    QTableWidget* table_ = nullptr;
    QLabel* summary_ = nullptr;
    QPushButton* undoButton_ = nullptr;
};

// A module that exists in the plan but has no controls yet. It deliberately has
// no Apply button.
QWidget* makePlaceholder(const QString& title, const QString& text);

}  // namespace vnm
