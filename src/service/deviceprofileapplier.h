#pragma once

#include <QObject>

class EcService;
class IOBuffer;
class SupportConfigRepository;

class DeviceProfileApplier : public QObject {
    Q_OBJECT
public:
    DeviceProfileApplier(EcService* service,
                         IOBuffer* ioBuffer,
                         SupportConfigRepository* supportConfig,
                         QObject* parent = nullptr);

    bool apply(QString* errorMessage = nullptr);

private:
    EcService* mService{nullptr};
    IOBuffer* mIoBuffer{nullptr};
    SupportConfigRepository* mSupportConfig{nullptr};
};
