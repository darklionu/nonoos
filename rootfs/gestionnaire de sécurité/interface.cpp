#include <QApplication>
#include <QAbstractItemView>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringList>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

class SecurityCenter final : public QWidget {
public:
    SecurityCenter() {
        setWindowTitle(QStringLiteral("Sécurité NonoOS"));
        resize(1000, 680);

        auto* root = new QVBoxLayout(this);
        auto* controls = new QHBoxLayout;
        status_ = new QLabel(QStringLiteral("Chargement des informations système…"));
        refreshButton_ = new QPushButton(QStringLiteral("Actualiser"));
        controls->addWidget(status_, 1);
        controls->addWidget(refreshButton_);
        root->addLayout(controls);

        tabs_ = new QTabWidget;
        processes_ = makeTable({QStringLiteral("PID"), QStringLiteral("Utilisateur"),
                                QStringLiteral("État"), QStringLiteral("CPU %"),
                                QStringLiteral("Mémoire %"), QStringLiteral("Commande")});
        services_ = makeTable({QStringLiteral("Service"), QStringLiteral("État"),
                               QStringLiteral("Description")});
        interfaces_ = makeTable({QStringLiteral("Interface"), QStringLiteral("Reçu total"),
                                 QStringLiteral("Envoyé total"),
                                 QStringLiteral("Envoyé depuis l’actualisation")});
        connections_ = makeTable({QStringLiteral("Connexions établies")});
        permissions_ = makeTable({QStringLiteral("Application Flatpak"), QStringLiteral("Nom"),
                                  QStringLiteral("Permissions déclarées")});
        tabs_->addTab(processes_, QStringLiteral("Processus"));
        tabs_->addTab(services_, QStringLiteral("Services"));
        tabs_->addTab(interfaces_, QStringLiteral("Réseau"));
        tabs_->addTab(connections_, QStringLiteral("Connexions"));
        tabs_->addTab(permissions_, QStringLiteral("Permissions"));
        root->addWidget(tabs_, 1);

        auto* actions = new QHBoxLayout;
        stopServiceButton_ = new QPushButton(QStringLiteral("Arrêter le service sélectionné…"));
        stopServiceButton_->setEnabled(false);
        actions->addWidget(new QLabel(
            QStringLiteral("Alerte de transfert sortant au-dessus de :")));
        threshold_ = new QDoubleSpinBox;
        threshold_->setRange(1.0, 100000.0);
        threshold_->setValue(10.0);
        threshold_->setSuffix(QStringLiteral(" MiB par intervalle"));
        actions->addWidget(threshold_);
        actions->addStretch();
        actions->addWidget(stopServiceButton_);
        root->addLayout(actions);

        networkNote_ = new QLabel(
            QStringLiteral("Le trafic est mesuré par interface, pas attribué à une application. "
                           "Les permissions détaillées sont disponibles pour les applications Flatpak."));
        networkNote_->setWordWrap(true);
        root->addWidget(networkNote_);

        connect(refreshButton_, &QPushButton::clicked, this, [this] { refresh(); });
        connect(services_, &QTableWidget::itemSelectionChanged, this, [this] {
            stopServiceButton_->setEnabled(services_->currentRow() >= 0);
        });
        connect(stopServiceButton_, &QPushButton::clicked, this, [this] { stopSelectedService(); });
        connect(&snapshotProcess_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    handleSnapshot(exitCode, exitStatus);
                });
        connect(&snapshotProcess_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError) {
                    if (snapshotProcess_.state() == QProcess::NotRunning) {
                        status_->setText(QStringLiteral("Impossible de lancer nonoos-security."));
                        refreshButton_->setEnabled(true);
                    }
                });
        connect(&actionProcess_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    const QString output = QString::fromLocal8Bit(actionProcess_.readAllStandardError())
                                               .trimmed();
                    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                        QMessageBox::warning(this, QStringLiteral("Arrêt refusé ou échoué"),
                                             output.isEmpty() ? QStringLiteral("La commande a échoué.")
                                                              : output);
                    } else {
                        QMessageBox::information(this, QStringLiteral("Service arrêté"),
                                                 QStringLiteral("Le service a été arrêté."));
                    }
                    refresh();
                });
        connect(&actionProcess_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        stopServiceButton_->setEnabled(services_->currentRow() >= 0);
                        QMessageBox::warning(this, QStringLiteral("Commande indisponible"),
                                             QStringLiteral("Impossible de lancer pkexec."));
                    }
                });

        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this] { refresh(); });
        timer->start(5000);
        refresh();
    }

