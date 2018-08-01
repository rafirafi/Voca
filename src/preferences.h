// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>

#include <QSettings>
#include <QVariant>
#include <QHash>

class Preferences : public QObject
{
    Q_OBJECT

    struct Property
    {
        QVariant::Type type;
        QVariant value;
    };

public:
    explicit Preferences(QObject *parent = 0);

    void init();
    QVariant getProperty(const QString &propertyName, bool getDefault = false);
    void resetProperty(const QString &propertyName, bool doEmit = false);
    void setProperty(const QString &propertyName, const QVariant &propertyValue, bool doEmit = false);

signals:
    void propertyChanged(const QString &propertyName);

public slots:

private:
    QHash<QString, Property> data_;
    QHash<QString, Property> defaultData_;
    QSettings *settings = nullptr;
};

#endif // PREFERENCES_H
