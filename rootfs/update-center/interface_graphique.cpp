#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace {

#ifndef NONOOS_PACKAGE_LIST_DIR
#define NONOOS_PACKAGE_LIST_DIR "/usr/share/newdistrib/packages"
#endif

const QString packageListDirectory = QStringLiteral(NONOOS_PACKAGE_LIST_DIR);
const QRegularExpression packageNamePattern(
    QStringLiteral("^[a-z0-9][a-z0-9+.-]*(?::[a-z0-9]+)?$"));

struct Application {
    QString name;
    QString category;
};

enum class ProcessAction {
    None,
    RefreshSources,
    CheckUpgrades,
    ApplyUpgrades,
    RefreshCatalog,
    InstallApplications,
    RemoveApplications
};

class CentreLogiciel : public QWidget
{
public:
    explicit CentreLogiciel(QWidget *parent = nullptr)
        : QWidget(parent), processus_(this)
    {
        setWindowTitle("Logiciels et mises à jour - NonoOS");
        resize(900, 680);

        auto *layout = new QVBoxLayout(this);
        auto *intro = new QLabel(
            "Installez les applications proposées par les profils NonoOS et "
            "mettez à jour les paquets Debian depuis les dépôts configurés.",
            this);
        intro->setWordWrap(true);
        layout->addWidget(intro);

        onglets_ = new QTabWidget(this);
        layout->addWidget(onglets_, 1);

        creerOngletMisesAJour();
        creerOngletApplications();

        journal_ = new QPlainTextEdit(this);
        journal_->setReadOnly(true);
        journal_->setMaximumBlockCount(400);
        journal_->setPlaceholderText("Les détails des opérations apparaîtront ici.");
        journal_->setMaximumHeight(150);
        layout->addWidget(journal_);

        QObject::connect(&processus_, &QProcess::readyReadStandardOutput, this,
                         [this] {
            const QByteArray donnees = processus_.readAllStandardOutput();
            sortieProcessus_.append(donnees);
            ajouterJournal(donnees);
        });
        QObject::connect(&processus_, &QProcess::readyReadStandardError, this,
                         [this] {
            const QByteArray donnees = processus_.readAllStandardError();
            erreurProcessus_.append(donnees);
            ajouterJournal(donnees);
        });
        QObject::connect(&processus_,
                         qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                         this, [this](int code, QProcess::ExitStatus status) {
            terminerCommande(code, status);
        });
        QObject::connect(&processus_, &QProcess::errorOccurred, this,
                         [this](QProcess::ProcessError erreur) {
            if (erreur == QProcess::FailedToStart) {
                const QString message =
                    "Impossible de démarrer la commande : " + processus_.errorString();
                terminerEchec(message);
            }
        });

        chargerCatalogue();
        actualiserCatalogue();
    }

private:
    void creerOngletMisesAJour()
    {
        auto *page = new QWidget(this);
        auto *layout = new QVBoxLayout(page);

        auto *explication = new QLabel(
            "La recherche actualise d’abord les index APT, puis simule une mise "
            "à niveau. L’installation utilise « apt-get upgrade » : elle ne "
            "supprime pas de paquets pour résoudre les dépendances. Le noyau "
            "NonoOS intégré à l’ISO n’est pas géré par APT.",
            page);
        explication->setWordWrap(true);
        layout->addWidget(explication);

        auto *actions = new QHBoxLayout;
        boutonRechercher_ = new QPushButton("Rechercher des mises à jour", page);
        boutonInstallerMisesAJour_ =
            new QPushButton("Installer les mises à jour", page);
        boutonInstallerMisesAJour_->setEnabled(false);
        actions->addWidget(boutonRechercher_);
        actions->addWidget(boutonInstallerMisesAJour_);
        actions->addStretch();
        layout->addLayout(actions);

        etatMisesAJour_ = new QLabel("Aucune recherche effectuée.", page);
        etatMisesAJour_->setWordWrap(true);
        layout->addWidget(etatMisesAJour_);

        misesAJour_ = new QTableWidget(0, 3, page);
        misesAJour_->setHorizontalHeaderLabels(
            {"Paquet", "Version installée", "Version proposée"});
        misesAJour_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        misesAJour_->setSelectionBehavior(QAbstractItemView::SelectRows);
        misesAJour_->setAlternatingRowColors(true);
        misesAJour_->verticalHeader()->setVisible(false);
        misesAJour_->horizontalHeader()->setSectionResizeMode(
            0, QHeaderView::Stretch);
        misesAJour_->horizontalHeader()->setSectionResizeMode(
            1, QHeaderView::ResizeToContents);
        misesAJour_->horizontalHeader()->setSectionResizeMode(
            2, QHeaderView::ResizeToContents);
        layout->addWidget(misesAJour_, 1);

        QObject::connect(boutonRechercher_, &QPushButton::clicked, this,
                         [this] { actualiserDepots(); });
        QObject::connect(boutonInstallerMisesAJour_, &QPushButton::clicked, this,
                         [this] { confirmerMiseAJour(); });

        onglets_->addTab(page, "Mises à jour");
    }

