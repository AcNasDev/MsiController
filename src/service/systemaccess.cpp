#include "systemaccess.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

bool QtFileSystemAccess::exists(const QString& path) const {
    return QFileInfo::exists(path);
}

bool QtFileSystemAccess::isWritable(const QString& path) const {
    return QFileInfo(path).isWritable();
}

QString QtFileSystemAccess::canonicalPath(const QString& path) const {
    return QFileInfo(path).canonicalFilePath();
}

QString QtFileSystemAccess::absolutePath(const QString& path) const {
    return QFileInfo(path).absoluteFilePath();
}

QString QtFileSystemAccess::fileName(const QString& path) const {
    return QFileInfo(path).fileName();
}

QString QtFileSystemAccess::readText(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll().trimmed());
}

bool QtFileSystemAccess::writeText(const QString& path, const QString& value, QString* errorMessage) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    const QByteArray bytes = value.toUtf8();
    if (file.write(bytes) != bytes.size() || !file.flush()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

QStringList QtFileSystemAccess::entryList(const QString& path, const QStringList& nameFilters, bool dirsOnly) const {
    QDir dir(path);
    const QDir::Filters filters =
        dirsOnly ? (QDir::Dirs | QDir::NoDotAndDotDot) : (QDir::AllEntries | QDir::NoDotAndDotDot);
    return nameFilters.isEmpty() ? dir.entryList(filters) : dir.entryList(nameFilters, filters);
}

QString QtProcessRunner::findExecutable(const QString& program) const {
    return QStandardPaths::findExecutable(program);
}

ProcessResult QtProcessRunner::run(const QString& program, const QStringList& arguments, int timeoutMs) const {
    ProcessResult result;
    QProcess process;
    process.start(program, arguments);
    result.started = process.waitForStarted(timeoutMs);
    if (!result.started) {
        result.standardError = process.errorString().toUtf8();
        return result;
    }
    result.finished = process.waitForFinished(timeoutMs);
    if (!result.finished) {
        process.kill();
        process.waitForFinished();
    } else {
        result.exitCode = process.exitCode();
    }
    result.standardOutput = process.readAllStandardOutput();
    result.standardError = process.readAllStandardError();
    if (!result.finished && result.standardError.isEmpty()) {
        result.standardError = "Process timed out";
    }
    return result;
}

SystemAccess::SystemAccess(FileSystemAccess& files, ProcessRunner& processes)
    : mFiles(&files), mProcesses(&processes) {}

FileSystemAccess& SystemAccess::files() const {
    return *mFiles;
}

ProcessRunner& SystemAccess::processes() const {
    return *mProcesses;
}

SystemAccess& defaultSystemAccess() {
    static QtFileSystemAccess files;
    static QtProcessRunner processes;
    static SystemAccess access(files, processes);
    return access;
}
