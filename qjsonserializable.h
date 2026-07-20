#pragma once

#include <QtCore>
#include <QDebug>
#include "qobjecthelper.h"


/**
 * @brief 可序列化为 JSON 的基础对象类
 * 
 * 继承自 QObject，利用 QObjectHelper 提供了基于 Qt 属性机制的 JSON 序列化与反序列化能力。
 * 项目中需要和 JSON/QVariantMap 互转的数据模型可以直接继承此类。
 */
class QJsonSerializable : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList ignoreList READ ignoreList WRITE setIgnoreList)

public:
    Q_INVOKABLE explicit QJsonSerializable(QObject *parent = nullptr);

    /**
     * @brief 获取在序列化时被忽略的属性列表
     * @return 忽略的属性名列表（默认包含 "objectName"）
     */
    QStringList ignoreList() const;

    /**
     * @brief 设置在序列化时要忽略的属性列表
     * @param list 属性名列表
     */
    void setIgnoreList(const QStringList& list);

    // =========================================================================
    // 对象克隆 (Object Cloning)
    // =========================================================================

    /**
     * @brief 克隆当前对象
     * 
     * 通过导出为 QVariantMap 后再导入到新实例的方式实现对象的深度拷贝。
     * @tparam T 目标对象类型（必须继承自 QJsonSerializable）
     * @param parent 新对象的父节点
     * @return 克隆得到的新对象指针
     */
    template<typename T>
    T* clone(QObject* parent = nullptr)
    {
        T* obj = new T(parent);
        obj->fromVariantMap(this->variantMap(false));
        return obj;
    }

    // =========================================================================
    // 序列化操作 (Serialization)
    // =========================================================================

    /**
     * @brief 序列化对象为 JSON 字符串
     * @param applyIgnore 是否应用 ignoreList 来过滤忽略的属性，默认为 true
     * @return 序列化后的 JSON 字符串
     */
    inline QString json(bool applyIgnore = true) {
        return QObjectHelper::qobject2json(this, applyIgnore ? m_ignoreList : QStringList());
    }

    /**
     * @brief 序列化对象为 QJsonObject
     * @param applyIgnore 是否应用 ignoreList 来过滤忽略的属性，默认为 true
     * @return 序列化后的 QJsonObject
     */
    inline QJsonObject jsonObject(bool applyIgnore = true) {
        return QObjectHelper::qobject2qjsonobject(this, applyIgnore ? m_ignoreList : QStringList());
    }

    /**
     * @brief 序列化对象为 QVariantMap
     * @param applyIgnore 是否应用 ignoreList 来过滤忽略的属性，默认为 true
     * @return 序列化后的 QVariantMap
     */
    Q_INVOKABLE inline QVariantMap variantMap(bool applyIgnore = true) {
        return QObjectHelper::qobject2variantmap(this, applyIgnore ? m_ignoreList : QStringList());
    }

    /**
     * @brief 将对象序列化为二进制字节流 (UTF-8 编码的 JSON)
     * @param applyIgnore 是否应用 ignoreList 来过滤忽略的属性，默认为 true
     * @return 紧凑格式的 JSON 字节流
     */
    inline QByteArray toByteArray(bool applyIgnore = true) {
        return QJsonDocument(jsonObject(applyIgnore)).toJson(QJsonDocument::Compact);
    }

    // =========================================================================
    // 反序列化操作 (Deserialization)
    // =========================================================================

    /**
     * @brief 从 QVariantMap 字典中反序列化数据到当前对象
     * @param map 包含数据的 QVariantMap
     */
    Q_INVOKABLE void fromVariantMap(const QVariantMap& map);

    /**
     * @brief 从 QJsonValue 对象中反序列化数据到当前对象
     * @param jsonVal 包含数据的 QJsonValue
     */
    void fromJsonValue(const QJsonValue &jsonVal);

    /**
     * @brief 从 JSON 字符串中反序列化数据到指定目标 QObject
     * @param json JSON 字符串数据
     * @param object 目标解析对象指针
     */
    inline virtual void json2qobject(const QString json, QObject *object){
        QObjectHelper::json2qobject(json, object);
    }

    // =========================================================================
    // 文件操作与状态 (File I/O)
    // =========================================================================

    /**
     * @brief 将当前对象序列化并保存到指定文件
     * @param fpath 保存的文件路径
     * @return 保存成功返回 true，否则返回 false
     */
    bool save(const QString& fpath);

    /**
     * @brief 从指定文件加载 JSON 字符串并反序列化到当前对象
     * @param fpath JSON 文件路径
     * @return 加载解析成功返回 true，否则返回 false
     */
    virtual bool load(const QString& fpath);

    /**
     * @brief 获取当前对象是否已完成 load(文件) 加载
     * @return 如果已加载返回 true，否则返回 false
     */
    inline bool isLoadFinish(){
        return loadFinish_;
    }

protected:
    /**
     * @brief 数据加载完成后的钩子函数
     * 
     * 子类可以重写此方法，在从文件反序列化完成后执行额外的数据校验、
     * 关联计算或状态更新操作。
     */
    virtual void onLoadFinished(){}

protected:
    bool loadFinish_{true};
    QStringList m_ignoreList;

private:
    friend QDebug operator<<(QDebug dbg, const QJsonSerializable &obj);
};

QDebug operator<<(QDebug dbg, const QJsonSerializable &obj);
