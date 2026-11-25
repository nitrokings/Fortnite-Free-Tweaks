#include "app/main_window.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "app/pages.h"

namespace vnm {
namespace {

struct ModuleEntry {
    const char* name;
    const char* planned;
};

// The order mirrors docs/ROADMAP.md. Only Windows Performance and Journal have
// real controls in release 0.1; the rest state plainly that they are not built.
const ModuleEntry kModules[] = {
    {"Windows Performance", ""},
    {"GPU", "Scheduled for release 0.2: per-app GPU preference, driver detection, iGPU warnings."},
    {"Network", "Scheduled for release 0.3: DNS with exact restore, ping/jitter measurement, MTU."},
    {"Input", "Scheduled for release 0.2: pointer precision, USB selective suspend, polling rate."},
    {"Games", "Scheduled for release 0.6: per-game profiles from games.json, sensitivity converter."},
    {"Game Booster", "Scheduled for release 0.4: background consumer audit, suspend, measured result."},
    {"Security Centre", "Scheduled for release 0.5: posture audit, autoruns, VirusTotal lookups."},
    {"Performance Monitor", "Scheduled for release 0.3: CPU/GPU/RAM plus PresentMon frame capture."},
    {"Cleanup", "Scheduled for release 0.9: age-gated temp cleaner with dry-run preview."},
    {"Presets", "Scheduled for release 0.9: reviewable bundles with a full change diff."},
    {"Aim Trainer", "Scheduled for release 0.7: Qt Quick 3D drills, sensitivity converter, progress."},
};

QLabel* label(const QString& text, const char* objectName) {
    auto* widget = new QLabel(text);
    widget->setObjectName(QString::fromLatin1(objectName));
    widget->setWordWrap(true);
    return widget;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("VNM TWEAKS ") + QStringLiteral(VNM_VERSION));
    resize(1180, 760);
    buildUi();
}

void MainWindow::buildUi() {
    auto* central = new QWidget();
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* sidebarPanel = new QWidget();
    sidebarPanel->setFixedWidth(240);
    sidebarPanel->setStyleSheet(QStringLiteral("background: #0b0f16;"));
    auto* sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(14, 20, 14, 18);
    sidebarLayout->setSpacing(4);

    sidebarLayout->addWidget(label(QStringLiteral("VNM TWEAKS"), "brand"));
    sidebarLayout->addWidget(label(QStringLiteral("Safe - reversible - measured"), "brandSub"));

    sidebar_ = new QListWidget();
    sidebar_->setObjectName(QStringLiteral("sidebar"));
    sidebar_->setFrameShape(QFrame::NoFrame);
    for (const auto& module : kModules) {
        sidebar_->addItem(QString::fromLatin1(module.name));
    }
    sidebar_->addItem(QStringLiteral("Journal"));
    sidebarLayout->addWidget(sidebar_, 1);

    sidebarLayout->addWidget(
        label(QStringLiteral("No cheats. No macros.\nNo game memory access."), "brandSub"));

    layout->addWidget(sidebarPanel);

    auto* content = new QWidget();
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 26, 30, 24);
    contentLayout->setSpacing(12);

    const auto moduleCount = static_cast<int>(sizeof(kModules) / sizeof(kModules[0]));
    contentLayout->addWidget(label(QString::fromLatin1(kModules[0].name), "pageTitle"));
    contentLayout->addWidget(label(QStringLiteral("Release 0.1 - one tweak, the whole pipeline."),
                                   "pageSubtitle"));

    pages_ = new QStackedWidget();

    gameModePage_ = new GameModePage();
    pages_->addWidget(gameModePage_);
    for (int i = 1; i < moduleCount; ++i) {
        pages_->addWidget(makePlaceholder(
            QString::fromLatin1(kModules[i].name),
            QString::fromLatin1(kModules[i].planned) +
                QStringLiteral(" Nothing here pretends to work, and this page has no Apply button.")));
    }
    journalPage_ = new JournalPage();
    pages_->addWidget(journalPage_);

    contentLayout->addWidget(pages_, 1);
    layout->addWidget(content, 1);

    setCentralWidget(central);

    connect(sidebar_, &QListWidget::currentRowChanged, this,
            [this](int row) { selectPage(row); });
    sidebar_->setCurrentRow(0);
}

void MainWindow::selectPage(int index) {
    if (index < 0) {
        return;
    }
    pages_->setCurrentIndex(index);
    if (index == 0) {
        gameModePage_->refresh();
    } else if (index == pages_->count() - 1) {
        journalPage_->refresh();
    }
}

}  // namespace vnm
