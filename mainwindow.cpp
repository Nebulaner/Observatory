#include "mainwindow.h"
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QtMath>
#include <QDateTime>
#include <QTimer>
#include <QTimeZone>

// ============= SkyMapWidget Implementation =============
SkyMapWidget::SkyMapWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(700, 600);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background-color: black; border: 2px solid #444; border-radius: 8px;");
    m_hasSelection = false;
    m_showLabels = true;
}

void SkyMapWidget::setVisibleBodies(const QList<CelestialBody>& bodies) {
    m_bodies = bodies;
    update();
}

void SkyMapWidget::setSelectedBody(const CelestialBody& body) {
    m_selectedBody = body;
    m_hasSelection = true;
    update();
}

void SkyMapWidget::clearSelection() {
    m_hasSelection = false;
    update();
}

void SkyMapWidget::setShowLabels(bool show) {
    m_showLabels = show;
    update();
}

QPointF SkyMapWidget::getBodyPosition(const CelestialBody& body, int centerX, int centerY, int radius) {
    double r = radius * tan((90.0 - body.altitude) * PI / 360.0);
    double angle = body.azimuth * PI / 180.0;

    double x = centerX + r * sin(angle);
    double y = centerY - r * cos(angle);

    return QPointF(x, y);
}

void SkyMapWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    update();
}

QColor SkyMapWidget::getStarColor(double magnitude, const QString& type) {
    if (type.contains("Галактика")) {
        return QColor(180, 200, 255);
    } else if (type.contains("Туманность") || type.contains("Остаток сверхновой")) {
        return QColor(255, 180, 180);
    } else if (type.contains("Скопление")) {
        return QColor(200, 200, 150);
    } else if (type.contains("Планетарная")) {
        return QColor(150, 255, 150);
    } else if (type.contains("Планета")) {
        return QColor(255, 220, 150);
    } else {
        if (magnitude < 2.0) {
            return QColor(255, 255, 255);
        } else if (magnitude < 4.0) {
            return QColor(255, 255, 200);
        } else if (magnitude < 6.0) {
            return QColor(255, 220, 180);
        } else {
            return QColor(255, 180, 150);
        }
    }
}

void SkyMapWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(5, 5, 20));
    gradient.setColorAt(0.7, QColor(15, 10, 30));
    gradient.setColorAt(1, QColor(40, 20, 40));

    painter.fillRect(rect(), gradient);

    int centerX = width() / 2;
    int centerY = height() / 2;
    int radius = qMin(width(), height()) / 2 - 30;

    painter.setPen(QPen(QColor(100, 100, 150, 100), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

    painter.setPen(QPen(QColor(80, 80, 150, 60), 1));
    for (int i = 0; i < 8; i++) {
        double angle = i * 45.0 * PI / 180.0;
        int x = centerX + radius * sin(angle);
        int y = centerY - radius * cos(angle);
        painter.drawLine(centerX, centerY, x, y);
    }

    painter.setPen(QPen(QColor(80, 80, 150, 40), 1));
    for (int alt = 30; alt < 90; alt += 30) {
        double r = radius * tan((90.0 - alt) * PI / 360.0);
        painter.drawEllipse(centerX - r, centerY - r, r * 2, r * 2);
    }

    painter.setBrush(QColor(255, 255, 255, 8));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(centerX - radius/2, centerY - radius/3, radius, radius*0.7);

    QList<CelestialBody> sortedBodies = m_bodies;
    std::sort(sortedBodies.begin(), sortedBodies.end(),
              [](const CelestialBody& a, const CelestialBody& b) {
                  return a.magnitude < b.magnitude;
              });

    for (const auto& body : sortedBodies) {
        if (body.altitude > 0) {
            QPointF pos = getBodyPosition(body, centerX, centerY, radius);

            if (pos.x() >= -50 && pos.x() < width() + 50 && pos.y() >= -50 && pos.y() < height() + 50) {
                bool isSelected = m_hasSelection &&
                                  body.name == m_selectedBody.name &&
                                  body.type == m_selectedBody.type;
                drawStar(painter, body, pos.x(), pos.y(), isSelected);
            }
        }
    }

    painter.setPen(QColor(150, 150, 200, 150));
    painter.setFont(QFont("Arial", 10));

    QStringList directions = {"С", "В", "Ю", "З"};
    double angles[] = {0, 90, 180, 270};
    for (int i = 0; i < 4; i++) {
        double angle = angles[i] * PI / 180.0;
        int x = centerX + (radius + 25) * sin(angle);
        int y = centerY - (radius + 25) * cos(angle);
        painter.drawText(x - 10, y - 10, 20, 20, Qt::AlignCenter, directions[i]);
    }
}

void SkyMapWidget::drawStar(QPainter& painter, const CelestialBody& body, int x, int y, bool isSelected) {
    int size;
    if (body.angularSize > 0.5) {
        size = qMax(4, static_cast<int>(body.angularSize / 3));
        size = qMin(25, size);
    } else {
        double mag = qBound(-2.0, body.magnitude, 10.0);
        size = 3 + static_cast<int>((10.0 - mag) * 0.8);
    }

    if (isSelected) {
        size += 4;
    }

    QColor color = getStarColor(body.magnitude, body.type);

    if (body.magnitude < 3.0 && body.type == "Звезда") {
        int glowSize = size * 2;
        for (int i = 0; i < 3; i++) {
            int alpha = 40 - i * 10;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(color.red(), color.green(), color.blue(), alpha));
            painter.drawEllipse(x - glowSize/2 + i, y - glowSize/2 + i,
                                glowSize - i*2, glowSize - i*2);
        }
    }

    if (isSelected) {
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(x - size/2 - 2, y - size/2 - 2, size + 4, size + 4);
    }

    painter.setPen(QPen(color.darker(150), 1));
    painter.setBrush(color);
    painter.drawEllipse(x - size/2, y - size/2, size, size);

    if (body.magnitude < 4.0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 150));
        painter.drawEllipse(x - size/4, y - size/4, size/3, size/3);
    }

    if (m_showLabels && (size > 4 || isSelected)) {
        painter.setPen(QColor(255, 255, 255, 200));
        painter.setFont(QFont("Arial", 8));

        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(body.name) + 6;
        int textHeight = fm.height() + 2;

        painter.setBrush(QColor(0, 0, 0, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(x + size/2 + 3, y - size/2 - textHeight, textWidth, textHeight, 3, 3);

        painter.setPen(Qt::white);
        painter.drawText(x + size/2 + 5, y - size/2 - 2, body.name);
    }
}

// ============= MainWindow Implementation =============
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onNetworkReply);

    m_useManualLocation = false;
    m_currentLocation.isValid = false;
    m_realTimeMode = true; // Включаем режим реального времени по умолчанию

    setupUI();
    updateLocation();

    connect(&m_observatory, &Observatory::catalogLoaded, this, &MainWindow::onCatalogLoaded);
    connect(&m_observatory, &Observatory::loadingProgress, this, &MainWindow::updateLoadingProgress);

    // Таймер для обновления в реальном времени
    m_realTimeTimer = new QTimer(this);
    connect(m_realTimeTimer, &QTimer::timeout, this, &MainWindow::onRealTimeUpdate);
    m_realTimeTimer->start(1000); // Обновление каждую секунду

    // Загружаем каталог из JSON файла
    loadCatalogFromFile();

    m_latitudeSpin->setValue(59.93);
    m_longitudeSpin->setValue(30.36);

    // Устанавливаем текущее время
    onSyncTimeButton();
}

MainWindow::~MainWindow() {}

void MainWindow::onRealTimeModeToggle(bool checked) {
    m_realTimeMode = checked;
    if (checked) {
        m_realTimeTimer->start(1000);
        m_realTimeButton->setText("⏸ Пауза");
        m_statusLabel->setText("Режим реального времени: ВКЛ");
        onSyncTimeButton(); // Синхронизируем время при включении
    } else {
        m_realTimeTimer->stop();
        m_realTimeButton->setText("▶ Реальное время");
        m_statusLabel->setText("Режим реального времени: ВЫКЛ");
    }
}

