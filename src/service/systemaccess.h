#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

struct ProcessResult {
    bool started{false};
    bool finished{false};
    int exitCode{-1};
    QByteArray standardOutput;
    QByteArray standardError;

    bool ok() const { return started && finished && exitCode == 0; }
};

class FileSystemAccess {
public:
    virtual ~FileSystemAccess() = default;

    virtual bool exists(const QString& path) const = 0;
    virtual bool isWritable(const QString& path) const = 0;
    virtual QString canonicalPath(const QString& path) const = 0;
    virtual QString absolutePath(const QString& path) const = 0;
    virtual QString fileName(const QString& path) const = 0;
    virtual QString readText(const QString& path) const = 0;
    virtual bool writeText(const QString& path, const QString& value, QString* errorMessage = nullptr) const = 0;
    virtual QStringList
    entryList(const QString& path, const QStringList& nameFilters = {}, bool dirsOnly = false) const = 0;
};

class ProcessRunner {
public:
    virtual ~ProcessRunner() = default;

    virtual QString findExecutable(const QString& program) const = 0;
    virtual ProcessResult run(const QString& program, const QStringList& arguments, int timeoutMs) const = 0;
};

class QtFileSystemAccess : public FileSystemAccess {
public:
    bool exists(const QString& path) const override;
    bool isWritable(const QString& path) const override;
    QString canonicalPath(const QString& path) const override;
    QString absolutePath(const QString& path) const override;
    QString fileName(const QString& path) const override;
    QString readText(const QString& path) const override;
    bool writeText(const QString& path, const QString& value, QString* errorMessage = nullptr) const override;
    QStringList
    entryList(const QString& path, const QStringList& nameFilters = {}, bool dirsOnly = false) const override;
};

class QtProcessRunner : public ProcessRunner {
public:
    QString findExecutable(const QString& program) const override;
    ProcessResult run(const QString& program, const QStringList& arguments, int timeoutMs) const override;
};

class SystemAccess {
public:
    SystemAccess(FileSystemAccess& files, ProcessRunner& processes);

    FileSystemAccess& files() const;
    ProcessRunner& processes() const;

private:
    FileSystemAccess* mFiles{nullptr};
    ProcessRunner* mProcesses{nullptr};
};

SystemAccess& defaultSystemAccess();
