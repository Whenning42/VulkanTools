#ifndef COLOR_TREE_WIDGET_H
#define COLOR_TREE_WIDGET_H

#include <QObject>
#include <QTreeWidget>

#include <unordered_map>
#include <unordered_set>

struct ColorTreeNode {
    bool has_multiple_colors;
    QColor collapse_color;

    ColorTreeNode(QColor color): has_multiple_colors(false), collapse_color(color) {}
};

class ColorTree {
    std::unordered_map<QTreeWidgetItem*, ColorTreeNode> node_map;

    void SetupTree(QTreeWidgetItem* q_widget, QColor color) {
        // Check that we haven't gone past the root node.
        if(q_widget) {
            // Add nodes if they don't exist.
            if(node_map.find(q_widget) == node_map.end()) {
                node_map.emplace(std::make_pair(q_widget, ColorTreeNode(color)));
            } else {
                if(node_map.at(q_widget).has_multiple_colors) {
                    // The tree from here on up has multiple colors and won't be changed by adding a color.
                    return;
                } else if(node_map.at(q_widget).collapse_color != color) {
                    node_map.at(q_widget).collapse_color = QColor(180, 180, 180);
                    node_map.at(q_widget).has_multiple_colors = true;
                }
            }

            if(q_widget->parent()) {
                SetupTree(q_widget->parent(), color);
            }
        }
    }

    void OnCollapse(QTreeWidgetItem* item) {
        if(node_map.find(item) == node_map.end()) {
            // Collapsed something outside of the colored nodes tree.
            return;
        }

        SetWidgetStoredColor(item);
    }

    void OnExpand(QTreeWidgetItem* item) {
        if(node_map.find(item) == node_map.end()) {
            // Expanded something outside of the colored nodes tree.
            return;
        }

        SetWidgetClear(item);
    }

    void SetWidgetStoredColor(QTreeWidgetItem* item) {
        item->setBackground(0, node_map.at(item).collapse_color);
    }

    void SetWidgetClear(QTreeWidgetItem* item) {
        item->setBackground(0, QColor(255, 255, 255));
    }
 public:
    ColorTree() {};
    ColorTree(QTreeWidget* tree) {
        QObject::connect(tree, &QTreeWidget::itemCollapsed, [this](QTreeWidgetItem* item) {OnCollapse(item);});
        QObject::connect(tree, &QTreeWidget::itemExpanded, [this](QTreeWidgetItem* item) {OnExpand(item);});
    }

    void AddColoredQWidget(QTreeWidgetItem* q_widget, QColor color) {
        node_map.emplace(std::make_pair(q_widget, ColorTreeNode(color)));
        SetupTree(q_widget->parent(), color);
    }
};

#endif  // COLOR_TREE_WIDGET_H