void MainWindow::onRealTimeUpdate() {
    if (m_realTimeMode && m_currentLocation.isValid) {
        // Обновляем время на текущее системное
        QDateTime now = QDateTime::currentDateTime();
        m_dateTimeEdit->setDateTime(now);

        // Обновляем данные
        m_observatory.calculatePositions(now, m_currentLocation);
        updateTable();
        updateSkyMap();

        // Обновляем информацию о выбранном объекте, если есть
        if (m_skyMapWidget->hasSelection()) {
            auto visibleBodies = m_observatory.getVisibleBodies();
            for (const auto& body : visibleBodies) {
                if (body.name == m_selectedBodyName) {
                    updateObjectInfo(body);
                    break;
                }
            }
        }

        // Обновляем статус с количеством видимых объектов
        int visibleCount = m_observatory.getVisibleBodies().size();
        m_statusLabel->setText(QString("🕐 %1 | Видимо: %2 объектов")
                                   .arg(now.toString("HH:mm:ss"))
                                   .arg(visibleCount));
    }
}

void MainWindow::loadCatalogFromFile() {
    m_statusLabel->setText("Загрузка каталога...");
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);

    // Ищем файл catalog.json в разных местах
    QStringList searchPaths;
    searchPaths << QDir::currentPath() + "/catalog.json";
    searchPaths << QCoreApplication::applicationDirPath() + "/catalog.json";

    bool loaded = false;
    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            if (m_observatory.loadCatalogFromFile(path)) {
                loaded = true;
                break;
            }
        }
    }

    if (!loaded) {
        m_statusLabel->setText("Ошибка: не найден файл каталога catalog.json");
        m_progressBar->setVisible(false);
    }
}

