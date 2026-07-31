#include "ScientificCalculator.h"

#include <QDateTime>
#include <QPointF>
#include <QSettings>
#include <QVariantMap>
#include <QtGlobal>
#include <cmath>
#include <limits>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEuler = 2.71828182845904523536;

class ExpressionParser final
{
public:
    ExpressionParser(const QString &expression, double xValue, bool degreeMode)
        : m_expression(expression)
        , m_xValue(xValue)
        , m_degreeMode(degreeMode)
    {
    }

    double evaluate(bool *ok, QString *errorMessage)
    {
        m_position = 0;
        m_error.clear();
        const double value = parseExpression();
        skipSpaces();
        if (m_error.isEmpty() && m_position != m_expression.size())
            setError(QStringLiteral("无法解析位置 %1 附近的内容").arg(m_position + 1));
        if (m_error.isEmpty() && !std::isfinite(value))
            setError(QStringLiteral("计算结果不是有限数值"));

        const bool success = m_error.isEmpty();
        if (ok)
            *ok = success;
        if (errorMessage)
            *errorMessage = m_error;
        return success ? value : std::numeric_limits<double>::quiet_NaN();
    }

private:
    double parseExpression()
    {
        double value = parseTerm();
        while (m_error.isEmpty()) {
            skipSpaces();
            if (consume(QLatin1Char('+')))
                value += parseTerm();
            else if (consume(QLatin1Char('-')))
                value -= parseTerm();
            else
                break;
        }
        return value;
    }

    double parseTerm()
    {
        double value = parseUnary();
        while (m_error.isEmpty()) {
            skipSpaces();
            if (consume(QLatin1Char('*'))) {
                value *= parseUnary();
            } else if (consume(QLatin1Char('/'))) {
                const double divisor = parseUnary();
                if (qFuzzyIsNull(divisor)) {
                    setError(QStringLiteral("除数不能为 0"));
                    return 0.0;
                }
                value /= divisor;
            } else {
                break;
            }
        }
        return value;
    }

    double parseUnary()
    {
        skipSpaces();
        if (consume(QLatin1Char('+')))
            return parseUnary();
        if (consume(QLatin1Char('-')))
            return -parseUnary();
        return parsePower();
    }

    double parsePower()
    {
        double base = parsePrimary();
        skipSpaces();
        if (consume(QLatin1Char('^'))) {
            const double exponent = parseUnary();
            base = std::pow(base, exponent);
        }
        return base;
    }

    double parsePrimary()
    {
        skipSpaces();
        if (consume(QLatin1Char('('))) {
            const double value = parseExpression();
            skipSpaces();
            if (!consume(QLatin1Char(')')))
                setError(QStringLiteral("缺少右括号"));
            return value;
        }

        if (m_position >= m_expression.size()) {
            setError(QStringLiteral("表达式不完整"));
            return 0.0;
        }

        const QChar current = m_expression.at(m_position);
        if (current.isDigit() || current == QLatin1Char('.'))
            return parseNumber();
        if (current.isLetter() || current == QLatin1Char('_'))
            return parseIdentifier();

        setError(QStringLiteral("不支持的字符：%1").arg(current));
        return 0.0;
    }

    double parseNumber()
    {
        const int start = m_position;
        bool hasExponent = false;
        while (m_position < m_expression.size()) {
            const QChar character = m_expression.at(m_position);
            if (character.isDigit() || character == QLatin1Char('.')) {
                ++m_position;
                continue;
            }
            if ((character == QLatin1Char('e') || character == QLatin1Char('E'))
                && !hasExponent) {
                hasExponent = true;
                ++m_position;
                if (m_position < m_expression.size()
                    && (m_expression.at(m_position) == QLatin1Char('+')
                        || m_expression.at(m_position) == QLatin1Char('-'))) {
                    ++m_position;
                }
                continue;
            }
            break;
        }

        bool ok = false;
        const double value = m_expression.mid(start, m_position - start).toDouble(&ok);
        if (!ok)
            setError(QStringLiteral("数字格式错误"));
        return value;
    }

