#include "cppcapture/event.h"
#include "cppcapture/breadcrumbs.h"
#include "cppcapture/configure.h"

#include "debugging.h"
#include "encoder/json_encoder.h"
#include "systemutils.h"

// we do not include the line number because the result is used as a search tag in the Sentry UI
// and it is more convenient to filter messages belonging to the same whole file rather
// than a specific file line number
static std::string makeCulprit(const char * functionName, const char * sourceFile) {
    std::string result;

    if (functionName != nullptr) {
        result = functionName;
    }

    if (sourceFile != nullptr) {
        const std::string path = cppcapture::normalizeSourceFileName(sourceFile);
        result += " (" + path + ")";
    }

    return result;
}

namespace cppcapture {
    Event & Event::WithException(const std::exception & e) {
        return WithException(makeExceptionType(e), e.what());
    }

    std::string Event::ToJSON() const {
        JsonEncoder json;

        // event_id is optional
        // json.append(json, "event_id", "32cebd5d-1542-4703-a5a3-be5eaa90af81");

        // A string representing the platform the SDK is submitting from. This will be used by the Sentry interface
        // to customize various components in the interface.
        // "c++" is not a supported olatform so for now we use c
        json.append("platform", "c");

        // Information about the SDK sending the event.
        json.beginBlock("sdk").append("name", CPPCAPTURE_NAME).append("version", CPPCAPTURE_VERSION).endBlock();

        /*
        if (!s_values.ServerName.empty()) {
        json.append("server_name", s_values.ServerName);
        }
        if (!s_values.ReleaseVersion.empty()) {
        json.append("release", s_values.ReleaseVersion);
        }
        if (!s_values.Environment.empty()) {
        json.append("environment", s_values.Environment);
        }
        */

        // The timestamp should be in ISO 8601 format, without a timezone (UTC)
        json.append("timestamp", makeTimestampISO8601());

        // The name of the logger which created the record
        // If none is provided, the source file path is a good replacement
        if (!m_loggerName.empty()) {
            json.append("logger", m_loggerName);
        } else if (m_sourceFile) {
            json.append("logger", normalizeSourceFileName(m_sourceFile));
        }

        if (m_sourceFile != nullptr) {
            json.append("culprit", makeCulprit(m_functionName, m_sourceFile));
        }

        json.append("level", ToString(m_level));
        json.append("tags", m_tags);

        // source file location
        auto extra = m_extra;
        if (m_sourceFile != nullptr) {
            std::string location = normalizeSourceFileName(m_sourceFile);
            if (m_lineNumber > 0) {
                location += ":" + std::to_string(m_lineNumber);
            }
            extra["File location"] = location;
        }
        if (m_functionName != nullptr) {
            extra["Function"] = m_functionName;
        }

        json.append("extra", extra);

        if (!m_exceptionType.empty()) {
            json.beginBlock("sentry.interfaces.Exception")
                .append("type", m_exceptionType)
                .append("value", m_exceptionValue)
                .endBlock();
        }

        if (!m_message.empty()) {
            json.beginBlock("sentry.interfaces.Message")
                .append("message", m_message)
                //.append("params", R"(["Param1"])")
                .endBlock();
        }

        Breadcrumbs::Instance().ToJSON(json);

        return json.complete();
    }
} // namespace cppcapture
