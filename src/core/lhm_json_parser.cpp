#include "lhm_json_parser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <optional>
#include <utility>

namespace monitor::detail
{

namespace
{

enum class HardwareKind
{
    Other,
    Cpu,
    Memory,
    NvidiaGpu,
    AmdGpu,
    IntelGpu
};

enum class SensorDimension
{
    Power,
    Clock,
    Temperature,
    Voltage,
    Data
};

struct HardwareContext
{
    HardwareKind kind{HardwareKind::Other};
    QString hardwareId;
    QString name;
};

struct SensorRecord
{
    QString hardwareId;
    QString sensorId;
    QString type;
    QString name;
    QString formattedValue;
    std::optional<double> rawValue;
};

struct ParsedFormattedValue
{
    double value{0.0};
    QString unit;
};

struct TraversalState
{
    QString cpuName;
    QString cpuHardwareId;
    bool hasCpu{false};
    bool cpuIsAmd{false};
    QString gpuName;
    QString gpuHardwareId;
    bool hasDiscreteGpu{false};
    QString memoryHardwareId;
    QVector<SensorRecord> sensors;
};

QString normalized(const QString& text)
{
    return text.trimmed().toCaseFolded();
}

QString normalizedSensorType(const QString& type)
{
    const QString normalizedType = normalized(type);
    return normalizedType == QStringLiteral("smalldata") ? QStringLiteral("data") : normalizedType;
}

bool matchesHardwareRoot(const QString& hardwareId, const QString& root)
{
    return hardwareId.compare(root, Qt::CaseInsensitive) == 0 || hardwareId.startsWith(root + QLatin1Char('/'), Qt::CaseInsensitive);
}

HardwareKind detectHardwareKind(const QJsonObject& node)
{
    const QString hardwareId = node.value(QStringLiteral("HardwareId")).toString().trimmed();
    if (!hardwareId.isEmpty())
    {
        if (matchesHardwareRoot(hardwareId, QStringLiteral("/intelcpu")) || matchesHardwareRoot(hardwareId, QStringLiteral("/amdcpu")))
            return HardwareKind::Cpu;

        if (matchesHardwareRoot(hardwareId, QStringLiteral("/gpu-nvidia")))
            return HardwareKind::NvidiaGpu;

        if (matchesHardwareRoot(hardwareId, QStringLiteral("/gpu-amd")))
            return HardwareKind::AmdGpu;

        if (matchesHardwareRoot(hardwareId, QStringLiteral("/gpu-intel")))
            return HardwareKind::IntelGpu;

        if (matchesHardwareRoot(hardwareId, QStringLiteral("/ram")))
            return HardwareKind::Memory;

        return HardwareKind::Other;
    }

    const QString imageUrl = normalized(node.value(QStringLiteral("ImageURL")).toString());
    if (imageUrl.endsWith(QStringLiteral("/cpu.png")))
        return HardwareKind::Cpu;
    if (imageUrl.endsWith(QStringLiteral("/nvidia.png")))
        return HardwareKind::NvidiaGpu;
    if (imageUrl.endsWith(QStringLiteral("/ati.png")))
        return HardwareKind::AmdGpu;
    if (imageUrl.endsWith(QStringLiteral("/intel.png")))
        return HardwareKind::IntelGpu;
    if (imageUrl.endsWith(QStringLiteral("/ram.png")))
        return HardwareKind::Memory;

    return HardwareKind::Other;
}

bool isAcceptedDiscreteGpu(HardwareKind kind, const QString& name)
{
    if (kind == HardwareKind::NvidiaGpu || kind == HardwareKind::AmdGpu)
        return true;

    if (kind != HardwareKind::IntelGpu)
        return false;

    const QString normalizedName = normalized(name);
    const bool isArc = normalizedName.contains(QStringLiteral("arc"));
    const bool isIntegrated = normalizedName.contains(QStringLiteral("hd")) || normalizedName.contains(QStringLiteral("uhd")) || normalizedName.contains(QStringLiteral("iris"));
    return isArc && !isIntegrated;
}

std::optional<double> readRawValue(const QJsonObject& sensor)
{
    const QJsonValue rawValue = sensor.value(QStringLiteral("RawValue"));
    double value = 0.0;

    if (rawValue.isDouble())
    {
        value = rawValue.toDouble();
    }
    else if (rawValue.isString())
    {
        bool conversionSucceeded = false;
        value = rawValue.toString().toDouble(&conversionSucceeded);
        if (!conversionSucceeded)
            return std::nullopt;
    }
    else
    {
        return std::nullopt;
    }

    if (!std::isfinite(value))
        return std::nullopt;

    return value;
}

std::optional<double> normalizeFormattedNumber(QString numberText)
{
    numberText = numberText.trimmed();
    if (numberText.isEmpty())
        return std::nullopt;

    const qsizetype lastDot = numberText.lastIndexOf(QLatin1Char('.'));
    const qsizetype lastComma = numberText.lastIndexOf(QLatin1Char(','));
    qsizetype decimalSeparator = -1;
    QChar groupingSeparator;

    if (lastDot >= 0 && lastComma >= 0)
    {
        decimalSeparator = std::max(lastDot, lastComma);
        groupingSeparator = lastDot < lastComma ? QLatin1Char('.') : QLatin1Char(',');
    }
    else if (lastDot >= 0 || lastComma >= 0)
    {
        decimalSeparator = std::max(lastDot, lastComma);
        const QChar candidate = lastDot >= 0 ? QLatin1Char('.') : QLatin1Char(',');
        const qsizetype separatorCount = numberText.count(candidate);
        if (separatorCount > 1)
        {
            const QString trailingDigits = numberText.mid(decimalSeparator + 1);
            if (trailingDigits.size() == 3)
            {
                decimalSeparator = -1;
                groupingSeparator = candidate;
            }
            else
            {
                groupingSeparator = candidate;
            }
        }
    }

    QString normalizedNumber;
    normalizedNumber.reserve(numberText.size());
    for (qsizetype index = 0; index < numberText.size(); ++index)
    {
        const QChar character = numberText.at(index);
        if (character.isDigit() || character == QLatin1Char('+') || character == QLatin1Char('-') || character == QLatin1Char('e') || character == QLatin1Char('E'))
        {
            normalizedNumber.append(character);
        }
        else if (index == decimalSeparator)
        {
            normalizedNumber.append(QLatin1Char('.'));
        }
        else if (!groupingSeparator.isNull() && character == groupingSeparator)
        {
            continue;
        }
        else
        {
            return std::nullopt;
        }
    }

    bool conversionSucceeded = false;
    const double value = normalizedNumber.toDouble(&conversionSucceeded);
    if (!conversionSucceeded || !std::isfinite(value))
        return std::nullopt;

    return value;
}

std::optional<ParsedFormattedValue> parseFormattedValue(const QString& text)
{
    static const QRegularExpression valueExpression(QStringLiteral(R"(^\s*([+-]?[0-9][0-9.,]*(?:[eE][+-]?[0-9]+)?)\s*(.*?)\s*$)"));
    const QRegularExpressionMatch match = valueExpression.match(text);
    if (!match.hasMatch())
        return std::nullopt;

    const std::optional<double> value = normalizeFormattedNumber(match.captured(1));
    if (!value.has_value())
        return std::nullopt;

    return ParsedFormattedValue{*value, match.captured(2).trimmed()};
}

QString dimensionBaseUnit(SensorDimension dimension)
{
    switch (dimension)
    {
    case SensorDimension::Power:
        return QStringLiteral("W");
    case SensorDimension::Clock:
        return QStringLiteral("MHz");
    case SensorDimension::Temperature:
        return QStringLiteral("°C");
    case SensorDimension::Voltage:
        return QStringLiteral("V");
    case SensorDimension::Data:
        return QStringLiteral("B");
    }

    return {};
}

bool convertDimensionValue(double value, const QString& sourceUnit, SensorDimension dimension, const QString& outputUnit, double& convertedValue)
{
    QString unit = sourceUnit.toCaseFolded().simplified().remove(QLatin1Char(' '));
    if (unit.isEmpty())
        unit = dimensionBaseUnit(dimension).toCaseFolded();

    switch (dimension)
    {
    case SensorDimension::Power:
        if (unit == QStringLiteral("w"))
            convertedValue = value;
        else if (unit == QStringLiteral("mw"))
            convertedValue = value / 1000.0;
        else if (unit == QStringLiteral("kw"))
            convertedValue = value * 1000.0;
        else
            return false;
        break;
    case SensorDimension::Clock:
        if (unit == QStringLiteral("mhz"))
            convertedValue = value;
        else if (unit == QStringLiteral("ghz"))
            convertedValue = value * 1000.0;
        else if (unit == QStringLiteral("khz"))
            convertedValue = value / 1000.0;
        else if (unit == QStringLiteral("hz"))
            convertedValue = value / 1000000.0;
        else
            return false;
        break;
    case SensorDimension::Temperature:
        if (unit == QStringLiteral("°c") || unit == QStringLiteral("c"))
            convertedValue = value;
        else if (unit == QStringLiteral("°f") || unit == QStringLiteral("f"))
            convertedValue = (value - 32.0) * 5.0 / 9.0;
        else if (unit == QStringLiteral("k"))
            convertedValue = value - 273.15;
        else
            return false;
        break;
    case SensorDimension::Voltage:
        if (unit == QStringLiteral("v"))
            convertedValue = value;
        else if (unit == QStringLiteral("mv"))
            convertedValue = value / 1000.0;
        else
            return false;
        break;
    case SensorDimension::Data:
    {
        double bytes = 0.0;
        if (unit == QStringLiteral("b") || unit == QStringLiteral("byte") || unit == QStringLiteral("bytes"))
            bytes = value;
        else if (unit == QStringLiteral("kb") || unit == QStringLiteral("kib"))
            bytes = value * 1024.0;
        else if (unit == QStringLiteral("mb") || unit == QStringLiteral("mib"))
            bytes = value * 1024.0 * 1024.0;
        else if (unit == QStringLiteral("gb") || unit == QStringLiteral("gib"))
            bytes = value * 1024.0 * 1024.0 * 1024.0;
        else if (unit == QStringLiteral("tb") || unit == QStringLiteral("tib"))
            bytes = value * 1024.0 * 1024.0 * 1024.0 * 1024.0;
        else
            return false;

        const QString normalizedOutputUnit = normalized(outputUnit);
        if (normalizedOutputUnit == QStringLiteral("mb"))
            convertedValue = bytes / (1024.0 * 1024.0);
        else if (normalizedOutputUnit == QStringLiteral("gb"))
            convertedValue = bytes / (1024.0 * 1024.0 * 1024.0);
        else
            return false;
        break;
    }
    }

    return std::isfinite(convertedValue);
}

MetricValue parseSensorValue(const SensorRecord& sensor, SensorDimension dimension, const QString& outputUnit)
{
    MetricValue result;
    result.unit = outputUnit;
    double convertedValue = 0.0;

    if (sensor.rawValue.has_value())
    {
        if (!convertDimensionValue(*sensor.rawValue, dimensionBaseUnit(dimension), dimension, outputUnit, convertedValue))
            return result;

        result.value = convertedValue;
        return result;
    }

    const std::optional<ParsedFormattedValue> formattedValue = parseFormattedValue(sensor.formattedValue);
    if (!formattedValue.has_value())
        return result;

    if (!convertDimensionValue(formattedValue->value, formattedValue->unit, dimension, outputUnit, convertedValue))
        return result;

    result.value = convertedValue;
    return result;
}

bool shouldRetainSensor(const HardwareContext& context, const QString& sensorType, const QString& sensorName, const TraversalState& state)
{
    const QString type = normalizedSensorType(sensorType);
    const QString name = normalized(sensorName);

    if (state.hasCpu && context.hardwareId == state.cpuHardwareId)
    {
        if (type == QStringLiteral("voltage") || type == QStringLiteral("clock"))
            return true;

        if (type == QStringLiteral("temperature"))
            return !name.contains(QStringLiteral("distance to tjmax"));

        if (type == QStringLiteral("power"))
            return name == QStringLiteral("cpu package") || (state.cpuIsAmd && name == QStringLiteral("package"));

        return false;
    }

    if (state.hasDiscreteGpu && context.hardwareId == state.gpuHardwareId)
    {
        if (type == QStringLiteral("power"))
            return name == QStringLiteral("gpu package") || name == QStringLiteral("gpu power");

        if (type == QStringLiteral("clock") || type == QStringLiteral("temperature"))
            return name == QStringLiteral("gpu core");

        if (type == QStringLiteral("data"))
            return name == QStringLiteral("gpu memory used") || name == QStringLiteral("d3d dedicated memory used");

        return false;
    }

    if (!state.memoryHardwareId.isEmpty() && context.hardwareId == state.memoryHardwareId && type == QStringLiteral("data"))
    {
        return name == QStringLiteral("memory used") ||
            name == QStringLiteral("memory available") ||
            name == QStringLiteral("total memory") ||
            name == QStringLiteral("memory total");
    }

    return false;
}

void traverseNode(const QJsonObject& node, const HardwareContext& parentContext, TraversalState& state)
{
    HardwareContext context = parentContext;
    const QString hardwareId = node.value(QStringLiteral("HardwareId")).toString().trimmed();
    if (!hardwareId.isEmpty())
    {
        context.kind = detectHardwareKind(node);
        context.hardwareId = hardwareId;
        context.name = node.value(QStringLiteral("Text")).toString().trimmed();

        if (context.kind == HardwareKind::Cpu && !state.hasCpu)
        {
            state.cpuName = context.name;
            state.cpuHardwareId = hardwareId;
            state.hasCpu = true;
            state.cpuIsAmd = matchesHardwareRoot(hardwareId, QStringLiteral("/amdcpu"));
        }
        else if (!state.hasDiscreteGpu && isAcceptedDiscreteGpu(context.kind, context.name))
        {
            state.gpuName = context.name;
            state.gpuHardwareId = hardwareId;
            state.hasDiscreteGpu = true;
        }
        else if (context.kind == HardwareKind::Memory && state.memoryHardwareId.isEmpty())
        {
            state.memoryHardwareId = hardwareId;
        }
    }

    const QString sensorId = node.value(QStringLiteral("SensorId")).toString().trimmed();
    const QString type = node.value(QStringLiteral("Type")).toString().trimmed();
    const QString sensorName = node.value(QStringLiteral("Text")).toString().trimmed();
    if (!sensorId.isEmpty() && !type.isEmpty() && shouldRetainSensor(context, type, sensorName, state))
    {
        state.sensors.append(SensorRecord{
            context.hardwareId,
            sensorId,
            type,
            sensorName,
            node.value(QStringLiteral("Value")).toString().trimmed(),
            readRawValue(node)
        });
    }

    const QJsonArray children = node.value(QStringLiteral("Children")).toArray();
    for (const QJsonValue& child : children)
    {
        if (child.isObject())
            traverseNode(child.toObject(), context, state);
    }
}

const SensorRecord* findExactSensor(const QVector<SensorRecord>& sensors, const QString& hardwareId, const QString& type, const QString& name)
{
    const QString normalizedType = normalizedSensorType(type);
    const QString normalizedName = normalized(name);
    for (const SensorRecord& sensor : sensors)
    {
        if (sensor.hardwareId == hardwareId && normalizedSensorType(sensor.type) == normalizedType && normalized(sensor.name) == normalizedName)
            return &sensor;
    }

    return nullptr;
}

MetricValue sensorMetric(const SensorRecord* sensor, SensorDimension dimension, const QString& outputUnit)
{
    if (sensor == nullptr)
        return MetricValue{std::nullopt, outputUnit};

    return parseSensorValue(*sensor, dimension, outputUnit);
}

MetricValue firstAvailableSensorMetric(const QVector<SensorRecord>& sensors, const QString& hardwareId, const QString& type, const std::initializer_list<QString>& names, SensorDimension dimension, const QString& outputUnit)
{
    for (const QString& name : names)
    {
        const SensorRecord* sensor = findExactSensor(sensors, hardwareId, type, name);
        if (sensor == nullptr)
            continue;

        const MetricValue metric = parseSensorValue(*sensor, dimension, outputUnit);
        if (metric.value.has_value())
            return metric;
    }

    return MetricValue{std::nullopt, outputUnit};
}

QVector<SensorReading> readingsForCategory(const QVector<SensorRecord>& sensors, const QString& hardwareId, const QString& type, SensorDimension dimension)
{
    QVector<SensorReading> readings;
    const QString normalizedType = normalizedSensorType(type);
    for (const SensorRecord& sensor : sensors)
    {
        if (sensor.hardwareId != hardwareId || normalizedSensorType(sensor.type) != normalizedType)
            continue;

        readings.append(SensorReading{sensor.sensorId, sensor.type, sensor.name, parseSensorValue(sensor, dimension, dimensionBaseUnit(dimension))});
    }

    return readings;
}

LhmParseResult parseFailure(QString message)
{
    return LhmParseResult{std::nullopt, std::move(message)};
}

} // namespace

LhmParseResult parseLhmResponse(const QByteArray& payload)
{
    if (payload.trimmed().isEmpty())
        return parseFailure(QStringLiteral("LHM 响应为空。"));

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return parseFailure(QStringLiteral("LHM JSON 解析失败：%1").arg(parseError.errorString()));

    if (!document.isObject())
        return parseFailure(QStringLiteral("LHM JSON 根节点不是对象。"));

    const QJsonObject root = document.object();
    const QJsonValue childrenValue = root.value(QStringLiteral("Children"));
    if (!childrenValue.isArray() || childrenValue.toArray().isEmpty())
        return parseFailure(QStringLiteral("LHM JSON 缺少硬件节点。"));

    TraversalState state;
    traverseNode(root, HardwareContext{}, state);
    if (!state.hasCpu)
        return parseFailure(QStringLiteral("LHM JSON 未找到 CPU 硬件节点。"));

    MonitorSnapshot snapshot;
    snapshot.cpu.name = state.cpuName;

    const SensorRecord* cpuPower = findExactSensor(state.sensors, state.cpuHardwareId, QStringLiteral("Power"), QStringLiteral("CPU Package"));
    if (cpuPower == nullptr && state.cpuIsAmd)
        cpuPower = findExactSensor(state.sensors, state.cpuHardwareId, QStringLiteral("Power"), QStringLiteral("Package"));
    snapshot.cpu.packagePower = sensorMetric(cpuPower, SensorDimension::Power, QStringLiteral("W"));

    static const QRegularExpression pCoreExpression(QStringLiteral(R"(^P-Core\s*#[0-9]+$)"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression amdCoreExpression(QStringLiteral(R"(^(?:CPU\s+)?Core\s*#[0-9]+$)"), QRegularExpression::CaseInsensitiveOption);
    std::optional<double> maximumPCoreClock;
    std::optional<double> maximumAmdCoreClock;
    for (const SensorRecord& sensor : state.sensors)
    {
        if (sensor.hardwareId != state.cpuHardwareId || normalized(sensor.type) != QStringLiteral("clock"))
            continue;

        const bool isPCore = pCoreExpression.match(sensor.name.trimmed()).hasMatch();
        const bool isAmdCore = state.cpuIsAmd && amdCoreExpression.match(sensor.name.trimmed()).hasMatch();
        if (!isPCore && !isAmdCore)
            continue;

        const MetricValue clock = parseSensorValue(sensor, SensorDimension::Clock, QStringLiteral("MHz"));
        if (!clock.value.has_value())
            continue;

        if (isPCore && (!maximumPCoreClock.has_value() || *clock.value > *maximumPCoreClock))
            maximumPCoreClock = clock.value;
        else if (isAmdCore && (!maximumAmdCoreClock.has_value() || *clock.value > *maximumAmdCoreClock))
            maximumAmdCoreClock = clock.value;
    }
    snapshot.cpu.maximumPCoreClock = MetricValue{maximumPCoreClock.has_value() ? maximumPCoreClock : maximumAmdCoreClock, QStringLiteral("MHz")};

    const SensorRecord* coreMaximum = findExactSensor(state.sensors, state.cpuHardwareId, QStringLiteral("Temperature"), QStringLiteral("Core Max"));
    snapshot.cpu.coreMaximumTemperature = sensorMetric(coreMaximum, SensorDimension::Temperature, QStringLiteral("°C"));

    snapshot.cpu.sensorGroups.append(SensorGroup{
        SensorCategory::Voltage,
        QStringLiteral("电压"),
        readingsForCategory(state.sensors, state.cpuHardwareId, QStringLiteral("Voltage"), SensorDimension::Voltage)
    });
    snapshot.cpu.sensorGroups.append(SensorGroup{
        SensorCategory::Clock,
        QStringLiteral("频率"),
        readingsForCategory(state.sensors, state.cpuHardwareId, QStringLiteral("Clock"), SensorDimension::Clock)
    });
    snapshot.cpu.sensorGroups.append(SensorGroup{
        SensorCategory::Temperature,
        QStringLiteral("温度"),
        readingsForCategory(state.sensors, state.cpuHardwareId, QStringLiteral("Temperature"), SensorDimension::Temperature)
    });

    snapshot.hasDiscreteGpu = state.hasDiscreteGpu;
    if (snapshot.hasDiscreteGpu)
    {
        snapshot.gpu.name = state.gpuName;

        snapshot.gpu.packagePower = firstAvailableSensorMetric(state.sensors, state.gpuHardwareId, QStringLiteral("Power"), {QStringLiteral("GPU Package"), QStringLiteral("GPU Power")}, SensorDimension::Power, QStringLiteral("W"));

        const SensorRecord* gpuClock = findExactSensor(state.sensors, state.gpuHardwareId, QStringLiteral("Clock"), QStringLiteral("GPU Core"));
        snapshot.gpu.coreClock = sensorMetric(gpuClock, SensorDimension::Clock, QStringLiteral("MHz"));

        const SensorRecord* gpuTemperature = findExactSensor(state.sensors, state.gpuHardwareId, QStringLiteral("Temperature"), QStringLiteral("GPU Core"));
        snapshot.gpu.coreTemperature = sensorMetric(gpuTemperature, SensorDimension::Temperature, QStringLiteral("°C"));

        snapshot.gpu.memoryUsed = firstAvailableSensorMetric(state.sensors, state.gpuHardwareId, QStringLiteral("Data"), {QStringLiteral("GPU Memory Used"), QStringLiteral("D3D Dedicated Memory Used")}, SensorDimension::Data, QStringLiteral("MB"));
    }

    MetricValue memoryUsed = firstAvailableSensorMetric(state.sensors, state.memoryHardwareId, QStringLiteral("Data"), {QStringLiteral("Memory Used")}, SensorDimension::Data, QStringLiteral("GB"));
    const MetricValue memoryAvailable = firstAvailableSensorMetric(state.sensors, state.memoryHardwareId, QStringLiteral("Data"), {QStringLiteral("Memory Available")}, SensorDimension::Data, QStringLiteral("GB"));
    MetricValue memoryTotal = firstAvailableSensorMetric(state.sensors, state.memoryHardwareId, QStringLiteral("Data"), {QStringLiteral("Total Memory"), QStringLiteral("Memory Total")}, SensorDimension::Data, QStringLiteral("GB"));

    if (!memoryUsed.value.has_value() && memoryTotal.value.has_value() && memoryAvailable.value.has_value())
        memoryUsed = MetricValue{*memoryTotal.value - *memoryAvailable.value, QStringLiteral("GB")};

    if (!memoryTotal.value.has_value() && memoryUsed.value.has_value() && memoryAvailable.value.has_value())
    {
        memoryTotal = MetricValue{*memoryUsed.value + *memoryAvailable.value, QStringLiteral("GB")};
    }

    snapshot.memory.used = memoryUsed;
    snapshot.memory.total = memoryTotal;

    if (!snapshot.hasDiscreteGpu)
    {
        snapshot.cpuGpuTotalPower = MetricValue{snapshot.cpu.packagePower.value, QStringLiteral("W")};
    }
    else if (snapshot.cpu.packagePower.value.has_value() && snapshot.gpu.packagePower.value.has_value())
    {
        snapshot.cpuGpuTotalPower = MetricValue{*snapshot.cpu.packagePower.value + *snapshot.gpu.packagePower.value, QStringLiteral("W")};
    }
    else
    {
        snapshot.cpuGpuTotalPower = MetricValue{std::nullopt, QStringLiteral("W")};
    }

    return LhmParseResult{snapshot, {}};
}

} // namespace monitor::detail
