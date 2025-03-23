#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonObject>

#include "managers/ConfigManager.h"
#include "core/APIManager.h"

// Mock APIManager for testing
class MockAPIManager : public APIManager
{
public:
    MockAPIManager(QObject* parent = nullptr) : APIManager(parent), m_shouldSucceed(true) {}

    void setShouldSucceed(bool succeed) { m_shouldSucceed = succeed; }
    void setMockConfig(const QJsonObject& config) { m_mockConfig = config; }

    bool getServerConfiguration(QJsonObject& configData) override {
        if (m_shouldSucceed) {
            configData = m_mockConfig;
            return true;
        }
        return false;
    }

    bool initialize(const QString& serverUrl) override {
        m_serverUrl = serverUrl;
        return true;
    }

private:
    bool m_shouldSucceed;
    QJsonObject m_mockConfig;
    QString m_serverUrl;
};

class ConfigManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Create temporary directory for config files
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        // Set environment variable to redirect config file location
        qputenv("ACTIVITY_TRACKER_CONFIG_DIR", m_tempDir->path().toUtf8());
    }

    void cleanupTestCase() {
        // Clean up temporary directory
        m_tempDir.reset();
    }

    void init() {
        // Set up fresh config manager for each test
        m_configManager = new ConfigManager();
        m_mockApi = new MockAPIManager();

        QVERIFY(m_configManager->initialize(m_mockApi));
    }

    void cleanup() {
        delete m_configManager;
        delete m_mockApi;
    }

    void testDefaultValues() {
        // Test default values are set correctly
        QCOMPARE(m_configManager->serverUrl(), QString("http://localhost:8080"));
        QCOMPARE(m_configManager->dataSendInterval(), 60000);
        QCOMPARE(m_configManager->idleTimeThreshold(), 300000);
        QCOMPARE(m_configManager->trackKeyboardMouse(), true);
        QCOMPARE(m_configManager->trackApplications(), true);
        QCOMPARE(m_configManager->trackSystemMetrics(), true);
        QCOMPARE(m_configManager->multiUserMode(), false);
        QCOMPARE(m_configManager->defaultUsername(), QString(""));
        QCOMPARE(m_configManager->logLevel(), QString("info"));
    }

    void testSaveAndLoad() {
        // Modify some values
        m_configManager->setServerUrl("https://example.com/api");
        m_configManager->setDataSendInterval(30000);
        m_configManager->setIdleTimeThreshold(120000);
        m_configManager->setMachineId("test-machine-id");
        m_configManager->setMultiUserMode(true);

        // Save to file
        QVERIFY(m_configManager->saveLocalConfig());

        // Create new config manager and load saved file
        ConfigManager newConfig;
        newConfig.initialize(nullptr);
        QVERIFY(newConfig.loadLocalConfig());

        // Verify values were loaded correctly
        QCOMPARE(newConfig.serverUrl(), QString("https://example.com/api"));
        QCOMPARE(newConfig.dataSendInterval(), 30000);
        QCOMPARE(newConfig.idleTimeThreshold(), 120000);
        QCOMPARE(newConfig.machineId(), QString("test-machine-id"));
        QCOMPARE(newConfig.multiUserMode(), true);
    }

    void testSignalsEmitted() {
        // Set up signal spy
        QSignalSpy configChangedSpy(m_configManager, &ConfigManager::configChanged);
        QSignalSpy machineIdChangedSpy(m_configManager, &ConfigManager::machineIdChanged);

        // Modify a value
        m_configManager->setServerUrl("https://newserver.com/api");

        // Verify signal was emitted
        QCOMPARE(configChangedSpy.count(), 1);

        // Modify machine ID - should trigger both signals
        m_configManager->setMachineId("new-machine-id");

        QCOMPARE(configChangedSpy.count(), 2);
        QCOMPARE(machineIdChangedSpy.count(), 1);
        QCOMPARE(machineIdChangedSpy.at(0).at(0).toString(), QString("new-machine-id"));
    }

    void testServerConfigUpdate() {
        // Set up mock server config
        QJsonObject serverConfig;
        serverConfig["ServerUrl"] = "https://server.example.com/api";
        serverConfig["DataSendInterval"] = 15000;
        serverConfig["TrackSystemMetrics"] = false;

        m_mockApi->setMockConfig(serverConfig);

        // Fetch server config
        QVERIFY(m_configManager->fetchServerConfig());

        // Verify values were updated
        QCOMPARE(m_configManager->serverUrl(), QString("https://server.example.com/api"));
        QCOMPARE(m_configManager->dataSendInterval(), 15000);
        QCOMPARE(m_configManager->trackSystemMetrics(), false);

        // Values not in server config should remain unchanged
        QCOMPARE(m_configManager->idleTimeThreshold(), 300000);
        QCOMPARE(m_configManager->trackKeyboardMouse(), true);
    }

    void testFetchServerConfigFailure() {
        // Set mock API to fail
        m_mockApi->setShouldSucceed(false);

        // Save original values
        QString originalUrl = m_configManager->serverUrl();
        int originalInterval = m_configManager->dataSendInterval();

        // Attempt to fetch server config
        QVERIFY(!m_configManager->fetchServerConfig());

        // Verify values remain unchanged
        QCOMPARE(m_configManager->serverUrl(), originalUrl);
        QCOMPARE(m_configManager->dataSendInterval(), originalInterval);
    }

    void testValidation() {
        // Test validation of values

        // Valid values should be accepted
        m_configManager->setDataSendInterval(10000);
        QCOMPARE(m_configManager->dataSendInterval(), 10000);

        // Invalid values (too low) should be corrected or rejected
        m_configManager->setDataSendInterval(-1000);
        QVERIFY(m_configManager->dataSendInterval() >= 0);

        m_configManager->setIdleTimeThreshold(500);
        QVERIFY(m_configManager->idleTimeThreshold() >= 1000);
    }

private:
    ConfigManager* m_configManager;
    MockAPIManager* m_mockApi;
    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(ConfigManagerTest)
#include "ConfigManagerTest.moc"