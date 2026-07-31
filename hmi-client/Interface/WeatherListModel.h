#ifndef WEATHERLISTMODEL_H
#define WEATHERLISTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class WeatherListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TimeTextRole = Qt::UserRole + 1,
        DateTextRole,
        WeekdayTextRole,
        ConditionRole,
        IconRole,
        TemperatureRole,
        HighTemperatureRole,
        LowTemperatureRole,
        PrecipitationRole,
        HumidityRole,
        WindSpeedRole,
        WeatherCodeRole
    };
    Q_ENUM(Role)

    struct Entry {
        QString timeText;
        QString dateText;
        QString weekdayText;
        QString condition;
        QString icon;
        int temperature = 0;
        int highTemperature = 0;
        int lowTemperature = 0;
        int precipitation = 0;
        int humidity = 0;
        int windSpeed = 0;
        int weatherCode = 0;
    };

    explicit WeatherListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<Entry> entries);
    void clear();

private:
    QVector<Entry> m_entries;
};

#endif
