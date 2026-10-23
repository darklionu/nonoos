// Chatbot.cpp
#include "Chatbot.h"
#include "TextProcessing.h"
#include "calcul.h"
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace chatia {
namespace {

std::string normalizeMathInput(std::string text) {
    std::string normalized = normalizeText(text);

    std::vector<std::pair<std::string, std::string>> replacements = {
        {"plus", "+"},
        {"moins", "-"},
        {"fois", "*"},
        {"multiplie", "*"},
        {"divise", "/"},
        {"divise par", "/"},
        {"divisepar", "/"},
        {"puissance", "^"},
        {"modulo", "%"},
        {"egal", "="},
        {"egale", "="},
        {"resultat", ""},
        {"résultat", ""},
        {"calcul", ""},
        {"calcule", ""},
        {"calculer", ""},
        {"combien", ""},
        {"fait", ""},
        {"donne", ""},
        {"donne moi", ""},
        {"quel est", ""},
        {"quelle est", ""},
        {"est", ""}
    };

    for (const auto& [from, to] : replacements) {
        std::string::size_type pos = 0;
        while ((pos = normalized.find(from, pos)) != std::string::npos) {
            normalized.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    std::string cleaned;
    for (char ch : normalized) {
        if (std::isdigit(static_cast<unsigned char>(ch)) ||
            ch == '.' || ch == '+' || ch == '-' || ch == '*' ||
            ch == '/' || ch == '%' || ch == '^' || ch == '(' ||
            ch == ')' || std::isspace(static_cast<unsigned char>(ch))) {
            cleaned += ch;
        }
    }

    while (!cleaned.empty() && std::isspace(static_cast<unsigned char>(cleaned.front()))) {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && std::isspace(static_cast<unsigned char>(cleaned.back()))) {
        cleaned.pop_back();
    }
    return cleaned;
}

class MathParser {
public:
    explicit MathParser(std::string expression) : expression_(std::move(expression)) {}

    double parse() {
        size_t index = 0;
        double result = parseExpression(index);
        skipSpaces(index);
        if (index != expression_.size()) {
            throw std::invalid_argument("Expression mathématique invalide");
        }
        return result;
    }

private:
    double parseExpression(size_t& index) {
        double value = parseTerm(index);
        while (true) {
            skipSpaces(index);
            if (index >= expression_.size()) {
                return value;
            }

            char ch = expression_[index];
            if (ch == '+') {
                ++index;
                value = calcul::addition(value, parseTerm(index));
            } else if (ch == '-') {
                ++index;
                value = calcul::soustraction(value, parseTerm(index));
            } else {
                return value;
            }
        }
    }

    double parseTerm(size_t& index) {
        double value = parsePower(index);
        while (true) {
            skipSpaces(index);
            if (index >= expression_.size()) {
                return value;
            }

            char ch = expression_[index];
            if (ch == '*') {
                ++index;
                value = calcul::multiplication(value, parsePower(index));
            } else if (ch == '/') {
                ++index;
                value = calcul::division(value, parsePower(index));
            } else if (ch == '%') {
                ++index;
                value = std::fmod(value, parsePower(index));
            } else {
                return value;
            }
        }
    }

    double parsePower(size_t& index) {
        double value = parseUnary(index);
        skipSpaces(index);
        if (index < expression_.size() && expression_[index] == '^') {
            ++index;
            value = std::pow(value, parsePower(index));
        }
        return value;
    }

    double parseUnary(size_t& index) {
        skipSpaces(index);
        if (index < expression_.size()) {
            char ch = expression_[index];
            if (ch == '+') {
                ++index;
                return parseUnary(index);
            }
            if (ch == '-') {
                ++index;
                return -parseUnary(index);
            }
        }
        return parsePrimary(index);
    }

    double parsePrimary(size_t& index) {
        skipSpaces(index);
        if (index >= expression_.size()) {
            throw std::invalid_argument("Expression incomplète");
        }

        if (expression_[index] == '(') {
            ++index;
            double value = parseExpression(index);
            skipSpaces(index);
            if (index >= expression_.size() || expression_[index] != ')') {
                throw std::invalid_argument("Parenthèse fermante manquante");
            }
            ++index;
            return value;
        }

        std::size_t start = index;
        while (index < expression_.size() &&
               (std::isdigit(static_cast<unsigned char>(expression_[index])) || expression_[index] == '.')) {
            ++index;
        }

        if (start == index) {
            throw std::invalid_argument("Nombre invalide");
        }

        return std::stod(expression_.substr(start, index - start));
    }

    void skipSpaces(size_t& index) const {
        while (index < expression_.size() && std::isspace(static_cast<unsigned char>(expression_[index]))) {
            ++index;
        }
    }

    std::string expression_;
};

std::string evaluateMathExpression(const std::string& input) {
    std::string expression = normalizeMathInput(input);
    if (expression.empty()) {
        return "Je n’ai pas trouvé d’expression mathématique à calculer.";
    }

    try {
        MathParser parser(expression);
        double result = parser.parse();

        std::ostringstream output;
        if (std::fabs(result - std::round(result)) < 1e-9) {
            output << "Le résultat est " << static_cast<long long>(std::llround(result)) << ".";
        } else {
            output << std::fixed << std::setprecision(2) << "Le résultat est " << result << ".";
        }
        return output.str();
    } catch (const std::exception&) {
        return "Je n’ai pas réussi à calculer cette expression. Essaie par exemple : 12 + 7 ou 8 * (3 + 2).";
    }
}

} // namespace

Chatbot::Chatbot() = default;

std::string Chatbot::generateReply(const std::string& input) {
    Intent intent = intentDetector_.detect(input);

    switch (intent) {
        case Intent::Greeting:
            return "Bonjour ! Je suis ton robot, comment puis-je t'aider ?";

        case Intent::Who:
            return "Je suis ton assistant robot, conçu pour répondre à tes questions.";

        case Intent::Weather: {
            std::string city = weatherService_.extractCityFromPrompt(input);
            return weatherService_.getWeatherForCity(city);
        }

        case Intent::Time: {
            time_t now = time(0);
            tm* local = localtime(&now);
            if (!local) return "Je ne peux pas lire l’heure actuellement.";
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%H:%M:%S", local);
            return std::string("Il est ") + buffer + ".";
        }

        case Intent::Date: {
            time_t now = time(0);
            tm* local = localtime(&now);
            if (!local) return "Je ne peux pas lire la date actuellement.";
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%d/%m/%Y", local);
            return std::string("Aujourd'hui, nous sommes le ") + buffer + ".";
        }

        case Intent::Thanks:
            return "Avec plaisir !";

        case Intent::Goodbye:
            return "Au revoir ! À bientôt.";

        case Intent::Calculator:
            return evaluateMathExpression(input);

        case Intent::Usb:
            return usbService_.listConnectedDevices();

        case Intent::Help:
            return "Commandes disponibles :\n- bonjour\n- météo\n- heure\n- date\n- merci\n- au revoir\n- calcul\n- usb\n- rappelle-moi de <tâche>\n- liste mes rappels\n- supprime le rappel <numéro>";
        case Intent::Rapelle:
            return rappelService_.handleRequest(input);

        default:
            return "Je ne comprends pas encore tout, mais je peux t'aider avec bonjour, météo, heure, date, calcul, usb, ou simplement parler.";
    }
}

} // namespace chatia