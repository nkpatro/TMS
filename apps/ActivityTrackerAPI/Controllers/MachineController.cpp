#include "MachineController.h"
#include "logger/logger.h"
#include "dbservice/dbmanager.h"
#include "httpserver/response.h"
#include <QJsonDocument>
#include <QUuid>
#include <Core/ModelFactory.h>

#include "SystemInfo.h"
#include "Repositories/MachineRepository.h"

// Constructor with default parameters
MachineController::MachineController(QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_repository(nullptr)
    , m_initialized(false)
{
    LOG_DEBUG("MachineController created");
}

// Constructor with repository
MachineController::MachineController(MachineRepository* repository, QObject *parent)
    : ApiControllerBase(parent)  // Changed from Http::Controller to ApiControllerBase
    , m_repository(repository)
    , m_initialized(false)
{
    LOG_DEBUG("MachineController created with existing repository");

    // If a repository was provided, check if it's initialized
    if (m_repository && m_repository->isInitialized()) {
        m_initialized = true;
    }
}

// Destructor
MachineController::~MachineController()
{
    if (m_repository) {
        delete m_repository;
        m_repository = nullptr;
    }
    LOG_DEBUG("MachineController destroyed");
}

// Initialize method
bool MachineController::initialize()
{
    if (m_initialized) {
        LOG_WARNING("MachineController already initialized");
        return true;
    }

    LOG_DEBUG("Initializing MachineController");

    try {
        // Ensure DbManager is initialized first
        if (!DbManager::instance().isInitialized()) {
            LOG_ERROR("DbManager not initialized");
            return false;
        }

        // Create repository - it should automatically use DbManager
        m_repository = new MachineRepository(this);

        // Check if repository is properly initialized
        if (!m_repository->isInitialized()) {
            LOG_ERROR("Failed to initialize MachineRepository");
            return false;
        }

        m_initialized = true;
        LOG_INFO("MachineController initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception during MachineController initialization: %1").arg(e.what()));
        return false;
    }
}