void MainWindow::setupUI() {
    setWindowTitle("Обсерватория - Карта звёздного неба (Реальное время)");
    setMinimumSize(1400, 900);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Левая панель
    QWidget* leftPanel = new QWidget();
    leftPanel->setFixedWidth(400);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);

    // ========== ГРУППА РЕЖИМА РАБОТЫ ==========
    QGroupBox* modeGroup = new QGroupBox("⚡ Режим работы");
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);

    m_realTimeButton = new QPushButton("⏸ Пауза");
    m_realTimeButton->setCheckable(true);
    m_realTimeButton->setChecked(true);
    m_realTimeButton->setStyleSheet("QPushButton:checked { background-color: #4CAF50; color: white; font-weight: bold; }"
                                    "QPushButton:!checked { background-color: #f44336; color: white; }");
    connect(m_realTimeButton, &QPushButton::toggled, this, &MainWindow::onRealTimeModeToggle);
    modeLayout->addWidget(m_realTimeButton);

    QLabel* modeDescLabel = new QLabel("В режиме реального времени карта\nобновляется автоматически каждую секунду");
    modeDescLabel->setStyleSheet("color: #888; font-size: 10px;");
    modeLayout->addWidget(modeDescLabel);

    leftLayout->addWidget(modeGroup);

    // ========== ГРУППА ВРЕМЕНИ ==========
    QGroupBox* timeGroup = new QGroupBox("🕐 Параметры времени");
    QVBoxLayout* timeLayout = new QVBoxLayout(timeGroup);

    // Дата и время
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Дата/время:"));
    m_dateTimeEdit = new QDateTimeEdit();
    m_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    m_dateTimeEdit->setCalendarPopup(true);
    m_dateTimeEdit->setDisplayFormat("dd.MM.yyyy hh:mm:ss");
    dateLayout->addWidget(m_dateTimeEdit);
    timeLayout->addLayout(dateLayout);

    // Кнопки управления временем
    QHBoxLayout* timeButtonsLayout = new QHBoxLayout();

    m_syncTimeButton = new QPushButton("🔄 Текущее время");
    m_syncTimeButton->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(m_syncTimeButton, &QPushButton::clicked, this, &MainWindow::onSyncTimeButton);
    timeButtonsLayout->addWidget(m_syncTimeButton);

    m_applyTimeButton = new QPushButton("✅ Применить время");
    m_applyTimeButton->setStyleSheet("background-color: #2196F3; color: white;");
    connect(m_applyTimeButton, &QPushButton::clicked, this, &MainWindow::onApplyTimeButton);
    timeButtonsLayout->addWidget(m_applyTimeButton);

    timeLayout->addLayout(timeButtonsLayout);

    // Информация о часовом поясе
    QLabel* timezoneLabel = new QLabel(QString("Часовой пояс: %1").arg(QTimeZone::systemTimeZoneId()));
    timezoneLabel->setStyleSheet("color: #888;");
    timeLayout->addWidget(timezoneLabel);

    leftLayout->addWidget(timeGroup);

    // ========== ГРУППА КООРДИНАТ ==========
    QGroupBox* coordGroup = new QGroupBox("📍 Параметры местоположения");
    QVBoxLayout* coordLayout = new QVBoxLayout(coordGroup);

    // Авто-определение местоположения
    QHBoxLayout* autoLocLayout = new QHBoxLayout();
    autoLocLayout->addWidget(new QLabel("Авто:"));
    m_locationEdit = new QLineEdit();
    m_locationEdit->setReadOnly(true);
    m_locationEdit->setPlaceholderText("Определение...");
    autoLocLayout->addWidget(m_locationEdit);

    m_refreshLocationButton = new QPushButton("🔄 Определить");
    m_refreshLocationButton->setStyleSheet("background-color: #FF9800; color: white;");
    connect(m_refreshLocationButton, &QPushButton::clicked, this, &MainWindow::updateLocation);
    autoLocLayout->addWidget(m_refreshLocationButton);
    coordLayout->addLayout(autoLocLayout);

    // Ручной ввод координат
    QHBoxLayout* manualLocLayout = new QHBoxLayout();
    manualLocLayout->addWidget(new QLabel("Широта:"));
    m_latitudeSpin = new QDoubleSpinBox();
    m_latitudeSpin->setRange(-90, 90);
    m_latitudeSpin->setDecimals(4);
    m_latitudeSpin->setSuffix("°");
    m_latitudeSpin->setFixedWidth(100);
    manualLocLayout->addWidget(m_latitudeSpin);

    manualLocLayout->addWidget(new QLabel("Долгота:"));
    m_longitudeSpin = new QDoubleSpinBox();
    m_longitudeSpin->setRange(-180, 180);
    m_longitudeSpin->setDecimals(4);
    m_longitudeSpin->setSuffix("°");
    m_longitudeSpin->setFixedWidth(100);
    manualLocLayout->addWidget(m_longitudeSpin);
    coordLayout->addLayout(manualLocLayout);

    // Кнопка применения координат
    m_applyLocationButton = new QPushButton("✅ Применить координаты");
    m_applyLocationButton->setStyleSheet("background-color: #2196F3; color: white;");
    connect(m_applyLocationButton, &QPushButton::clicked, this, &MainWindow::onManualLocationChanged);
    coordLayout->addWidget(m_applyLocationButton);

    leftLayout->addWidget(coordGroup);

    // ========== ДОПОЛНИТЕЛЬНЫЕ НАСТРОЙКИ ==========
    QGroupBox* optionsGroup = new QGroupBox("⚙️ Настройки отображения");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_showLabelsCheck = new QCheckBox("🏷 Показывать подписи объектов");
    m_showLabelsCheck->setChecked(true);
    connect(m_showLabelsCheck, &QCheckBox::toggled, this, &MainWindow::onShowLabelsToggled);
    optionsLayout->addWidget(m_showLabelsCheck);

    leftLayout->addWidget(optionsGroup);

    // Прогресс бар
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumHeight(15);
    leftLayout->addWidget(m_progressBar);

    // ========== ИНФОРМАЦИЯ ОБ ОБЪЕКТЕ ==========
    QGroupBox* infoGroup = new QGroupBox("ℹ️ Информация об объекте");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

    m_infoNameLabel = new QLabel("Название: —");
    m_infoTypeLabel = new QLabel("Тип: —");
    m_infoPositionLabel = new QLabel("Склонение: —");
    m_infoSizeLabel = new QLabel("Угловой размер: —");
    m_infoMagnitudeLabel = new QLabel("Яркость: —");
    m_infoAltitudeLabel = new QLabel("Высота: —");
    m_infoAzimuthLabel = new QLabel("Азимут: —");

    infoLayout->addWidget(m_infoNameLabel);
    infoLayout->addWidget(m_infoTypeLabel);
    infoLayout->addWidget(m_infoPositionLabel);
    infoLayout->addWidget(m_infoSizeLabel);
    infoLayout->addWidget(m_infoMagnitudeLabel);
    infoLayout->addWidget(m_infoAltitudeLabel);
    infoLayout->addWidget(m_infoAzimuthLabel);
    infoLayout->addStretch();

    leftLayout->addWidget(infoGroup);

    // ========== ТАБЛИЦА ОБЪЕКТОВ ==========
    QGroupBox* tableGroup = new QGroupBox("🔭 Наблюдаемые объекты");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableGroup);

    m_tableWidget = new QTableWidget();
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderLabels({"Название", "Тип"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setMinimumHeight(200);

    connect(m_tableWidget, &QTableWidget::itemClicked, this, &MainWindow::onTableItemClicked);

    tableLayout->addWidget(m_tableWidget);
    leftLayout->addWidget(tableGroup);

    // ========== ПРАВАЯ ПАНЕЛЬ - КАРТА ==========
    QGroupBox* mapGroup = new QGroupBox("🗺 Карта звёздного неба");
    QVBoxLayout* mapLayout = new QVBoxLayout(mapGroup);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: black; }");

    m_skyMapWidget = new SkyMapWidget();
    scrollArea->setWidget(m_skyMapWidget);
    mapLayout->addWidget(scrollArea);

    m_statusLabel = new QLabel("✅ Готов к работе");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("QLabel { background-color: #333; color: white; padding: 5px; border-radius: 3px; }");
    mapLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(mapGroup, 1);
}

