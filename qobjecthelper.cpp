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
/**
* @brief Class used to convert QObject into QVariant and vivce-versa.
* During these operations only the class attributes defined as properties will
* be considered.
* Properties marked as 'non-stored' will be ignored.
*
* Suppose the declaration of the Person class looks like this:
* \code
* class Person : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString name READ name WRITE setName)
  Q_PROPERTY(int phoneNumber READ phoneNumber WRITE setPhoneNumber)
  Q_PROPERTY(Gender gender READ gender WRITE setGender)
  Q_PROPERTY(QDate dob READ dob WRITE setDob)
  Q_ENUMS(Gender)

 public:
    Person(QObject* parent = 0);
    ~Person();

    QString name() const;
    void setName(const QString& name);

    int phoneNumber() const;
    void setPhoneNumber(const int  phoneNumber);

    enum Gender {Male, Female};
    void setGender(Gender gender);
    Gender gender() const;

    QDate dob() const;
    void setDob(const QDate& dob);

  private:
    QString m_name;
    int m_phoneNumber;
    Gender m_gender;
    QDate m_dob;
};
\endcode

The following code will serialize an instance of Person to JSON :

\code
    Person person;
    person.setName("Flavio");
    person.setPhoneNumber(123456);
    person.setGender(Person::Male);
    person.setDob(QDate(1982, 7, 12));

    QString  result = QObjectHelper::qobject2json(&person);
    QObjectHelper::json2qobject(result, &person);
    qDebug() << result
    qDebug() << person;
\endcode

The generated output will be:
\code
    "{\n    \"dob\": \"1982-07-12\",\n    \"gender\": 0,\n    \"name\": \"Flavio\",\n    \"phoneNumber\": 123456\n}\n"
    Person({"name":"Flavio","phoneNumber":"123456","gender":"0","dob":"1982-07-12"})
\endcode
*/


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
* @param ignoredProperties Properties that won't be converted.
*/
QJsonObject QObjectHelper::qobject2qjsonobject( const QObject* object,
                              const QStringList& ignoredProperties)
{
    QJsonObject result;
    if (!object)
        return result;

    const QMetaObject *metaobject = object->metaObject();
    int count = metaobject->propertyCount();
    for (int i=0; i<count; ++i) {
        QMetaProperty metaproperty = metaobject->property(i);
        const char *name = metaproperty.name();

        if (ignoredProperties.contains(QLatin1String(name)) || (!metaproperty.isReadable()))
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

QVariantMap QObjectHelper::qobject2variantmap(const QObject* object,
                                              const QStringList& ignoredProperties)
{
    QVariantMap result;
    if (!object) return result;

    const QMetaObject* metaobject = object->metaObject();
    int count = metaobject->propertyCount();

    for (int i = 0; i < count; ++i) {
        QMetaProperty metaproperty = metaobject->property(i);
        const char* name = metaproperty.name();

        if (!metaproperty.isReadable())
            continue;
        if (ignoredProperties.contains(QLatin1String(name)))
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
* @param ignoredProperties Properties that won't be converted.
*/
QString QObjectHelper::qobject2json(const QObject *object, const QStringList &ignoredProperties)
{
    QString result("");
    QJsonObject jsonobj = QObjectHelper::qobject2qjsonobject(object, ignoredProperties);
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
    const QMetaObject *metaobject = object->metaObject();
    QJsonObject::const_iterator iter;
    for (iter = jsonobj.constBegin(); iter != jsonobj.constEnd(); ++iter) {
        int pIdx = metaobject->indexOfProperty(iter.key().toLatin1());

        if (pIdx < 0) {
            continue;
        }
        QMetaProperty metaproperty = metaobject->property(pIdx);
        QVariant::Type type = metaproperty.type();
        QVariant v(iter.value());

        if (iter.value().isObject()) {
            /**
             * @brief 对象字段优先走属性专用 FromJson。
             * @details 例如 parameterFromJson 可继续把第二层 channels 交给 channelsFromJson。
             */
            QByteArray methodName = iter.key().toLatin1() + "FromJson";
            if (QMetaObject::invokeMethod(object, methodName.constData(),
                                          Q_ARG(QJsonValue, iter.value()))) {
                continue;
            }
        }

        if (iter.value().isArray() && type != QMetaType::QStringList) {
            QByteArray methodName = iter.key().toLatin1() + "FromJson";

            // 先优先走 xxxFromJson(QJsonArray)，用于 QList<QObject*> / 自定义列表属性
            bool ok = QMetaObject::invokeMethod(object, methodName.constData(),
                                                Q_ARG(QJsonArray, iter.value().toArray()));

            if (!ok) {
                QJsonArray ja = iter.value().toArray();

                if (type == QMetaType::QVariantList) {
                    metaproperty.write(object, ja.toVariantList());
                } else if (type == QMetaType::QJsonArray) {
                    metaproperty.write(object, ja);
                } else {
                    // 其余数组类型尽量按 QVariant 直接写，避免把 QVariantList 误写成 QJsonArray
                    QVariant arrayVariant = ja.toVariantList();
                    if (arrayVariant.canConvert(type)) {
                        arrayVariant.convert(type);
                        metaproperty.write(object, arrayVariant);
                    } else {
                        metaproperty.write(object, ja);
                    }
                }
            }
        }  else if (type == QMetaType::QJsonObject){
             QJsonValue jv(iter.value());
             metaproperty.write(object, jv.toObject());
        } else if (type == QVariant::Map && iter.value().isObject()) {
            metaproperty.write(object, iter.value().toObject().toVariantMap());
        } else if (type == QMetaType::QJsonArray){
            QJsonValue jv(iter.value());
            metaproperty.write(object, jv.toArray());
        } else if (type == QMetaType::QByteArray){
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
        }else if (type == QMetaType::QStringList && iter->type() == QJsonValue::Array){
            QJsonValue jv(iter.value());
            QJsonArray ja = jv.toArray();
            QStringList sl;
            for (int i=0; i<ja.size(); i++)
            {
                sl.append(ja[i].toString());
            }
            metaproperty.write(object, sl);
        }else if (metaproperty.userType() >= QMetaType::User) {
            const int typeId = metaproperty.userType();
            const QMetaObject* mo = QMetaType::metaObjectForType(typeId);

            if (!mo) {
                qWarning() << "No metaObject for type:" << metaproperty.typeName();
                continue;
            }

            // newInstance() 要求有 Q_INVOKABLE 构造函数
            QObject* obj = mo->newInstance(Q_ARG(QObject*, object));
            if (!obj) {
                qWarning() << "newInstance() 失败，检查构造函数是否有 Q_INVOKABLE:"
                           << metaproperty.typeName();
                continue;
            }

            QJsonSerializable* helper = qobject_cast<QJsonSerializable*>(obj);
            if (!helper) {
                qWarning() << "类型未继承 QJsonSerializable:" << metaproperty.typeName();
                delete obj;
                continue;
            }

            helper->fromJsonValue(iter.value());

            // ⚠️ 关键修复：属性元类型是 FileCollection* 这种具体子类指针，
            // 而 QVariant::fromValue(QObject*) 装出来的 userType 是 QObject*，
            // 直接 write 会因类型不匹配静默失败，导致 setter 不被触发、字段全丢。
            // 这里按 metaproperty.userType() 构造同类型 QVariant，再写入。
            QVariant var(typeId, &obj);
            if (!metaproperty.write(object, var)) {
                qWarning() << "写入失败:" << iter.key()
                           << "type:" << metaproperty.typeName();
                delete obj;
            }
            // 写入成功后由 setter 内部接管/拷贝；这里 obj 仍是临时对象，
            // newInstance 时已经设过 parent=object，会随 object 一起销毁。
        }
        else if (v.canConvert(type)) {
            v.convert(type);
            metaproperty.write(object, v);
        }else if (QString(QLatin1String("QVariant")).compare(QLatin1String(metaproperty.typeName())) == 0) {
            metaproperty.write(object, v);
        }
        else if (iter->type() == QJsonValue::String) {
            QJsonValue jv(iter.value());
            QVariant v1(jv.toString());
            if (v1.canConvert(type)) {
                v1.convert(type);
                metaproperty.write(object, v1);
            }
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

bool QObjectHelper::save(const QObject *object, const QString &fpath, const QStringList &ignoredProperties)
{
    bool ret = false;
    QFile f(fpath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString json = QObjectHelper::qobject2json(object, ignoredProperties);
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
