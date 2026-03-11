#ifndef OBSERVATORY_H
#define OBSERVATORY_H

#include <QString>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

struct CelestialBody {
    QString name;
    QString type;
    double declination;      // в градусах
    double angularSize;      // в угловых минутах
    double magnitude;        // яркость (меньше = ярче)
    double rightAscension;   // прямое восхождение (часы)
    double altitude;         // высота над горизонтом (градусы)
    double azimuth;          // азимут (градусы)
    QString catalogId;       // ID в каталоге

    bool operator==(const CelestialBody& other) const {
        return name == other.name && type == other.type;
    }
};

struct GeoLocation {
    double latitude;
    double longitude;
    QString city;
    QString country;
    bool isValid;
};

class Observatory : public QObject {
    Q_OBJECT

public:
    Observatory();

    // Загрузка каталога из JSON файла
    bool loadCatalogFromFile(const QString& filename);

    // Расчет позиций всех небесных тел
    void calculatePositions(const QDateTime& dateTime, const GeoLocation& location);

    // Получение списка видимых объектов
    QList<CelestialBody> getVisibleBodies() const { return m_visibleBodies; }

    // Сортировка по типу
    QMap<QString, QList<CelestialBody>> getBodiesByType() const;

    // Статус загрузки
    bool isLoading() const { return m_isLoading; }
    QString getLoadingStatus() const { return m_loadingStatus; }

    // Публичная константа PI
    static constexpr double PI = 3.14159265358979323846;

signals:
    void catalogLoaded();
    void loadingProgress(const QString& status);

private:
    // Астрономические расчёты
    double calculateJulianDay(const QDateTime& dateTime);
    double calculateLocalSiderealTime(double jd, double longitude);
    double calculateAltitude(double dec, double lat, double hourAngle);
    double calculateAzimuth(double dec, double lat, double hourAngle, double alt);
    double calculateHourAngle(double lst, double ra);

    QList<CelestialBody> m_catalog;
    QList<CelestialBody> m_visibleBodies;
    bool m_isLoading;
    QString m_loadingStatus;
};

#endif // OBSERVATORY_H
