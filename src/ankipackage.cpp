// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#include "ankipackage.h"

#include <cassert>

#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlQuery>
#include <QVector>

#include <KArchive>
#include <KZip>

#include <QtDebug>

AnkiPackage::AnkiPackage(const QString &deckName)
    : deckName_(deckName)
{
    QString dateTimeStr = QDateTime::currentDateTime().toString("dd.MM.yyyy-hh:mm:ss");

    if (deckName_.isEmpty()) {
        deckName_ = QString("Deck-%1").arg(dateTimeStr);
    }

    if (!wrkDir_.isValid()) {
        qDebug() << __func__ << "QTemporaryDir is not available";
        abort();
    }
    qDebug() << __func__ << wrkDir_.path();

    db_ = QSqlDatabase::addDatabase("QSQLITE", QString("db-%1").arg(dateTimeStr));
    const QString &dbName("collection.anki2");
    db_.setDatabaseName(QString("%1%2%3").arg(wrkDir_.path()).arg(QDir::separator()).arg(dbName));

    if (!db_.open()) {
        qDebug() << __func__ << "db open : failed";
        abort();
    }

    createBasicDeck();
}

AnkiPackage::~AnkiPackage()
{
    db_.close();
}

// NOTE(rafi) :
// documentation for anki db format
// https://github.com/ankidroid/Anki-Android/wiki/Database-Structure
// https://github.com/dae/anki/blob/master/anki/collection.py

