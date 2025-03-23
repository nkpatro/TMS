#include "AppMonitorMac.h"
#include "../../utils/Logger.h"
#include <QFileInfo>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

// Objective-C delegate to handle NSWorkspace notifications
@interface AppMonitorDelegate : NSObject
{
    AppMonitorMac* m_monitor;
}

- (id)initWithMonitor:(AppMonitorMac*)monitor;
- (void)applicationActivated:(NSNotification*)notification;
- (void)applicationDeactivated:(NSNotification*)notification;

@end

@implementation AppMonitorDelegate

- (id)initWithMonitor:(AppMonitorMac*)monitor
{
    self = [super init];
    if (self) {
        m_monitor = monitor;
    }
    return self;
}

- (void)applicationActivated:(NSNotification*)notification
{
    NSRunningApplication* app = [[notification userInfo] objectForKey:NSWorkspaceApplicationKey];

    if (app) {
        NSString* appName = [app localizedName];
        NSString* appPath = [[app bundleURL] path];

        QString qAppName = QString::fromNSString(appName);
        QString qAppPath = QString::fromNSString(appPath);

        // Get the window title separately since NSRunningApplication doesn't provide it
        QString qWindowTitle = m_monitor->currentWindowTitle();

        QMetaObject::invokeMethod(m_monitor, "appFocused", Qt::QueuedConnection,
                                 Q_ARG(QString, qAppName),
                                 Q_ARG(QString, qWindowTitle),
                                 Q_ARG(QString, qAppPath));

        QMetaObject::invokeMethod(m_monitor, "appChanged", Qt::QueuedConnection,
                                 Q_ARG(QString, qAppName),
                                 Q_ARG(QString, qWindowTitle),
                                 Q_ARG(QString, qAppPath));
    }
}

- (void)applicationDeactivated:(NSNotification*)notification
{
    NSRunningApplication* app = [[notification userInfo] objectForKey:NSWorkspaceApplicationKey];

    if (app) {
        NSString* appName = [app localizedName];
        NSString* appPath = [[app bundleURL] path];

        QString qAppName = QString::fromNSString(appName);
        QString qAppPath = QString::fromNSString(appPath);

        // Get the window title separately since NSRunningApplication doesn't provide it
        QString qWindowTitle = m_monitor->currentWindowTitle();

        QMetaObject::invokeMethod(m_monitor, "appUnfocused", Qt::QueuedConnection,
                                 Q_ARG(QString, qAppName),
                                 Q_ARG(QString, qWindowTitle),
                                 Q_ARG(QString, qAppPath));
    }
}

@end

// AppMonitorMac implementation
AppMonitorMac::AppMonitorMac(QObject *parent)
    : AppMonitor(parent)
    , m_isRunning(false)
    , m_delegate(nil)
{
    // Set up polling timer for checking active window title
    connect(&m_pollTimer, &QTimer::timeout, this, &AppMonitorMac::checkActiveWindow);
    m_pollTimer.setInterval(1000); // Check active window every second
}

AppMonitorMac::~AppMonitorMac()
{
    if (m_isRunning) {
        stop();
    }

    if (m_delegate) {
        [[NSNotificationCenter defaultCenter] removeObserver:m_delegate];
        [m_delegate release];
        m_delegate = nil;
    }
}

bool AppMonitorMac::initialize()
{
    LOG_INFO("Initializing AppMonitorMac");

    // Create our delegate object
    m_delegate = [[AppMonitorDelegate alloc] initWithMonitor:this];

    return true;
}

bool AppMonitorMac::start()
{
    if (m_isRunning) {
        LOG_WARNING("AppMonitorMac is already running");
        return true;
    }

    LOG_INFO("Starting AppMonitorMac");

    // Register for workspace notifications
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    [[workspace notificationCenter] addObserver:m_delegate
                                      selector:@selector(applicationActivated:)
                                          name:NSWorkspaceDidActivateApplicationNotification
                                        object:nil];

    [[workspace notificationCenter] addObserver:m_delegate
                                      selector:@selector(applicationDeactivated:)
                                          name:NSWorkspaceDidDeactivateApplicationNotification
                                        object:nil];

    // Initialize with current window
    updateWindowInfo();

    // Start polling timer for window title updates
    m_pollTimer.start();
    m_isRunning = true;

    LOG_INFO("AppMonitorMac started successfully");
    return true;
}

bool AppMonitorMac::stop()
{
    if (!m_isRunning) {
        LOG_WARNING("AppMonitorMac is not running");
        return true;
    }

    LOG_INFO("Stopping AppMonitorMac");

    // Unregister for workspace notifications
    if (m_delegate) {
        [[NSNotificationCenter defaultCenter] removeObserver:m_delegate];
    }

    // Stop polling timer
    m_pollTimer.stop();
    m_isRunning = false;

    LOG_INFO("AppMonitorMac stopped successfully");
    return true;
}

QString AppMonitorMac::currentAppName() const
{
    return m_currentAppName;
}

QString AppMonitorMac::currentWindowTitle() const
{
    return m_currentWindowTitle;
}

QString AppMonitorMac::currentAppPath() const
{
    return m_currentAppPath;
}

void AppMonitorMac::checkActiveWindow()
{
    // This only updates the window title, since app changes are handled via notifications
    QString oldTitle = m_currentWindowTitle;
    updateWindowInfo();

    // If only the title changed, we don't emit appChanged
    if (m_currentWindowTitle != oldTitle && !m_currentAppName.isEmpty()) {
        emit appChanged(m_currentAppName, m_currentWindowTitle, m_currentAppPath);
    }
}

void AppMonitorMac::updateWindowInfo()
{
    m_currentAppName = getAppName();
    m_currentWindowTitle = getWindowTitle();
    m_currentAppPath = getAppPath();
}

QString AppMonitorMac::getAppName() const
{
    NSRunningApplication* frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
    if (frontApp) {
        NSString* appName = [frontApp localizedName];
        return QString::fromNSString(appName);
    }
    return "";
}

QString AppMonitorMac::getAppPath() const
{
    NSRunningApplication* frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
    if (frontApp) {
        NSString* appPath = [[frontApp bundleURL] path];
        return QString::fromNSString(appPath);
    }
    return "";
}

QString AppMonitorMac::getWindowTitle() const
{
    // Using the Accessibility API to get the window title
    // This is a simplified approach; a more robust implementation would need to handle
    // accessibility permissions and various app-specific edge cases

    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);

    if (windowList) {
        CFIndex count = CFArrayGetCount(windowList);

        // Find the frontmost window (index 0)
        if (count > 0) {
            CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, 0);

            CFStringRef windowName = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
            if (windowName) {
                QString title = QString::fromCFString(windowName);
                CFRelease(windowList);
                return title;
            }
        }

        CFRelease(windowList);
    }

    return "";
}