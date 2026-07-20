#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include "qjsonserializable.h"

template<class T>
class ObjectListModel : public QAbstractListModel
{
public:
    static_assert(std::is_base_of<QObject, T>::value,
                  "T must inherit QObject");
    static_assert(std::is_base_of<QJsonSerializable, T>::value,
                  "T must inherit QJsonSerializable");

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    explicit ObjectListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent) {}

    // ===== Model 基本接口（QAbstractListModel 必须）=====
    int rowCount(const QModelIndex &) const override {
        return m_items.size();
    }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
            return {};

        if (role == ObjectRole)
            return QVariant::fromValue(m_items.at(index.row()));

        return {};
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            { ObjectRole, "object" }
        };
    }

protected:
    // ===== C++ 内部操作（不给 QML 直接用）=====
    T* createImpl() {
        auto *item = new T();
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(item);
        endInsertRows();
        return item;
    }

    void appendImpl(T* item) {
        if (!item) return;
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(item);
        endInsertRows();
    }

    void removeAtImpl(int index) {
        if (index < 0 || index >= m_items.size()) return;
        beginRemoveRows({}, index, index);
        delete m_items.takeAt(index);
        endRemoveRows();
    }

    void clearImpl() {
        beginResetModel();
        qDeleteAll(m_items);
        m_items.clear();
        endResetModel();
    }

    int countImpl() const {
        return m_items.size();
    }

    T* getImpl(int index) const {
        return (index >= 0 && index < m_items.size()) ? m_items.at(index) : nullptr;
    }

    int indexOfImpl(T* item) const {
        return m_items.indexOf(item);
    }

    // ===== JSON =====
    QJsonArray toJsonImpl() const {
        QJsonArray arr;
        for (auto *item : m_items)
            arr.append(item->json());
        return arr;
    }

    void fromJsonImpl(const QJsonArray &arr) {
        beginResetModel();
        qDeleteAll(m_items);
        m_items.clear();

        for (const auto &v : arr) {
            auto *item = new T(this);
            item->fromJsonValue(v.toObject());
            m_items.append(item);
        }
        endResetModel();
    }

protected:
    QList<T*> m_items;
};