// TODO => load or create
// return values/populate values for modelId_, deckId_ needed to create notes
void AnkiPackage::createBasicDeck()
{
    assert(db_.isOpen());

    QSqlQuery query(db_);

    QVector<QString> strs;

    strs.append(R"(CREATE TABLE cards (
                id              integer primary key,   /* 0 */
                nid             integer not null,      /* 1 */
                did             integer not null,      /* 2 */
                ord             integer not null,      /* 3 */
                mod             integer not null,      /* 4 */
                usn             integer not null,      /* 5 */
                type            integer not null,      /* 6 */
                queue           integer not null,      /* 7 */
                due             integer not null,      /* 8 */
                ivl             integer not null,      /* 9 */
                factor          integer not null,      /* 10 */
                reps            integer not null,      /* 11 */
                lapses          integer not null,      /* 12 */
                left            integer not null,      /* 13 */
                odue            integer not null,      /* 14 */
                odid            integer not null,      /* 15 */
                flags           integer not null,      /* 16 */
                data            text not null          /* 17 */
            ))");
    strs.append(R"(CREATE TABLE col (
            id              integer primary key,
            crt             integer not null,
            mod             integer not null,
            scm             integer not null,
            ver             integer not null,
            dty             integer not null,
            usn             integer not null,
            ls              integer not null,
            conf            text not null,
            models          text not null,
            decks           text not null,
            dconf           text not null,
            tags            text not null
        ))");
    strs.append(R"(CREATE TABLE graves (
                usn             integer not null,
                oid             integer not null,
                type            integer not null
            ))");
    strs.append(R"(CREATE TABLE notes (
                id              integer primary key,   /* 0 */
                guid            text not null,         /* 1 */
                mid             integer not null,      /* 2 */
                mod             integer not null,      /* 3 */
                usn             integer not null,      /* 4 */
                tags            text not null,         /* 5 */
                flds            text not null,         /* 6 */
                sfld            integer not null,      /* 7 */
                csum            integer not null,      /* 8 */
                flags           integer not null,      /* 9 */
                data            text not null          /* 10 */
            ))");
    strs.append(R"(CREATE TABLE revlog (
                id              integer primary key,
                cid             integer not null,
                usn             integer not null,
                ease            integer not null,
                ivl             integer not null,
                lastIvl         integer not null,
                factor          integer not null,
                time            integer not null,
                type            integer not null
            ))");

    strs.append(R"(CREATE INDEX ix_cards_nid on cards (nid))");
    strs.append(R"(CREATE INDEX ix_cards_sched on cards (did, queue, due))");
    strs.append(R"(CREATE INDEX ix_cards_usn on cards (usn))");
    strs.append(R"(CREATE INDEX ix_notes_csum on notes (csum))");
    strs.append(R"(CREATE INDEX ix_notes_usn on notes (usn))");
    strs.append(R"(CREATE INDEX ix_revlog_cid on revlog (cid))");
    strs.append(R"(CREATE INDEX ix_revlog_usn on revlog (usn))");

    strs.append(R"(ANALYZE sqlite_master)");
    strs.append(R"(INSERT INTO "sqlite_stat1" VALUES('col',NULL,'1'))");

    for (const auto &str : strs) {
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
    }

    // entry in collection table
    QString str("INSERT into COL values(:id,:crt,:mod,:scm,:ver,:dty,:usn,:ls,:conf,:models,:decks,:dconf,:tags)");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    auto msDate = QDateTime::currentMSecsSinceEpoch();
    auto dateTime = QDateTime::fromMSecsSinceEpoch(msDate - 4 * 60 * 60 * 1000);
    auto crt = QDateTime(dateTime.date()).toMSecsSinceEpoch() / 1000 + 4 * 60 * 60;
    // NOTE(rafi): Cf anki src for crt but for the
    // various values derived from msDate, I just looked at a random collection generated by anki and
    // made up something approaching it
    query.bindValue(":id", "1");
    query.bindValue(":crt", crt);
    query.bindValue(":mod", msDate);
    query.bindValue(":scm", msDate - 10);
    query.bindValue(":ver", "11");
    query.bindValue(":dty", "0");
    query.bindValue(":usn", "0");
    query.bindValue(":ls", "0");

    auto conf = QString(R"nkstr({"nextPos": 1, "estTimes": true, "activeDecks": [1], "sortType": "noteFld", "timeLim": 0, "sortBackwards": false, "addToCur": true, "curDeck": 1, "newBury": true, "newSpread": 0, "dueCounts": true, "curModel": "%1", "collapseTime": 1200})nkstr").arg(msDate - 10);
    query.bindValue(":conf", conf);

    // NOTE(rafi): Basic will be renamed to Basic-xxxxx, it's ugly but it's ok
    modelId_ = msDate;
    // id : "model ID, matches notes.mid",
    auto model = QString(R"nkstr({"%1": {"vers": [], "name": "Basic", "tags": [], "did": 1, "usn": -1, "req": [[0, "all", [0]]], "flds": [{"size": 20, "name": "Front", "media": [], "rtl": false, "ord": 0, "font": "Arial", "sticky": false}, {"size": 20, "name": "Back", "media": [], "rtl": false, "ord": 1, "font": "Arial", "sticky": false}], "sortf": 0, "latexPre": "\\documentclass[12pt]{article}\n\\special{papersize=3in,5in}\n\\usepackage[utf8]{inputenc}\n\\usepackage{amssymb,amsmath}\n\\pagestyle{empty}\n\\setlength{\\parindent}{0in}\n\\begin{document}\n", "tmpls": [{"afmt": "{{FrontSide}}\n\n<hr id=answer>\n\n{{Back}}", "name": "Card 1", "qfmt": "{{Front}}", "did": null, "ord": 0, "bafmt": "", "bqfmt": ""}], "latexPost": "\\end{document}", "type": 0, "id": "%1", "css": ".card {\n font-family: arial;\n font-size: 20px;\n text-align: center;\n color: black;\n background-color: white;\n}\n", "mod": %2}}
)nkstr").arg(modelId_).arg(msDate / 1000);
    query.bindValue(":models", model);

    // deck identified by deck name
    // id matches cards.did
    deckId_ = msDate;
    auto deck = QString(R"nkstr({"1": {"desc": "", "name": "Default", "extendRev": 50, "usn": 0, "collapsed": false, "newToday": [0, 0], "timeToday": [0, 0], "dyn": 0, "extendNew": 10, "conf": 1, "revToday": [0, 0], "lrnToday": [0, 0], "id": 1, "mod": %1}, "%2": {"desc": "", "name": "%4", "extendRev": 50, "usn": -1, "collapsed": false, "newToday": [0, 0], "timeToday": [0, 0], "dyn": 0, "extendNew": 10, "conf": 1, "revToday": [0, 0], "lrnToday": [0, 0], "id": %2, "mod": %3}})nkstr").arg(msDate / 1000 - 1).arg(deckId_).arg(msDate / 1000).arg(deckName_);
    query.bindValue(":decks", deck);

    auto dconf = QString(R"nkstr({"1": {"name": "Default", "replayq": true, "lapse": {"leechFails": 8, "minInt": 1, "delays": [10], "leechAction": 0, "mult": 0}, "rev": {"perDay": 100, "fuzz": 0.05, "ivlFct": 1, "maxIvl": 36500, "ease4": 1.3, "bury": true, "minSpace": 1}, "timer": 0, "maxTaken": 60, "usn": 0, "new": {"perDay": 20, "delays": [1, 10], "separate": true, "ints": [1, 4, 7], "initialFactor": 2500, "bury": true, "order": 1}, "mod": 0, "id": 1, "autoplay": true}})nkstr");
    query.bindValue(":dconf", dconf);

    // no tag
    query.bindValue(":tags", "{}");

    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
}

