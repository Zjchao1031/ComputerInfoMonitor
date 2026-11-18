#include "lhm_parser_worker.h"

#include "lhm_json_parser.h"

#include <QStringLiteral>

#include <exception>
#include <utility>

namespace monitor
{

LhmParserWorker::LhmParserWorker(QObject* parent)
    : QObject(parent)
{
}

void LhmParserWorker::parse(quint64 requestSerial, const QByteArray& payload)
{
    try
    {
        detail::LhmParseResult result = detail::parseLhmResponse(payload);
        if (!result.snapshot.has_value())
        {
            emit parseFailed(requestSerial, std::move(result.errorMessage));
            return;
        }

        emit parseSucceeded(requestSerial, *result.snapshot);
    }
    catch (const std::exception& exception)
    {
        emit parseFailed(requestSerial, QStringLiteral("解析工作线程异常：%1").arg(QString::fromUtf8(exception.what())));
    }
    catch (...)
    {
        emit parseFailed(requestSerial, QStringLiteral("解析工作线程发生未知异常。"));
    }
}

} // namespace monitor
