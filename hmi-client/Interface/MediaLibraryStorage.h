#ifndef MEDIALIBRARYSTORAGE_H
#define MEDIALIBRARYSTORAGE_H

#include <QFileInfo>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

class MediaLibraryStorage
{
public:
    struct ImportBatchResult {
        QStringList importedPaths;
        int skippedCount = 0;
        QStringList errors;
    };

    static QString musicDirectory();
    static QString videoDirectory();
    static QStringList scanFiles(const QString &directoryPath,
                                 const QStringList &allowedExtensions);
    static ImportBatchResult importFiles(const QList<QUrl> &sourceUrls,
                                         const QString &destinationDirectory,
                                         const QStringList &allowedExtensions);

private:
    static QString libraryDirectory(const QString &category);
    static QString uniqueDestinationPath(const QString &directoryPath,
                                         const QFileInfo &sourceInfo,
                                         bool *alreadyImported);
    static bool copyFileAtomically(const QString &sourcePath,
                                   const QString &destinationPath,
                                   QString *errorMessage);
};

#endif
