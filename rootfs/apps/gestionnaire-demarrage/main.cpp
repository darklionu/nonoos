#include <QApplication>
#include <QAbstractItemView>
#include <QByteArray>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {

const QRegularExpression serviceNamePattern(
    QStringLiteral("^[A-Za-z0-9:_.@-]+\\.service$"));

enum class Operation {
    None,
    Loading,
    Changing
};

class GestionnaireDemarrage final : public QWidget
{
public:
    explicit GestionnaireDemarrage(QWidget *parent = nullptr)
        : QWidget(parent), process_(this)
    {
        setWindowTitle("Démarrage des services - NonoOS");
        resize(760, 520);

        auto *layout = new QVBoxLayout(this);
        auto *description = new QLabel(
            "Consultez les services systemd configurés pour démarrer avec "
            "NonoOS. Les modifications s’appliquent aux prochains "
            "démarrages ; elles ne démarrent ni n’arrêtent un service tout "
            "de suite. Certains services essentiels sont protégés.",
            this);
        description->setWordWrap(true);
        layout->addWidget(description);

        filtre_ = new QLineEdit(this);
        filtre_->setPlaceholderText("Filtrer les services…");
        layout->addWidget(filtre_);

        services_ = new QTableWidget(0, 3, this);
        services_->setHorizontalHeaderLabels(
            {"Service", "État au démarrage", "Gestion"});
        services_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        services_->setSelectionBehavior(QAbstractItemView::SelectRows);
        services_->setSelectionMode(QAbstractItemView::SingleSelection);
        services_->setAlternatingRowColors(true);
        services_->verticalHeader()->setVisible(false);
        services_->horizontalHeader()->setSectionResizeMode(
            0, QHeaderView::Stretch);
        services_->horizontalHeader()->setSectionResizeMode(
            1, QHeaderView::ResizeToContents);
        services_->horizontalHeader()->setSectionResizeMode(
            2, QHeaderView::ResizeToContents);
        layout->addWidget(services_, 1);

        auto *actions = new QHBoxLayout;
        actualiser_ = new QPushButton("Actualiser", this);
        activer_ = new QPushButton("Activer au démarrage", this);
        desactiver_ = new QPushButton("Désactiver au démarrage", this);
        actions->addWidget(actualiser_);
        actions->addStretch();
        actions->addWidget(activer_);
        actions->addWidget(desactiver_);
        layout->addLayout(actions);

        etat_ = new QLabel("Chargement des services…", this);
        etat_->setWordWrap(true);
        layout->addWidget(etat_);

        connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
            sortie_.append(process_.readAllStandardOutput());
        });
        connect(&process_, &QProcess::readyReadStandardError, this, [this] {
            erreur_.append(process_.readAllStandardError());
        });
        connect(&process_,
                qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus status) {
                    terminerCommande(code, status);
                });
        connect(&process_, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart &&
                        operation_ != Operation::None) {
                        const QString message =
                            "Impossible de lancer la commande : " +
                            process_.errorString();
                        operation_ = Operation::None;
                        actualiser_->setEnabled(true);
                        etat_->setText(message);
                        QMessageBox::critical(this, "Erreur", message);
                        actualiserEtatActions();
                    }
                });

        connect(actualiser_, &QPushButton::clicked, this,
                [this] { chargerServices(); });
        connect(filtre_, &QLineEdit::textChanged, this,
                [this] { appliquerFiltre(); });
        connect(services_, &QTableWidget::itemSelectionChanged, this,
                [this] { actualiserEtatActions(); });
        connect(activer_, &QPushButton::clicked, this,
                [this] { confirmerChangement(true); });
        connect(desactiver_, &QPushButton::clicked, this,
                [this] { confirmerChangement(false); });

        chargerServices();
    }

