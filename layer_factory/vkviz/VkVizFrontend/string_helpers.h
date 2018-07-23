/* Copyright (C) 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: William Henning <whenning@google.com>
 */

#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

#include <QString>
#include <sstream>
#include <string>

inline std::string PointerToString(void* v) {
    std::stringstream temp;
    temp << v;
    return temp.str();
}

inline QString PointerToQString(void* v) { return QString::fromStdString(PointerToString(v)); }

inline QTreeWidgetItem* NewWidget(const std::string& name) {
    QTreeWidgetItem* widget = new QTreeWidgetItem();
    widget->setText(0, QString::fromStdString(name));
    return widget;
}


#endif  // STRING_HELPERS_H
