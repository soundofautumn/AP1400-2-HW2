#include "server.h"
#include "crypto.h"

#include <random>


Server::Server() = default;

std::shared_ptr<Client> Server::add_client(std::string id) const {
    static std::random_device rd;
    static std::mt19937 g(rd());
    for (const auto &[client, _]: clients) {
        if (client->get_id() == id) {
            id += std::to_string(g() % 10000);
        }
    }
    auto client = std::make_shared<Client>(id, *this);
    clients[client] = 5.0;
    return client;
}

std::shared_ptr<Client> Server::get_client(const std::string &id) const {
    for (const auto &[client, _]: clients) {
        if (client->get_id() == id) {
            return client;
        }
    }
    return nullptr;
}

double Server::get_wallet(const std::string &id) const {
    return clients[get_client(id)];
}

bool Server::parse_trx(const std::string &trx, std::string &sender, std::string &receiver, double &value) {
    const size_t first = trx.find('-');
    const size_t second = trx.find('-', first + 1);
    if (first == std::string::npos ||
        second == std::string::npos ||
        first == second) {
        throw std::runtime_error("invalid trx string");
    }
    sender = trx.substr(0, first);
    receiver = trx.substr(first + 1, second - (first + 1));
    value = std::stod(trx.substr(second + 1));
    return true;
}

bool Server::add_pending_trx(const std::string &trx, const std::string &signature) const {
    std::string sender, receiver;
    double value;
    parse_trx(trx, sender, receiver, value);
    if (!crypto::verifySignature(get_client(sender)->get_publickey(), trx, signature)) {
        return false;
    }
    pending_trxs.push_back(trx);
    return true;
}

size_t Server::mine() {
    std::string mempool;
    for (const auto &trx: pending_trxs) {
        mempool += trx;
    }
    while (true) {
        for (auto &[client, _]: clients) {
            const size_t nonce = client->generate_nonce();
            const std::string sha256 = crypto::sha256(mempool + std::to_string(nonce));
            if (sha256.substr(0, 10).find("000") != std::string::npos) {
                for (const auto &trx: pending_trxs) {
                    std::string sender, receiver;
                    double value;
                    parse_trx(trx, sender, receiver, value);
                    clients[get_client(sender)] -= value;
                    clients[get_client(receiver)] += value;
                }
                pending_trxs.clear();
                clients[client] += 6.25;
                return nonce;
            }
        }
    }
}