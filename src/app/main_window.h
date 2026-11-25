#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;

namespace vnm {

class GameModePage;
class JournalPage;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void selectPage(int index);

    QListWidget* sidebar_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    GameModePage* gameModePage_ = nullptr;
    JournalPage* journalPage_ = nullptr;
};

}  // namespace vnm
