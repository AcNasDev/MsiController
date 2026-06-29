#pragma once

class EcService;
class SupportConfigRepository;
class SystemAccess;

bool registerEcSys(EcService& service, SupportConfigRepository& supportConfig, SystemAccess* systemAccess = nullptr);
