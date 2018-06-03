// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#include "collection.h"

#include <cassert>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <QtDebug>

Collection::Collection(QObject *parent) : QObject(parent)
{
    dbOpen();
}

Collection::~Collection()
{
    dbClose();
}

void Collection::dbOpen()
{
    db_ = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "vocadb"));

    QString dbLoc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dbLoc.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "no writable location";
        abort();
    }
    if (!QFileInfo::exists(dbLoc) && !QDir().mkpath(dbLoc)) {
        qDebug() << Q_FUNC_INFO << "not possible to create writable location";
        abort();
    }
    qDebug() << "database file at" << dbLoc << QDir::separator() << "vocadb.sqlite";

    db_->setDatabaseName(dbLoc + QDir::separator() + "vocadb.sqlite");
    //db_->setDatabaseName(":memory:");
    if (!db_->open()) {
        qDebug() << Q_FUNC_INFO << "db open failed";
        abort();
    }

    QSqlQuery query(*db_);
    QString str = "create table if not exists voca (word text, meaning text, deckid integer);";
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }

    str = "create unique index if not exists idx_voca_word on voca (word);";
    ok = query.prepare(str);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }

    str = "create table if not exists decks (id integer primary key autoincrement, name text not null unique);";
    ok = query.prepare(str);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
}

void Collection::dbClose()
{
    db_->close();
    auto name = db_->connectionName();
    delete db_;
    db_ = nullptr;
    QSqlDatabase::removeDatabase(name);
}

int Collection::getDeckId(const QString &deckName) const
{
    QSqlQuery query(*db_);
    QString str = QString("select id from decks where name=:deckName;");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":deckName", deckName);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
    if (query.next()) {
        return query.value("id").toInt();
    }
    return -1;
}

QString Collection::getDeckName(int deckId)
{
    QSqlQuery query(*db_);
    QString str = QString("select name from decks where id=:deckId;");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":deckId", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
    if (query.next()) {
        return query.value("name").toString();
    }
    return QString();
}

bool Collection::deckExists(const QString &deckName) const
{
    return getDeckId(deckName) != -1;
}

void Collection::addDeck(const QString &deckName)
{
    assert(!deckExists(deckName));
    QSqlQuery query(*db_);
    QString str = "insert into decks (name) values (:name);";
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    query.bindValue(":name", deckName);
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
}

QString Collection::getMeaning(int deckId, const QString &word)
{
    QSqlQuery query(*db_);
    QString str = QString("select meaning from voca where word=:word and deckid=:deckid limit 1");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":word", word);
    query.bindValue(":deckid", deckId);
        ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
    if (query.next()) {
        return query.value("meaning").toString();
    }
    return QString();
}

void Collection::upsertWord(int deckId, const QString &word, const QString &meaning)
{
    QSqlQuery query(*db_);
    QString str = QString("replace into voca (word, meaning, deckid) values (:word,:meaning,:deckid)");

    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":word", word);
    query.bindValue(":meaning", meaning);
    query.bindValue(":deckid", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
}

void Collection::renameDeck(int deckId, const QString &deckName)
{
    assert(getDeckId(deckName) == deckId || getDeckId(deckName) == -1); // noop or ok

    QSqlQuery query(*db_);
    QString str = QString("update decks set name=:name where id=:id");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":name", deckName);
    query.bindValue(":id", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
}

DeckContent::DeckContent(QSqlQuery &query) : query_(query)
{
    empty_ = !query_.next();
}

bool DeckContent::isEmpty() const
{
    return empty_;
}

bool DeckContent::getNext(QString &word, QString &meaning)
{
    if (empty_) {
        word.clear();
        meaning.clear();
        return false;
    }
    word = query_.value("word").toString();
    meaning = query_.value("meaning").toString();
    empty_ = !query_.next();
    return true;
}

DeckContent Collection::getDeckContent(int deckId)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    QString str = QString("select word, meaning from voca where deckid=:deckid;");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":deckid", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
    return DeckContent(query);
}

// delete deck with deckId, deckId must exist, the caller is responsible for ui coherence
void Collection::deleteDeck(int deckId)
{
    // remove deck content
    QSqlQuery query(*db_);
    QString str = QString("delete from voca where deckid=:deckid;");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":deckid", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }

    // remove deck itself
    str = QString("delete from decks where id=:deckid;");
    ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    query.bindValue(":deckid", deckId);
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
}