    double parseIdentifier()
    {
        const int start = m_position;
        while (m_position < m_expression.size()) {
            const QChar character = m_expression.at(m_position);
            if (!character.isLetterOrNumber() && character != QLatin1Char('_'))
                break;
            ++m_position;
        }

        const QString identifier = m_expression.mid(start, m_position - start).toLower();
        if (identifier == QStringLiteral("x"))
            return m_xValue;
        if (identifier == QStringLiteral("pi"))
            return kPi;
        if (identifier == QStringLiteral("e"))
            return kEuler;

        skipSpaces();
        if (!consume(QLatin1Char('('))) {
            setError(QStringLiteral("未知标识符：%1").arg(identifier));
            return 0.0;
        }

        const double argument = parseExpression();
        skipSpaces();
        if (!consume(QLatin1Char(')'))) {
            setError(QStringLiteral("函数 %1 缺少右括号").arg(identifier));
            return 0.0;
        }
        if (!m_error.isEmpty())
            return 0.0;

        const double trigArgument = m_degreeMode ? argument * kPi / 180.0 : argument;
        if (identifier == QStringLiteral("sin")) return std::sin(trigArgument);
        if (identifier == QStringLiteral("cos")) return std::cos(trigArgument);
        if (identifier == QStringLiteral("tan")) return std::tan(trigArgument);
        if (identifier == QStringLiteral("asin")) {
            const double value = std::asin(argument);
            return m_degreeMode ? value * 180.0 / kPi : value;
        }
        if (identifier == QStringLiteral("acos")) {
            const double value = std::acos(argument);
            return m_degreeMode ? value * 180.0 / kPi : value;
        }
        if (identifier == QStringLiteral("atan")) {
            const double value = std::atan(argument);
            return m_degreeMode ? value * 180.0 / kPi : value;
        }
        if (identifier == QStringLiteral("sqrt")) {
            if (argument < 0.0) {
                setError(QStringLiteral("负数不能在实数范围内开平方"));
                return 0.0;
            }
            return std::sqrt(argument);
        }
        if (identifier == QStringLiteral("abs")) return std::abs(argument);
        if (identifier == QStringLiteral("ln")) {
            if (argument <= 0.0) {
                setError(QStringLiteral("ln 的参数必须大于 0"));
                return 0.0;
            }
            return std::log(argument);
        }
        if (identifier == QStringLiteral("log")) {
            if (argument <= 0.0) {
                setError(QStringLiteral("log 的参数必须大于 0"));
                return 0.0;
            }
            return std::log10(argument);
        }
        if (identifier == QStringLiteral("exp")) return std::exp(argument);
        if (identifier == QStringLiteral("floor")) return std::floor(argument);
        if (identifier == QStringLiteral("ceil")) return std::ceil(argument);

        setError(QStringLiteral("不支持的函数：%1").arg(identifier));
        return 0.0;
    }

    void skipSpaces()
    {
        while (m_position < m_expression.size() && m_expression.at(m_position).isSpace())
            ++m_position;
    }

    bool consume(QChar character)
    {
        if (m_position < m_expression.size() && m_expression.at(m_position) == character) {
            ++m_position;
            return true;
        }
        return false;
    }

    void setError(const QString &message)
    {
        if (m_error.isEmpty())
            m_error = message;
    }

    QString m_expression;
    int m_position = 0;
    double m_xValue = 0.0;
    bool m_degreeMode = false;
    QString m_error;
};

double evaluateExpression(const QString &expression,
                          double xValue,
                          bool degreeMode,
                          bool *ok,
                          QString *errorMessage)
{
    ExpressionParser parser(expression, xValue, degreeMode);
    return parser.evaluate(ok, errorMessage);
}
}