    void creerOngletApplications()
    {
        auto *page = new QWidget(this);
        auto *layout = new QVBoxLayout(page);
        auto *filtres = new QHBoxLayout;
        auto *explication = new QLabel(
            "Cochez une application disponible pour l’installer ou une "
            "application installée pour la supprimer. APT ne supprime pas "
            "automatiquement les dépendances.",
            page);
        explication->setWordWrap(true);
        layout->addWidget(explication);

        filtreApplications_ = new QLineEdit(page);
        filtreApplications_->setPlaceholderText("Rechercher un paquet…");
        categorie_ = new QComboBox(page);
        categorie_->addItem("Tous les profils", QString());
        filtres->addWidget(filtreApplications_, 1);
        filtres->addWidget(categorie_);
        layout->addLayout(filtres);

        etatApplications_ = new QLabel("Chargement du catalogue…", page);
        etatApplications_->setWordWrap(true);
        layout->addWidget(etatApplications_);

        applications_ = new QTableWidget(0, 3, page);
        applications_->setHorizontalHeaderLabels(
            {"Sélection", "Paquet", "Profil", "État"});
        applications_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        applications_->setSelectionBehavior(QAbstractItemView::SelectRows);
        applications_->setAlternatingRowColors(true);
        applications_->verticalHeader()->setVisible(false);
        applications_->horizontalHeader()->setSectionResizeMode(
            0, QHeaderView::ResizeToContents);
        applications_->horizontalHeader()->setSectionResizeMode(
            1, QHeaderView::Stretch);
        applications_->horizontalHeader()->setSectionResizeMode(
            2, QHeaderView::ResizeToContents);
        applications_->horizontalHeader()->setSectionResizeMode(
            3, QHeaderView::ResizeToContents);
        layout->addWidget(applications_, 1);

        auto *actions = new QHBoxLayout;
        boutonInstallerApplications_ =
            new QPushButton("Installer la sélection", page);
        boutonSupprimerApplications_ =
            new QPushButton("Supprimer la sélection", page);
        boutonActualiserCatalogue_ = new QPushButton("Actualiser", page);
        boutonInstallerApplications_->setEnabled(false);
        boutonSupprimerApplications_->setEnabled(false);
        actions->addWidget(boutonInstallerApplications_);
        actions->addWidget(boutonSupprimerApplications_);
        actions->addWidget(boutonActualiserCatalogue_);
        actions->addStretch();
        layout->addLayout(actions);

        QObject::connect(filtreApplications_, &QLineEdit::textChanged, this,
                         [this] { filtrerApplications(); });
        QObject::connect(categorie_,
                         qOverload<int>(&QComboBox::currentIndexChanged), this,
                         [this] { filtrerApplications(); });
        QObject::connect(boutonInstallerApplications_, &QPushButton::clicked, this,
                         [this] { confirmerInstallationApplications(); });
        QObject::connect(boutonSupprimerApplications_, &QPushButton::clicked, this,
                         [this] { confirmerSuppressionApplications(); });
        QObject::connect(boutonActualiserCatalogue_, &QPushButton::clicked, this,
                         [this] { actualiserCatalogue(); });
        QObject::connect(applications_, &QTableWidget::itemChanged, this,
                         [this](QTableWidgetItem *item) {
            if (item->column() == 0) {
                actualiserActionsApplications();
            }
        });

        onglets_->addTab(page, "Applications");
    }