void AnkiPackage::addBasicCard(const QString &front, const QString &back)
{
    assert(db_.isOpen());

    // NOTE
    QSqlQuery query(db_);
    QString str("INSERT INTO notes VALUES(:id,:guid,:mid,:mod,:usn,:tags,:flds,:sfld,:csum,:flags,:data)");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    auto msDate = QDateTime::currentMSecsSinceEpoch();
    auto noteID = msDate;
    query.bindValue(":id", noteID);
    /// TODO : check this, sj=hould be random or whatev'
    QString guid = QString("%1").arg(msDate).toUtf8().toBase64().right(12).left(10);
    query.bindValue(":guid", guid);
    //  model id
    query.bindValue(":mid", QString("%1").arg(modelId_));
    query.bindValue(":mod", msDate / 1000);
    // don't touch
    query.bindValue(":usn", "-1");
    // no tags
    query.bindValue(":tags", "");
    auto sep = '\x1f';
    // Card content, front and back
    // QString front("Bonjour"); QString back("Hello World!");
    query.bindValue(":flds", QString("%2%1%3").arg(sep).arg(front).arg(back));
    // Card front content without html (first part of flds, filtered).
    auto sfld = QString("%1").arg(front);
    query.bindValue(":sfld", sfld);
    // A string SHA1 checksum of sfld, limited to 8 digits
    qlonglong csum = QCryptographicHash::hash(sfld.toUtf8(),QCryptographicHash::Sha1).toHex().left(8).toLongLong(&ok, 16);
    if (!ok) { // TODEL
        qDebug() << Q_FUNC_INFO << "bad csum";
        abort();
    }
    query.bindValue(":csum", csum);
    // flag at 0
    query.bindValue(":flags", "0");
    // no data
    query.bindValue(":data", "");

    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }

    // CARD
    str = QString("INSERT INTO cards VALUES(:id,:nid,:did,:ord,:mod,:usn,:type,:queue,:due,:ivl,:factor,:reps,:lapses,:left,:odue,:odid,:flags,:data)");
    ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    auto cardId = msDate;
    query.bindValue(":id", cardId);
    // note id
    query.bindValue(":nid", noteID);
    // TODO deck id : should be read from collection if any
    query.bindValue(":did", QString("%1").arg(deckId_));
    //??
    query.bindValue(":ord", "0");
    //mod date in secs
    query.bindValue(":mod", msDate / 1000);
    query.bindValue(":usn", "-1");
    query.bindValue(":type", "0");
    query.bindValue(":queue", "0");
    // new card => doesn't matter
    query.bindValue(":due", "1");
    query.bindValue(":ivl", "0");
    query.bindValue(":factor", "0");
    query.bindValue(":reps", "0");
    query.bindValue(":lapses", "0");
    query.bindValue(":left", "0");
    query.bindValue(":odue", "0");
    query.bindValue(":odid", "0");
    query.bindValue(":flags", "0");
    query.bindValue(":data", "");

    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }
}

void AnkiPackage::exportAsApkg(const QString &filePath, const QString &baseName, bool askOverwrite)
{
    assert(!baseName.isEmpty());
    QString filename = QString("%1%2%3.apkg").arg(filePath).arg(QDir::separator()).arg(baseName);
    if (QFileInfo::exists(filename)) {
        if (askOverwrite) {
            QMessageBox msgBox;
            msgBox.setText(AnkiPackage::tr("Overwriting existing apkg ?"));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec() ==  QMessageBox::Cancel) {
                qDebug() << __func__ << "don't overwrite";
                return;
            }
        }
        if (!QFile::remove(filename)) {
            qDebug() << __func__ << "failed to remove existing file";
            abort();
        }
    }

    QFile file(wrkDir_.path() + QDir::separator() + "media");
    if (!file.open(QIODevice::ReadWrite)) {
        qDebug() << __func__ << "failed to create media file";
        abort();
    }
    file.write("{}");
    file.close();

    KZip archive(filename);
    if (!archive.open(QIODevice::WriteOnly)) {
        qDebug() << __func__ << "failed to create archie file";
        abort();
    }
    archive.addLocalFile(file.fileName(), "media");
    archive.addLocalFile(db_.databaseName(), "collection.anki2");
    archive.close();
}
