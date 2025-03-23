#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QMap>
#include <QThread>

// Define the logger_global macro for export/import
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define DECL_EXPORT __declspec(dllexport)
#  define DECL_IMPORT __declspec(dllimport)
#else
#  define DECL_EXPORT     __attribute__((visibility("default")))
#  define DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(LOGGER_LIBRARY)
#  define LOGGER_EXPORT DECL_EXPORT
#else
#  define LOGGER_EXPORT DECL_IMPORT
#endif

/**
 * @brief Singleton logger class that provides thread-safe logging functionality
 *
 * This class implements a thread-safe logging system with support for different log levels,
 * console output, and file output. It uses the Meyer's singleton pattern with
 * double-checked locking for thread safety.
 */
class LOGGER_EXPORT Logger : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Log levels supported by the logger
    */

    enum LogLevel {
        Debug,    ///< Detailed debugging information
        Info,     ///< General informational messages
        Warning,  ///< Warning messages for potentially harmful situations
        Error,    ///< Error messages for serious problems
        Fatal     ///< Critical errors that may cause program termination
    };

    /**
     * @brief Gets the singleton instance of the logger
    * @return Pointer to the Logger instance
    */
    static Logger* instance();

    /**
     * @brief Sets the output log file path
     * @param filePath The full path to the log file
     */
    void setLogFile(const QString& filePath);
    /**
     * @brief Sets the minimum log level for message filtering
     * @param level The minimum LogLevel to output
     */
    void setLogLevel(LogLevel level);
    /**
     * @brief Enables or disables console output
     * @param enable True to enable console output, false to disable
     * @return The new console output state
     */
    bool enableConsoleOutput(bool enable);

    /**
     * @brief Logs a debug message
     * @param message The log message
     * @param source The source function or class name
     */
    void debug(const QString& message, const QString& source = QString());
    /**
     * @brief Logs an informational message
     * @param message The log message
     * @param source The source function or class name
     */
    void info(const QString& message, const QString& source = QString());
    /**
     * @brief Logs a warning message
     * @param message The log message
     * @param source The source function or class name
     */
    void warning(const QString& message, const QString& source = QString());
    /**
     * @brief Logs an error message
     * @param message The log message
     * @param source The source function or class name
     */
    void error(const QString& message, const QString& source = QString());
    /**
     * @brief Logs a fatal error message
     * @param message The log message
     * @param source The source function or class name
     */
    void fatal(const QString& message, const QString& source = QString());

    /**
     * @brief Logs a message with key-value pairs
     * @param level The log level
     * @param data The key-value pairs to log
     * @param source The source function or class name
     */
    void log(LogLevel level, const QString& message, const QString& source = QString());

    /**
     * @brief Logs a message with key-value pairs
     * @param level The log level
     * @param data The key-value pairs to log
     * @param source The source function or class name
     */
    void logData(LogLevel level, const QMap<QString, QVariant>& data, const QString& source = QString());

    /**
     * @brief Gets the current log level
     * @return The current log level
     */
    LogLevel getLogLevel() const;

    /**
     * @brief Gets the current log file path
     * @return The current log file path
     */
    QString getLogFilePath() const;

    /**
     * @brief Checks if console output is enabled
     * @return True if console output is enabled, false otherwise
     */
    bool isConsoleOutputEnabled() const;

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger();

    static Logger* m_instance;
    QFile m_logFile;
    QTextStream m_logStream;
    LogLevel m_logLevel;
    bool m_consoleOutput;
    QMutex m_mutex;
    QString m_logFilePath;

    /**
     * @brief Converts a LogLevel to its string representation
     * @param level The LogLevel to convert
     * @return String representation of the log level
     */
    QString logLevelToString(LogLevel level);
    /**
     * @brief Formats a log message with timestamp and metadata
     * @param level The log level for the message
     * @param message The message to format
     * @param source The source of the message
     * @return Formatted log message string
     */
    QString formatLogMessage(LogLevel level, const QString& message, const QString& source);
    /**
     * @brief Writes a message to the log file
     * @param message The formatted message to write
     */
    void writeToLog(const QString& message);
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::instance()->debug(msg, Q_FUNC_INFO)
#define LOG_INFO(msg) Logger::instance()->info(msg, Q_FUNC_INFO)
#define LOG_WARNING(msg) Logger::instance()->warning(msg, Q_FUNC_INFO)
#define LOG_ERROR(msg) Logger::instance()->error(msg, Q_FUNC_INFO)
#define LOG_FATAL(msg) Logger::instance()->fatal(msg, Q_FUNC_INFO)

// Macro for logging with data
#define LOG_DATA(level, data) Logger::instance()->logData(level, data, Q_FUNC_INFO)
