#ifndef SCIENTIFICCALCULATOR_H
#define SCIENTIFICCALCULATOR_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

class ScientificCalculator : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Calculator)

    Q_PROPERTY(QString resultText READ resultText NOTIFY resultChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY resultChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(bool degreeMode READ degreeMode WRITE setDegreeMode NOTIFY degreeModeChanged)

public:
    explicit ScientificCalculator(QObject *parent = nullptr);

    QString resultText() const;
    QString errorString() const;
    QVariantList history() const;
    bool degreeMode() const;
    void setDegreeMode(bool enabled);

    Q_INVOKABLE void calculate(const QString &expression);
    Q_INVOKABLE void calculateIntegral(const QString &expression,
                                       double lowerBound,
                                       double upperBound);
    Q_INVOKABLE void calculateLimit(const QString &expression, double point);
    Q_INVOKABLE QVariantList sampleFunction(const QString &expression,
                                            double minimumX,
                                            double maximumX,
                                            int sampleCount) const;
    Q_INVOKABLE void clearHistory();

signals:
    void resultChanged();
    void historyChanged();
    void degreeModeChanged();

private:
    void publishResult(const QString &operation,
                       const QString &expression,
                       double value);
    void publishError(const QString &message);
    void appendHistory(const QString &operation,
                       const QString &expression,
                       const QString &result);
    void loadSettings();
    void saveSettings() const;
    static QString formatNumber(double value);

    QString m_resultText = QStringLiteral("0");
    QString m_errorString;
    QVariantList m_history;
    bool m_degreeMode = false;
};

#endif