    void chargerCatalogue()
    {
        applicationsCatalogue_.clear();
        categorie_->clear();
        categorie_->addItem("Tous les profils", QString());
        QSet<QString> categories;
        QSet<QString> noms;

        QString cheminCatalogue = packageListDirectory;
        if (!QDir(cheminCatalogue).exists()) {
            const QString catalogueLocal =
                QDir::current().filePath("rootfs/usr/share/newdistrib/packages");
            if (QDir(catalogueLocal).exists()) {
                cheminCatalogue = catalogueLocal;
            }
        }
        QDir dossier(cheminCatalogue);
        const QFileInfoList fichiers =
            dossier.entryInfoList({"*.list"}, QDir::Files, QDir::Name);
        for (const QFileInfo &fichier : fichiers) {
            QFile entree(fichier.absoluteFilePath());
            if (!entree.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ajouterJournal(QString("Impossible de lire %1 : %2")
                                   .arg(fichier.fileName(), entree.errorString())
                                   .toUtf8());
                continue;
            }

            const QString profil = nomProfil(fichier.completeBaseName());
            categories.insert(profil);
            while (!entree.atEnd()) {
                const QString paquet =
                    QString::fromUtf8(entree.readLine()).trimmed();
                if (paquet.isEmpty() || paquet.startsWith('#') ||
                    !packageNamePattern.match(paquet).hasMatch() ||
                    noms.contains(paquet)) {
                    continue;
                }
                noms.insert(paquet);
                applicationsCatalogue_.append({paquet, profil});
            }
        }

        QStringList categoriesTriees = categories.values();
        categoriesTriees.sort(Qt::CaseInsensitive);
        for (const QString &profil : categoriesTriees) {
            categorie_->addItem(profil, profil);
        }

        if (fichiers.isEmpty()) {
            etatApplications_->setText(
                "Catalogue introuvable : " + cheminCatalogue);
        } else {
            etatApplications_->setText(
                QString("%1 paquet(s) proposé(s) dans %2 profil(s).")
                    .arg(applicationsCatalogue_.size())
                    .arg(categories.size()));
        }
    }

    static QString nomProfil(const QString &identifiant)
    {
        static const QHash<QString, QString> noms{
            {"custom", "Personnalisé"},
            {"cybersec", "Cybersécurité"},
            {"developing", "Développement"},
            {"education", "Éducation"},
            {"gaming", "Jeux"},
            {"multimedia", "Multimédia"},
            {"office", "Bureautique"},
            {"serveur", "Serveur"},
            {"vieprivee", "Vie privée"}};
        return noms.value(identifiant, identifiant);
    }

    void actualiserCatalogue()
    {
        if (processus_.state() != QProcess::NotRunning) {
            return;
        }
        lancerCommande(ProcessAction::RefreshCatalog, "/usr/bin/dpkg-query",
                       {"-W", "-f=${binary:Package}\t${db:Status-Abbrev}\\n"},
                       "Lecture des applications installées…");
    }

    void actualiserDepots()
    {
        if (processus_.state() != QProcess::NotRunning) {
            return;
        }
        journal_->clear();
        lancerCommande(ProcessAction::RefreshSources, "/usr/bin/pkexec",
                       {"/usr/bin/apt-get", "update"},
                       "Actualisation des dépôts APT… Une autorisation administrateur "
                       "peut être demandée.");
    }

    void verifierMisesAJour()
    {
        lancerCommande(ProcessAction::CheckUpgrades, "/usr/bin/apt-get",
                       {"-s", "upgrade"},
                       "Recherche des mises à jour disponibles…");
    }

