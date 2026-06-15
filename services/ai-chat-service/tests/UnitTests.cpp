#include "../AIApps/ChatServer/include/AIUtil/BoundedExecutor.h"
#include "../AIApps/ChatServer/include/AIUtil/ImageValidation.h"
#include "../AIApps/ChatServer/include/AIUtil/SseStreamParser.h"
#include "../HttpServer/include/session/Session.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{

void testSessionConcurrentAccess()
{
    auto session = std::make_shared<http::session::Session>(
        "session",
        nullptr,
        3600);
    std::vector<std::thread> writers;
    for (int i = 0; i < 8; ++i)
    {
        writers.emplace_back([session, i] {
            for (int iteration = 0; iteration < 500; ++iteration)
            {
                session->setValue(
                    "key-" + std::to_string(i),
                    std::to_string(iteration));
                (void)session->getValue("key-" + std::to_string(i));
            }
        });
    }
    for (auto& writer : writers)
    {
        writer.join();
    }
    for (int i = 0; i < 8; ++i)
    {
        assert(session->getValue("key-" + std::to_string(i)) == "499");
    }
}

void testSessionExpiry()
{
    http::session::Session session("expired", nullptr, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    assert(session.isExpired());
}

void testExecutorBackpressure()
{
    BoundedExecutor executor(1, 1);
    std::promise<void> taskStarted;
    std::promise<void> releaseTask;
    std::shared_future<void> releaseSignal =
        releaseTask.get_future().share();
    std::atomic<int> completed{0};

    assert(executor.trySubmit([&] {
        taskStarted.set_value();
        releaseSignal.wait();
        ++completed;
    }));
    taskStarted.get_future().wait();

    assert(executor.trySubmit([&] {
        ++completed;
    }));
    assert(!executor.trySubmit([] {}));

    releaseTask.set_value();
    executor.shutdown();
    assert(completed.load() == 2);
}

void testSseParserAcrossChunkBoundaries()
{
    std::vector<std::string> tokens;
    SseStreamParser parser([&tokens](const std::string& token) {
        tokens.push_back(token);
    });

    const std::string first =
        "event: message\r\ndata: {\"choices\":[{\"delta\":{\"content\":\"hel";
    const std::string second =
        "lo\"}}]}\r\n\r\ndata: {\"choices\":[{\"delta\":{\"content\":\" world\"}}]}\n\n"
        "data: [DONE]";

    parser.append(first.data(), first.size());
    parser.append(second.data(), second.size());
    parser.finish();

    assert(parser.error().empty());
    assert(parser.done());
    assert(parser.answer() == "hello world");
    assert(tokens.size() == 2);
}

void testSseParserError()
{
    SseStreamParser parser;
    const std::string response =
        "data: {\"error\":{\"message\":\"upstream failed\"}}\n\n";
    parser.append(response.data(), response.size());
    parser.finish();
    assert(parser.error() == "upstream failed");
}

void testSseParserWithoutCompletionMarker()
{
    SseStreamParser parser;
    const std::string response =
        "data: {\"choices\":[{\"delta\":{\"content\":\"partial\"}}]}\n\n";
    parser.append(response.data(), response.size());
    parser.finish();

    assert(parser.error().empty());
    assert(!parser.done());
    assert(parser.answer() == "partial");
}

void testImageDimensionValidation()
{
    const std::vector<unsigned char> pngHeader = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U,
        0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x02U
    };
    const auto dimensions =
        image_validation::validateEncodedImage(pngHeader);
    assert(dimensions.width == 2U);
    assert(dimensions.height == 2U);
}

void testOversizedImageIsRejected()
{
    const std::vector<unsigned char> oversizedPngHeader = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U,
        0x00U, 0x01U, 0x86U, 0xA0U, 0x00U, 0x01U, 0x86U, 0xA0U
    };

    bool rejected = false;
    try
    {
        image_validation::validateEncodedImage(oversizedPngHeader);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    assert(rejected);
}

} // namespace

int main()
{
    testSessionConcurrentAccess();
    testSessionExpiry();
    testExecutorBackpressure();
    testSseParserAcrossChunkBoundaries();
    testSseParserError();
    testSseParserWithoutCompletionMarker();
    testImageDimensionValidation();
    testOversizedImageIsRejected();
    return 0;
}
