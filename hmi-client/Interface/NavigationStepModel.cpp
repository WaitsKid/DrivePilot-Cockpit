#include "NavigationStepModel.h"

#include <QtGlobal>
#include <utility>

NavigationStepModel::NavigationStepModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NavigationStepModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_steps.size();
}

QVariant NavigationStepModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_steps.size())
        return {};

    const NavigationStep &step = m_steps.at(index.row());
    switch (role) {
    case InstructionRole:
    case Qt::DisplayRole: return step.instruction;
    case RoadNameRole: return step.roadName;
    case ActionRole: return step.action;
    case DistanceMetersRole: return step.distanceMeters;
    case DistanceTextRole:
        if (step.distanceMeters < 1000.0)
            return QStringLiteral("%1 m").arg(qRound(step.distanceMeters));
        return QStringLiteral("%1 km").arg(step.distanceMeters / 1000.0, 0, 'f', 1);
    case ActiveRole: return index.row() == m_activeIndex;
    default: return {};
    }
}

QHash<int, QByteArray> NavigationStepModel::roleNames() const
{
    return {
        {InstructionRole, "instruction"},
        {RoadNameRole, "roadName"},
        {ActionRole, "action"},
        {DistanceMetersRole, "distanceMeters"},
        {DistanceTextRole, "distanceText"},
        {ActiveRole, "active"}
    };
}

void NavigationStepModel::setSteps(QVector<NavigationStep> steps)
{
    beginResetModel();
    m_steps = std::move(steps);
    m_activeIndex = m_steps.isEmpty() ? -1 : 0;
    endResetModel();
}

void NavigationStepModel::clear()
{
    if (m_steps.isEmpty() && m_activeIndex == -1)
        return;
    beginResetModel();
    m_steps.clear();
    m_activeIndex = -1;
    endResetModel();
}

void NavigationStepModel::setActiveIndex(int index)
{
    const int normalized = (index >= 0 && index < m_steps.size()) ? index : -1;
    if (m_activeIndex == normalized)
        return;

    const int old = m_activeIndex;
    m_activeIndex = normalized;
    if (old >= 0)
        emit dataChanged(this->index(old, 0), this->index(old, 0), {ActiveRole});
    if (m_activeIndex >= 0)
        emit dataChanged(this->index(m_activeIndex, 0), this->index(m_activeIndex, 0), {ActiveRole});
}

int NavigationStepModel::activeIndex() const
{
    return m_activeIndex;
}

const NavigationStep *NavigationStepModel::stepAt(int row) const
{
    if (row < 0 || row >= m_steps.size())
        return nullptr;
    return &m_steps.at(row);
}
