#include <QApplication>

#include "app/main_window.h"
#include "app/theme.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("VNM TWEAKS"));
    app.setApplicationVersion(QStringLiteral(VNM_VERSION));
    app.setOrganizationName(QStringLiteral("VNM"));
    app.setStyleSheet(vnm::theme::stylesheet());

    vnm::MainWindow window;
    window.show();
    return app.exec();
}
