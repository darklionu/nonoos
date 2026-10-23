#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QScreen>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kWidth = 80;
constexpr int kHeight = 80;

bool get_displays(std::vector<QRect>& displays) {
    displays.clear();
    for (QScreen* screen : QGuiApplication::screens()) {
        const QRect bounds = screen->geometry();
        if (bounds.width() >= kWidth && bounds.height() >= kHeight) {
            displays.push_back(bounds);
        }
    }

    if (displays.empty()) {
        std::fprintf(stderr, "Aucun écran n'est assez grand pour afficher le sprite.\n");
        return false;
    }
    return true;
}

QPoint random_position(const std::vector<QRect>& displays) {
    const QRect& bounds =
        displays[static_cast<std::size_t>(QRandomGenerator::global()->bounded(
            static_cast<int>(displays.size())))];
    return QPoint(
        bounds.x() + QRandomGenerator::global()->bounded(bounds.width() - kWidth + 1),
        bounds.y() + QRandomGenerator::global()->bounded(bounds.height() - kHeight + 1));
}

bool position_is_visible(const std::vector<QRect>& displays, const QPoint& position) {
    const QRect window_bounds(position, QSize(kWidth, kHeight));
    return std::any_of(displays.begin(), displays.end(), [&window_bounds](const QRect& bounds) {
        return bounds.contains(window_bounds);
    });
}

int step_toward(int from, int to) {
    if (std::abs(to - from) < 3) {
        return to;
    }
    return from + (to > from ? 3 : -3);
}

void draw_pet(QPainter& painter, int mouse_x, int mouse_y) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(80, 160, 255));
    painter.drawEllipse(QRectF(0, 0, kWidth, kHeight));

    painter.setBrush(Qt::white);
    painter.drawEllipse(QRectF(22, 25, 14, 17));
    painter.drawEllipse(QRectF(44, 25, 14, 17));

    const auto draw_pupil = [&painter, mouse_x, mouse_y](int center_x, int center_y) {
        float dx = static_cast<float>(mouse_x - center_x);
        float dy = static_cast<float>(mouse_y - center_y);
        const float distance = std::sqrt(dx * dx + dy * dy);
        constexpr float max_offset = 4.0F;
        if (distance > max_offset) {
            const float scale = max_offset / distance;
            dx *= scale;
            dy *= scale;
        }

        painter.setBrush(Qt::black);
        painter.drawEllipse(QRectF(
            center_x + static_cast<int>(dx) - 3,
            center_y + static_cast<int>(dy) - 4,
            6, 8));
    };

    draw_pupil(29, 33);
    draw_pupil(51, 33);
}

}  // namespace

std::string askRobot(const std::string& message) {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return "Erreur : impossible de créer la connexion au robot.";
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
        close(socket_fd);
        return "Erreur : adresse IP invalide.";
    }

    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        close(socket_fd);
        return "Erreur : le robot n'est pas démarré.";
    }

    std::size_t sent = 0;
    while (sent < message.size()) {
        const ssize_t result = send(socket_fd, message.data() + sent, message.size() - sent, 0);
        if (result <= 0) {
            close(socket_fd);
            return "Erreur : impossible d'envoyer le message au robot.";
        }
        sent += static_cast<std::size_t>(result);
    }

    char buffer[4096];
    const ssize_t received = recv(socket_fd, buffer, sizeof(buffer), 0);
    close(socket_fd);
    if (received <= 0) {
        return "Erreur : aucune réponse du robot.";
    }
    return std::string(buffer, static_cast<std::size_t>(received));
}

class PetWindow final : public QWidget {
public:
    PetWindow()
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
        setFixedSize(kWidth, kHeight);
        setAttribute(Qt::WA_TranslucentBackground);
        setFocusPolicy(Qt::StrongFocus);

        get_displays(displays_);
        move(random_position(displays_));
        target_ = random_position(displays_);

        connect(&timer_, &QTimer::timeout, this, [this] { advance(); });
        timer_.start(30);

        connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen*) {
            refresh_screens();
        });
        connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) {
            refresh_screens();
        });
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        const QPoint mouse_position = mapFromGlobal(QCursor::pos());
        draw_pet(painter, std::clamp(mouse_position.x(), 0, kWidth),
                 std::clamp(mouse_position.y(), 0, kHeight));
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::RightButton) {
            close();
            return;
        }
        if (event->button() == Qt::LeftButton) {
            dragging_ = true;
            drag_moved_ = false;
            drag_offset_ = event->globalPosition().toPoint() - pos();
            setFocus();
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging_ && (event->buttons() & Qt::LeftButton) != 0) {
            const QPoint new_position = event->globalPosition().toPoint() - drag_offset_;
            if (new_position != pos()) {
                drag_moved_ = true;
                move(new_position);
            }
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            if (dragging_ && !drag_moved_) {
                chatting_ = !chatting_;
            }
            dragging_ = false;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if ((event->modifiers() & Qt::ControlModifier) != 0) {
            if (event->key() == Qt::Key_C) {
                chatting_ = !chatting_;
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_R) {
                chatting_ = false;
                event->accept();
                return;
            }
        }
        QWidget::keyPressEvent(event);
    }

private:
    void refresh_screens() {
        if (!get_displays(displays_)) {
            close();
            return;
        }
        if (!position_is_visible(displays_, pos())) {
            move(random_position(displays_));
        }
        target_ = random_position(displays_);
    }

    void advance() {
        if (!chatting_ && !dragging_) {
            if (pos() == target_) {
                target_ = random_position(displays_);
            }
            move(step_toward(pos().x(), target_.x()),
                 step_toward(pos().y(), target_.y()));
        }
        if (++tick_ % 100 == 0) {
            raise();
        }
        update();
    }

    std::vector<QRect> displays_;
    QPoint target_;
    QPoint drag_offset_;
    QTimer timer_;
    bool chatting_ = false;
    bool dragging_ = false;
    bool drag_moved_ = false;
    int tick_ = 0;
};

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    std::vector<QRect> displays;
    if (!get_displays(displays)) {
        return 1;
    }

    PetWindow window;
    window.show();
    return application.exec();
}
