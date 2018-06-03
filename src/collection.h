// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#ifndef COLLECTION_H
#define COLLECTION_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>

class DeckContent
{
    friend class Collection;
    explicit DeckContent(QSqlQuery &query);
    QSqlQuery query_;
    bool empty_;
public:
    bool isEmpty() const;
    bool getNext(QString &word, QString &meaning);
};

class Collection : public QObject
{
    Q_OBJECT
public:
    explicit Collection(QObject *parent = 0);
    ~Collection();

    int getDeckId(const QString &deckName) const;
    QString getDeckName(int deckId);
    bool deckExists(const QString &deckName) const;
    void addDeck(const QString &deckName);
    QString getMeaning(int deckId, const QString &word);
    void upsertWord(int deckId, const QString &word, const QString &meaning);
    void renameDeck(int deckId, const QString &deckName);
    DeckContent getDeckContent(int deckId);
    void deleteDeck(int deckId);

    // TMP
    const QSqlDatabase *db() const
    {
        return db_;
    }
    void dbClose();

private:
    QSqlDatabase *db_ = nullptr;

    void dbOpen();
};

#endif // COLLECTION_H
