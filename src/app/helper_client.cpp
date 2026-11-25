#include "app/helper_client.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace vnm {
namespace {

HelperResult parse_output(const QByteArray& output, int exitCode) {
    HelperResult result;
    result.exitCode = exitCode;
    result.raw = QString::fromUtf8(output).trimmed();

    const QJsonDocument document = QJsonDocument::fromJson(output);
    if (!document.isObject()) {
        result.detail = QStringLiteral("VNMHelper returned something unexpected.");
        if (!result.raw.isEmpty()) {
            result.detail += QStringLiteral(" Output: ") + result.raw;
        }
        return result;
    }

    const QJsonObject object = document.object();
    result.ok = object.value(QStringLiteral("ok")).toString() == QLatin1String("1");
    result.status = object.value(QStringLiteral("status")).toString();
    result.detail = object.value(QStringLiteral("detail")).toString();
    result.journalId = object.value(QStringLiteral("journalId")).toString();
    if (!result.ok) {
        const QString error = object.value(QStringLiteral("error")).toString();
        if (!error.isEmpty()) {
            result.detail = error;
        }
    }
    return result;
}

QString quoted(const QString& value) { return QStringLiteral("\"") + value + QStringLiteral("\""); }

}  // namespace

QString helper_path() {
    const QString directory = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    return QDir(directory).filePath(QStringLiteral("VNMHelper.exe"));
#else
    return QDir(directory).filePath(QStringLiteral("VNMHelper"));
#endif
}

HelperResult run_helper(const QStringList& arguments, int timeoutMs) {
    HelperResult result;
    const QString exe = helper_path();
    if (!QFileInfo::exists(exe)) {
        result.detail = QStringLiteral("VNMHelper was not found next to the app: ") + exe;
        return result;
    }

    QProcess process;
    process.setProgram(exe);
    process.setArguments(arguments);
    process.start();
    if (!process.waitForStarted(15000)) {
        result.detail = QStringLiteral("Could not start VNMHelper.");
        return result;
    }
    result.started = true;
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        result.detail = QStringLiteral("VNMHelper did not finish in time.");
        return result;
    }

    const int exitCode = process.exitCode();
    return parse_output(process.readAllStandardOutput(), exitCode);
}

HelperResult run_helper_elevated(const QStringList& arguments, int timeoutMs) {
    HelperResult result;
    const QString exe = helper_path();
    if (!QFileInfo::exists(exe)) {
        result.detail = QStringLiteral("VNMHelper was not found next to the app: ") + exe;
        return result;
    }

    const QString outPath =
        QDir(QDir::tempPath())
            .filePath(QStringLiteral("vnm-result-") +
                      QUuid::createUuid().toString(QUuid::Id128) + QStringLiteral(".json"));

#ifdef Q_OS_WIN
    QStringList fullArgs = arguments;
    fullArgs << QStringLiteral("--out") << outPath;

    QString parameterLine;
    for (const QString& argument : fullArgs) {
        if (!parameterLine.isEmpty()) {
            parameterLine += QLatin1Char(' ');
        }
        parameterLine += quoted(argument);
    }

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    info.lpVerb = L"runas";
    const std::wstring file = exe.toStdWString();
    const std::wstring parameters = parameterLine.toStdWString();
    info.lpFile = file.c_str();
    info.lpParameters = parameters.c_str();
    info.nShow = SW_HIDE;

    if (ShellExecuteExW(&info) == 0 || info.hProcess == nullptr) {
        result.detail = QStringLiteral(
            "Administrator approval was refused or unavailable, so no restore point could be "
            "created.");
        return result;
    }

    result.started = true;
    const DWORD wait = WaitForSingleObject(info.hProcess, static_cast<DWORD>(timeoutMs));
    DWORD exitCode = 1;
    GetExitCodeProcess(info.hProcess, &exitCode);
    CloseHandle(info.hProcess);

    if (wait == WAIT_TIMEOUT) {
        result.detail = QStringLiteral("The elevated helper did not finish in time.");
        QFile::remove(outPath);
        return result;
    }

    QFile file2(outPath);
    const bool opened = file2.open(QIODevice::ReadOnly);
    const QByteArray payload = opened ? file2.readAll() : QByteArray();
    file2.close();
    QFile::remove(outPath);

    if (!opened || payload.isEmpty()) {
        result.detail = QStringLiteral("The elevated helper returned no result");
        result.detail += QStringLiteral(" (exit code ") + QString::number(static_cast<qulonglong>(exitCode)) +
                         QStringLiteral(").");
        return result;
    }
    return parse_output(payload, static_cast<int>(exitCode));
#else
    Q_UNUSED(arguments);
    Q_UNUSED(timeoutMs);
    result.detail = QStringLiteral("Elevation is only implemented on Windows.");
    return result;
#endif
}

}  // namespace vnm
