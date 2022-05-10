
/*
 * Copyright (C) 2022 UnionTech Technology Co., Ltd.
 *
 * Author:     yeshanshan <yeshanshan@uniontech.com>
 *
 * Maintainer: yeshanshan <yeshanshan@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "editmodepanel.h"
#include "pluginspec.h"
#include "widgethandler.h"
#include "instancemodel.h"
#include "widgetmanager.h"
#include "utils.h"

#include <QScrollArea>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMoveEvent>
#include <QMimeData>
#include <QDrag>
#include <QLabel>

#include <DIconButton>
#include <DFontManager>
#include <DFontSizeManager>

DGUI_USE_NAMESPACE

EditModePanelCell::EditModePanelCell(Instance *instance, QWidget *parent)
    : InstancePanelCell(instance, parent)
{
}

void EditModePanelCell::init(const QString &title)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(UI::defaultMargins);
    auto topLayout = new  QHBoxLayout();
    auto topMargin = UI::defaultMargins;
    topMargin.setLeft(UI::Edit::titleLeftMargin);
    topLayout->setContentsMargins(topMargin);
    auto topTitle = new QLabel(title);
    {
        QFont font;
        font.setBold(true);
        topTitle->setFont(DFontSizeManager::instance()->t8(font));
    }
    if (isCustom()) {
        //TODO hide or show, there is lacking of communication between plugin with frame.
    }
    topTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLayout->addWidget(topTitle);
    topLayout->addStretch(1);

    if (!isFixted()) {
        auto action = new DIconButton(DStyle::SP_DeleteButton, this);
        action->setFlat(true);
        topLayout->addWidget(action);
        connect(action, &DIconButton::clicked, this, [this](){
            Q_EMIT removeWidget(m_instance->handler()->id());
        });
    }
    layout->addLayout(topLayout);
}

void EditModePanelCell::setView()
{
    layout()->addWidget(view());
    // TODO it's exist the `spacing` if only hide the view, it maybe DFlowLayout's bug.
    setVisible(!isCustom());
}

EditModePanel::EditModePanel(WidgetManager *manager, QWidget *parent)
    : InstancePanel(manager, parent)
{
    setContentsMargins(UI::Edit::leftMargin, UI::Edit::topMargin, UI::Edit::rightMargin, UI::Edit::bottomMargin);
}

void EditModePanel::init()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(UI::defaultMargins);
    m_views->setFixedWidth(width());
    m_views->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto scrollArea = new QScrollArea();
    scrollArea->setWidget(m_views);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    layout->addWidget(scrollArea);

    auto button = new QPushButton();
    button->setText(tr("complete"));
    button->setFixedSize(UI::Edit::CompleteSize);
    connect(button, &QPushButton::clicked, this, &EditModePanel::editCompleted);
    layout->addWidget(button, 0, Qt::AlignHCenter);
}

InstancePanelCell *EditModePanel::createWidget(Instance *instance)
{
    auto cell = new EditModePanelCell(instance, m_views);
    auto plugin = m_manager->getPlugin(instance->handler()->pluginId());
    cell->init(plugin->title());
    connect(cell, &EditModePanelCell::removeWidget, m_model, &InstanceModel::removeInstance);
    return cell;
}

void EditModePanel::dragEnterEvent(QDragEnterEvent *event)
{
    InstancePanel::dragEnterEvent(event);
    if (event->isAccepted())
        return;

    if (event->mimeData()->hasFormat(EditModeMimeDataFormat)) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void EditModePanel::dragMoveEvent(QDragMoveEvent *event)
{
    InstancePanel::dragMoveEvent(event);

    if (event->isAccepted())
        return;

    if (event->mimeData()->hasFormat(EditModeMimeDataFormat)) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void EditModePanel::dropEvent(QDropEvent *event)
{
    InstancePanel::dropEvent(event);
    if (event->isAccepted())
        return;

    if (event->mimeData()->hasFormat(EditModeMimeDataFormat)) {
        QByteArray itemData = event->mimeData()->data(EditModeMimeDataFormat);
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString pluginId;
        int ttype = IWidget::Small;
        dataStream >> pluginId >> ttype;
        IWidget::Type type = static_cast<IWidget::Type>(ttype);

        auto posIndex = positionCell(event->pos(), WidgetHandlerImpl::size(type));

        if (!canDragDrop(posIndex)) {
            event->ignore();
            return;
        }

        m_model->addInstance(pluginId, type, posIndex);

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}
