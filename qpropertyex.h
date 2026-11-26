#pragma once
#include <QMetaType>
#include <QQmlProperty>
#include <QQmlListProperty>

#define Q_PROPERTY_AUTO(TYPE, NAME) \
Q_PROPERTY_AUTO_STORED(TYPE, NAME, true)

#define Q_PROPERTY_AUTO_STORED(TYPE, NAME, STORE)                                           \
Q_PROPERTY(TYPE NAME READ NAME WRITE set##NAME NOTIFY NAME##Changed STORED STORE)       \
    public:                                                                             \
    Q_SIGNAL void NAME##Changed(const TYPE &);                                          \
    TYPE NAME() const {                                                                 \
        return m_##NAME;                                                                \
}                                                                                       \
    void set##NAME(const TYPE &value) {                                                 \
        if (m_##NAME == value)                                                          \
        return;                                                                         \
        m_##NAME = value;                                                               \
        emit NAME##Changed(m_##NAME);                                                   \
}                                                                                       \
    private:                                                                            \
    TYPE m_##NAME;

// QObject 指针型自动属性（Qt 5 / QML 安全）
// 适用于 Type : QObject
#define Q_PROPERTY_AUTO_P(TYPE, M) \
Q_PROPERTY_AUTO_P_STORED(TYPE, M, true)

#define Q_PROPERTY_AUTO_P_STORED(TYPE, M, STORE)                                                                 \
    Q_PROPERTY(TYPE M MEMBER m_##M NOTIFY M##Changed STORED STORE)                                              \
public:                                                                                            \
    Q_SIGNAL void M##Changed();                                                                    \
    void M(TYPE in_##M) {                                                                          \
        if (in_##M == m_##M)                                                                       \
            return;                                                                                \
        m_##M = in_##M;                                                                            \
        Q_EMIT M##Changed();                                                                       \
    }                                                                                              \
    TYPE M() const {                                                                               \
        return m_##M;                                                                              \
    }                                                                                              \
private:                                                                                           \
    TYPE m_##M = nullptr;


#define Q_PROPERTY_AUTOINIT(TYPE, NAME, DEFAULT_VALUE) \
Q_PROPERTY_AUTOINIT_STORED(TYPE, NAME, DEFAULT_VALUE, true)

#define Q_PROPERTY_AUTOINIT_STORED(TYPE, NAME, DEFAULT_VALUE, STORE)                        \
Q_PROPERTY(TYPE NAME READ NAME WRITE set##NAME NOTIFY NAME##Changed STORED STORE)       \
    public:                                                                             \
    Q_SIGNAL void NAME##Changed(const TYPE &);                                          \
    TYPE NAME() const {                                                                 \
        return m_##NAME;                                                                \
}                                                                                       \
    void set##NAME(const TYPE &value) {                                                 \
        if (m_##NAME == value)                                                          \
        return;                                                                         \
        m_##NAME = value;                                                               \
        emit NAME##Changed(m_##NAME);                                                   \
}                                                                                       \
    private:                                                                            \
    TYPE m_##NAME{DEFAULT_VALUE};

#define Q_PROPERTY_AUTOGEN_VIRTUAL(TYPE, NAME, DEFAULT_VALUE) \
Q_PROPERTY_AUTOGEN_VIRTUAL_STORED(TYPE, NAME, DEFAULT_VALUE, true)

#define Q_PROPERTY_AUTOGEN_VIRTUAL_STORED(TYPE, NAME, DEFAULT_VALUE, STORE)                 \
Q_PROPERTY(TYPE NAME READ NAME WRITE set##NAME NOTIFY NAME##Changed STORED STORE)       \
    public:                                                                             \
    Q_SIGNAL void NAME##Changed(const TYPE &);                                          \
    virtual TYPE NAME() const {                                                         \
        return m_##NAME;                                                                \
}                                                                                       \
    virtual void set##NAME(const TYPE &value) {                                         \
        if (m_##NAME == value)                                                          \
        return;                                                                         \
        m_##NAME = value;                                                               \
        emit NAME##Changed(m_##NAME);                                                   \
}                                                                                       \
    protected:                                                                          \
    TYPE m_##NAME = DEFAULT_VALUE;

/*
 * Q_PROPERTY_QML（VariantMap / 单对象模型）
 *
 * 设计目标：
 * 1. 对外暴露 NAME（QVariantMap，数据态），用于 QML / JSON / 参数整体赋值；
 * 2. 对外暴露 NAMEQml（TYPE*，对象态），用于 QML 直接访问和修改字段；
 * 3. 给 NAME 赋值时，自动创建 TYPE 并调用 fromVariantMap 反序列化；
 * 4. QML 首次访问 NAMEQml 时，自动 lazy 初始化 TYPE（自动 new）；
 * 5. 修改 TYPE 后，调用 syncNAME() 可将对象态序列化回 NAME（QVariantMap）；
 * 6. 不在 getNAME() 中产生副作用，行为与 Q_PROPERTY_QMLLIST 保持一致；
 * 7. 适用于 TYPE 继承 QJsonSerializable / QObjectHelper 体系的工程化数据模型。
 *
 */
#define Q_PROPERTY_QML(TYPE, NAME) \
Q_PROPERTY_QML_STORED(TYPE, NAME, true)

#define Q_PROPERTY_QML_STORED(TYPE, NAME, STORE)                                                \
    Q_PROPERTY(QVariantMap NAME READ get##NAME WRITE set##NAME NOTIFY NAME##Changed STORED STORE)\
    public:                                                                                 \
    Q_SIGNAL void NAME##Changed();                                                          \
    QVariantMap get##NAME() {                                                               \
        if (m_##NAME == nullptr) {                                                          \
            m_##NAME = new TYPE(this);                                                      \
        }                                                                                   \
        return QObjectHelper::qobject2variantmap(m_##NAME);                                 \
    }                                                                                       \
    void set##NAME(const QVariantMap& value) {                                              \
        if (m_##NAME) {                                                                     \
            m_##NAME->deleteLater();                                                        \
            m_##NAME = nullptr;                                                             \
        }                                                                                   \
        if (!value.isEmpty()) {                                                             \
            m_##NAME = new TYPE(this);                                                      \
            m_##NAME->fromVariantMap(value);                                                \
        }                                                                                   \
        emit NAME##Changed();                                                               \
    }                                                                                       \
    bool update##NAME(const QVariantMap& value) {                                           \
        if (m_##NAME) {                                                                     \
            m_##NAME->fromVariantMap(value);                                                \
            emit NAME##Changed();                                                           \
            return true;                                                                    \
        }                                                                                   \
        return false;                                                                       \
    }                                                                                       \
    private:                                                                                \
    TYPE* m_##NAME = nullptr;

#define Q_PROPERTY_OBJECT(TYPE, NAME) \
Q_PROPERTY_OBJECT_STORED(TYPE, NAME, true)

#define Q_PROPERTY_OBJECT_STORED(TYPE, NAME, STORE)                                      \
    Q_PROPERTY(TYPE* NAME READ get##NAME WRITE set##NAME NOTIFY NAME##Changed STORED STORE)\
public:                                                                     \
    Q_SIGNAL void NAME##Changed();                                          \
    TYPE* get##NAME() {                                                     \
        if (!m_##NAME) {                                                    \
            m_##NAME = new TYPE(this);                                      \
        }                                                                   \
        return m_##NAME;                                                    \
    }                                                                       \
    void set##NAME(TYPE* value) {                                           \
        if (m_##NAME == value)                                              \
            return;                                                          \
        if (!m_##NAME)                                                      \
            m_##NAME = new TYPE(this);                                      \
        m_##NAME->fromVariantMap(value->variantMap());                      \
        emit NAME##Changed();                                               \
    }                                                                       \
    /* ===== JSON (单对象版本) ===== */                                      \
    Q_INVOKABLE QJsonObject NAME##ToJson() const {                          \
        if (!m_##NAME)                                                      \
            return QJsonObject();                                           \
        return m_##NAME->jsonObject();                                      \
    }                                                                       \
    Q_INVOKABLE void NAME##FromJson(const QJsonValue& val) {                \
        if (!m_##NAME)                                                      \
            m_##NAME = new TYPE(this);                                      \
        m_##NAME->fromJsonValue(val);                                       \
        emit NAME##Changed();                                               \
    }                                                                       \
private:                                                                    \
    TYPE* m_##NAME = nullptr;

/**
 * @brief Qt 5.15 的 QML 列表属性宏（QQmlListProperty 静态函数实现版）
 *
 * 该宏用于快速在 C++ ViewModel 中声明一个可被 QML 直接操作的对象列表属性，
 * 基于 QQmlListProperty<T>，完整支持 QML 侧的增 / 删 / 查 / 清空等行为，
 * 同时在 C++ 侧维护真实的 QList<T*> 数据结构。
 *
 * 设计目标：
 * - 兼容 Qt 5.15（不依赖 lambda / std::function）
 * - 兼容 MSVC2019 编译器
 * - 适合“基础框架层 / ViewModel 层”长期复用
 *
 * 核心机制说明：
 * - QQmlListProperty::data 指向内部 QList<T*>，避免依赖具体类类型
 * - 所有回调函数使用 static 函数，符合 Qt 元对象系统要求
 * - 通过 QMetaObject::invokeMethod 触发 NAMEChanged 信号，解耦具体类名
 *
 * 对 QML 暴露的能力：
 * - 列表属性：NAME
 * - 创建元素：NAMECreate()
 * - 删除元素：NAMERemoveAt(index)
 * - 清空列表：NAMEClearAll()
 * - 按对象指针查索引：NAMEIndexOf(item)
 *
 * 数据序列化支持：
 * - NAMEToJson()   ：对象列表 → QJsonArray
 * - NAMEFromJson() ：QJsonArray → 对象列表（重建）
 *
 * 使用约定：
 * - TYPE 必须继承 QObject
 * - TYPE 必须实现：
 *     - QJsonObject jsonObject() const
 *     - void fromJsonValue(const QJsonValue &)
 * - 列表中对象的生命周期由该列表统一管理
 *
 * @param TYPE 列表元素类型（如 FilterCard）
 * @param NAME 列表属性名（如 filterCardList）
 */
#define Q_PROPERTY_LIST_AUTO(TYPE, NAME) \
Q_PROPERTY_LIST_AUTO_STORED(TYPE, NAME, true)

#define Q_PROPERTY_LIST_AUTO_STORED(TYPE, NAME, STORE)                                                    \
    Q_CLASSINFO("JsonList_"#NAME, "true") \
    Q_PROPERTY(QList<QObject*> NAME READ NAME##QML NOTIFY NAME##Changed STORED STORE)       \
public:                                                                                     \
    Q_SIGNAL void NAME##Changed();                                                          \
    /* 只读暴露给 QML（QML 当 var 用） */                                                     \
    QList<QObject*> NAME##QML() {                                               \
        QList<QObject*> res;                                                                \
        res.reserve(m_##NAME.size());                                                       \
        for (TYPE* item : m_##NAME) {                                                       \
            res.append(static_cast<QObject*>(item));                                        \
        }                                                                                   \
        return res;                                                                         \
    }                                                                                       \
    QList<TYPE*> NAME() {                                                                   \
        return m_##NAME;                                                                    \
    }                                                                                       \
    /* 一次性设置整个列表 */                                                                 \
    Q_INVOKABLE void NAME##SetAll(const QList<TYPE*> &list) {                               \
        for (auto *item : m_##NAME) {                                                   \
            if (item)                                                                   \
                item->deleteLater();                                                        \
        }                                                                                   \
        m_##NAME.clear();                                                                   \
        for (auto *item : list) {                                                           \
            if (item) item->setParent(this);                                               \
            m_##NAME.append(item);                                                          \
        }                                                                                   \
        emit NAME##Changed();                                                               \
    }                                                                                       \
    /* 创建并添加（推荐用法） */                                                               \
    Q_INVOKABLE TYPE* NAME##Create() {                                                      \
        auto *item = new TYPE(this);                                                        \
        m_##NAME.append(item);                                                              \
        emit NAME##Changed();                                                               \
        return item;                                                                        \
    }                                                                                       \
    /* 追加已有对象 */                                                                      \
    Q_INVOKABLE void NAME##Append(TYPE* item) {                                             \
        if (!item) return;                                                                  \
        m_##NAME.append(item);                                                              \
        emit NAME##Changed();                                                               \
    }                                                                                       \
    /* 按 index 删除 */                                                                     \
    Q_INVOKABLE void NAME##RemoveAt(int index) {                                            \
        if (index < 0 || index >= m_##NAME.size()) return;                                  \
        TYPE* obj = m_##NAME.takeAt(index);                                                 \
        obj->deleteLater();                                                                 \
        emit NAME##Changed();                                                               \
    }                                                                                       \
    Q_INVOKABLE void NAME##ClearAll() {                                                     \
        for (auto *item : m_##NAME) {                                                   \
            if (item)                                                                   \
                item->deleteLater();                                                        \
        }                                                                                   \
        m_##NAME.clear();                                                                   \
        emit NAME##Changed();                                                               \
    }                                                                                       \
    /* 仅供 C++ 使用，不推荐 QML 调 */                                                       \
    int NAME##IndexOf(TYPE* item) const {                                                   \
        return m_##NAME.indexOf(item);                                                      \
    }                                                                                       \
    /* ===== JSON ===== */                                                                \
    Q_INVOKABLE QJsonArray NAME##ToJson() const {                                           \
        QJsonArray arr;                                                                   \
        for (auto *item : m_##NAME)                                                       \
            arr.append(item ? item->jsonObject() : QJsonObject());                          \
        return arr;                                                                       \
    }                                                                                     \
    Q_INVOKABLE void NAME##FromJson(const QJsonArray &arr) {                              \
        for (auto *item : m_##NAME) {                                                   \
            if (item)                                                                   \
                item->deleteLater();                                                        \
        }                                                                                   \
        m_##NAME.clear();                                                                   \
        for (const auto &v : arr) {                                                         \
            auto *item = new TYPE(this);                                                    \
            item->fromJsonValue(v.toObject());                                              \
            m_##NAME.append(item);                                                          \
        }                                                                                   \
        emit NAME##Changed();                                                               \
    }                                                                                       \
private:                                                                                    \
    QList<TYPE*> m_##NAME;
