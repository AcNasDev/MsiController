#pragma once
#include <QDBusContext>
#include <QDBusVariant>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <functional>

#include "ecserviceabstract.h"
#include "parameter.h"

class IOBuffer;
class SupportConfigRepository;

inline size_t qHash(const QVariant& v, size_t seed = 0) noexcept {
    return qHashMulti(seed, v.toString(), v.typeId());
}

class EcService : public EcServiceAbstract {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec")
public:
    class ProfileRegistrationScope {
    public:
        explicit ProfileRegistrationScope(EcService& service);
        ~ProfileRegistrationScope();

        ProfileRegistrationScope(const ProfileRegistrationScope&) = delete;
        ProfileRegistrationScope& operator=(const ProfileRegistrationScope&) = delete;

    private:
        EcService* mService{nullptr};
    };

    explicit EcService(QObject* parent = nullptr);
    ~EcService();
    void registerParameter(Parameter* param);
    void registerProfileObject(QObject* object);
    void clearProfileScope();
    Parameter* parameter(const QVariant& name) const;
    void setIoBuffer(IOBuffer* buffer);
    void setSupportConfigRepository(SupportConfigRepository* repository);
    void setSupportProfileApplier(std::function<bool(QString*)> applier);
    bool applySupportProfile(QString* errorMessage = nullptr);

    QDBusVariant availableParameters() const override;
    QDBusVariant readParameter(const QDBusVariant& name) const override;
    QDBusVariant availableValues(const QDBusVariant& name) const override;
    bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) override;
    QDBusVariant writeParameters(const QDBusVariant& updates) override;
    QDBusVariant readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const override;
    QDBusVariant writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) override;
    QDBusVariant writeEcMemoryBits(const QDBusVariant& offset,
                                   const QDBusVariant& mask,
                                   const QDBusVariant& value) override;
    QDBusVariant supportedDeviceProfiles() const override;
    QDBusVariant activeDeviceProfile() const override;
    QDBusVariant saveSupportedDeviceProfile(const QDBusVariant& profile) override;
    QDBusVariant removeSupportedDeviceProfile(const QDBusVariant& profileId) override;

private:
    QHash<QVariant, Parameter*> mParameters;
    QSet<QVariant> mProfileParameterNames;
    QList<QObject*> mProfileObjects;
    IOBuffer* mIoBuffer{nullptr};
    SupportConfigRepository* mSupportConfigRepository{nullptr};
    std::function<bool(QString*)> mSupportProfileApplier;
    int mProfileRegistrationDepth{0};

    void beginProfileRegistration();
    void endProfileRegistration();
};