private:
    static bool serviceProtege(const QString &unit)
    {
        static const QStringList protectedServices = {
            "dbus.service",
            "display-manager.service",
            "NetworkManager.service",
            "networking.service",
            "polkit.service",
            "ssh.service",
            "sshd.service",
            "systemd-logind.service",
            "systemd-networkd.service",
            "systemd-resolved.service",
            "systemd-user-sessions.service"
        };

        return protectedServices.contains(unit) ||
               unit.startsWith("systemd-") || unit.startsWith("getty@") ||
               unit.startsWith("user-runtime-dir@");
    }

    QString serviceSelectionne() const
    {
        const int row = services_->currentRow();
        if (row < 0 || services_->isRowHidden(row)) {
            return {};
        }

        const auto *item = services_->item(row, 0);
        return item ? item->data(Qt::UserRole).toString() : QString();
    }

    QString etatServiceSelectionne() const
    {
        const int row = services_->currentRow();
        const auto *item = row >= 0 ? services_->item(row, 1) : nullptr;
        return item ? item->data(Qt::UserRole).toString() : QString();
    }

    void actualiserEtatActions()
    {
        const QString unit = serviceSelectionne();
        const QString state = etatServiceSelectionne();
        const bool disponible =
            operation_ == Operation::None && !unit.isEmpty() &&
            !serviceProtege(unit);

        activer_->setEnabled(disponible && state == "disabled");
        desactiver_->setEnabled(disponible && state == "enabled");
    }

    void chargerServices()
    {
        if (operation_ != Operation::None) {
            return;
        }

        operation_ = Operation::Loading;
        sortie_.clear();
        erreur_.clear();
        actualiser_->setEnabled(false);
        etat_->setText("Lecture de la configuration systemd…");
        actualiserEtatActions();
        process_.start("systemctl",
                       {"list-unit-files", "--type=service", "--no-legend",
                        "--no-pager"});
    }

    void afficherServices()
    {
        services_->setRowCount(0);
        const QStringList lignes =
            QString::fromLocal8Bit(sortie_).split('\n', Qt::SkipEmptyParts);

        for (const QString &ligne : lignes) {
            const QStringList colonnes =
                ligne.trimmed().split(QRegularExpression("\\s+"),
                                      Qt::SkipEmptyParts);
            if (colonnes.size() < 2 ||
                !serviceNamePattern.match(colonnes.at(0)).hasMatch()) {
                continue;
            }

            const QString unit = colonnes.at(0);
            const QString state = colonnes.at(1);
            const bool protegee = serviceProtege(unit);
            const bool gerable = !protegee &&
                                  (state == "enabled" || state == "disabled");
            const int row = services_->rowCount();
            services_->insertRow(row);

            auto *nom = new QTableWidgetItem(unit);
            nom->setData(Qt::UserRole, unit);
            auto *etat = new QTableWidgetItem(state);
            etat->setData(Qt::UserRole, state);
            auto *gestion = new QTableWidgetItem(
                protegee ? "Protégé" : gerable ? "Modifiable" : "Lecture seule");
            services_->setItem(row, 0, nom);
            services_->setItem(row, 1, etat);
            services_->setItem(row, 2, gestion);
        }

        appliquerFiltre();
        actualiserEtatActions();
    }

    void appliquerFiltre()
    {
        const QString recherche = filtre_->text().trimmed();
        for (int row = 0; row < services_->rowCount(); ++row) {
            const auto *item = services_->item(row, 0);
            const bool correspond =
                item && item->text().contains(recherche, Qt::CaseInsensitive);
            services_->setRowHidden(row, !correspond);
        }
        services_->clearSelection();
        actualiserEtatActions();
    }

    void confirmerChangement(bool activer)
    {
        const QString unit = serviceSelectionne();
        const QString state = etatServiceSelectionne();
        if (unit.isEmpty() || serviceProtege(unit) ||
            (activer && state != "disabled") ||
            (!activer && state != "enabled")) {
            return;
        }

        const QString action = activer ? "activer" : "désactiver";
        const auto choix = QMessageBox::warning(
            this, "Confirmer la modification",
            QString("Voulez-vous %1 « %2 » au démarrage ?\n\n"
                    "Cette modification peut affecter le fonctionnement "
                    "du système. Elle ne démarre ni n’arrête le service "
                    "immédiatement.")
                .arg(action, unit),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (choix != QMessageBox::Yes) {
            return;
        }

        operation_ = Operation::Changing;
        sortie_.clear();
        erreur_.clear();
        etat_->setText("Demande d’autorisation administrateur…");
        actualiser_->setEnabled(false);
        actualiserEtatActions();
        process_.start("pkexec",
                       {"systemctl", activer ? "enable" : "disable", unit});
    }

    void terminerCommande(int code, QProcess::ExitStatus status)
    {
        const Operation operation = operation_;
        operation_ = Operation::None;
        actualiser_->setEnabled(true);

        if (status != QProcess::NormalExit || code != 0) {
            QString message = QString::fromLocal8Bit(erreur_).trimmed();
            if (message.isEmpty()) {
                message = operation == Operation::Loading
                              ? "systemctl n’a pas pu lire la liste des services."
                              : "La modification a échoué ou l’autorisation "
                                "administrateur a été annulée.";
            }
            etat_->setText("Échec : " + message);
            QMessageBox::warning(this, "Opération échouée", message);
            actualiserEtatActions();
            return;
        }

        if (operation == Operation::Loading) {
            afficherServices();
            etat_->setText(QString("%1 service(s) chargé(s).")
                               .arg(services_->rowCount()));
            return;
        }

        etat_->setText("Modification effectuée. Actualisation de la liste…");
        chargerServices();
    }

    QProcess process_;
    QTableWidget *services_ = nullptr;
    QLineEdit *filtre_ = nullptr;
    QPushButton *actualiser_ = nullptr;
    QPushButton *activer_ = nullptr;
    QPushButton *desactiver_ = nullptr;
    QLabel *etat_ = nullptr;
    QByteArray sortie_;
    QByteArray erreur_;
    Operation operation_ = Operation::None;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    GestionnaireDemarrage gestionnaire;
    gestionnaire.show();
    return application.exec();
}
