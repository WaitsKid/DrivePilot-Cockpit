#include "MediaLibraryStorage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
QString normalizedExtension(const QString &path)
{
    return QFileInfo(path).suffix().trimmed().toLower();
}

bool extensionIsAllowed(const QString &path, const QStringList &allowedExtensions)
{
    return allowedExtensions.contains(normalizedExtension(path), Qt::CaseInsensitive);
}
}

QString MediaLibraryStorage::musicDirectory()
{
    return libraryDirectory(QStringLiteral("music"));
}

QString MediaLibraryStorage::videoDirectory()
{
    return libraryDirectory(QStringLiteral("videos"));
}

QStringList MediaLibraryStorage::scanFiles(const QString &directoryPath,
                                           const QStringList &allowedExtensions)
{
    QDir directory(directoryPath);
    if (!directory.exists())
        return {};

    QStringList filters;
    filters.reserve(allowedExtensions.size());
    for (const QString &extension : allowedExtensions)
        filters.append(QStringLiteral("*.%1").arg(extension));

    const QFileInfoList entries = directory.entryInfoList(
        filters,
        QDir::Files | QDir::Readable | QDir::NoSymLinks,
        QDir::Name | QDir::IgnoreCase);

    QStringList result;
    result.reserve(entries.size());
    for (const QFileInfo &entry : entries)
        result.append(entry.absoluteFilePath());
    return result;
}

MediaLibraryStorage::ImportBatchResult MediaLibraryStorage::importFiles(
    const QList<QUrl> &sourceUrls,
    const QString &destinationDirectory,
    const QStringList &allowedExtensions)
{
    ImportBatchResult result;

    if (!QDir().mkpath(destinationDirectory)) {
        result.errors.append(QStringLiteral("无法创建媒体库目录：%1").arg(destinationDirectory));
        return result;
    }

    for (const QUrl &sourceUrl : sourceUrls) {
        const QString sourcePath = sourceUrl.toLocalFile();
        const QFileInfo sourceInfo(sourcePath);

        if (!sourceUrl.isLocalFile() || !sourceInfo.exists() || !sourceInfo.isFile()) {
            ++result.skippedCount;
            result.errors.append(QStringLiteral("无法读取：%1").arg(sourceUrl.toDisplayString()));
            continue;
        }

        if (!extensionIsAllowed(sourcePath, allowedExtensions)) {
            ++result.skippedCount;
            result.errors.append(QStringLiteral("不支持的格式：%1").arg(sourceInfo.fileName()));
            continue;
        }

        bool alreadyImported = false;
        const QString destinationPath = uniqueDestinationPath(
            destinationDirectory, sourceInfo, &alreadyImported);
        if (alreadyImported) {
            ++result.skippedCount;
            continue;
        }

        QString errorMessage;
        if (!copyFileAtomically(sourcePath, destinationPath, &errorMessage)) {
            ++result.skippedCount;
            result.errors.append(errorMessage);
            continue;
        }

        result.importedPaths.append(destinationPath);
    }

    return result;
}

QString MediaLibraryStorage::libraryDirectory(const QString &category)
{
    const QString appDataRoot = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    const QString directoryPath = QDir(appDataRoot).filePath(
        QStringLiteral("media_library/%1").arg(category));
    QDir().mkpath(directoryPath);
    return QDir::cleanPath(directoryPath);
}

QString MediaLibraryStorage::uniqueDestinationPath(const QString &directoryPath,
                                                   const QFileInfo &sourceInfo,
                                                   bool *alreadyImported)
{
    if (alreadyImported)
        *alreadyImported = false;

    const QString originalPath = QDir(directoryPath).filePath(sourceInfo.fileName());
    const QFileInfo originalDestination(originalPath);
    if (!originalDestination.exists())
        return originalPath;

    if (originalDestination.size() == sourceInfo.size()) {
        if (alreadyImported)
            *alreadyImported = true;
        return originalPath;
    }

    const QString baseName = sourceInfo.completeBaseName();
    const QString suffix = sourceInfo.suffix();
    for (int number = 2; number < 10000; ++number) {
        const QString candidateName = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(baseName).arg(number)
            : QStringLiteral("%1 (%2).%3").arg(baseName).arg(number).arg(suffix);
        const QString candidatePath = QDir(directoryPath).filePath(candidateName);
        if (!QFileInfo::exists(candidatePath))
            return candidatePath;
    }

    return QDir(directoryPath).filePath(
        QStringLiteral("%1_%2.%3")
            .arg(baseName)
            .arg(QDateTime::currentMSecsSinceEpoch())
            .arg(suffix));
}

bool MediaLibraryStorage::copyFileAtomically(const QString &sourcePath,
                                             const QString &destinationPath,
                                             QString *errorMessage)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法打开：%1").arg(QFileInfo(sourcePath).fileName());
        return false;
    }

    QSaveFile destination(destinationPath);
    if (!destination.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入媒体库：%1")
                                .arg(QFileInfo(destinationPath).fileName());
        return false;
    }

    constexpr qint64 chunkSize = 1024 * 1024;
    while (!source.atEnd()) {
        const QByteArray chunk = source.read(chunkSize);
        if (chunk.isEmpty() && source.error() != QFile::NoError) {
            destination.cancelWriting();
            if (errorMessage)
                *errorMessage = QStringLiteral("读取文件失败：%1")
                                    .arg(QFileInfo(sourcePath).fileName());
            return false;
        }

        if (destination.write(chunk) != chunk.size()) {
            destination.cancelWriting();
            if (errorMessage)
                *errorMessage = QStringLiteral("复制文件失败：%1")
                                    .arg(QFileInfo(sourcePath).fileName());
            return false;
        }
    }

    if (!destination.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("保存文件失败：%1")
                                .arg(QFileInfo(destinationPath).fileName());
        return false;
    }

    return true;
}
