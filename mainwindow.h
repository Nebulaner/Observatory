#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QScrollArea>
#include <QTimer>
#include "observatory.h"

class SkyMapWidget : public QWidget {
    Q_OBJECT
public:
    explicit SkyMapWidget(QWidget *parent = nullptr);
    void setVisibleBodies(const QList<CelestialBody>& bodies);
    void setSelectedBody(const CelestialBody& body);
    void clearSelection();
    void setShowLabels(bool show);
    bool hasSelection() const { return m_hasSelection; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QList<CelestialBody> m_bodies;
    CelestialBody m_selectedBody;
    bool m_hasSelection;
    bool m_showLabels;

    void drawStar(QPainter& painter, const CelestialBody& body, int x, int y, bool isSelected);
    QPointF getBodyPosition(const CelestialBody& body, int centerX, int centerY, int radius);
    QColor getStarColor(double magnitude, const QString& type);

    static constexpr double PI = 3.14159265358979323846;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateLocation();
    void refreshData();
    void onNetworkReply(QNetworkReply* reply);
    void onTableItemClicked(QTableWidgetItem* item);
    void onManualLocationChanged();
    void onShowLabelsToggled(bool checked);
    void onCatalogLoaded();
    void updateLoadingProgress(const QString& status);
    void onSyncTimeButton();
    void onApplyTimeButton();
    void onRealTimeModeToggle(bool checked);
    void onRealTimeUpdate();

private:
    void setupUI();
    void updateSkyMap();
    void updateTable();
    void updateObjectInfo(const CelestialBody& body);
    void loadCatalogFromFile();

    Observatory m_observatory;
    QNetworkAccessManager* m_networkManager;
    GeoLocation m_currentLocation;
    QDateTime m_currentDateTime;
    bool m_useManualLocation;
    bool m_realTimeMode;
    QString m_selectedBodyName;
    QTimer* m_realTimeTimer;

    // UI Elements
    QDateTimeEdit* m_dateTimeEdit;
    QLineEdit* m_locationEdit;
    QPushButton* m_refreshLocationButton;
    QPushButton* m_applyLocationButton;
    QPushButton* m_syncTimeButton;
    QPushButton* m_applyTimeButton;
    QPushButton* m_realTimeButton;
    QTableWidget* m_tableWidget;
    SkyMapWidget* m_skyMapWidget;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QCheckBox* m_showLabelsCheck;

    // Info labels
    QLabel* m_infoNameLabel;
    QLabel* m_infoTypeLabel;
    QLabel* m_infoPositionLabel;
    QLabel* m_infoSizeLabel;
    QLabel* m_infoMagnitudeLabel;
    QLabel* m_infoAltitudeLabel;
    QLabel* m_infoAzimuthLabel;

    // Manual location controls
    QDoubleSpinBox* m_latitudeSpin;
    QDoubleSpinBox* m_longitudeSpin;
};

#endif // MAINWINDOW_H
