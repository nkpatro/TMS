// File: ActivityTrackerDbTemplates.cpp
#include "dbservice/dbservice.hpp"
#include "Repositories/BaseRepository.h"

// Include your model headers
#include "Models/UserModel.h"
#include "Models/MachineModel.h"
#include "Models/SessionModel.h"
#include "Models/ActivityEventModel.h"
#include "Models/AfkPeriodModel.h"
#include "Models/ApplicationModel.h"
#include "Models/AppUsageModel.h"
#include "Models/DisciplineModel.h"
#include "Models/SystemMetricsModel.h"
#include "Models/RoleModel.h"
#include "Models/SessionEventModel.h"
#include "Models/UserRoleDisciplineModel.h"

// Include your repository headers
// #include "Repositories/UserRepository.h"
// #include "Repositories/RoleRepository.h"
// #include "Repositories/DisciplineRepository.h"
// #include "Repositories/MachineRepository.h"
// #include "Repositories/ApplicationRepository.h"
// #include "Repositories/SessionRepository.h"
// #include "Repositories/AfkPeriodRepository.h"
// #include "Repositories/AppUsageRepository.h"
// #include "Repositories/SystemMetricsRepository.h"
// #include "Repositories/SessionEventRepository.h"
// #include "Repositories/UserRoleDisciplineRepository.h"


// Explicit template instantiations for each model type
template class DbService<UserModel>;
template class DbService<MachineModel>;
template class DbService<SessionModel>;
template class DbService<ActivityEventModel>;
template class DbService<AfkPeriodModel>;
template class DbService<ApplicationModel>;
template class DbService<AppUsageModel>;
template class DbService<DisciplineModel>;
template class DbService<SystemMetricsModel>;
template class DbService<RoleModel>;
template class DbService<SessionEventModel>;
template class DbService<UserRoleDisciplineModel>;

template class BaseRepository<UserModel>;
template class BaseRepository<MachineModel>;
template class BaseRepository<SessionModel>;
template class BaseRepository<ActivityEventModel>;
template class BaseRepository<AfkPeriodModel>;
template class BaseRepository<ApplicationModel>;
template class BaseRepository<AppUsageModel>;
template class BaseRepository<DisciplineModel>;
template class BaseRepository<SystemMetricsModel>;
template class BaseRepository<RoleModel>;
template class BaseRepository<SessionEventModel>;
template class BaseRepository<UserRoleDisciplineModel>;

// template class UserRepository;
// template class RoleRepository;
// template class DisciplineRepository;
// template class MachineRepository;
// template class ApplicationRepository;
// template class SessionRepository;
// template class AfkPeriodRepository;
// template class AppUsageRepository;
// template class SystemMetricsRepository;
// template class SessionEventRepository;
// template class UserRoleDisciplineRepository;

