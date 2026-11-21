#include <QApplication>
#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class PowerWindow final : public QWidget {
public:
    PowerWindow() : process_(new QProcess(this)) {
        setWindowTitle(QStringLiteral("Gestion de l’alimentation NonoOS"));
        setMinimumWidth(440);

        auto *layout = new QVBoxLayout(this);
        auto *heading = new QLabel(QStringLiteral("Gestion de l’alimentation"));
        QFont headingFont = heading->font();
        headingFont.setPointSize(16);
        headingFont.setBold(true);
        heading->setFont(headingFont);
        layout->addWidget(heading);

        statusLabel_ = new QLabel(QStringLiteral("Lecture de l’état du système…"));
        statusLabel_->setWordWrap(true);
        statusLabel_->setMinimumHeight(72);
        layout->addWidget(statusLabel_);

        auto *profileHeading = new QLabel(QStringLiteral("Profil d’alimentation"));
        layout->addWidget(profileHeading);

        auto *profileRow = new QHBoxLayout;
        profileCombo_ = new QComboBox;
        profileCombo_->addItem(QStringLiteral("Équilibré"), QStringLiteral("balanced"));
        profileCombo_->addItem(QStringLiteral("Performance"), QStringLiteral("performance"));
        profileCombo_->addItem(QStringLiteral("Économie d’énergie"), QStringLiteral("power-saver"));
        profileRow->addWidget(profileCombo_, 1);

        auto *applyProfileButton = new QPushButton(QStringLiteral("Appliquer"));
        profileRow->addWidget(applyProfileButton);
        layout->addLayout(profileRow);

        auto *actionsHeading = new QLabel(QStringLiteral("Actions système"));
        layout->addWidget(actionsHeading);

        auto *actionRow = new QHBoxLayout;
        addActionButton(actionRow, QStringLiteral("Veille"), QStringLiteral("suspend"));
        addActionButton(actionRow, QStringLiteral("Hibernation"), QStringLiteral("hibernate"));
        layout->addLayout(actionRow);

        auto *powerRow = new QHBoxLayout;
        addActionButton(powerRow, QStringLiteral("Redémarrer"), QStringLiteral("reboot"));
        addActionButton(powerRow, QStringLiteral("Éteindre"), QStringLiteral("poweroff"));
        layout->addLayout(powerRow);

        auto *refreshButton = new QPushButton(QStringLiteral("Actualiser l’état"));
        layout->addWidget(refreshButton);

        connect(applyProfileButton, &QPushButton::clicked, this, [this] {
            runCommand({QStringLiteral("profile"), profileCombo_->currentData().toString()});
        });
        connect(refreshButton, &QPushButton::clicked, this, [this] {
            refreshStatus();
        });
        connect(process_, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    handleFinished(exitCode, exitStatus);
                });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                setBusy(false);
                QMessageBox::critical(
                    this, QStringLiteral("Erreur"),
                    QStringLiteral("Impossible de lancer nonoos-power. Vérifiez que le script "
                                   "est installé dans /usr/local/bin."));
            }
        });

        refreshStatus();
        auto *refreshTimer = new QTimer(this);
        connect(refreshTimer, &QTimer::timeout, this, [this] {
            if (process_->state() == QProcess::NotRunning) {
                refreshStatus();
            }
        });
        refreshTimer->start(30000);
    }

private:
    void addActionButton(QHBoxLayout *layout, const QString &label, const QString &command) {
        auto *button = new QPushButton(label);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, label, command] {
            const auto answer = QMessageBox::question(
                this, label, QStringLiteral("Voulez-vous vraiment %1 le système ?").arg(
                                 command == QStringLiteral("suspend") ? QStringLiteral("mettre en veille") :
                                 command == QStringLiteral("hibernate") ? QStringLiteral("mettre en hibernation") :
                                 command == QStringLiteral("reboot") ? QStringLiteral("redémarrer") :
                                                                        QStringLiteral("éteindre")),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) {
                runCommand({command});
            }
        });
    }

    void refreshStatus() {
        runCommand({QStringLiteral("status")});
    }

    void runCommand(const QStringList &arguments) {
        if (process_->state() != QProcess::NotRunning) {
            return;
        }
        setBusy(true);
        process_->start(QStringLiteral("/usr/local/bin/nonoos-power"), arguments);
    }

    void setBusy(bool busy) {
        profileCombo_->setEnabled(!busy);
        for (auto *button : findChildren<QPushButton *>()) {
            button->setEnabled(!busy);
        }
    }

    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        const QString output = QString::fromLocal8Bit(process_->readAllStandardOutput()).trimmed();
        const QString error = QString::fromLocal8Bit(process_->readAllStandardError()).trimmed();
        setBusy(false);

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            const QString details = error.isEmpty() ? output : error;
            QMessageBox::critical(
                this, QStringLiteral("Échec de l’opération"),
                details.isEmpty() ? QStringLiteral("La commande a échoué.") : details);
            if (output.contains(QStringLiteral("Gestion de l’alimentation"))) {
                statusLabel_->setText(QStringLiteral("Impossible de lire l’état de l’alimentation."));
            }
            return;
        }

        if (output.startsWith(QStringLiteral("Gestion de l’alimentation"))) {
            updateStatus(output);
        } else if (output.startsWith(QStringLiteral("Profil d’alimentation appliqué"))) {
            refreshStatus();
        } else if (!output.isEmpty()) {
            statusLabel_->setText(output);
        }
    }

    void updateStatus(const QString &output) {
        QStringList details;
        QString currentProfile;
        for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            if (line.startsWith(QStringLiteral("Profil : "))) {
                currentProfile = line.mid(QStringLiteral("Profil : ").size()).trimmed();
                const int index = profileCombo_->findData(currentProfile);
                if (index >= 0) {
                    profileCombo_->setCurrentIndex(index);
                }
                details.append(QStringLiteral("Profil : %1").arg(profileName(currentProfile)));
            } else if (line.startsWith(QStringLiteral("Gouverneur CPU : "))) {
                details.append(line);
            } else if (line.startsWith(QStringLiteral("Secteur ")) ||
                       line.startsWith(QStringLiteral("Batterie "))) {
                details.append(line);
            } else if (line.contains(QStringLiteral("power-profiles-daemon indisponible"))) {
                details.append(QStringLiteral("Service des profils : indisponible"));
            }
        }
        if (details.isEmpty()) {
            details.append(QStringLiteral("Aucune information d’alimentation détectée."));
        }
        statusLabel_->setText(details.join(QLatin1Char('\n')));
    }

    QString profileName(const QString &profile) const {
        if (profile == QStringLiteral("balanced")) {
            return QStringLiteral("Équilibré");
        }
        if (profile == QStringLiteral("performance")) {
            return QStringLiteral("Performance");
        }
        if (profile == QStringLiteral("power-saver")) {
            return QStringLiteral("Économie d’énergie");
        }
        return profile;
    }

    QProcess *process_;
    QLabel *statusLabel_ = nullptr;
    QComboBox *profileCombo_ = nullptr;
};

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    PowerWindow window;
    window.show();
    return application.exec();
}