private:
    static QTableWidget* makeTable(const QStringList& headers) {
        auto* table = new QTableWidget(0, headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
        return table;
    }

    static void setRows(QTableWidget* table, const QJsonArray& rows,
                        const QStringList& keys) {
        table->setRowCount(static_cast<int>(rows.size()));
        for (qsizetype row = 0; row < rows.size(); ++row) {
            const QJsonObject object = rows.at(row).toObject();
            for (qsizetype column = 0; column < keys.size(); ++column) {
                const QString value = object.value(keys.at(column)).toVariant().toString();
                auto* item = new QTableWidgetItem(value);
                item->setToolTip(value);
                table->setItem(static_cast<int>(row), static_cast<int>(column), item);
            }
        }
    }

    void refresh() {
        if (snapshotProcess_.state() != QProcess::NotRunning) {
            return;
        }
        refreshButton_->setEnabled(false);
        status_->setText(QStringLiteral("Analyse en cours…"));
        snapshotProcess_.start(QStringLiteral("/usr/bin/nonoos-security"),
                               {QStringLiteral("snapshot")});
    }

    void handleSnapshot(int exitCode, QProcess::ExitStatus exitStatus) {
        refreshButton_->setEnabled(true);
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            const QString error = QString::fromLocal8Bit(snapshotProcess_.readAllStandardError())
                                      .trimmed();
            status_->setText(error.isEmpty() ? QStringLiteral("Échec de l’analyse.") : error);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(snapshotProcess_.readAllStandardOutput(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            status_->setText(QStringLiteral("Réponse invalide du diagnostic système."));
            return;
        }

        const QJsonObject snapshot = document.object();
        const QJsonArray processRows = snapshot.value(QStringLiteral("processes")).toArray();
        setRows(processes_, processRows,
                {QStringLiteral("pid"), QStringLiteral("user"), QStringLiteral("state"),
                 QStringLiteral("cpu"), QStringLiteral("memory"), QStringLiteral("command")});

        const QJsonArray serviceRows = snapshot.value(QStringLiteral("services")).toArray();
        setRows(services_, serviceRows,
                {QStringLiteral("unit"), QStringLiteral("state"),
                 QStringLiteral("description")});
        for (int row = 0; row < services_->rowCount(); ++row) {
            services_->item(row, 0)->setData(Qt::UserRole, serviceRows.at(row).toObject()
                                                               .value(QStringLiteral("unit"))
                                                               .toString());
        }

        const QString networkAlert =
            updateInterfaces(snapshot.value(QStringLiteral("interfaces")).toArray());

        const QJsonArray connectionRows = snapshot.value(QStringLiteral("connections")).toArray();
        connections_->setRowCount(connectionRows.size());
        for (qsizetype row = 0; row < connectionRows.size(); ++row) {
            const QString line = connectionRows.at(row).toString();
            auto* item = new QTableWidgetItem(line);
            item->setToolTip(line);
            connections_->setItem(static_cast<int>(row), 0, item);
        }

        const QJsonArray appRows = snapshot.value(QStringLiteral("flatpak_apps")).toArray();
        setRows(permissions_, appRows,
                {QStringLiteral("id"), QStringLiteral("name"),
                 QStringLiteral("permissions")});
        if (!snapshot.value(QStringLiteral("flatpak_available")).toBool()) {
            permissions_->setRowCount(1);
            permissions_->setItem(0, 0, new QTableWidgetItem(QStringLiteral("Flatpak non installé")));
            permissions_->setItem(0, 1, new QTableWidgetItem(QStringLiteral("")));
            permissions_->setItem(0, 2, new QTableWidgetItem(QStringLiteral(
                "Les permissions par application sont inspectées pour les applications Flatpak.")));
        } else if (appRows.isEmpty()) {
            permissions_->setRowCount(1);
            permissions_->setItem(0, 0, new QTableWidgetItem(QStringLiteral("Aucune application Flatpak")));
        }

        status_->setText(
            !networkAlert.isEmpty()
                ? QStringLiteral("Alerte : %1").arg(networkAlert)
                : QStringLiteral("Dernière analyse : %1 — actualisation automatique toutes les 5 s")
                      .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss"))));
    }

    QString updateInterfaces(const QJsonArray& rows) {
        const qint64 now = networkClock_.isValid() ? networkClock_.elapsed() : -1;
        const qint64 intervalMs = now >= 0 && lastNetworkSampleMs_ >= 0
                                      ? now - lastNetworkSampleMs_
                                      : 0;
        const double intervalSeconds = intervalMs > 0 ? intervalMs / 1000.0 : 0.0;
        QStringList alerts;
        interfaces_->setRowCount(static_cast<int>(rows.size()));

        for (qsizetype row = 0; row < rows.size(); ++row) {
            const QJsonObject object = rows.at(row).toObject();
            const QString name = object.value(QStringLiteral("name")).toString();
            const qint64 received = object.value(QStringLiteral("rx_bytes")).toVariant().toLongLong();
            const qint64 sent = object.value(QStringLiteral("tx_bytes")).toVariant().toLongLong();
            const bool hasPrevious = previousTx_.contains(name);
            const qint64 delta = hasPrevious ? qMax<qint64>(0, sent - previousTx_.value(name)) : 0;
            if (name != QStringLiteral("lo") && intervalMs > 0 &&
                delta >= threshold_->value() * 1024.0 * 1024.0) {
                alerts.append(QStringLiteral("%1 : %2 envoyés")
                                  .arg(name, formatBytes(delta)));
            }
            previousTx_.insert(name, sent);

            const QStringList values{
                name, formatBytes(received), formatBytes(sent),
                hasPrevious && intervalSeconds > 0
                    ? QStringLiteral("%1 (%2 MiB/s)")
                          .arg(formatBytes(delta))
                          .arg(delta / intervalSeconds / (1024.0 * 1024.0), 0, 'f', 2)
                    : QStringLiteral("Mesure en cours…")};
            for (qsizetype column = 0; column < values.size(); ++column) {
                interfaces_->setItem(static_cast<int>(row), static_cast<int>(column),
                                     new QTableWidgetItem(values.at(column)));
            }
        }

        if (!networkClock_.isValid()) {
            networkClock_.start();
        }
        lastNetworkSampleMs_ = networkClock_.elapsed();
        return alerts.join(QStringLiteral(" ; "));
    }

    static QString formatBytes(qint64 bytes) {
        constexpr double kib = 1024.0;
        if (bytes >= kib * kib) {
            return QStringLiteral("%1 MiB").arg(bytes / (kib * kib), 0, 'f', 2);
        }
        if (bytes >= kib) {
            return QStringLiteral("%1 KiB").arg(bytes / kib, 0, 'f', 2);
        }
        return QStringLiteral("%1 octets").arg(bytes);
    }

    void stopSelectedService() {
        const int row = services_->currentRow();
        if (row < 0 || !services_->item(row, 0)) {
            return;
        }
        const QString unit = services_->item(row, 0)->data(Qt::UserRole).toString();
        static const QRegularExpression validUnit(
            QStringLiteral("^[A-Za-z0-9_][A-Za-z0-9_.@:-]*\\.service$"));
        if (!validUnit.match(unit).hasMatch()) {
            QMessageBox::warning(this, QStringLiteral("Service invalide"),
                                 QStringLiteral("Le nom du service sélectionné n’est pas valide."));
            return;
        }
        const auto answer = QMessageBox::warning(
            this, QStringLiteral("Confirmer l’arrêt"),
            QStringLiteral("Arrêter « %1 » ? Cette action peut interrompre une fonction système. "
                           "Le service ne sera pas désactivé au prochain démarrage.")
                .arg(unit),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes) {
            return;
        }

        stopServiceButton_->setEnabled(false);
        actionProcess_.start(QStringLiteral("/usr/bin/pkexec"),
                             {QStringLiteral("/usr/bin/systemctl"), QStringLiteral("stop"), unit});
    }

    QLabel* status_ = nullptr;
    QLabel* networkNote_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* stopServiceButton_ = nullptr;
    QDoubleSpinBox* threshold_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QTableWidget* processes_ = nullptr;
    QTableWidget* services_ = nullptr;
    QTableWidget* interfaces_ = nullptr;
    QTableWidget* connections_ = nullptr;
    QTableWidget* permissions_ = nullptr;
    QProcess snapshotProcess_;
    QProcess actionProcess_;
    QElapsedTimer networkClock_;
    qint64 lastNetworkSampleMs_ = -1;
    QMap<QString, qint64> previousTx_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    SecurityCenter window;
    window.show();
    return app.exec();
}
