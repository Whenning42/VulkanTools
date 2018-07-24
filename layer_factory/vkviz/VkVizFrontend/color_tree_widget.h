#ifndef COLOR_TREE_WIDGET_H
#define COLOR_TREE_WIDGET_H

#include <QTreeWidget>

#include <unordered_map>
#include <unordered_set>

class ColorTreeNode {
    std::unordered_set<ColorTreeNode*> children_with_color;
    QTreeWidgetItem* item;

 public:
    QColor current_color;

    void SetColor(QColor color) {
        item->setBackground(0, QBrush(color));
        current_color = color;
    }

    void SetMultipleColors() {
        SetColor(QColor(180, 180, 180));
    }

    void SetClear() {
        SetColor(QColor(255, 255, 255)); // Clear
    }

    void AddChildWithColor(ColorTreeNode* child) {
        children_with_color.insert(child);
    }

    void RemoveChildWithColor(ColorTreeNode* child) {
        children_with_color.erase(child);
    }

    size_t ColoredChildrenCount() {
        return children_with_color.size();
    }

    void Collapse(ColorTreeNode* parent) {
        if(ColoredChildrenCount() == 1) {
            SetColor((*children_with_color.begin())->GetColor());
            if(parent) parent->AddChildWithColor(this);
        } if (ColoredChildrenCount() > 1) {
            SetMultipleColors();
            if(parent) parent->AddChildWithColor(this);
        }
    }

    void Expand(ColorTreeNode* parent) {
        SetClear();
        if(parent) parent->RemoveChildWithColor();
    }

    ColorTreeNode(QTreeWidgetItem* item): item(item) {}
    ColorTreeNode(QTreeWidgetItem* item, QColor color): item(item), current_color(color) {}
};

class ColorTree {
    std::unordered_map<QTreeWidgetItem*, ColorTreeNode> node_map;

    void SetupTree(QTreeWidgetItem* q_widget) {
        // Check that we haven't gone past the root node.
        if(q_widget) {
            // Add nodes if they don't exist.
            if(node_map.find(q_widget) == node_map.end()) {
                node_map[q_widget] = ColorTreeNode(q_widget);
            }
        }
    }

    void OnCollapse(QTreeWidgetItem* item) {
        if(node_map.find(item) == node_map.end()) {
            // Collapsed something outside of the colored nodes tree.
            return;
        }

        node_map.at(item).Collapse();
    }

    void OnExpand(QTreeWidgetItem* item) {
        if(node_map.find(item) == node_map.end()) {
            // Expanded something outside of the colored nodes tree.
            return;
        }

        node_map.at(item).Expand();
    }

 public:
    ColorTree(QTreeWidget* tree) {
        connect(tree, QTreeWidget::itemCollapsed, [this](QTreeWidgetItem* item) {OnCollapse(item);})
        connect(tree, QTreeWidget::itemExpanded, [this](QTreeWidgetItem* item) {OnExpand(item);})
    }

    void AddColoredQWidget(QTreeWidgetItem* q_widget, QColor color) {
        node_map[q_widget] = ColorTreeNode(q_widget, color);
        SetupTree(q_widget->parent());
        if(q_widget->parent()) {
            node_map[q_widget].AddColoredChild(node_map[q_widget]);
        }
    }
}

#endif  // COLOR_TREE_WIDGET_H
