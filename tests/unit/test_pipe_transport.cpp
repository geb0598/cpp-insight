#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "insight/archive.h"
#include "insight/pipe_transport.h"

namespace {

// class PipeTransportTest : public ::testing::Test {
// protected:
//     void SetUp() override {
//         server_thread_ = std::thread([this]() {
//             insight::PipeServer server;

//             auto result = server.Listen(insight::DATA_PIPE_NAME, PIPE_ACCESS_INBOUND);
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

//             insight::PacketHeader header;
//             insight::ByteBuffer   payload;
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
//     insight::PacketType  received_type_;
//     insight::ByteBuffer  received_data_;
// };

// TEST_F(PipeTransportTest, ConnectAndSendHandshake) {
//     insight::PipeClient client;
//     auto result = client.Connect();
//     ASSERT_TRUE(result) << result.error.message();

//     insight::BinaryWriter writer;
//     std::string msg = "hello";
//     writer << msg;

//     result = client.Send(insight::PacketType::HANDSHAKE, writer.GetBuffer());
//     ASSERT_TRUE(result) << result.error.message();

//     ASSERT_TRUE(receive_done_.get_future().get());
//     EXPECT_EQ(received_type_, insight::PacketType::HANDSHAKE);

//     insight::BinaryReader reader(received_data_);
//     std::string received_msg;
//     reader << received_msg;
//     EXPECT_EQ(msg, received_msg);
// }

} // namespace