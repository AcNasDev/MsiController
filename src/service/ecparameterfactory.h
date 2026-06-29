#pragma once

#include <QMap>
#include <QVariant>

class EcService;
class IOBuffer;

class EcParameterFactory {
public:
    explicit EcParameterFactory(IOBuffer* ioBuffer);

    void registerFirmwareParameters(EcService& service) const;
    void registerProfileParameters(EcService& service, const QMap<QString, QVariant>& config) const;

private:
    IOBuffer* mIoBuffer{nullptr};
};
