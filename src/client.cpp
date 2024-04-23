#include "client.h"
#include "crypto.h"

#include <utility>
#include <random>

Client::Client(std::string id, const Server &server) : id(std::move(id)), server(&server) {
    crypto::generate_key(public_key, private_key);
}

std::string Client::get_id() const {
    return id;
}

std::string Client::get_publickey() const {
    return public_key;
}

double Client::get_wallet() const {
    return server->get_wallet(id);
}

std::string Client::sign(const std::string &txt) const {
    return crypto::signMessage(private_key, txt);
}

bool Client::transfer_money(const std::string &receiver, double value) const {
    if (server->get_client(receiver) == nullptr) {
        return false;
    }
    if (server->get_wallet(receiver) < value) {
        return false;
    }
    std::string trx = this->get_id() + '-' + receiver + '-' + std::to_string(value);
    return server->add_pending_trx(trx, sign(trx));
}

size_t Client::generate_nonce() const {
    std::random_device rd;
    std::mt19937 g(rd());
    return g();
}

