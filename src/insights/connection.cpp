#include "insights/connection.h"

namespace insights {

void Connection::StartWorkers() {
    is_running_  = true;
    send_thread_ = std::thread(&Connection::SendWorker, this);
    recv_thread_ = std::thread(&Connection::RecvWorker, this);
}

void Connection::StopWorkers() {
    is_running_ = false;
    send_cv_.notify_all();

    if (send_thread_.joinable()) { 
        send_thread_.join(); 
    }
    if (recv_thread_.joinable()) { 
        recv_thread_.join(); 
    }

    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        std::queue<Packet> empty;
        std::swap(send_queue_, empty);
    }
}

void Connection::EnqueuePacket(PacketType type, ByteBuffer payload) {
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        send_queue_.push({ type, std::move(payload) });
    }
    send_cv_.notify_one();
}

void Connection::SendWorker() {
    while (is_running_) {
        std::unique_lock<std::mutex> lock(send_mutex_);
        send_cv_.wait(lock, [this]() {
            return !send_queue_.empty() || !is_running_;
        });

        if (!is_running_ && send_queue_.empty()) {
            return;
        }

        Packet packet = std::move(send_queue_.front());
        send_queue_.pop();
        lock.unlock();

        auto result = Send(packet.type, packet.payload);
        if (!result) {
            bool expected = true;
            if (is_running_.compare_exchange_strong(expected, false)) {
                OnDisconnected();
            }
            return;
        }
    }
}

void Connection::RecvWorker() {
    while (is_running_) {
        PacketHeader header;
        ByteBuffer   payload;

        auto result = Receive(header, payload);
        if (!result) {
            bool expected = true;
            if (is_running_.compare_exchange_strong(expected, false)) {
                OnDisconnected();
            }
            return;
        }

        OnPacketReceived(header, payload);
    }
}

} // namespace insights