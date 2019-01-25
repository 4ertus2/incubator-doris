// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <vector>

#include "olap/tablet_schema.h"
#include "olap/field_info.h"

namespace doris {

TabletColumn::TabletColumn() {}

TabletColumn::TabletColumn(FieldAggregationMethod agg, FieldType type) {
    _aggregation = agg;
    _type = type;
}

OLAPStatus TabletColumn::init_from_pb(const ColumnPB& column) {
    _unique_id = column.unique_id(); 
    _col_name = column.name();
    _type = FieldInfo::get_field_type_by_string(column.type());
    _is_key = column.is_key();
    _is_nullable = column.is_nullable();

    _has_default_value = column.has_default_value(); 
    if (_has_default_value) {
        _default_value = column.default_value();
    }

    if (column.has_precision()) {
        _is_decimal = true;
        _precision = column.precision();
    }
    if (column.has_frac()) {
        _precision = column.frac();
    }
    _length = column.length();
    _index_length = column.length();
    _is_bf_column = column.is_bf_column();
    _has_referenced_column = column.has_referenced_column_id();
    if (_has_referenced_column) {
        _referenced_column_id = column.referenced_column_id();
    }
    return OLAP_SUCCESS;
}

OLAPStatus TabletColumn::to_schema_pb(ColumnPB* column) {
    column->set_unique_id(_unique_id);
    column->set_name(_col_name);
    column->set_type(FieldInfo::get_string_by_field_type(_type));
    column->set_is_key(_is_key);
    column->set_is_nullable(_is_nullable);
    if (_is_decimal) {
        column->set_precision(_precision);
        column->set_frac(_frac);
    }
    column->set_length(_length);
    column->set_index_length(_index_length);
    column->set_is_bf_column(_is_bf_column);
    if (_has_referenced_column)
    column->set_referenced_column_id(_referenced_column_id);
    return OLAP_SUCCESS;
}

TabletSchema::TabletSchema()
    : _num_columns(0),
      _num_key_columns(0),
      _num_null_columns(0),
      _num_short_key_columns(0) { }

OLAPStatus TabletSchema::init_from_pb(const TabletSchemaPB& schema) {
    for (auto& column_pb : schema.column()) {
        TabletColumn column;
        column.init_from_pb(column_pb);
        _cols.push_back(column);
        _num_columns++;
        if (column.is_key()) {
            _num_key_columns++;
        }
        if (column.is_nullable()) {
            _num_null_columns++;
        }
    }
    _num_short_key_columns = schema.num_short_key_columns();
    _num_rows_per_row_block = schema.num_rows_per_row_block();
    _keys_type = schema.keys_type();
    _compress_kind = schema.compress_kind();
    _next_column_unique_id = schema.next_column_unique_id();
    if (schema.has_bf_fpp()) {
        _bf_fpp = schema.bf_fpp(); 
    } else {
        _bf_fpp = BLOOM_FILTER_DEFAULT_FPP;
    }
    return OLAP_SUCCESS;
}

OLAPStatus TabletSchema::to_schema_pb(TabletSchemaPB* tablet_meta_pb) {
    for (auto& col : _cols) {
        ColumnPB* column = tablet_meta_pb->add_column();
        col.to_schema_pb(column);
    }
    tablet_meta_pb->set_num_short_key_columns(_num_short_key_columns);
    tablet_meta_pb->set_num_rows_per_row_block(_num_rows_per_row_block);
    tablet_meta_pb->set_keys_type(_keys_type);
    tablet_meta_pb->set_compress_kind(_compress_kind);
    tablet_meta_pb->set_bf_fpp(_bf_fpp);
    tablet_meta_pb->set_next_column_unique_id(_next_column_unique_id);

    return OLAP_SUCCESS;
}

size_t TabletSchema::row_size() const {
    size_t size = 0;
    for (auto& column : _cols) {
        size += column.length();
    }
    size += (_num_columns + 7) / 8;

    return size;
}

} // namespace doris