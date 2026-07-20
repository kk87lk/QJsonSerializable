# QJsonSerializable

一个强大且轻量级的 Qt C++ JSON 序列化与 QML 属性增强库。它不仅让你摆脱手动编写冗长的 `fromJson` 和 `toJson` 的烦恼，更提供了丰富的宏定义，一键实现属性与 QML 的双向绑定及高级数据结构的封装。

## 核心特性

- **继承即用**：C++ 类继承 `QJsonSerializable`，自动获得与 JSON 的双向序列化/反序列化能力。
- **丰富的属性宏**：支持快速生成带有 `READ`, `WRITE`, `NOTIFY` 的规范 Qt 属性。
- **QML 列表属性全自动映射**：使用 `Q_PROPERTY_QMLLIST` 等宏，一键将 C++ 的对象列表 `QList<T*>` 封装为 QML 友好的 `QJsonArray` 并提供开箱即用的增删改查（CRUD）接口。
- **无感知的 JSON 同步**：提供如 `Q_PROPERTY_QML` 的能力，当你在 QML 中设置属性时，自动映射并反序列化回底层 C++ 对象。

## 如何接入

直接在您的 `.pro` 或 `.pri` 工程文件中 include 本模块即可：
```qmake
include($$PWD/QJsonSerializable/QJsonSerializable.pri)
```

## 使用示例

### 1. 基础属性宏 (替代手写冗长的 Getter/Setter)
```cpp
#include "qpropertyex.h"

class MyData : public QObject {
    Q_OBJECT
    // 自动生成 m_id, id(), setId(), idChanged() 信号
    Q_PROPERTY_AUTO(int, id)
    Q_PROPERTY_AUTO(QString, name)
};
```

### 2. 结合 JSON 的自动序列化
```cpp
#include "qjsonserializable.h"

class UserInfo : public QJsonSerializable {
    Q_OBJECT
    Q_PROPERTY_AUTO(QString, username)
    Q_PROPERTY_AUTO(int, age)
};

// 序列化
UserInfo user;
user.setusername("Admin");
user.setage(25);
QJsonObject json = user.jsonObject();

// 反序列化
UserInfo user2;
user2.fromJsonValue(json);
```

### 3. 高级 QML 列表绑定 (`Q_PROPERTY_QMLLIST`)
对于需要在 QML 中展示表格并进行持久化的场景，手动写 `QQmlListProperty` 会非常繁琐，本库提供 `Q_PROPERTY_QMLLIST` 宏解决痛点：

```cpp
class UserListModel : public QObject {
    Q_OBJECT
    // 自动维护一个 QList<UserInfo*>，并在 QML 中暴露为 JsonArray，
    // 同时提供 usersCount(), usersGetAt(), usersAppend(), usersRemove() 等接口
    Q_PROPERTY_QMLLIST(UserInfo, users)
};
```
在 QML 中可以直接如此调用：
```qml
// 获取用户列表长度
console.log(model.usersCount())
// 追加一个新用户 (传入 JS 对象)
model.usersAppend({ "username": "NewUser", "age": 20 })
```

## 适用场景
- **工业软件 / 配置编辑器**：参数的保存、加载以及在界面上的双向同步。
- **网络接口映射**：接收后端的 JSON 数据，直接由框架转换为强类型 C++ 对象列表。
- **QML 前端展示**：自动化的信号绑定机制，让你完全不需要手写 boilerplate 代码。
