#include "app/theme.h"

namespace vnm::theme {

QString stylesheet() {
    return QStringLiteral(R"(
        QWidget { background: #12161f; color: #e6ebf5; font-size: 13px; }
        QListWidget#sidebar {
            background: #0b0f16; border: none; outline: 0; padding: 8px 6px;
        }
        QListWidget#sidebar::item {
            padding: 10px 12px; border-radius: 8px; color: #b7c2d6;
        }
        QListWidget#sidebar::item:selected {
            background: #1d2836; color: #ffffff;
        }
        QListWidget#sidebar::item:hover { background: #16202c; }
        QLabel#brand { font-size: 17px; font-weight: 600; color: #ffffff; }
        QLabel#brandSub { color: #6f8bb5; font-size: 11px; }
        QLabel#pageTitle { font-size: 24px; font-weight: 600; }
        QLabel#pageSubtitle { color: #8b98ad; }
        QFrame#card {
            background: #182030; border: 1px solid #243146; border-radius: 12px;
        }
        QLabel#cardTitle { font-size: 15px; font-weight: 600; }
        QLabel#cardBody { color: #97a3b8; }
        QLabel#stateValue { font-size: 22px; font-weight: 600; color: #ffffff; }
        QPushButton {
            background: #22304a; border: 1px solid #2f4366; border-radius: 8px;
            padding: 9px 16px; color: #e6ebf5;
        }
        QPushButton:hover { background: #2a3b59; }
        QPushButton:disabled { background: #1a2231; color: #5b6779; border-color: #222c3d; }
        QPushButton#primary { background: #2f6df6; border-color: #2f6df6; color: #ffffff; }
        QPushButton#primary:hover { background: #3d7bff; }
        QPushButton#primary:disabled { background: #24365e; border-color: #24365e; color: #7f8ea8; }
        QTableWidget {
            background: #151c29; border: 1px solid #243146; border-radius: 10px;
            gridline-color: #243146;
        }
        QHeaderView::section {
            background: #1b2434; color: #9fb0c9; border: none; padding: 8px;
        }
        QLabel#notice { color: #8b98ad; font-size: 12px; }
        QLabel#gate { color: #8b98ad; font-size: 12px; }
    )");
}

}  // namespace vnm::theme
