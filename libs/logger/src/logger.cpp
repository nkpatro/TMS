#include "logger/logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

// Initialize static member to nullptr
Logger* Logger::m_instance = nullptr;

// Use Meyer's singleton pattern with double-checked locking for thread safety
Logger* Logger::instance() {
    // Use double-checked locking for thread safety
    if (m_instance == nullptr) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);

        // Check again after locking
        if (m_instance == nullptr) {
            m_instance = new Logger();
        }
    }
    return m_instance;
}

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_logLevel(Info)
    , m_consoleOutput(true)
    , m_logFilePath("")
{
    // Default log file location
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    QString logPath = logDir + "/application.log";

    // Instead of calling setLogFile which calls log methods,
    // initialize the file directly
    m_logFilePath = logPath;
    m_logFile.setFileName(logPath);

    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
    } else {
        qWarning() << "Failed to open log file:" << logPath;
    }
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
}

void Logger::setLogFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }

    m_logFilePath = filePath;
    m_logFile.setFileName(filePath);
    bool opened = m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

    if (opened) {
        m_logStream.setDevice(&m_logFile);
        // Use direct logging without going through log() to avoid potential recursion
        writeToLog(formatLogMessage(Info, QString("Log file opened: %1").arg(filePath), ""));
    } else {
        qWarning() << "Failed to open log file:" << filePath;
    }
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
    // Directly log without using log() to avoid potential recursion
    writeToLog(formatLogMessage(Info, QString("Log level set to: %1").arg(logLevelToString(level)), ""));
}

bool Logger::enableConsoleOutput(bool enable) {
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enable;
    return m_consoleOutput;
}

void Logger::debug(const QString& message, const QString& source) {
    log(Debug, message, source);
}

void Logger::info(const QString& message, const QString& source) {
    log(Info, message, source);
}

void Logger::warning(const QString& message, const QString& source) {
    log(Warning, message, source);
}

void Logger::error(const QString& message, const QString& source) {
    log(Error, message, source);
}

void Logger::fatal(const QString& message, const QString& source) {
    log(Fatal, message, source);
}

void Logger::log(LogLevel level, const QString& message, const QString& source) {
    // Only log if level is sufficient
    if (level < m_logLevel) {
        return;
    }

    // Create formatted message outside the lock to minimize lock time
    QString formattedMessage = formatLogMessage(level, message, source);

    QMutexLocker locker(&m_mutex);
    writeToLog(formattedMessage);

    if (m_consoleOutput) {
        switch (level) {
            case Debug:
                qDebug().noquote() << formattedMessage;
                break;
            case Info:
                qInfo().noquote() << formattedMessage;
                break;
            case Warning:
                qWarning().noquote() << formattedMessage;
                break;
            case Error:
            case Fatal:
                qCritical().noquote() << formattedMessage;
                break;
        }
    }
}

void Logger::logData(LogLevel level, const QMap<QString, QVariant>& data, const QString& source) {
    if (level < m_logLevel) {
        return;
    }

    QStringList logParts;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        logParts.append(QString("%1: %2").arg(it.key(), it.value().toString()));
    }

    log(level, logParts.join(", "), source);
}

QString Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARNING";
        case Error:   return "ERROR";
        case Fatal:   return "FATAL";
        default:      return "UNKNOWN";
    }
}

QString Logger::formatLogMessage(LogLevel level, const QString& message, const QString& source) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString pid = QString::number(QCoreApplication::applicationPid());
    QString threadId = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    QString levelStr = logLevelToString(level);

    QString formattedMsg;
    if (source.isEmpty()) {
        formattedMsg = QString("[%1] [%2] [PID:%3] [TID:%4] %5")
            .arg(timestamp, levelStr, pid, threadId, message);
    } else {
        // For enhanced visualization, extract just ClassName::methodName
        QString sourceInfo = source;

        // Extract the fully qualified name without parameters
        int parenPos = source.indexOf('(');
        if (parenPos > 0) {
            QString fullName = source.left(parenPos);

            // Count double-colons to determine if we have namespaces
            int colonCount = fullName.count("::");

            if (colonCount >= 1) {
                // We have at least one class/namespace separator
                int lastColonPos = fullName.lastIndexOf("::");

                if (colonCount >= 2) {
                    // We have both namespace and class, try to get just ClassName::methodName
                    int secondLastColonPos = fullName.lastIndexOf("::", lastColonPos - 1);
                    sourceInfo = fullName.mid(secondLastColonPos + 2);
                } else {
                    // Only one separator, use as is (likely ClassName::methodName)
                    sourceInfo = fullName;
                }
            } else {
                // No class separator, use the full name (likely just a function)
                sourceInfo = fullName;
            }
        }

        sourceInfo = sourceInfo.replace("__cdecl ", "");
        formattedMsg = QString("[%1] [%2] [PID:%3] [TID:%4] [%5] %6")
            .arg(timestamp, levelStr, pid, threadId, sourceInfo, message);
    }

    return formattedMsg;
}

void Logger::writeToLog(const QString& message) {
    // No need for a mutex here as this is always called from within a locked method
    if (m_logFile.isOpen()) {
        m_logStream << message << Qt::endl;
        m_logStream.flush();
    }
}

Logger::LogLevel Logger::getLogLevel() const {
    return m_logLevel;
}

QString Logger::getLogFilePath() const {
    return m_logFilePath;
}

bool Logger::isConsoleOutputEnabled() const {
    return m_consoleOutput;
}