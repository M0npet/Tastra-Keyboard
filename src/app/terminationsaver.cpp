// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminationsaver.h"

#include <QObject>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace Tastra {
namespace {

// The handler only writes the signal number to a socket (self-pipe trick:
// nothing else is safe in a signal handler); the event loop does the rest.
int signalSocket[2] = {-1, -1};

void onSignal(int signal)
{
    const char number = char(signal);
    [[maybe_unused]] const ssize_t written = ::write(signalSocket[0], &number, 1);
}

}

bool saveBeforeTermination(QObject *parent, std::function<void()> save)
{
    if (signalSocket[0] >= 0) return false;
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, signalSocket) != 0) return false;
    auto *notifier = new QSocketNotifier(signalSocket[1], QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated, parent, [save = std::move(save)] {
        char number = 0;
        if (::read(signalSocket[1], &number, 1) != 1) return;
        save();
        std::signal(number, SIG_DFL);
        std::raise(number);
    });
    struct sigaction action = {};
    action.sa_handler = onSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGTERM, &action, nullptr);
    ::sigaction(SIGINT, &action, nullptr);
    return true;
}

}
