#include "observatory.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtMath>

Observatory::Observatory() : QObject() {
    m_isLoading = false;
}

bool Observatory::loadCatalogFromFile(const QString& filename) {
    m_isLoading = true;
    m_loadingStatus = "Загрузка каталога из файла...";
    emit loadingProgress(m_loadingStatus);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        m_loadingStatus = "Ошибка: не удалось открыть файл " + filename;
        emit loadingProgress(m_loadingStatus);
        m_isLoading = false;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        m_loadingStatus = "Ошибка: неверный формат JSON";
        emit loadingProgress(m_loadingStatus);
        m_isLoading = false;
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("objects") || !root.contains("metadata")) {
        m_loadingStatus = "Ошибка: неверная структура JSON файла";
        emit loadingProgress(m_loadingStatus);
        m_isLoading = false;
        return false;
    }

    QJsonObject metadata = root["metadata"].toObject();
    QString catalogName = metadata["name"].toString();
    int totalObjects = metadata["total_objects"].toInt();

    m_loadingStatus = QString("Загрузка каталога '%1' (%2 объектов)...").arg(catalogName).arg(totalObjects);
    emit loadingProgress(m_loadingStatus);

    QJsonArray objects = root["objects"].toArray();
    m_catalog.clear();

    int loadedCount = 0;
    for (const QJsonValue& value : objects) {
        QJsonObject obj = value.toObject();

        CelestialBody body;
        body.name = obj["name"].toString();
        body.type = obj["type"].toString();
        body.declination = obj["declination"].toDouble();
        body.angularSize = obj["angularSize"].toDouble();
        body.magnitude = obj["magnitude"].toDouble();
        body.rightAscension = obj["rightAscension"].toDouble();
        body.catalogId = obj["catalogId"].toString();
        body.altitude = 0.0;
        body.azimuth = 0.0;

        m_catalog.append(body);
        loadedCount++;
    }

    m_loadingStatus = QString("Загружено объектов: %1").arg(loadedCount);
    emit loadingProgress(m_loadingStatus);

    m_isLoading = false;
    emit catalogLoaded();

    return true;
}

void Observatory::calculatePositions(const QDateTime& dateTime, const GeoLocation& location) {
    double jd = calculateJulianDay(dateTime);
    double lst = calculateLocalSiderealTime(jd, location.longitude);

    m_visibleBodies.clear();

    for (auto& body : m_catalog) {
        double ha = calculateHourAngle(lst, body.rightAscension);

        body.altitude = calculateAltitude(body.declination, location.latitude, ha);
        body.azimuth = calculateAzimuth(body.declination, location.latitude, ha, body.altitude);

        if (body.altitude > 0) {
            m_visibleBodies.append(body);
        }
    }
}

QMap<QString, QList<CelestialBody>> Observatory::getBodiesByType() const {
    QMap<QString, QList<CelestialBody>> result;
    for (const auto& body : m_visibleBodies) {
        result[body.type].append(body);
    }
    return result;
}

double Observatory::calculateJulianDay(const QDateTime& dateTime) {
    QDate date = dateTime.date();
    QTime time = dateTime.time();

    int year = date.year();
    int month = date.month();
    int day = date.day();
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();

    if (month <= 2) {
        year -= 1;
        month += 12;
    }

    int A = year / 100;
    int B = 2 - A + A / 4;

    double jd = floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
    jd += (hour + minute / 60.0 + second / 3600.0) / 24.0;

    return jd;
}

double Observatory::calculateLocalSiderealTime(double jd, double longitude) {
    double jd2000 = 2451545.0;
    double T = (jd - jd2000) / 36525.0;

    double gmst = 280.46061837 + 360.98564736629 * (jd - jd2000) + 0.000387933 * T * T - T * T * T / 38710000.0;
    gmst = fmod(gmst, 360.0);
    if (gmst < 0) gmst += 360.0;

    double lst = gmst + longitude;
    lst = fmod(lst, 360.0);
    if (lst < 0) lst += 360.0;

    return lst * 15.0;
}

double Observatory::calculateHourAngle(double lst, double ra) {
    double ha = lst - ra * 15.0;
    ha = fmod(ha, 360.0);
    if (ha > 180) ha -= 360;
    if (ha < -180) ha += 360;
    return ha;
}

double Observatory::calculateAltitude(double dec, double lat, double hourAngle) {
    double sinAlt = sin(dec * PI / 180.0) * sin(lat * PI / 180.0) +
                    cos(dec * PI / 180.0) * cos(lat * PI / 180.0) *
                        cos(hourAngle * PI / 180.0);

    if (sinAlt > 1.0) sinAlt = 1.0;
    if (sinAlt < -1.0) sinAlt = -1.0;

    return asin(sinAlt) * 180.0 / PI;
}

double Observatory::calculateAzimuth(double dec, double lat, double hourAngle, double alt) {
    double denominator = cos(lat * PI / 180.0) * cos(alt * PI / 180.0);
    if (qAbs(denominator) < 1e-10) {
        return 0.0;
    }

    double cosAz = (sin(dec * PI / 180.0) - sin(lat * PI / 180.0) *
                                                sin(alt * PI / 180.0)) / denominator;

    if (cosAz > 1.0) cosAz = 1.0;
    if (cosAz < -1.0) cosAz = -1.0;

    double az = acos(cosAz) * 180.0 / PI;

    if (sin(hourAngle * PI / 180.0) > 0) {
        az = 360.0 - az;
    }

    if (az < 0) az += 360.0;
    if (az >= 360.0) az -= 360.0;

    return az;
}
