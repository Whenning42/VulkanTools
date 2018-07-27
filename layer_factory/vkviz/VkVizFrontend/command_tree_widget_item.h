#ifndef COMMAND_TREE_WIDGET_ITEM_H
#define COMMAND_TREE_WIDGET_ITEM_H

#include "synchronization.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>

Q_DECLARE_METATYPE(CommandRef);
Q_DECLARE_METATYPE(BarrierOccurance);

class CommandTreeWidgetItem : QTreeWidgetItem {
    static const uint32_t kCollapseColorRole = 0x100;
    static const uint32_t kHasMultipleCollapseColorsRole = 0x101;
    static const uint32_t kExpandColorRole = 0x102;
    static const uint32_t kCommandLocationRole = 0x103;
    static const uint32_t kBarrierOccuranceRole = 0x103;

    template <typename T>
    void SetRole(uint32_t role, const T& value) {
        QVariant variant;
        variant.setValue(value);
        setData(0, role, variant);
    }

    /*
    // QVariant's don't seem to support const pointers well. A fix is to remove the const before putting the pointer into variants.
    template <typename T>
    void SetRole(uint32_t role, const T* value) {
        T* value_var = const_cast<T*>(value);
        SetRole(role, value_var);
    }*/

    template <typename T>
    T GetRoleOrDefault(uint32_t role, const T& default_value) const {
        if (data(0, role).isNull()) {
            return default_value;
        } else {
            return data(0, role).value<T>();
        }
    }
    bool RoleIsSet(uint32_t role) const { return !data(0, role).isNull(); }

    void SetCollapseColor(const QColor& color) {
        SetRole(kCollapseColorRole, color);
        if (!isExpanded()) {
            setBackground(0, color);
        }
    }
    QColor GetCollapseColor() const { return GetRoleOrDefault(kCollapseColorRole, Colors::kDefaultCollapseColor); }
    bool HasCollapseColor() const { return RoleIsSet(kCollapseColorRole); }

    void SetHasMultipleCollapseColors() {
        SetRole(kHasMultipleCollapseColorsRole, true);
        SetCollapseColor(Colors::kMultipleColors);
    }
    bool HasMultipleCollapseColors() const { return RoleIsSet(kHasMultipleCollapseColorsRole); }

    void SetExpandColor(const QColor& color) {
        SetRole(kExpandColorRole, color);
        if (isExpanded()) {
            setBackground(0, color);
        }
    }
    QColor GetExpandColor() const { return GetRoleOrDefault(kExpandColorRole, Colors::kDefaultExpandColor); }
    bool HasExpandColor() const { return RoleIsSet(kExpandColorRole); }

    void SetCommandLocation(const CommandRef& command_location) { SetRole(kCommandLocationRole, command_location); }
    CommandRef GetCommandLocation() { return GetRoleOrDefault(kCommandLocationRole, CommandRef{nullptr, 0}); }

    void PropagateCollapseColorUpwards(const QColor& color) {
        if (!HasCollapseColor()) {
            SetCollapseColor(color);
        } else {
            if (HasMultipleCollapseColors()) {
                // Coloring this node and it's ancestors won't change anything.
                return;
            } else if (GetCollapseColor() != color) {
                // This node was set to a different color and is now set to multicolored.
                SetHasMultipleCollapseColors();
            } else {
                // This node has already been colored as the given color.
                return;
            }
        }

        if (parent()) {
            parent()->PropagateCollapseColorUpwards(color);
        }
    }

    CommandTreeWidgetItem(): QTreeWidgetItem() {};
    CommandTreeWidgetItem(const std::string& name): QTreeWidgetItem() { setText(0, QString::fromStdString(name)); }

   public:
    static QTreeWidgetItem* ToQItem(CommandTreeWidgetItem* item) { return static_cast<QTreeWidgetItem*>(item); }
    static CommandTreeWidgetItem* FromQItem(QTreeWidgetItem* item) { return static_cast<CommandTreeWidgetItem*>(item); }

    static CommandTreeWidgetItem* NewWidget(const std::string& name) { return new CommandTreeWidgetItem(name); }

    static CommandTreeWidgetItem* NewBarrier(const std::string& name, const BarrierOccurance& barrier) {
        CommandTreeWidgetItem* widget = new CommandTreeWidgetItem(name);
        widget->SetBarrierOccurance(barrier);
        return widget;
    }

    static CommandTreeWidgetItem* NewColoredWidget(const std::string& name, const QColor& color) {
        CommandTreeWidgetItem* widget = new CommandTreeWidgetItem(name);
        widget->SetColor(color);
        return widget;
    }

    static CommandTreeWidgetItem* AddNewChildWidget(CommandTreeWidgetItem* parent, const std::string& name) {
        CommandTreeWidgetItem* widget = new CommandTreeWidgetItem(name);
        parent->addChild(widget);
        return widget;
    }

    CommandTreeWidgetItem* parent() const { return static_cast<CommandTreeWidgetItem*>(QTreeWidgetItem::parent()); }

    void addChild(CommandTreeWidgetItem* child) {
        QTreeWidgetItem::addChild(child);

        if (child->HasCollapseColor()) {
            PropagateCollapseColorUpwards(child->GetCollapseColor());
        }
    }

    bool IsCommand() const { return RoleIsSet(kCommandLocationRole); }

    void SetColor(const QColor& color) {
        SetExpandColor(color);
        PropagateCollapseColorUpwards(color);
    }

    void Collapse() { setBackground(0, GetCollapseColor()); }

    void Expand() { setBackground(0, GetExpandColor()); }

    void SetBarrierOccurance(const BarrierOccurance& barrier) {
        SetRole(kBarrierOccuranceRole, barrier);
    }
    BarrierOccurance GetBarrierOccurance() const {
        return GetRoleOrDefault(kBarrierOccuranceRole, BarrierOccurance{});
    }
    bool HasBarrierOccurance() const {
        return RoleIsSet(kBarrierOccuranceRole);
    }
};

inline void AddTopLevelItem(QTreeWidget* tree, CommandTreeWidgetItem* item) {
    tree->addTopLevelItem(CommandTreeWidgetItem::ToQItem(item));
}

inline CommandTreeWidgetItem* AddNewChildWidget(CommandTreeWidgetItem* parent, const std::string& name) {
    return CommandTreeWidgetItem::AddNewChildWidget(parent, name);
}

#endif  // COMMAND_TREE_WIDGET_ITEM_H
