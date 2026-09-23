#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include "type.h"

using TableId = uint32_t;
using IndexId = uint32_t;
using ColumnId = uint16_t;

struct Column {
    ColumnId id{};
    std::string name;
    Type type;

    bool nullable = true;

    std::optional<std::string> default_expr;
};

enum class IndexKind : uint8_t {
    BPlusTree
};

struct IndexSchema {
    IndexId id{};
    std::string name;
    IndexKind kind = IndexKind::BPlusTree;

    // columns (in order for indexes in tuple)
    std::vector<ColumnId> key_columns;

    bool unique = false;
    bool is_primary = false;

    // indexed
    std::vector<ColumnId> include_columns;
};


struct TableSchema {
    TableId id{};
    std::string name;

    std::vector<Column> columns;
    std::vector<IndexSchema> indexes;
    std::optional<IndexId> primary_index_id;

    const Column& column_by_id(ColumnId cid) const {
        auto it = std::find_if(columns.begin(), columns.end(), [&](const Column& c){ return c.id == cid; });
        if (it == columns.end()) throw std::runtime_error("column_by_id: unknown column id");
        return *it;
    }

    const Column& column_by_name(const std::string& n) const {
        auto it = std::find_if(columns.begin(), columns.end(), [&](const Column& c){ return c.name == n; });
        if (it == columns.end()) throw std::runtime_error("column_by_name: unknown column name: " + n);
        return *it;
    }

    const IndexSchema* primary_index() const {
        if (!primary_index_id) return nullptr;
        auto it = std::find_if(indexes.begin(), indexes.end(), [&](const IndexSchema& ix){ return ix.id == *primary_index_id; });
        return (it == indexes.end()) ? nullptr : &*it;
    }
};


class TableSchemaBuilder {
public:
    TableSchemaBuilder(TableId id, std::string name)
        : schema_{id, std::move(name)} {}

    ColumnId add_column(std::string name, Type type, bool nullable = true,
                        std::optional<std::string> default_expr = std::nullopt) {

        for (auto& c : schema_.columns) {
            if (c.name == name) throw std::runtime_error("duplicate column name: " + name);
        }

        ColumnId cid = next_col_id_++;
        schema_.columns.push_back(Column{cid, std::move(name), type, nullable, std::move(default_expr)});
        return cid;
    }

    // unique b+-tree
    IndexId add_unique_index(std::string name, const std::vector<ColumnId>& key_cols,
                             const std::vector<ColumnId>& include_cols = {}) {
        return add_index_impl(std::move(name), key_cols, true, false, include_cols);
    }

    IndexId set_primary_key(std::string index_name, const std::vector<ColumnId>& key_cols) {
        if (schema_.primary_index_id) throw std::runtime_error("primary key already set");

        // not null forced on pk
        for (ColumnId cid : key_cols) {
            auto it = std::find_if(schema_.columns.begin(), schema_.columns.end(), [&](const Column& c){ return c.id == cid; });
            if (it == schema_.columns.end()) {
                throw std::runtime_error("PK references unknown column id");
            }
            it->nullable = false;
        }

        IndexId ixid = add_index_impl(std::move(index_name), key_cols, true, true, {});
        schema_.primary_index_id = ixid;
        return ixid;
    }

    const TableSchema& build() const { return schema_; }
    TableSchema&& move_build() { return std::move(schema_); }

private:
    IndexId add_index_impl(std::string name, const std::vector<ColumnId>& key_cols,
                           bool unique, bool is_primary,
                           const std::vector<ColumnId>& include_cols) {

        for (auto& ix : schema_.indexes) {
            if (ix.name == name) throw std::runtime_error("duplicate index name: " + name);
        }
        if (key_cols.empty()) throw std::runtime_error("index must have at least one key column");

        IndexId id = next_index_id_++;
        schema_.indexes.push_back(IndexSchema{
            id,
            std::move(name),
            IndexKind::BPlusTree,
            key_cols,
            unique,
            is_primary,
            include_cols
        });
        return id;
    }

    TableSchema schema_;
    ColumnId next_col_id_ = 0;
    IndexId  next_index_id_ = 0;
};