ScientificCalculator::ScientificCalculator(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString ScientificCalculator::resultText() const { return m_resultText; }
QString ScientificCalculator::errorString() const { return m_errorString; }
QVariantList ScientificCalculator::history() const { return m_history; }
bool ScientificCalculator::degreeMode() const { return m_degreeMode; }

void ScientificCalculator::setDegreeMode(bool enabled)
{
    if (m_degreeMode == enabled)
        return;
    m_degreeMode = enabled;
    emit degreeModeChanged();
    saveSettings();
}

void ScientificCalculator::calculate(const QString &expression)
{
    const QString normalized = expression.trimmed();
    if (normalized.isEmpty()) {
        publishError(QStringLiteral("请输入表达式"));
        return;
    }

    bool ok = false;
    QString error;
    const double value = evaluateExpression(normalized, 0.0, m_degreeMode, &ok, &error);
    if (!ok) {
        publishError(error);
        return;
    }
    publishResult(QStringLiteral("计算"), normalized, value);
}

void ScientificCalculator::calculateIntegral(const QString &expression,
                                             double lowerBound,
                                             double upperBound)
{
    const QString normalized = expression.trimmed();
    if (normalized.isEmpty() || !std::isfinite(lowerBound) || !std::isfinite(upperBound)) {
        publishError(QStringLiteral("积分表达式或上下限无效"));
        return;
    }
    if (qFuzzyCompare(lowerBound, upperBound)) {
        publishResult(QStringLiteral("积分"), normalized, 0.0);
        return;
    }

    constexpr int segmentCount = 1200;
    const double step = (upperBound - lowerBound) / segmentCount;
    double sum = 0.0;
    for (int index = 0; index <= segmentCount; ++index) {
        const double x = lowerBound + index * step;
        bool ok = false;
        QString error;
        const double y = evaluateExpression(normalized, x, m_degreeMode, &ok, &error);
        if (!ok || !std::isfinite(y)) {
            publishError(QStringLiteral("积分区间内计算失败：%1").arg(error));
            return;
        }
        const int weight = (index == 0 || index == segmentCount) ? 1 : (index % 2 == 0 ? 2 : 4);
        sum += weight * y;
    }

    const double result = sum * step / 3.0;
    const QString display = QStringLiteral("∫[%1,%2] %3 dx")
        .arg(formatNumber(lowerBound))
        .arg(formatNumber(upperBound))
        .arg(normalized);
    publishResult(QStringLiteral("积分"), display, result);
}

void ScientificCalculator::calculateLimit(const QString &expression, double point)
{
    const QString normalized = expression.trimmed();
    if (normalized.isEmpty() || !std::isfinite(point)) {
        publishError(QStringLiteral("极限表达式或趋近点无效"));
        return;
    }

    double leftValue = std::numeric_limits<double>::quiet_NaN();
    double rightValue = std::numeric_limits<double>::quiet_NaN();
    for (int power = 2; power <= 8; ++power) {
        const double delta = std::pow(10.0, -power);
        bool leftOk = false;
        bool rightOk = false;
        QString error;
        leftValue = evaluateExpression(normalized, point - delta,
                                       m_degreeMode, &leftOk, &error);
        rightValue = evaluateExpression(normalized, point + delta,
                                        m_degreeMode, &rightOk, &error);
        if (!leftOk || !rightOk)
            continue;
    }

    if (!std::isfinite(leftValue) || !std::isfinite(rightValue)) {
        publishError(QStringLiteral("无法在趋近点两侧取得有限值"));
        return;
    }

    const double scale = qMax(1.0, qMax(std::abs(leftValue), std::abs(rightValue)));
    if (std::abs(leftValue - rightValue) > 1e-4 * scale) {
        publishError(QStringLiteral("左右极限不一致，极限可能不存在"));
        return;
    }

    const double result = (leftValue + rightValue) / 2.0;
    const QString display = QStringLiteral("lim x→%1  %2")
        .arg(formatNumber(point))
        .arg(normalized);
    publishResult(QStringLiteral("极限"), display, result);
}

QVariantList ScientificCalculator::sampleFunction(const QString &expression,
                                                   double minimumX,
                                                   double maximumX,
                                                   int sampleCount) const
{
    QVariantList points;
    const QString normalized = expression.trimmed();
    if (normalized.isEmpty() || !std::isfinite(minimumX) || !std::isfinite(maximumX)
        || maximumX <= minimumX) {
        return points;
    }

    const int boundedCount = qBound(80, sampleCount, 1200);
    points.reserve(boundedCount);
    const double step = (maximumX - minimumX) / (boundedCount - 1);
    for (int index = 0; index < boundedCount; ++index) {
        const double x = minimumX + index * step;
        bool ok = false;
        QString error;
        const double y = evaluateExpression(normalized, x, m_degreeMode, &ok, &error);
        points.append(QVariant::fromValue(QPointF(
            x, ok && std::isfinite(y) ? y : std::numeric_limits<double>::quiet_NaN())));
    }
    return points;
}

void ScientificCalculator::clearHistory()
{
    if (m_history.isEmpty())
        return;
    m_history.clear();
    emit historyChanged();
    saveSettings();
}

void ScientificCalculator::publishResult(const QString &operation,
                                         const QString &expression,
                                         double value)
{
    m_resultText = formatNumber(value);
    m_errorString.clear();
    emit resultChanged();
    appendHistory(operation, expression, m_resultText);
}

void ScientificCalculator::publishError(const QString &message)
{
    m_errorString = message;
    emit resultChanged();
}

void ScientificCalculator::appendHistory(const QString &operation,
                                         const QString &expression,
                                         const QString &result)
{
    QVariantMap item;
    item.insert(QStringLiteral("operation"), operation);
    item.insert(QStringLiteral("expression"), expression);
    item.insert(QStringLiteral("result"), result);
    item.insert(QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm")));
    m_history.prepend(item);
    while (m_history.size() > 30)
        m_history.removeLast();
    emit historyChanged();
    saveSettings();
}

void ScientificCalculator::loadSettings()
{
    QSettings settings;
    m_degreeMode = settings.value(QStringLiteral("calculator/degreeMode"), false).toBool();
    m_history = settings.value(QStringLiteral("calculator/history")).toList();
    while (m_history.size() > 30)
        m_history.removeLast();
}

void ScientificCalculator::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("calculator/degreeMode"), m_degreeMode);
    settings.setValue(QStringLiteral("calculator/history"), m_history);
}

QString ScientificCalculator::formatNumber(double value)
{
    if (!std::isfinite(value))
        return QStringLiteral("未定义");
    if (qFuzzyIsNull(value))
        return QStringLiteral("0");

    const double absolute = std::abs(value);
    if (absolute >= 1e10 || absolute < 1e-7)
        return QString::number(value, 'g', 12);

    QString text = QString::number(value, 'f', 10);
    while (text.contains(QLatin1Char('.')) && text.endsWith(QLatin1Char('0')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}
