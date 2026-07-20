#ifndef QOBJECTHELPER_H
#define QOBJECTHELPER_H

#include <QtCore/QLatin1String>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

/**
 * @brief QObject 与 JSON 数据转换的核心辅助类
 * 
 * 提供了一系列静态方法，利用 Qt 的元对象系统 (Meta-Object System)，
 * 实现 QObject 的属性 (Properties) 与 JSON 对象、字符串、以及 QVariantMap 之间的互相转换。
 */
class QObjectHelper {
public:
    QObjectHelper();
    ~QObjectHelper();

    // =========================================================================
    // 序列化 (Serialization: QObject -> JSON)
    // =========================================================================

    /**
     * @brief 将 QObject 对象转换为 QJsonObject
     * @param object 待转换的 QObject 对象指针
     * @param ignoredProperties 需要忽略不进行序列化的属性名列表（默认为 "objectName"）
     * @return 转换后得到的 QJsonObject
     */
    static QJsonObject qobject2qjsonobject(const QObject* object,
                                           const QStringList& ignoredProperties = QStringList(QString(QLatin1String("objectName"))));

    /**
     * @brief 将 QObject 对象转换为 QVariantMap
     * @param object 待转换的 QObject 对象指针
     * @param ignoredProperties 需要忽略不进行序列化的属性名列表（默认为 "objectName"）
     * @return 转换后得到的 QVariantMap
     */
    static QVariantMap qobject2variantmap(const QObject* object,
                                          const QStringList& ignoredProperties = QStringList(QStringLiteral("objectName")));

    /**
     * @brief 将 QObject 对象转换为 JSON 字符串格式
     * @param object 待转换的 QObject 对象指针
     * @param ignoredProperties 需要忽略不进行序列化的属性名列表（默认为 "objectName"）
     * @return 转换后得到的 JSON 字符串
     */
    static QString qobject2json(const QObject* object,
                                const QStringList& ignoredProperties = QStringList(QString(QLatin1String("objectName"))));


    // =========================================================================
    // 反序列化 (Deserialization: JSON -> QObject)
    // =========================================================================

    /**
     * @brief 将 QJsonObject 的数据反序列化到指定的 QObject 对象中
     * @param jsonobj 包含数据的 QJsonObject
     * @param object 目标 QObject 对象指针，其对应的属性将会被覆写更新
     */
    static void qjsonobject2qobject(const QJsonObject &jsonobj, QObject* object);

    /**
     * @brief 将 JSON 字符串反序列化到指定的 QObject 对象中
     * @param json 包含数据的 JSON 字符串
     * @param object 目标 QObject 对象指针，其对应的属性将会被覆写更新
     */
    static void json2qobject(const QString& json, QObject* object);


    // =========================================================================
    // 文件操作 (File I/O)
    // =========================================================================

    /**
     * @brief 将 QObject 对象的内容序列化后写入到指定文件中
     * @param object 待写入的 QObject 对象指针
     * @param fpath 目标文件路径
     * @param ignoredProperties 需要忽略不进行序列化的属性名列表（默认为 "objectName"）
     * @return 保存成功返回 true，否则返回 false
     */
    static bool save(const QObject* object, const QString& fpath, const QStringList& ignoredProperties = QStringList(QString(QLatin1String("objectName"))));

    /**
     * @brief 从指定文件加载 JSON 字符串并反序列化到指定的 QObject 对象中
     * @param fpath 源文件路径
     * @param object 接收数据的目标 QObject 对象指针
     * @return 加载解析成功返回 true，否则返回 false
     */
    static bool load(const QString& fpath, QObject* object);

private:
    Q_DISABLE_COPY(QObjectHelper)
    class QObjectHelperPrivate;
    QObjectHelperPrivate* const d;
};

#endif // QOBJECTHELPER_H