void MainWindow::updateLocation() {
    m_statusLabel->setText("🔍 Определение местоположения по IP...");
    m_useManualLocation = false;

    QNetworkRequest request(QUrl("http://ip-api.com/json/"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Observatory/1.0");
    m_networkManager->get(request);
}

void MainWindow::onManualLocationChanged() {
    m_useManualLocation = true;
    m_currentLocation.latitude = m_latitudeSpin->value();
    m_currentLocation.longitude = m_longitudeSpin->value();
    m_currentLocation.isValid = true;
    m_currentLocation.city = "Ручной ввод";
    m_currentLocation.country = "";

    m_locationEdit->setText(QString("Ручные (%1°, %2°)")
                                .arg(m_currentLocation.latitude, 0, 'f', 4)
                                .arg(m_currentLocation.longitude, 0, 'f', 4));

    refreshData();
    m_statusLabel->setText("✅ Координаты применены");
}

void MainWindow::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText("⚠️ Ошибка определения местоположения. Используйте ручной ввод.");
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (!m_useManualLocation && obj.contains("lat")) {
        m_currentLocation.latitude = obj["lat"].toDouble();
        m_currentLocation.longitude = obj["lon"].toDouble();
        m_currentLocation.city = obj["city"].toString();
        m_currentLocation.country = obj["country"].toString();
        m_currentLocation.isValid = true;

        m_locationEdit->setText(QString("%1, %2 (%3°, %4°)")
                                    .arg(m_currentLocation.city)
                                    .arg(m_currentLocation.country)
                                    .arg(m_currentLocation.latitude, 0, 'f', 4)
                                    .arg(m_currentLocation.longitude, 0, 'f', 4));

        m_latitudeSpin->setValue(m_currentLocation.latitude);
        m_longitudeSpin->setValue(m_currentLocation.longitude);

        m_statusLabel->setText("✅ Местоположение определено");
        refreshData();
    }

    reply->deleteLater();
}

void MainWindow::onSyncTimeButton() {
    // Устанавливаем текущее системное время
    QDateTime now = QDateTime::currentDateTime();
    m_dateTimeEdit->setDateTime(now);
    m_statusLabel->setText("Время синхронизировано с системным");

    if (!m_realTimeMode) {
        refreshData();
    }
}

void MainWindow::onApplyTimeButton() {
    // Применяем введенное пользователем время
    m_statusLabel->setText("Применено пользовательское время");

    // Если включен режим реального времени, временно отключаем автообновление
    if (m_realTimeMode) {
        m_realTimeButton->setChecked(false);
    }

    refreshData();
}

void MainWindow::onCatalogLoaded() {
    m_progressBar->setVisible(false);

    int catalogSize = m_observatory.getVisibleBodies().size();
    m_statusLabel->setText(QString("✅ Каталог загружен: %1 объектов").arg(catalogSize));

    refreshData();
}

void MainWindow::updateLoadingProgress(const QString& status) {
    m_progressBar->setVisible(true);
    m_statusLabel->setText(status);
}

void MainWindow::refreshData() {
    if (!m_currentLocation.isValid) {
        m_statusLabel->setText("⚠️ Нет координат! Введите координаты вручную или определите автоматически.");
        return;
    }

    m_observatory.calculatePositions(m_dateTimeEdit->dateTime(), m_currentLocation);

    updateTable();
    updateSkyMap();

    int visibleCount = m_observatory.getVisibleBodies().size();
    m_statusLabel->setText(QString("✅ Данные обновлены. Видимых объектов: %1").arg(visibleCount));
}

void MainWindow::updateTable() {
    auto visibleBodies = m_observatory.getVisibleBodies();

    m_tableWidget->setRowCount(visibleBodies.size());
    m_tableWidget->clearSelection();
    m_skyMapWidget->clearSelection();

    for (int row = 0; row < visibleBodies.size(); ++row) {
        const auto& body = visibleBodies[row];

        QTableWidgetItem* nameItem = new QTableWidgetItem(body.name);
        nameItem->setData(Qt::UserRole, QVariant::fromValue(body));

        m_tableWidget->setItem(row, 0, nameItem);
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(body.type));

        double altitude = body.altitude;
        if (altitude > 60) {
            nameItem->setBackground(QBrush(QColor(0, 70, 0)));
        } else if (altitude > 30) {
            nameItem->setBackground(QBrush(QColor(70, 70, 0)));
        } else {
            nameItem->setBackground(QBrush(QColor(70, 0, 0)));
        }
    }
}

