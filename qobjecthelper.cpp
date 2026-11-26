#include "qobjecthelper.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QFile>
#include <QDebug>
#include "qjsonserializable.h"

class QObjectHelper::QObjectHelperPrivate {
};

QObjectHelper::QObjectHelper()
  : d (new QObjectHelperPrivate)
{
}

QObjectHelper::~QObjectHelper()
{
  delete d;
}

/**
* This method converts a QObject instance into a QJsonObject.
*
* @param object The QObject instance to be converted.
*/
QJsonObject QObjectHelper::qobject2qjsonobject( const QObject* object)
{
    QJsonObject result;
    if (!object)
        return result;

    const QMetaObject *metaObject = object->metaObject();
    int count = metaObject->propertyCount();
    for (int i=0; i<count; ++i) {
        QMetaProperty metaproperty = metaObject->property(i);
        const char *name = metaproperty.name();

        if ((!metaproperty.isStored()) || (!metaproperty.isReadable()) || name == QLatin1String("objectName"))
            continue;

        QVariant value = object->property(name);
        if (value.canConvert<QList<QObject*>>()) {
            QJsonArray list;
            auto objs = value.value<QList<QObject*>>();
            for (QObject* obj : objs)
                list.append(obj ? qobject2qjsonobject(obj) : QJsonObject());
            result.insert(name, list);
        } else if (value.canConvert<QObject*>()) {
            QObject* child = value.value<QObject*>();
            result.insert(name, child ? qobject2qjsonobject(child) : QJsonObject());
        } else if (value.userType() == QMetaType::QByteArray) {
            QByteArray ba = value.toByteArray();
            result.insert(name, QString::fromUtf8(ba.toBase64()));
        } else {
            result.insert(QLatin1String(name), QJsonValue::fromVariant(value));
        }
    }
    return result;
}

QVariantMap QObjectHelper::qobject2variantmap(const QObject* object)
{
    QVariantMap result;
    if (!object) return result;

    const QMetaObject* metaObject = object->metaObject();
    int count = metaObject->propertyCount();

    for (int i = 0; i < count; ++i) {
        QMetaProperty metaproperty = metaObject->property(i);
        const char* name = metaproperty.name();

        if ((!metaproperty.isStored()) || (!metaproperty.isReadable()) || name == QLatin1String("objectName"))
            continue;

        QVariant value = object->property(name);
        if (!value.isValid())
            continue;

        // QObjectList / QList<QObject*>
        if (value.canConvert<QList<QObject*>>()) {
            QVariantList list;
            auto objs = value.value<QList<QObject*>>();
            for (QObject* obj : objs)
                list.append(obj ? qobject2variantmap(obj) : QVariant());
            result.insert(name, list);
        }
        /**
         * @brief QObject* 嵌套模型递归处理。
         * @details 使用 canConvert 覆盖 FileCollection* 这类具体子类指针。
         */
        else if (value.canConvert<QObject*>()) {
            QObject* child = value.value<QObject*>();
            result.insert(name, child ? qobject2variantmap(child) : QVariantMap());
        }
        // QByteArray → Base64（和你 json2qobject 对称）
        else if (value.userType() == QMetaType::QByteArray) {
            QByteArray ba = value.toByteArray();
            result.insert(name, QString::fromUtf8(ba.toBase64()));
        }
        // 普通类型（int / double / string / map / list）
        else {
            result.insert(name, value);
        }
    }
    return result;
}


/**
* This method converts a QObject instance into a json string.
*
* @param object The QObject instance to be converted.
*/
QString QObjectHelper::qobject2json(const QObject *object)
{
    QString result("");
    QJsonObject jsonobj = QObjectHelper::qobject2qjsonobject(object);
    QJsonDocument doc(jsonobj);
    return doc.toJson(QJsonDocument::Compact);
}


