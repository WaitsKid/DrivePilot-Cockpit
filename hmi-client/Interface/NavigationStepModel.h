#ifndef NAVIGATIONSTEPMODEL_H
#define NAVIGATIONSTEPMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct NavigationStep
{
    QString instruction;
    QString roadName;
    QString action;
    double distanceMeters = 0.0;
    double cumulativeStart = 0.0;
    double cumulativeEnd = 0.0;
};

class NavigationStepModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        InstructionRole = Qt::UserRole + 1,
        RoadNameRole,
        ActionRole,
        DistanceMetersRole,
        DistanceTextRole,
        ActiveRole
    };
    Q_ENUM(Role)

    explicit NavigationStepModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSteps(QVector<NavigationStep> steps);
    void clear();
    void setActiveIndex(int index);
    int activeIndex() const;
    const NavigationStep *stepAt(int row) const;

private:
    QVector<NavigationStep> m_steps;
    int m_activeIndex = -1;
};

#endif
