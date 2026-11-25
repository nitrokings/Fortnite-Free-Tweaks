#pragma once

#include <QString>
#include <QStringList>

namespace vnm {

struct HelperResult {
    bool ok = false;
    QString status;      // restore-point status, when the command reports one
    QString detail;      // human readable message from the helper
    QString journalId;
    QString raw;         // raw stdout, for the diagnostics view
    int exitCode = -1;
    bool started = false;
};

QString helper_path();

// Runs VNMHelper without elevation. Used for apply / undo / log-ack.
HelperResult run_helper(const QStringList& arguments, int timeoutMs = 120000);

// Runs VNMHelper through the UAC prompt. Used for restore points and for
// enabling System Protection. The helper writes its JSON result to a temporary
// file because an elevated child's stdout cannot be captured.
HelperResult run_helper_elevated(const QStringList& arguments, int timeoutMs = 300000);

}  // namespace vnm
