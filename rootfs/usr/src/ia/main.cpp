#include "Chatbot.h"
#include <cerrno>
#include <cctype>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

bool isSudoRequest(const std::string& input) {
    return input.compare(0, 5, "sudo ") == 0 ||
           input.compare(0, 5, "sudo\t") == 0;
}

bool parseCommand(const std::string& input, std::vector<std::string>& args) {
    std::string token;
    char quote = '\0';
    bool escaped = false;
    bool tokenStarted = false;

    for (char ch : input) {
        if (escaped) {
            token += ch;
            escaped = false;
            tokenStarted = true;
        } else if (quote == '\'') {
            if (ch == quote) {
                quote = '\0';
            } else {
                token += ch;
            }
        } else if (ch == '\\' && quote != '\'') {
            escaped = true;
            tokenStarted = true;
        } else if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            } else {
                token += ch;
            }
        } else if (ch == '\'' || ch == '"') {
            quote = ch;
            tokenStarted = true;
        } else if (std::isspace(static_cast<unsigned char>(ch))) {
            if (tokenStarted) {
                args.push_back(token);
                token.clear();
                tokenStarted = false;
            }
        } else {
            token += ch;
            tokenStarted = true;
        }
    }

    if (quote != '\0' || escaped) {
        return false;
    }
    if (tokenStarted) {
        args.push_back(token);
    }
    return !args.empty() && !args.front().empty();
}

void runSudoCommand(const std::string& input) {
    std::vector<std::string> command;
    if (!parseCommand(input.substr(5), command)) {
        std::cout << "Commande invalide : vérifie les guillemets et les arguments.\n";
        return;
    }

    std::cout << "La commande suivante sera exécutée avec les droits administrateur :\n"
              << input << "\nConfirmer ? Tape exactement « oui » : ";
    std::string confirmation;
    if (!std::getline(std::cin, confirmation) || confirmation != "oui") {
        std::cout << "Commande annulée.\n";
        return;
    }

    std::vector<std::string> arguments{"sudo", "--"};
    arguments.insert(arguments.end(), command.begin(), command.end());
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Impossible de démarrer la commande.\n";
        return;
    }
    if (pid == 0) {
        execvp(argv[0], argv.data());
        perror("Impossible de lancer sudo");
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            std::cerr << "Erreur en attendant la fin de la commande.\n";
            return;
        }
    }

    if (WIFEXITED(status)) {
        std::cout << "Commande terminée (code " << WEXITSTATUS(status) << ").\n";
    } else if (WIFSIGNALED(status)) {
        std::cout << "Commande interrompue (signal " << WTERMSIG(status) << ").\n";
    }
}

} // namespace

int main() {
    chatia::Chatbot chatbot;

    std::string input;
    while (std::getline(std::cin, input)) {
        if (isSudoRequest(input)) {
            runSudoCommand(input);
        } else {
            std::cout << chatbot.generateReply(input) << std::endl;
        }
    }

    return 0;
}