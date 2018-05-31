#include "preferences.h"

#include <cassert>

#include <QCoreApplication>
#include <QDir>

#include <QtDebug>

Preferences::Preferences(QObject *parent) : QObject(parent)
{
    settings = new QSettings(QSettings::IniFormat,
                             QSettings::UserScope,
                             qApp->applicationName(),
                             qApp->applicationName(),
                             this);

    qDebug() << "preferences file at" << settings->fileName();
}

void Preferences::init()
{
    // NOTE : add property name and default value directly here
    defaultData_.insert("last-font-size", {QVariant::Int, -1}); // invalid
    defaultData_.insert("last-deck-name", {QVariant::String, ""}); // invalid
    defaultData_.insert("show-current-deck-name", {QVariant::Bool, true});
    defaultData_.insert("show-last-word-found", {QVariant::Bool, true});

    // set data to default
    data_ = defaultData_;

    // overwrite with stored values
    for (auto &k : data_.keys()) {
        QVariant v = settings->value(k);
        if (v.isValid()) {
            data_[k] = { data_[k].type, v };
        }
    }
}

QVariant Preferences::getProperty(const QString &propertyName, bool getDefault)
{
    if (getDefault) {
        assert(defaultData_.contains(propertyName));
        return defaultData_[propertyName].value;
    }
    assert(data_.contains(propertyName));
    return data_[propertyName].value;
}

void Preferences::resetProperty(const QString &propertyName, bool doEmit)
{
    assert(defaultData_.contains(propertyName));
    setProperty(propertyName, defaultData_[propertyName].value, doEmit);
}

void Preferences::setProperty(const QString &propertyName, const QVariant &propertyValue, bool doEmit)
{
    assert(data_.contains(propertyName));
    assert(propertyValue.type() == data_[propertyName].type);

    if (propertyValue == data_[propertyName].value) {
        //qDebug() << __PRETTY_FUNCTION__ << "same value";
        return;
    }

    settings->setValue(propertyName, propertyValue);
    data_[propertyName].value = propertyValue;

    if (doEmit) {
        emit propertyChanged(propertyName);
    }
}
