#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "insights/archive.h"
#include "insights/pipe_transport.h"

namespace {

// class PipeTransportTest : public ::testing::Test {
// protected:
//     void SetUp() override {
//         server_thread_ = std::thread([this]() {
//             insights::PipeServer server;

//             auto result = server.Listen(insights::DATA_PIPE_NAME, PIPE_ACCESS_INBOUND);
//             if (!result) {
//                 server_ready_.set_value(false);
//                 return;
//             }
//             server_ready_.set_value(true);  

//             result = server.Accept();       
//             if (!result) {
//                 receive_done_.set_value(false);
//                 return;
//             }

//             insights::PacketHeader header;
//             insights::ByteBuffer   payload;
//             result = server.Receive(header, payload);
//             if (!result) {
//                 receive_done_.set_value(false);
//                 return;
//             }

//             received_type_ = header.type;
//             received_data_ = std::move(payload);
//             receive_done_.set_value(true);
//         });

//         ASSERT_TRUE(server_ready_.get_future().get());
//     }

//     void TearDown() override {
//         if (server_thread_.joinable()) {
//             server_thread_.join();
//         }
//     }

//     std::thread          server_thread_;
//     std::promise<bool>   server_ready_;
//     std::promise<bool>   receive_done_;
//     insights::PacketType  received_type_;
//     insights::ByteBuffer  received_data_;
// };

// TEST_F(PipeTransportTest, ConnectAndSendHandshake) {
//     insights::PipeClient client;
//     auto result = client.Connect();
//     ASSERT_TRUE(result) << result.error.message();

//     insights::BinaryWriter writer;
//     std::string msg = "hello";
//     writer << msg;

//     result = client.Send(insights::PacketType::HANDSHAKE, writer.GetBuffer());
//     ASSERT_TRUE(result) << result.error.message();

//     ASSERT_TRUE(receive_done_.get_future().get());
//     EXPECT_EQ(received_type_, insights::PacketType::HANDSHAKE);

//     insights::BinaryReader reader(received_data_);
//     std::string received_msg;
//     reader << received_msg;
//     EXPECT_EQ(msg, received_msg);
// }

} // namespace