    void confirmerMiseAJour()
    {
        QStringList noms;
        for (int ligne = 0; ligne < misesAJour_->rowCount(); ++ligne) {
            noms.append(misesAJour_->item(ligne, 0)->text());
        }
        if (noms.isEmpty()) {
            return;
        }

        const auto choix = QMessageBox::question(
            this, "Confirmer les mises à jour",
            QString("Installer %1 paquet(s) depuis les dépôts APT configurés ?\n\n"
                    "La commande apt-get upgrade ne supprimera pas de paquets. "
                    "Les dépôts Debian vérifient les signatures des paquets.")
                .arg(noms.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choix != QMessageBox::Yes) {
            return;
        }

        journal_->clear();
        lancerCommande(
            ProcessAction::ApplyUpgrades, "/usr/bin/pkexec",
            {"/usr/bin/apt-get", "-y",
             "-o", "Dpkg::Options::=--force-confdef",
             "-o", "Dpkg::Options::=--force-confold",
             "upgrade"},
            "Installation des mises à jour… Une autorisation administrateur "
            "peut être demandée.");
    }

    void confirmerInstallationApplications()
    {
        const QStringList paquets = selectionApplications(false);
        if (paquets.isEmpty()) {
            QMessageBox::information(
                this, "Aucune application sélectionnée",
                "Cochez au moins un paquet qui n’est pas déjà installé.");
            return;
        }

        const auto choix = QMessageBox::question(
            this, "Confirmer l’installation",
            QString("Installer les paquets suivants ?\n\n%1\n\nAPT pourra "
                    "également installer leurs dépendances.")
                .arg(paquets.join(", ")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choix != QMessageBox::Yes) {
            return;
        }

        QStringList arguments{
            "/usr/bin/apt-get", "-y",
            "-o", "Dpkg::Options::=--force-confdef",
            "-o", "Dpkg::Options::=--force-confold",
            "install", "--"};
        arguments.append(paquets);
        journal_->clear();
        lancerCommande(
            ProcessAction::InstallApplications, "/usr/bin/pkexec", arguments,
            "Installation en cours… Une autorisation administrateur peut être "
            "demandée.");
    }

    void confirmerSuppressionApplications()
    {
        const QStringList paquets = selectionApplications(true);
        if (paquets.isEmpty()) {
            QMessageBox::information(
                this, "Aucune application sélectionnée",
                "Cochez au moins un paquet déjà installé.");
            return;
        }

        const auto choix = QMessageBox::question(
            this, "Confirmer la suppression",
            QString("Supprimer les paquets suivants ?\n\n%1\n\nLes dépendances "
                    "ne seront pas supprimées automatiquement.")
                .arg(paquets.join(", ")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choix != QMessageBox::Yes) {
            return;
        }

        QStringList arguments{
            "/usr/bin/apt-get", "-y",
            "-o", "Dpkg::Options::=--force-confdef",
            "-o", "Dpkg::Options::=--force-confold",
            "remove", "--"};
        arguments.append(paquets);
        journal_->clear();
        lancerCommande(
            ProcessAction::RemoveApplications, "/usr/bin/pkexec", arguments,
            "Suppression en cours… Une autorisation administrateur peut être "
            "demandée.");
    }

    QStringList selectionApplications(bool installees) const
    {
        QStringList paquets;
        for (int ligne = 0; ligne < applications_->rowCount(); ++ligne) {
            const auto *casePaquet = applications_->item(ligne, 0);
            if (!casePaquet || casePaquet->checkState() != Qt::Checked) {
                continue;
            }
            const QString nom = applications_->item(ligne, 1)->text();
            if (paquetsInstalles_.contains(nom) == installees &&
                packageNamePattern.match(nom).hasMatch()) {
                paquets.append(nom);
            }
        }
        return paquets;
    }

    void lancerCommande(ProcessAction action, const QString &programme,
                        const QStringList &arguments, const QString &message)
    {
        action_ = action;
        sortieProcessus_.clear();
        erreurProcessus_.clear();
        desactiverActions(true);
        if (action == ProcessAction::InstallApplications) {
            etatApplications_->setText(message);
        } else if (action == ProcessAction::RemoveApplications) {
            etatApplications_->setText(message);
        } else if (action != ProcessAction::RefreshCatalog) {
            etatMisesAJour_->setText(message);
        }

        QProcessEnvironment environnement = QProcessEnvironment::systemEnvironment();
        environnement.insert("LANG", "C");
        environnement.insert("LC_ALL", "C");
        if (action == ProcessAction::ApplyUpgrades ||
            action == ProcessAction::InstallApplications ||
            action == ProcessAction::RemoveApplications) {
            environnement.insert("DEBIAN_FRONTEND", "noninteractive");
        }
        processus_.setProcessEnvironment(environnement);
        processus_.start(programme, arguments);
    }

    void terminerCommande(int code, QProcess::ExitStatus status)
    {
        sortieProcessus_.append(processus_.readAllStandardOutput());
        erreurProcessus_.append(processus_.readAllStandardError());
        const ProcessAction action = action_;
        action_ = ProcessAction::None;
        const QString details =
            QString::fromLocal8Bit(erreurProcessus_).trimmed();

        if (status != QProcess::NormalExit || code != 0) {
            const QString erreur = details.isEmpty()
                ? QString("La commande s’est terminée avec le code %1.").arg(code)
                : details;
            terminerEchec(erreur);
            return;
        }

        desactiverActions(false);
        switch (action) {
        case ProcessAction::RefreshSources:
            verifierMisesAJour();
            break;
        case ProcessAction::CheckUpgrades:
            afficherMisesAJour(sortieProcessus_);
            break;
        case ProcessAction::ApplyUpgrades:
            etatMisesAJour_->setText("Mises à jour installées.");
            ajouterJournal("Mises à jour installées avec succès.\n");
            actualiserCatalogue();
            break;
        case ProcessAction::RefreshCatalog:
            afficherCatalogue(sortieProcessus_);
            break;
        case ProcessAction::InstallApplications:
            etatApplications_->setText("Installation terminée.");
            ajouterJournal("Installation terminée avec succès.\n");
            actualiserCatalogue();
            break;
        case ProcessAction::RemoveApplications:
            etatApplications_->setText("Suppression terminée.");
            ajouterJournal("Suppression terminée avec succès.\n");
            actualiserCatalogue();
            break;
        case ProcessAction::None:
            break;
        }
    }

    void terminerEchec(const QString &erreur)
    {
        const ProcessAction action = action_;
        action_ = ProcessAction::None;
        desactiverActions(false);
        if (action == ProcessAction::RefreshCatalog) {
            etatApplications_->setText(
                "Impossible de lire les paquets installés : " + erreur);
        } else if (action == ProcessAction::InstallApplications) {
            etatApplications_->setText("Installation échouée ou annulée.");
        } else if (action == ProcessAction::RemoveApplications) {
            etatApplications_->setText("Suppression échouée ou annulée.");
        } else {
            etatMisesAJour_->setText("Opération échouée ou annulée.");
        }
        QMessageBox::warning(this, "Opération impossible", erreur);
    }

    void afficherMisesAJour(const QByteArray &sortie)
    {
        misesAJour_->setUpdatesEnabled(false);
        misesAJour_->setRowCount(0);
        const QRegularExpression ligneInst(
            QStringLiteral("^Inst\\s+(\\S+)\\s+\\[([^]]*)\\]\\s+\\((\\S+)"));
        const QString texte = QString::fromLocal8Bit(sortie);
        for (const QString &ligne : texte.split('\n')) {
            const auto resultat = ligneInst.match(ligne);
            if (!resultat.hasMatch()) {
                continue;
            }
            const int index = misesAJour_->rowCount();
            misesAJour_->insertRow(index);
            misesAJour_->setItem(index, 0,
                                 new QTableWidgetItem(resultat.captured(1)));
            misesAJour_->setItem(index, 1,
                                 new QTableWidgetItem(resultat.captured(2)));
            misesAJour_->setItem(index, 2,
                                 new QTableWidgetItem(resultat.captured(3)));
        }
        misesAJour_->setUpdatesEnabled(true);

        const int nombre = misesAJour_->rowCount();
        boutonInstallerMisesAJour_->setEnabled(nombre > 0);
        etatMisesAJour_->setText(
            nombre == 0
                ? "Aucune mise à jour disponible. Le système est à jour selon "
                  "les index APT actuels."
                : QString("%1 mise(s) à jour prête(s) à être installée(s).")
                      .arg(nombre));
    }

    void afficherCatalogue(const QByteArray &sortie)
    {
        paquetsInstalles_.clear();
        for (const QString &ligne :
             QString::fromLocal8Bit(sortie).split('\n', Qt::SkipEmptyParts)) {
            const QStringList champs = ligne.split('\t');
            if (champs.size() == 2 && champs[1].trimmed().startsWith("ii")) {
                const QString nom = champs[0].trimmed();
                paquetsInstalles_.insert(nom);
                const qsizetype architecture = nom.indexOf(':');
                if (architecture >= 0) {
                    paquetsInstalles_.insert(nom.left(architecture));
                }
            }
        }

        applications_->setUpdatesEnabled(false);
        applications_->setRowCount(applicationsCatalogue_.size());
        for (int ligne = 0; ligne < applicationsCatalogue_.size(); ++ligne) {
            const Application &application =
                applicationsCatalogue_[static_cast<std::size_t>(ligne)];
            const bool installee = paquetsInstalles_.contains(application.name);
            auto *casePaquet = new QTableWidgetItem;
            casePaquet->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable |
                                 Qt::ItemIsSelectable);
            casePaquet->setCheckState(Qt::Unchecked);
            applications_->setItem(ligne, 0, casePaquet);
            applications_->setItem(ligne, 1,
                                   new QTableWidgetItem(application.name));
            auto *profil = new QTableWidgetItem(application.category);
            profil->setData(Qt::UserRole, application.category);
            applications_->setItem(ligne, 2, profil);
            applications_->setItem(
                ligne, 3,
                new QTableWidgetItem(installee ? "Installé" : "Disponible"));
        }
        applications_->setUpdatesEnabled(true);
        filtrerApplications();
        actualiserActionsApplications();
        etatApplications_->setText(
            QString("%1 application(s) au catalogue ; %2 déjà installée(s).")
                .arg(applicationsCatalogue_.size())
                .arg(static_cast<int>(std::count_if(
                    applicationsCatalogue_.cbegin(), applicationsCatalogue_.cend(),
                    [this](const Application &application) {
                        return paquetsInstalles_.contains(application.name);
                    }))));
    }

    void actualiserActionsApplications()
    {
        const bool operationEnCours =
            processus_.state() != QProcess::NotRunning;
        boutonInstallerApplications_->setEnabled(
            !operationEnCours && !selectionApplications(false).isEmpty());
        boutonSupprimerApplications_->setEnabled(
            !operationEnCours && !selectionApplications(true).isEmpty());
    }

    void filtrerApplications()
    {
        const QString filtre = filtreApplications_->text().trimmed();
        const QString profil = categorie_->currentData().toString();
        for (int ligne = 0; ligne < applications_->rowCount(); ++ligne) {
            const bool correspond =
                applications_->item(ligne, 1)->text().contains(
                    filtre, Qt::CaseInsensitive) &&
                (profil.isEmpty() ||
                 applications_->item(ligne, 2)->data(Qt::UserRole).toString() ==
                     profil);
            applications_->setRowHidden(ligne, !correspond);
        }
    }

    void desactiverActions(bool desactive)
    {
        boutonRechercher_->setEnabled(!desactive);
        boutonInstallerMisesAJour_->setEnabled(
            !desactive && misesAJour_->rowCount() > 0);
        boutonInstallerApplications_->setEnabled(!desactive);
        boutonSupprimerApplications_->setEnabled(!desactive);
        boutonActualiserCatalogue_->setEnabled(!desactive);
    }

    void ajouterJournal(const QByteArray &texte)
    {
        if (texte.isEmpty()) {
            return;
        }
        journal_->moveCursor(QTextCursor::End);
        journal_->insertPlainText(QString::fromLocal8Bit(texte));
        journal_->moveCursor(QTextCursor::End);
    }

    QTabWidget *onglets_ = nullptr;
    QTableWidget *misesAJour_ = nullptr;
    QTableWidget *applications_ = nullptr;
    QLineEdit *filtreApplications_ = nullptr;
    QComboBox *categorie_ = nullptr;
    QLabel *etatMisesAJour_ = nullptr;
    QLabel *etatApplications_ = nullptr;
    QPushButton *boutonRechercher_ = nullptr;
    QPushButton *boutonInstallerMisesAJour_ = nullptr;
    QPushButton *boutonInstallerApplications_ = nullptr;
    QPushButton *boutonSupprimerApplications_ = nullptr;
    QPushButton *boutonActualiserCatalogue_ = nullptr;
    QPlainTextEdit *journal_ = nullptr;
    QProcess processus_;
    ProcessAction action_ = ProcessAction::None;
    QByteArray sortieProcessus_;
    QByteArray erreurProcessus_;
    QList<Application> applicationsCatalogue_;
    QSet<QString> paquetsInstalles_;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    CentreLogiciel fenetre;
    fenetre.show();
    return application.exec();
}
