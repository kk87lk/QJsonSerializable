#include "qjsonserializable.h"
#include <QMetaProperty>
#include <QVariant>
#include <QJsonDocument>


QJsonSerializable::QJsonSerializable(QObject *parent) : QObject(parent)
{
    loadFinish_ = false;
    m_ignoredProperties << "objectName";
}

QStringList QJsonSerializable::ignoredProperties() const
{
    return m_ignoredProperties;
}

void QJsonSerializable::setIgnoredProperties(const QStringList& list)
{
    m_ignoredProperties = list;
}

bool QJsonSerializable::save(const QString& fpath){
    return QObjectHelper::save(this, fpath, m_ignoredProperties);
}

bool QJsonSerializable::load(const QString& fpath){
    bool ret = QObjectHelper::load(fpath, this);
    if (ret) {
        loadFinish_ = true;
    }
    onLoadFinished();
    return ret;
}

void QJsonSerializable::fromVariantMap(const QVariantMap& map)
{
    QObjectHelper::qjsonobject2qobject(QJsonObject::fromVariantMap(map), this);
}

void QJsonSerializable::fromJsonValue(const QJsonValue &jsonVal){
    QObjectHelper::qjsonobject2qobject(jsonVal.toObject(), this);
}

QDebug operator<<(QDebug dbg, const QJsonSerializable &obj)
{
    QStringList ignoredProperties = obj.ignoredProperties();
    const QMetaObject *metaobject = obj.metaObject();
    dbg.nospace() << metaobject->className() << "({";
    int count = metaobject->propertyCount();
    for (int i=0; i<count; ++i) {
      QMetaProperty metaproperty = metaobject->property(i);
      const char *name = metaproperty.name();
      if (ignoredProperties.contains(QLatin1String(name)) || (!metaproperty.isReadable()) || name == QLatin1String("ignoredProperties"))
        continue;
      QVariant value = obj.property(name);
      if( i <= (count - 1)){
          if (value.canConvert<QJsonObject>()){
             dbg.nospace() << name << ":" << value.toJsonObject() << ",";
          }else if(value.toString().count() == 0){
              dbg.nospace() << name << ":" << value << ",";
          }else{
             dbg.nospace() << name << ":" << value.toString() << ",";
          }
      }
    }
    dbg.nospace() << "}) ";
    return dbg;
}