/**
* This method converts a QVariantMap instance into a QObject
*
* @param variant Attributes to assign to the object.
* @param object The QObject instance to update.
*/
void QObjectHelper::qjsonobject2qobject(const QJsonObject& jsonobj, QObject* object)
{
    if(jsonobj.isEmpty()){
        qWarning()<<"jsonobj is empty";
        return;
    }
    if(object == nullptr){
        qWarning()<<"object is nullptr";
        return;
    }
    const QMetaObject *metaObject = object->metaObject();
    QJsonObject::const_iterator iter;
    for (iter = jsonobj.constBegin(); iter != jsonobj.constEnd(); ++iter) {
        int pIdx = metaObject->indexOfProperty(iter.key().toLatin1());

        if (pIdx < 0) {
            continue;
        }
        QMetaProperty metaproperty = metaObject->property(pIdx);
        #if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QMetaType metaType = metaproperty.metaType();
        #else
        QMetaType metaType = QMetaType(metaproperty.userType());
        #endif
        const int type = metaType.id();
        auto metaTypeObj = metaType.metaObject();
        QJsonValue val = iter.value();
        QVariant v(iter.value());
        // 传入其中所有的UserType 都继承QJsonSerializable类

        // Array情况
        if (iter.value().isArray()) {
            QJsonArray jsonArray = iter.value().toArray();

            /// @brief 对象字段优先走属性专用 FromJson。
            ///属性为 Q_PROPERTY_LIST_AUTO
            if (metaType.name() == QByteArrayLiteral("QList<QObject*>")){
                QByteArray methodName = iter.key().toLatin1() + "FromJson";
                bool ok = QMetaObject::invokeMethod(object, methodName.constData(),Q_ARG(QJsonArray, jsonArray));
                if (ok) {
                    continue;
                }
            }
            switch(type){
            case QMetaType::QJsonArray:
                metaproperty.write(object, jsonArray);
                break;
            case QMetaType::QVariantList:
                metaproperty.write(object, jsonArray.toVariantList());
                break;
            case QMetaType::QStringList:{
                QStringList sl;
                for (int i=0; i<jsonArray.size(); i++)
                    sl.append(jsonArray[i].toString());
                metaproperty.write(object, sl);
                break;
            }
            default:
                // 其余数组类型尽量按 QVariant 直接写，避免把 QVariantList 误写成 QJsonArray
                QVariant variantList = jsonArray.toVariantList();
                if (variantList.canConvert(type)) {
                    variantList.convert(type);
                    metaproperty.write(object, variantList);
                } else {
                    metaproperty.write(object, jsonArray);
                }
            }
        }else if (metaTypeObj && metaTypeObj->inherits(&QJsonSerializable::staticMetaObject)) {
            /// @brief 对象字段优先走属性专用 FromJson。
            ///属性为 Q_PROPERTY_OBJECT
            QByteArray methodName = iter.key().toLatin1() + "FromJson";
            bool ok = QMetaObject::invokeMethod(object, methodName.constData(), Q_ARG(QJsonValue, iter.value()));
            if (ok) {
                continue;
            }
        }else if (type == QMetaType::QJsonObject){
             metaproperty.write(object, val.toObject());
        }else if (type == QMetaType::QVariantMap) {
            metaproperty.write(object, iter.value().toObject().toVariantMap());
        }else if (type == QMetaType::QJsonArray){
            metaproperty.write(object, val.toArray());
        }else if (type == QMetaType::QByteArray){
            // Base64 一定是字符串
            QString s = iter.value().toString();

            // 去掉可能的空白 / 换行（防御性）
            QByteArray raw = QByteArray::fromBase64(
                s.toUtf8(),
                QByteArray::Base64Encoding
                );

            if (raw.isEmpty() && !s.isEmpty()) {
                qWarning() << "Base64 decode failed";
            }

            metaproperty.write(object, raw);
        }else if (QString(QLatin1String("QVariant")).compare(QLatin1String(metaproperty.typeName())) == 0) {
            metaproperty.write(object, v);
        }else if (v.canConvert(type)) {
            v.convert(type);
            metaproperty.write(object, v);
        }else{
            qDebug()<<"序列化错误"<<metaproperty.name()<<type<<val;
        }
    }
}


/**
* This method converts a json string instance into a QObject
*
* @param variant Attributes to assign to the object.
* @param object The QObject instance to update.
*/
void QObjectHelper::json2qobject(const QString &json, QObject *object)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toStdString().data(), &error);
    if (error.error == QJsonParseError::NoError){
        QObjectHelper::qjsonobject2qobject(doc.object(), object);
    }else{
        qDebug() << error.errorString();
    }
}

bool QObjectHelper::save(const QObject *object, const QString &fpath)
{
    bool ret = false;
    QFile f(fpath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString json = QObjectHelper::qobject2json(object);
        f.write(json.toUtf8());
        ret = true;
    } else {
        qDebug() << "File[" << fpath << "]open error: " << f.errorString();
    }
    f.close();
    return ret;
}

bool QObjectHelper::load(const QString &fpath, QObject *object)
{
    bool ret = false;
    QFile f(fpath);
    if (f.open(QIODevice::ReadOnly)) {
        QString content = f.readAll();
        QObjectHelper::json2qobject(content, object);
        ret = true;
    } else {
        qDebug() << "File[" << fpath << "]open error: " << f.errorString();
    }
    f.close();
    return ret;
}