void MachineController::setupRoutes(QHttpServer& server)
{
    LOG_INFO("Setting up MachineController routes");

    // GET routes
    // Get all machines
    server.route("/api/machines", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->getAllMachines(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get active machines
    server.route("/api/machines/active", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->getActiveMachines(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Get machines by name
    server.route("/api/machines/name/<arg>", QHttpServerRequest::Method::Get,
        [this](const QString &name) {
            auto response = this->getMachinesByName(name);
            return createSuccessResponse(machinesToJsonArray(response));
        });

    // Get specific machine by ID
    server.route("/api/machines/<arg>", QHttpServerRequest::Method::Get,
        [this](const QString &id) {
            auto response = this->getMachineById(id);
            return response;
        });

    // POST routes
    // Create new machine
    server.route("/api/machines", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->createMachine(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Register machine (with more flexible registration process)
    server.route("/api/machines/register", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->registerMachine(request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // PUT routes
    // Update machine by ID
    server.route("/api/machines/<arg>", QHttpServerRequest::Method::Put,
        [this](const QString &id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->updateMachine(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update machine status
    server.route("/api/machines/<arg>/status", QHttpServerRequest::Method::Put,
        [this](const QString &id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->updateMachineStatus(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // Update last seen timestamp
    server.route("/api/machines/<arg>/lastseen", QHttpServerRequest::Method::Put,
        [this](const QString &id, const QHttpServerRequest &request) {
            logRequestReceived(request);
            auto response = this->updateLastSeen(id, request);
            logRequestCompleted(request, response.statusCode());
            return response;
        });

    // DELETE routes
    // Remove machine by ID
    server.route("/api/machines/<arg>", QHttpServerRequest::Method::Delete,
        [this](const QString &id) {
            auto response = this->deleteMachine(id);
            return response;
        });

    LOG_INFO("MachineController routes configured successfully");
}

QHttpServerResponse MachineController::getMachineById(const QString &id)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing GET machine by ID request: %1").arg(id));

    QUuid machineId = QUuid(id);
    if (machineId.isNull()) {
        LOG_WARNING("Invalid machine ID format");
        return createErrorResponse("Invalid machine ID format", QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        auto machine = m_repository->getById(machineId);
        if (!machine) {
            LOG_WARNING(QString("Machine not found with ID: %1").arg(id));
            return Http::Response::notFound("Machine not found");
        }

        return createSuccessResponse(machineToJson(machine.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting machine: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve machine: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::getAllMachines(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET all machines request");

    try {
        auto machines = m_repository->getAll();
        return createSuccessResponse(machinesToJsonArray(machines));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting all machines: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve machines: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::getActiveMachines(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing GET active machines request");

    try {
        auto machines = m_repository->getActiveMachines();
        return createSuccessResponse(machinesToJsonArray(machines));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting active machines: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to retrieve active machines: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::createMachine(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing CREATE machine request");

    // Parse request body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("JSON parse error: %1").arg(parseError.errorString()));
        return createErrorResponse("Invalid JSON: " + parseError.errorString(), QHttpServerResponder::StatusCode::BadRequest);
    }

    if (!doc.isObject()) {
        LOG_WARNING("Request body is not a JSON object");
        return createErrorResponse("Request body must be a JSON object", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = doc.object();

    // Validate required fields
    QStringList missingFields;
    if (!validateMachineJson(json, missingFields)) {
        LOG_WARNING(QString("Missing required fields: %1").arg(missingFields.join(", ")));
        return createErrorResponse("Missing required fields: " + missingFields.join(", "), QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        // Check for duplicate uniqueId
        if (!json["machineUniqueId"].toString().isEmpty()) {
            auto existingMachine = m_repository->getByUniqueId(json["machineUniqueId"].toString());
            if (existingMachine) {
                LOG_WARNING(QString("Machine with uniqueId %1 already exists").arg(json["machineUniqueId"].toString()));
                return createErrorResponse("Machine with this unique ID already exists", QHttpServerResponder::StatusCode::BadRequest);
            }
        }

        // Check for duplicate macAddress
        if (!json["macAddress"].toString().isEmpty()) {
            auto existingMachine = m_repository->getByMacAddress(json["macAddress"].toString());
            if (existingMachine) {
                LOG_WARNING(QString("Machine with MAC address %1 already exists").arg(json["macAddress"].toString()));
                return createErrorResponse("Machine with this MAC address already exists", QHttpServerResponder::StatusCode::BadRequest);
            }
        }

        // Create machine
        MachineModel *machine = new MachineModel();
        machine->setName(json["name"].toString());
        machine->setMachineUniqueId(json["machineUniqueId"].toString());
        machine->setMacAddress(json["macAddress"].toString());
        machine->setOperatingSystem(json["operatingSystem"].toString());
        machine->setCpuInfo(json.value("cpuInfo").toString());
        machine->setGpuInfo(json.value("gpuInfo").toString());
        machine->setRamSizeGB(json.value("ramSizeGB").toInt());
        machine->setLastKnownIp(json.value("lastKnownIp").toString());

        // Set user ID if provided
        if (json.contains("userId") && !json["userId"].toString().isEmpty()) {
            machine->setCreatedBy(QUuid(json["userId"].toString()));
            machine->setUpdatedBy(QUuid(json["userId"].toString()));
        }

        // Use ModelFactory to set creation timestamps
        ModelFactory::setCreationTimestamps(machine);

        if (m_repository->save(machine)) {
            QJsonObject result = machineToJson(machine);
            delete machine; // Repository makes a copy, safe to delete
            return createSuccessResponse(result, QHttpServerResponder::StatusCode::Created);
        }

        delete machine;
        return createErrorResponse("Failed to save machine", QHttpServerResponder::StatusCode::InternalServerError);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception creating machine: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to create machine: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::updateMachine(const QString &id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE machine request: %1").arg(id));

    QUuid machineId = QUuid(id);
    if (machineId.isNull()) {
        LOG_WARNING("Invalid machine ID format");
        return createErrorResponse("Invalid machine ID format", QHttpServerResponder::StatusCode::BadRequest);
    }

    // Parse request body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("JSON parse error: %1").arg(parseError.errorString()));
        return createErrorResponse("Invalid JSON: " + parseError.errorString(), QHttpServerResponder::StatusCode::BadRequest);
    }

    if (!doc.isObject()) {
        LOG_WARNING("Request body is not a JSON object");
        return createErrorResponse("Request body must be a JSON object", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = doc.object();

    try {
        // Get existing machine
        auto machine = m_repository->getById(machineId);
        if (!machine) {
            LOG_WARNING(QString("Machine not found with ID: %1").arg(id));
            return Http::Response::notFound("Machine not found");
        }

        // Update fields if provided
        if (json.contains("name")) {
            machine->setName(json["name"].toString());
        }

        if (json.contains("machineUniqueId")) {
            // Check for uniqueness if changed
            QString newUniqueId = json["machineUniqueId"].toString();
            if (newUniqueId != machine->machineUniqueId() && !newUniqueId.isEmpty()) {
                auto existingMachine = m_repository->getByUniqueId(newUniqueId);
                if (existingMachine && existingMachine->id() != machine->id()) {
                    LOG_WARNING(QString("Machine with uniqueId %1 already exists").arg(newUniqueId));
                    return createErrorResponse("Machine with this unique ID already exists", QHttpServerResponder::StatusCode::BadRequest);
                }
            }
            machine->setMachineUniqueId(newUniqueId);
        }

        if (json.contains("macAddress")) {
            // Check for uniqueness if changed
            QString newMacAddress = json["macAddress"].toString();
            if (newMacAddress != machine->macAddress() && !newMacAddress.isEmpty()) {
                auto existingMachine = m_repository->getByMacAddress(newMacAddress);
                if (existingMachine && existingMachine->id() != machine->id()) {
                    LOG_WARNING(QString("Machine with MAC address %1 already exists").arg(newMacAddress));
                    return createErrorResponse("Machine with this MAC address already exists", QHttpServerResponder::StatusCode::BadRequest);
                }
            }
            machine->setMacAddress(newMacAddress);
        }

        if (json.contains("operatingSystem")) {
            machine->setOperatingSystem(json["operatingSystem"].toString());
        }

        if (json.contains("cpuInfo")) {
            machine->setCpuInfo(json["cpuInfo"].toString());
        }

        if (json.contains("gpuInfo")) {
            machine->setGpuInfo(json["gpuInfo"].toString());
        }

        if (json.contains("ramSizeGB")) {
            machine->setRamSizeGB(json["ramSizeGB"].toInt());
        }

        if (json.contains("lastKnownIp")) {
            machine->setLastKnownIp(json["lastKnownIp"].toString());
        }

        // User ID for tracking changes
        QUuid userId;
        if (json.contains("userId") && !json["userId"].toString().isEmpty()) {
            userId = QUuid(json["userId"].toString());
        }

        // Use ModelFactory to update timestamps
        ModelFactory::setUpdateTimestamps(machine.data(), userId);

        if (m_repository->update(machine.data())) {
            return createSuccessResponse(machineToJson(machine.data()));
        }

        return createErrorResponse("Failed to update machine", QHttpServerResponder::StatusCode::InternalServerError);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating machine: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update machine: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::deleteMachine(const QString &id)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing DELETE machine request: %1").arg(id));

    QUuid machineId = QUuid(id);
    if (machineId.isNull()) {
        LOG_WARNING("Invalid machine ID format");
        return createErrorResponse("Invalid machine ID format", QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        // Verify machine exists
        auto machine = m_repository->getById(machineId);
        if (!machine) {
            LOG_WARNING(QString("Machine not found with ID: %1").arg(id));
            return Http::Response::notFound("Machine not found");
        }

        if (m_repository->remove(machineId)) {
            return createSuccessResponse(QJsonObject{{"deleted", true}});
        }

        return createErrorResponse("Failed to delete machine", QHttpServerResponder::StatusCode::InternalServerError);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception deleting machine: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to delete machine: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::updateMachineStatus(const QString &id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE machine status request: %1").arg(id));

    QUuid machineId = QUuid(id);
    if (machineId.isNull()) {
        LOG_WARNING("Invalid machine ID format");
        return createErrorResponse("Invalid machine ID format", QHttpServerResponder::StatusCode::BadRequest);
    }

    // Parse request body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("JSON parse error: %1").arg(parseError.errorString()));
        return createErrorResponse("Invalid JSON: " + parseError.errorString(), QHttpServerResponder::StatusCode::BadRequest);
    }

    if (!doc.isObject()) {
        LOG_WARNING("Request body is not a JSON object");
        return createErrorResponse("Request body must be a JSON object", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = doc.object();

    if (!json.contains("active")) {
        LOG_WARNING("Missing 'active' field in request");
        return createErrorResponse("Missing 'active' field", QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        // Get existing machine
        auto machine = m_repository->getById(machineId);
        if (!machine) {
            LOG_WARNING(QString("Machine not found with ID: %1").arg(id));
            return Http::Response::notFound("Machine not found");
        }

        bool active = json["active"].toBool();

        if (machine->active() == active) {
            // No change needed
            return createSuccessResponse(machineToJson(machine.data()));
        }

        machine->setActive(active);

        // Get user ID if provided
        QUuid userId;
        if (json.contains("userId") && !json["userId"].toString().isEmpty()) {
            userId = QUuid(json["userId"].toString());
        }

        // Use ModelFactory to update timestamps
        ModelFactory::setUpdateTimestamps(machine.data(), userId);

        if (m_repository->update(machine.data())) {
            return createSuccessResponse(machineToJson(machine.data()));
        }

        return createErrorResponse("Failed to update machine status", QHttpServerResponder::StatusCode::InternalServerError);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating machine status: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update machine status: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::updateLastSeen(const QString &id, const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG(QString("Processing UPDATE last seen request: %1").arg(id));

    QUuid machineId = QUuid(id);
    if (machineId.isNull()) {
        LOG_WARNING("Invalid machine ID format");
        return createErrorResponse("Invalid machine ID format", QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        // Verify machine exists
        auto machine = m_repository->getById(machineId);
        if (!machine) {
            LOG_WARNING(QString("Machine not found with ID: %1").arg(id));
            return Http::Response::notFound("Machine not found");
        }

        QDateTime timestamp = QDateTime::currentDateTimeUtc();

        // Check if request body contains timestamp
        if (request.body().size() > 0) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject json = doc.object();
                if (json.contains("timestamp")) {
                    timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
                    if (!timestamp.isValid()) {
                        timestamp = QDateTime::currentDateTimeUtc();
                    }
                }
            }
        }

        if (m_repository->updateLastSeen(machineId, timestamp)) {
            // Reload machine with updated data
            machine = m_repository->getById(machineId);
            return createSuccessResponse(machineToJson(machine.data()));
        }

        return createErrorResponse("Failed to update last seen timestamp", QHttpServerResponder::StatusCode::InternalServerError);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception updating last seen: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to update last seen timestamp: %1").arg(e.what()));
    }
}

QHttpServerResponse MachineController::registerMachine(const QHttpServerRequest &request)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return createErrorResponse("Controller not initialized", QHttpServerResponder::StatusCode::InternalServerError);
    }

    LOG_DEBUG("Processing REGISTER machine request");

    // Check authentication - we accept service tokens for registration
    QJsonObject userData;
    bool isAuthorized = isServiceTokenAuthorized(request, userData);

    // We'll allow registration even without authentication,
    // but we'll use the auth data if available

    // Parse request body
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("JSON parse error: %1").arg(parseError.errorString()));
        return createErrorResponse("Invalid JSON: " + parseError.errorString(), QHttpServerResponder::StatusCode::BadRequest);
    }

    if (!doc.isObject()) {
        LOG_WARNING("Request body is not a JSON object");
        return createErrorResponse("Request body must be a JSON object", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = doc.object();

    // Validate required fields for registration
    if (!json.contains("name") || !json.contains("operatingSystem")) {
        LOG_WARNING("Missing required fields for machine registration");
        return createErrorResponse("Missing required fields. Need: name, operatingSystem", QHttpServerResponder::StatusCode::BadRequest);
    }

    try {
        QString name = json["name"].toString();
        QString uniqueId = json.contains("machineUniqueId") ? json["machineUniqueId"].toString() : "";
        QString macAddress = json.contains("macAddress") ? json["macAddress"].toString() : "";
        QString os = json["operatingSystem"].toString();

        // At least one of machineUniqueId or macAddress must be provided
        if (uniqueId.isEmpty() && macAddress.isEmpty()) {
            LOG_WARNING("Both machineUniqueId and macAddress are empty");
            return createErrorResponse("At least one of machineUniqueId or macAddress must be provided",
                                     QHttpServerResponder::StatusCode::BadRequest);
        }

        // Get IP from request if not provided
        QString ipStr = json.contains("lastKnownIp") ? json["lastKnownIp"].toString() : request.remoteAddress().toString();
        QHostAddress ipAddress(ipStr);

        // Get user ID if provided or from auth
        QUuid userId;
        if (json.contains("userId") && !json["userId"].toString().isEmpty()) {
            userId = QUuid(json["userId"].toString());
        } else if (isAuthorized && userData.contains("id")) {
            userId = QUuid(userData["id"].toString());
        }

        // First try to find existing machine by unique ID if provided
        QSharedPointer<MachineModel> machine;

        if (!uniqueId.isEmpty()) {
            LOG_DEBUG(QString("Looking for existing machine with unique ID: %1").arg(uniqueId));
            machine = m_repository->getByUniqueId(uniqueId);
        }

        // Then try by MAC address if provided
        if (!machine && !macAddress.isEmpty()) {
            LOG_DEBUG(QString("Looking for existing machine with MAC: %1").arg(macAddress));
            machine = m_repository->getByMacAddress(macAddress);
        }

        // Finally, try by name
        if (!machine) {
            LOG_DEBUG(QString("Looking for existing machine with name: %1").arg(name));
            auto machinesByName = m_repository->getMachinesByName(name);
            if (!machinesByName.isEmpty()) {
                machine = machinesByName.first();
            }
        }

        // If found, update the machine
        if (machine) {
            LOG_INFO(QString("Found existing machine: %1 (ID: %2)").arg(machine->name(), machine->id().toString()));

            // Update fields if needed
            bool needsUpdate = false;

            if (!macAddress.isEmpty() && machine->macAddress() != macAddress) {
                machine->setMacAddress(macAddress);
                needsUpdate = true;
            }

            if (!uniqueId.isEmpty() && machine->machineUniqueId() != uniqueId) {
                machine->setMachineUniqueId(uniqueId);
                needsUpdate = true;
            }

            if (machine->operatingSystem() != os) {
                machine->setOperatingSystem(os);
                needsUpdate = true;
            }

            if (!ipStr.isEmpty() && machine->lastKnownIp() != ipStr) {
                machine->setLastKnownIp(ipStr);
                needsUpdate = true;
            }

            // Update CPU, GPU, RAM info if provided
            if (json.contains("cpuInfo") && !json["cpuInfo"].toString().isEmpty() &&
                json["cpuInfo"].toString() != machine->cpuInfo()) {
                machine->setCpuInfo(json["cpuInfo"].toString());
                needsUpdate = true;
            }

            if (json.contains("gpuInfo") && !json["gpuInfo"].toString().isEmpty() &&
                json["gpuInfo"].toString() != machine->gpuInfo()) {
                machine->setGpuInfo(json["gpuInfo"].toString());
                needsUpdate = true;
            }

            if (json.contains("ramSizeGB") && json["ramSizeGB"].toInt() > 0 &&
                json["ramSizeGB"].toInt() != machine->ramSizeGB()) {
                machine->setRamSizeGB(json["ramSizeGB"].toInt());
                needsUpdate = true;
            }

            // Update last seen time and user info
            machine->setLastSeenAt(QDateTime::currentDateTimeUtc());
            needsUpdate = true;

            if (!userId.isNull()) {
                machine->setUpdatedBy(userId);
            }

            // Save updates if needed
            if (needsUpdate) {
                // Use ModelFactory to update timestamps
                ModelFactory::setUpdateTimestamps(machine.data(), userId);

                LOG_INFO("Updating machine with new information");
                m_repository->update(machine.data());
            }
        }
        // If not found, create new machine
        else {
            LOG_INFO(QString("Creating new machine: %1").arg(name));

            MachineModel *newMachine = new MachineModel();
            newMachine->setName(name);
            newMachine->setMachineUniqueId(uniqueId);
            newMachine->setMacAddress(macAddress);
            newMachine->setOperatingSystem(os);
            newMachine->setLastKnownIp(ipStr);
            newMachine->setLastSeenAt(QDateTime::currentDateTimeUtc());

            if (json.contains("cpuInfo") && !json["cpuInfo"].toString().isEmpty()) {
                newMachine->setCpuInfo(json["cpuInfo"].toString());
            }

            if (json.contains("gpuInfo") && !json["gpuInfo"].toString().isEmpty()) {
                newMachine->setGpuInfo(json["gpuInfo"].toString());
            }

            if (json.contains("ramSizeGB") && json["ramSizeGB"].toInt() > 0) {
                newMachine->setRamSizeGB(json["ramSizeGB"].toInt());
            }

            // Use ModelFactory to set creation timestamps
            ModelFactory::setCreationTimestamps(newMachine, userId);

            if (m_repository->save(newMachine)) {
                LOG_INFO(QString("New machine created with ID: %1").arg(newMachine->id().toString()));
                machine = QSharedPointer<MachineModel>(newMachine);
            } else {
                LOG_ERROR("Failed to save new machine");
                delete newMachine;
                return createErrorResponse("Failed to create machine record",
                                          QHttpServerResponder::StatusCode::InternalServerError);
            }
        }

        // Return the machine info
        return createSuccessResponse(machineToJson(machine.data()));
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception registering machine: %1").arg(e.what()));
        return createErrorResponse(QString("Failed to register machine: %1").arg(e.what()));
    }
}

QList<QSharedPointer<MachineModel>> MachineController::getMachinesByName(const QString &name)
{
    if (!m_initialized) {
        LOG_ERROR("MachineController not initialized");
        return QList<QSharedPointer<MachineModel>>();
    }

    LOG_DEBUG(QString("Getting machines by name: %1").arg(name));

    try {
        return m_repository->getMachinesByName(name);
    }
    catch (const std::exception &e) {
        LOG_ERROR(QString("Exception getting machines by name: %1").arg(e.what()));
        return QList<QSharedPointer<MachineModel>>();
    }
}

QJsonObject MachineController::machineToJson(const MachineModel* machine) const {
    if (!machine) {
        return QJsonObject();
    }

    return ModelFactory::modelToJson(machine);
}

QJsonArray MachineController::machinesToJsonArray(const QList<QSharedPointer<MachineModel>>& machines) const {
    return ModelFactory::modelsToJsonArray(machines);
}

bool MachineController::validateMachineJson(const QJsonObject& json, QStringList& missingFields) const {
    missingFields.clear();

    // Check for required fields
    if (!json.contains("name") || json["name"].toString().isEmpty()) {
        missingFields.append("name");
    }

    // Either machineUniqueId or macAddress is required
    if ((!json.contains("machineUniqueId") || json["machineUniqueId"].toString().isEmpty()) &&
        (!json.contains("macAddress") || json["macAddress"].toString().isEmpty())) {
        missingFields.append("machineUniqueId or macAddress");
        }

    if (!json.contains("operatingSystem") || json["operatingSystem"].toString().isEmpty()) {
        missingFields.append("operatingSystem");
    }

    return missingFields.isEmpty();
}

bool MachineController::isServiceTokenAuthorized(const QHttpServerRequest& request, QJsonObject& userData) {
    // Use the method from ApiControllerBase
    return ApiControllerBase::isServiceTokenAuthorized(request, userData);
}