void MainWindow::updateSkyMap() {
    m_skyMapWidget->setVisibleBodies(m_observatory.getVisibleBodies());
}

void MainWindow::onTableItemClicked(QTableWidgetItem* item) {
    if (item) {
        int row = item->row();
        QTableWidgetItem* nameItem = m_tableWidget->item(row, 0);
        if (nameItem) {
            CelestialBody body = nameItem->data(Qt::UserRole).value<CelestialBody>();
            m_skyMapWidget->setSelectedBody(body);
            m_tableWidget->selectRow(row);
            m_selectedBodyName = body.name;
            updateObjectInfo(body);
        }
    }
}

void MainWindow::updateObjectInfo(const CelestialBody& body) {
    m_infoNameLabel->setText("Название: " + body.name);
    m_infoTypeLabel->setText("Тип: " + body.type);
    m_infoPositionLabel->setText(QString("Склонение: %1°  Прямое восх.: %2h")
                                     .arg(body.declination, 0, 'f', 2)
                                     .arg(body.rightAscension, 0, 'f', 2));
    m_infoSizeLabel->setText(QString("Угловой размер: %1'").arg(body.angularSize, 0, 'f', 2));
    m_infoMagnitudeLabel->setText(QString("Яркость: %1m").arg(body.magnitude, 0, 'f', 2));
    m_infoAltitudeLabel->setText(QString("Высота над горизонтом: %1°").arg(body.altitude, 0, 'f', 1));
    m_infoAzimuthLabel->setText(QString("Азимут: %1°").arg(body.azimuth, 0, 'f', 1));
}

void MainWindow::onShowLabelsToggled(bool checked) {
    m_skyMapWidget->setShowLabels(checked);
}
