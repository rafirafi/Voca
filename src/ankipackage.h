// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#ifndef ANKIPACKAGE_H
#define ANKIPACKAGE_H

#include <cstdint>

#include <QSqlDatabase>
#include <QString>
#include <QTemporaryDir>

//NOTE(rafi) : no error path, will abort if pb
class AnkiPackage
{
public:
    explicit AnkiPackage(const QString &deckName = "");
    ~AnkiPackage();

    void addBasicCard(const QString &front, const QString &back);
    void exportAsApkg(const QString &filePath, const QString &baseName = "");

private:
    QString deckName_;
    QTemporaryDir wrkDir_;
    QSqlDatabase db_;

    void createBasicDeck();
    uint64_t modelId_ = 0;
    uint64_t deckId_ = 0;
};

#endif // ANKIPACKAGE_H
