#include <ram_memory.h>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>

int main(int argc, char* argv[]) {
    // Initialise Qt avant de créer les widgets de l’interface.
    QApplication application(argc, argv);

    QMainWindow fenetre;
    fenetre.setWindowTitle("Centre de contrôle - Mémoire");
    fenetre.resize(760, 520);

    auto* contenu = new QWidget(&fenetre);
    auto* disposition = new QVBoxLayout(contenu);
    auto* titre = new QLabel("Utilisation de la mémoire", contenu);
    auto* valeur = new QLabel(contenu);
    auto* barre = new QProgressBar(contenu);
    auto* etat = new QLabel("Démarrage de la surveillance…", contenu);
    auto* tableau = new QTableWidget(0, 3, contenu);

    // Configure le tableau pour l’affichage et non la modification des mesures.
    titre->setStyleSheet("font-size: 20px; font-weight: bold;");
    barre->setRange(0, 100);
    tableau->setHorizontalHeaderLabels({"Processus", "PID", "RAM (Mio)"});
    tableau->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableau->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableau->setAlternatingRowColors(true);
    tableau->verticalHeader()->setVisible(false);
    tableau->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    tableau->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    tableau->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);

    disposition->addWidget(titre);
    disposition->addWidget(valeur);
    disposition->addWidget(barre);
    disposition->addWidget(etat);
    disposition->addWidget(tableau);
    fenetre.setCentralWidget(contenu);

    QTimer minuterie(&fenetre);
    const auto actualiser = [&] {
        try {
            // Lit les valeurs système puis met à jour les indicateurs de RAM.
            const RamInfo ram = lireRAM();
            const double totaleGio = ram.totaleKo / (1024.0 * 1024.0);
            const double utiliseeGio = ram.utiliseeKo() / (1024.0 * 1024.0);
            valeur->setText(
                QString("RAM utilisée : %1 / %2 Gio")
                    .arg(utiliseeGio, 0, 'f', 1)
                    .arg(totaleGio, 0, 'f', 1));
            barre->setValue(static_cast<int>(
                100.0 * ram.utiliseeKo() / ram.totaleKo));

            // Reconstruit la liste des processus, déjà triée par RAM utilisée.
            const std::vector<ProcessusRAM> processus = lireProcessusRAM();
            tableau->setRowCount(static_cast<int>(processus.size()));
            for (int ligne = 0; ligne < tableau->rowCount(); ++ligne) {
                const ProcessusRAM& processusCourant =
                    processus[static_cast<std::size_t>(ligne)];
                tableau->setItem(
                    ligne, 0,
                    new QTableWidgetItem(QString::fromUtf8(
                        processusCourant.nom.c_str())));
                tableau->setItem(
                    ligne, 1,
                    new QTableWidgetItem(QString::number(processusCourant.pid)));
                tableau->setItem(
                    ligne, 2,
                    new QTableWidgetItem(
                        QString::number(processusCourant.ramKo / 1024.0, 'f', 1)));
            }
            etat->setText("Actualisation automatique toutes les 2 secondes");
        } catch (const std::exception& erreur) {
            // Affiche l’erreur dans la fenêtre plutôt que de masquer le problème.
            etat->setText(
                QString("Erreur de lecture des informations système : %1")
                    .arg(QString::fromUtf8(erreur.what())));
        }
    };

    QObject::connect(&minuterie, &QTimer::timeout, &fenetre, actualiser);
    // Effectue une première lecture immédiatement, puis actualise toutes les 2 s.
    actualiser();
    minuterie.start(2000);

    fenetre.show();
    return application.exec();
}