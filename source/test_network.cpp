/**
 * @file test_network.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-12-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "entry.hpp"

extern "C"
{
    #include "mongoose.h"
}

#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"
#include "boost/beast/version.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/config.hpp"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;   // from <boost/asio/ip/tcp.hpp>

int httplib_server()
{
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("hello world", "text/plain");
    });

    return 1;
}

int beast_http_server_sync(Message& message)
{
    std::string address = "0.0.0.0";
    unsigned int port = 13000;
    std::string doc_root = ".";

    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, tcp::endpoint(net::ip::address_v4::from_string(address), port)};
    acceptor.listen();
    for (;;)
    {
        // This will receive the new connection
        tcp::socket socket{ioc};

        // Block until we get a connect
        acceptor.accept(socket);
        try 
        {
            beast::flat_buffer buffer;
            http::request<http::string_body> request;

            http::read(socket, buffer, request);

            http::response<http::string_body> response{http::status::ok, request.version()};
            response.set(http::field::server, "Boost HTTP Server");
            response.body() = "Hello, World!";
            response.prepare_payload();

            http::write(socket, response);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return 1;
}

int beast_http_client_sync(Message& message)
{
    try 
    {
        std::string address = "192.169.4.16";
        unsigned int port = 13000;
        std::string target = "/hello";
        int version = 10;
        net::io_context ioc;
        beast::tcp_stream stream(ioc);

        stream.connect(tcp::endpoint(net::ip::address_v4::from_string(address), port));
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, address);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        LOG(INFO) << "Full response: \n" << res << std::endl;
        LOG(INFO) << "response's body: " << beast::buffers_to_string(res.body().data()) << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        if (ec && ec != beast::errc::not_connected)
        {
            throw ec;
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;
    }

    return 1;
}

static void handle_request(const http::request<http::string_body>& req, http::response<http::string_body>& res)
{
    if (req.method() == http::verb::get && req.target() == "/hello")
    {
        res.result(http::status::ok);
        res.set(http::field::server, "AsyncBoostServer");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Hello, Boost.Beast";
        res.prepare_payload();
    }
    else 
    {
        res.result(http::status::ok);
        res.set(http::field::server, "AsyncBoostServer");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Not Found";
        res.prepare_payload();
    }
}

static void do_session(tcp::socket& socket)
{
    beast::error_code ec;
    beast::flat_buffer buffer;

    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec == http::error::end_of_stream)
    {
        LOG(WARNING) << "ec is end_of_stream\n";
        socket.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }
    else if (ec)
    {
        LOG(ERROR) << "Error reading HTTP request: " << ec.message() << std::endl;
        return;
    }
    LOG(INFO) << "request's target: " << req.target() << "\n";

    http::response<http::string_body> res;
    handle_request(req, res);

    http::write(socket, res, ec);
    if (ec)
    {
        LOG(ERROR) << "Error writing HTTP response: " << ec.message() << std::endl;
        return;
    }
    socket.shutdown(tcp::socket::shutdown_both, ec);
}

int beast_http_server_async(Message& message)
{
    // boost::asio::ip::tcp::endpoint end_point(boost::asio::ip::make_address("0.0.0.0"), 13001);
    // boost::asio::io_context ioc{4};

    // beast::error_code ec;
    // boost::asio::ip::tcp::acceptor acceptor(boost::asio::make_strand(ioc));
    // const std::string doc_root{"/home/user"};

    // acceptor.open(end_point.protocol(), ec);
    // if (ec)
    // {
    //     LOG(ERROR) << "open failed\n";
    //     return 0;
    // }

    // acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    // if (ec)
    // {
    //     LOG(ERROR) << "set option failed\n";
    //     return 0;
    // }

    // acceptor.bind(end_point, ec);
    // if (ec)
    // {
    //     LOG(ERROR) << "bind failed\n";
    //     return 0;
    // }

    // acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    // if (ec)
    // {
    //     LOG(ERROR) << "listen failed\n";
    //     return 0;
    // }

    // auto handle_accept = [](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
    // {
    //     if (ec)
    //     {
    //         LOG(ERROR) << "accept failed\n";
    //         return;
    //     }

    //     // boost::beast::tcp_stream stream(std::move(socket));
    //     // boost::asio::dispatch(stream.get_executor(), [&]()
    //     // {
    //     //     boost::beast::http::request<http::string_body> req = {};
    //     //     beast::flat_buffer buffer;

    //     //     stream.expires_after(std::chrono::seconds(30));
    //     //     boost::beast::http::async_read(stream, buffer, req, [&](boost::beast::error_code ec, std::size_t bytes_transferred)
    //     //     {
    //     //         boost::ignore_unused(bytes_transferred);
    //     //         if (ec == boost::beast::http::error::end_of_stream)
    //     //         {
    //     //             stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    //     //         }

    //     //         if (ec)
    //     //         {
    //     //             LOG(ERROR) << "read failed\n";
    //     //             return;
    //     //         }

    //     //         boost::beast::http::response<http::string_body> res;
    //     //         res.version(11);
    //     //         res.result(boost::beast::http::status::ok);
    //     //         res.set(boost::beast::http::field::server, "Boost.Beast");
    //     //         boost::beast::http::message msg(res);
    //     //         // 放弃了，这个boost.beast真tm难用
    //     //     });
    //     });

    // };

    // acceptor.async_accept(boost::asio::make_strand(ioc), std::bind(handle_accept, std::placeholders::_1, std::move(boost::asio::make_strand(ioc))));



    // return 1;
}

static void handle_event(mg_connection *connect, int ev, void *ev_data, void *fn_data)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ds(1, 300);
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    LOG(INFO) << "ev: " << ev << "\n";
    switch (ev)
    {
        case MG_EV_HTTP_MSG:
            std::this_thread::sleep_for(std::chrono::milliseconds(ds(gen)));
            mg_http_reply(connect, 200, "text/plain", "hello, mongoose");
            connect->is_draining = 1;
            break;
        default:
            break;
    }
}

int mongoose_http_server(Message& message)
{
    struct mg_mgr mgr;
    struct mg_connection *connect;
    mg_event_handler_t func = handle_event;

    mg_mgr_init(&mgr);

    connect = mg_http_listen(&mgr, "0.0.0.0:13001", func, nullptr);

    for (;;)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 1;
}

void EventHandler(mg_connection* connect, struct mg_http_message* ev_data)
{
    mg_http_reply(connect, 200, "text/plain", "hello mongoose");
    connect->is_draining = 1;

    return;
}

int mongoose_http_server_sync(Message& message)
{
    struct mg_mgr mgr;
    struct mg_connection *connect;
    mg_event_handler_t func = [](mg_connection *connect, int ev, void *ev_data, void *fn_data)
    {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        LOG(INFO) << "ev: " << ev << "\n";
        switch (ev)
        {
            case MG_EV_HTTP_MSG:
            {
                EventHandler(connect, hm);
                break;
            }
            default:
                break;
        }
    };

    mg_mgr_init(&mgr);

    connect = mg_http_listen(&mgr, "0.0.0.0:13000", func, nullptr);

    for (;;)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 1;
}

int mongoose_http_server_async(Message& message)
{
    struct mg_mgr mgr;
    struct mg_connection *connect;
    mg_event_handler_t func = [](mg_connection *connect, int ev, void *ev_data, void *fn_data)
    {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        LOG(INFO) << "ev: " << ev << "\n";
        switch (ev)
        {
            case MG_EV_HTTP_MSG:
            {
                std::thread tmp(EventHandler, connect, hm);
                tmp.detach();
                break;
            }
            default:
                break;
        }
    };

    mg_mgr_init(&mgr);

    connect = mg_http_listen(&mgr, "0.0.0.0:13000", func, nullptr);

    for (;;)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 1;
}

int mongoose_http_client(Message& message)
{
    bool done = false;
    struct mg_mgr mgr;
    struct mg_connection* connect = NULL;
    mg_event_handler_t func = [](mg_connection *connect, int ev, void *ev_data, void *fn_data)
    {
        LOG(INFO) << "ev: " << ev << "\n";
        switch (ev)
        {
            case MG_EV_CONNECT:
            {
                struct mg_str host = mg_url_host("http://192.169.4.16:13000/api/");
                mg_printf(connect, "GET %s HTTP/1.0 \r\nContent-Type:text/plain\r\n\r\n", mg_url_uri("http://192.169.4.16:13000/api/"));
                mg_printf(connect, "Hello, mongoose server\r\n");
                break;
            }
            case MG_EV_HTTP_MSG:
            {
                struct mg_http_message* hm = static_cast<struct mg_http_message*>(ev_data);
                printf("%.*s", (int)hm->message.len, hm->message.ptr);
                *(bool *)fn_data = true;
                break;
            }
            default:
                break;
        }
    };

    mg_mgr_init(&mgr);

    for (;;)
    {
        // if (done)
        // {
        //     break;
        // }
        mg_http_connect(&mgr, "http://192.169.4.16:13000/api/", func, &done);
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 1;
}

int test_httplib_server_async_close(Message& message)
{
    httplib::Server svr;
    auto server_thread = [](httplib::Server* svr)
    {
        auto ret = svr->set_mount_point("/", "/home/user/zjy-190/Media/");
        if (!ret)
        {
            LOG(ERROR) << "the specified base directory doesn't exist...\n";
            return ;
        }
        svr->listen("0.0.0.0", 12001);
    };

    std::thread server(server_thread, &svr);

    while (true)
    {
        int cmd;
        LOG(INFO) << "input command: (0 to quit)\n";
        std::cin >> cmd;
        if (cmd == 0)
        {
            LOG(ERROR) << "quit...\n";
            svr.stop();
            if (server.joinable())
            {
                server.join();
            }
            break;
        }
    }

    return 1;
}

int test_mongoose_server_async_close(Message& message)
{
    struct mg_mgr mgr;
    struct mg_connection *connect;
    mg_event_handler_t func = [](mg_connection *connect, int ev, void *ev_data, void *fn_data)
    {
        if (ev == MG_EV_HTTP_MSG)
        {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            struct mg_http_serve_opts opts = {.root_dir = "/home/user/zjy-190/Media/"};
            mg_http_serve_dir(connect, hm, &opts);
        }
    };

    mg_mgr_init(&mgr);
    connect = mg_http_listen(&mgr, "0.0.0.0:13000", func, nullptr);
    bool done = false;
    auto tmp_func = [](struct mg_mgr* mgr, bool* done)
    {
        for (;;)
        {
            if (*done)
            {
                LOG(WARNING) << "quit...\n";
                break;
            }
            mg_mgr_poll(mgr, 1000);
        }
    };

    std::thread server(tmp_func, &mgr, &done);

    while (true)
    {
        int cmd;
        LOG(INFO) << "input command: (0 to quit)\n";
        std::cin >> cmd;
        if (cmd == 0)
        {
            done = true;
            if (server.joinable())
            {
                server.join();
            }
            break;
        }
    }

    return 1;
}

int test_mongoose_server_async_close_in_class(Message& message)
{
    class RK
    {
    private:
        bool m_done {false};

    public:
        void StartMediaServer()
        {
            auto tmp_func = [](bool* done)
            {
                struct mg_mgr mgr;
                struct mg_connection *connect;
                mg_event_handler_t func = [](mg_connection *connect, int ev, void *ev_data, void *fn_data)
                {
                    if (ev == MG_EV_HTTP_MSG)
                    {
                        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
                        struct mg_http_serve_opts opts = {.root_dir = "/home/user/zjy-190/Media/"};
                        mg_http_serve_dir(connect, hm, &opts);
                    }
                };

                mg_mgr_init(&mgr);
                connect = mg_http_listen(&mgr, "0.0.0.0:13000", func, nullptr);
                for (;;)
                {
                    if (*done)
                    {
                        LOG(WARNING) << "quit...\n";
                        break;
                    }
                    mg_mgr_poll(&mgr, 1000);
                }
            };

            std::thread server(tmp_func, &m_done);
            server.detach();
        }

        void yield()
        {
            while (true)
            {
                int cmd;
                LOG(INFO) << "input command: (0 to quit)\n";
                std::cin >> cmd;
                if (cmd == 0)
                {
                    m_done = true;
                    break;
                }
            }
        }
    };

    RK rk;

    rk.StartMediaServer();
    rk.yield();


    return 1;
}

void parse_http_request_data(const std::string& http_request)
{
    std::istringstream request_stream(http_request);

    std::string request_line;
    std::getline(request_stream, request_line, '\n');

    std::istringstream request_line_stream(request_line);
    std::string method, path, protocol;
    request_line_stream >> method >> path >> protocol;
    LOG(INFO) << "method: " << method << "\n"
              << "path: " << path << "\n"
              << "protocol: " << path << "\n";

    std::string header_line;
    while (std::getline(request_stream, header_line) && header_line != "\r")
    {
        LOG(INFO) << "header: " << header_line << "\n";
    }

    std::string request_body;
    std::getline(request_stream, request_body, '\0');
    LOG(INFO) << "body: " << request_body << "\n";
}

static void handle_request(int client_socket)
{
    size_t buffer_size = 1024 * 10;
    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);

    // read data from client
    read(client_socket, buffer, buffer_size - 1);
    printf("Received request:\n%s\n", buffer);
    parse_http_request_data(std::string(buffer));

    // send HTTP response
    const char* response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<h1>Hello, this is a simple HTTP server in C</hi>";
    write(client_socket, response, strlen(response));

    // close connection of client
    close(client_socket);
}

static int test_c_http_server(Message& message)
{
    const unsigned int port = 8080;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t addr_len = sizeof(client_addr);

    // create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Socket creation failed\n");
        return EXIT_FAILURE;
    }

    // set address and port 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = htons(port);

    // bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))  == -1 )
    {
        perror("Socket listen failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server running on port %d...\n", port);

    if (listen(server_socket, 5) == -1)
    {
        perror("listen failed");
        return EXIT_FAILURE;
    }

    while (1)
    {
        // accept connection of client
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1)
        {
            perror("Accept failed\n");
            continue;;
        }

        // process request of client
        handle_request(client_socket);
    }

    close(server_socket);

    return EXIT_SUCCESS;
}

int StartListen()
{
    size_t port = 8080;
    struct sockaddr_in server_addr;
    int server_socket = -1;
    memset(&server_addr, 0, sizeof(server_addr));

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        LOG(ERROR) << "socket creation failed\n";
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = htons(port);

    int opt = 1;
    int res = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (res < 0)
    {
        LOG(ERROR) << "setsockopt failed\n";
        return 0;
    }

    res = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1)
    {
        LOG(ERROR) << "socket listen failed, port: " << port << "\n";
        return 0;
    }

    res = listen(server_socket, 10);
    if (res == -1)
    {
        LOG(ERROR) << "listen failed\n";
        return 0;
    }

    return server_socket;
}

int StartWaiting(bool &flag, struct pollfd *fds, int max_size)
{
    while (1)
    {
        if (flag)
        {
            break;
        }
        int activity = poll(fds, max_size, 5);
        if (activity < 0)
        {
            LOG(ERROR) << "poll error \n";
            return 0;
        }

        if (fds[0].revents & POLLIN)
        {
            int new_socket;
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t len = sizeof(client_addr);
            new_socket = accept(fds[0].fd, (struct sockaddr *)&client_addr, &len);
            if (new_socket < 0)
            {
                LOG(ERROR) << "accept failed\n";
                return 0;
            }

            for (int i = 1; i < max_size; i++)
            {
                if (fds[i].fd == 0)
                {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN;
                }
            }
        }
    }

    return 1;
}

int test_start_server(Message& message)
{
    const int max_size = 100;
    int server_socket = StartListen();

    if (server_socket == 0)
    {
        LOG(ERROR) << "start listen \n";
        return 0;
    }

    bool flag = false;
    struct pollfd fds[max_size];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;
    for (int i = 1; i < max_size; i++)
    {
        fds[i].fd = 0;
    }

    std::thread tmp = std::thread(StartWaiting, std::ref(flag), fds, max_size);

    while (1)
    {
        for (int i = 1; i < max_size; i++)
        {
            if (fds[i].fd != 0 && fds[i].revents & POLLIN)
            {
                int valread;
                size_t buffer_max_size = 10240;
                char buffer[buffer_max_size];
                valread = read(fds[i].fd, buffer, buffer_max_size);
                if (valread <= 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = 0;
                }
                else 
                {
                    printf("Received request:\n%s\n", std::string(buffer, valread).c_str());
                    const char* response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<h1>Hello, this is a simple HTTP server in C</hi>\n";
                    write(fds[i].fd, response, strlen(response));
                    close(fds[i].fd);
                    fds[i].fd = 0;
                }
            }
        }
    }

    close(server_socket);
    flag = true;
    return 1;
}

int test_httpd(Message& message)
{
    Httpd httpd(100);
    Httpd::httpd_ret res;

    res = httpd.Listen();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "listen failed \n";
        return 0;
    }

    res = httpd.Start();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "start failed \n";
        return 0;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::vector<struct pollfd> client_fds;
        res = httpd.PopEvent(client_fds);
        if (res != Httpd::httpd_ret::SUCCESS)
        {
            LOG(ERROR) << "empty event  \n";
            continue;
        }
        for (auto& fds : client_fds)
        {
            if (fds.revents & POLLIN)
            {
                int valread;
                size_t buffer_max_size = 10240;
                std::vector<char> buffer(buffer_max_size);
                valread = read(fds.fd, buffer.data(), buffer.size());
                if (valread <= 0)
                {
                    close(fds.fd);
                    continue;
                }

                printf("Received request:\n%s\n", std::string(buffer.data(), valread).c_str());
                const char* response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<h1>Hello, this is a simple HTTP server in C</hi>\n";
                std::string version = "HTTP/1.1 200 OK";
                std::string response_header = "Content-Type: application/json";
                std::string response_body = R"({"hello":"world"})";
                httpd.HTTPResponse(fds, version, response_header, response_body);
            }
        }
    }

    httpd.Stop();

    return 1;
}

int test_httpdv2(Message& message)
{
    Httpd httpd(100);
    Httpd::httpd_ret res;

    res = httpd.Listen();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "listen failed \n";
        return 0;
    }

    res = httpd.Start();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "start failed \n";
        return 0;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        struct pollfd fds;
        std::string request_body;
        res = httpd.HTTPRequest(fds, request_body);
        if (res != Httpd::httpd_ret::SUCCESS)
        {
            LOG(ERROR) << "empty event  \n";
            continue;
        }

        LOG(INFO) << "request data: \n" << request_body << "\n";
        std::string version = "HTTP/1.1 200 OK";
        std::string response_header = "Content-Type: application/json";
        std::string response_body = R"({"hello":"world"})";
        httpd.HTTPResponse(fds, version, response_header, response_body);
    }

    httpd.Stop();

    return 1;
}

int test_httpdv3(Message& message)
{
    Httpd httpd(100);
    Httpd::httpd_ret res;

    res = httpd.Listen();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "listen failed \n";
        return 0;
    }

    res = httpd.Start();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "start failed \n";
        return 0;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        struct pollfd fds;
        Httpd::Request_t request;
        std::string request_body;
        res = httpd.HTTPRequest(request);
        if (res != Httpd::httpd_ret::SUCCESS)
        {
            LOG(ERROR) << "empty event  \n";
            continue;
        }

        LOG(INFO) << "request data: \n" << request.body << "\n";
        Httpd::Response_t response;
        response.fds = request.fds;
        response.protocol = "HTTP/1.1";
        response.code = 200;
        response.status = "OK";
        response.header_arr.push_back(std::string("Content-Type: application/json"));
        response.body = R"({"hello":"world"})";
        httpd.HTTPResponse(response);
    }

    httpd.Stop();

    return 1;
}

int test_httpdv4(Message& message)
{
    Httpd httpd(100);
    Httpd::httpd_ret res;

    res = httpd.Listen();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "listen failed \n";
        return 0;
    }

    res = httpd.Start();
    if (res != Httpd::httpd_ret::SUCCESS)
    {
        LOG(ERROR) << "start failed \n";
        return 0;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        struct pollfd fds;
        Httpd::Request_t request;
        std::string request_body;
        res = httpd.HTTPRequest(request);
        if (res != Httpd::httpd_ret::SUCCESS)
        {
            LOG(ERROR) << "empty event  \n";
            continue;
        }
        std::thread tmp_thread = std::thread([](Httpd* this_p, Httpd::Request_t request){
            LOG(INFO) << "request data: \n" << request.body << "\n";
            Httpd::Response_t response;
            response.fds = request.fds;
            response.protocol = "HTTP/1.1";
            response.code = 200;
            response.status = "OK";
            response.header_arr.push_back(std::string("Content-Type: application/json"));
            response.body = R"({"hello":"world"})";
            this_p->HTTPResponse(response);
        }, std::addressof(httpd), request);
        tmp_thread.detach();
    }

    httpd.Stop();

    return 1;
}

#include "http_server_async.hpp"

int tmp_test_http_server_async(Message& message)
{
    test_http_server_async();

    return 1;
}

class EventDispather
{
public:
    void Dispatch(NetworkMessage &network_message)
    {
        LOG(INFO) << "dispatch network message\n"
                  << "request path: " << network_message.path << "\n";
        if (network_message.path == "/api/task")
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
};

class NetworkWithMongoose
{
private:
    struct mg_mgr mgr;
    struct mg_connection *connect;
    EventDispather m_event_dispather;
    std::atomic<bool> m_flag{false};
    std::thread m_thread;

public:
    NetworkWithMongoose() = delete;
    NetworkWithMongoose(InitConfig init_config)
    {
        mg_mgr_init(&mgr);
    }
    ~NetworkWithMongoose()
    {
        mg_mgr_free(&mgr);
    }

    int Start(std::string ip, size_t port)
    {
        connect = mg_http_listen(&mgr, "0.0.0.0:13001", handle_event, this);

        auto dpfunc = [](NetworkWithMongoose* this_ptr)->void 
        {
            while (true)
            {
                if (this_ptr->m_flag.load())
                {
                    LOG(WARNING) << "network module quit...\n";
                    break;
                }
                mg_mgr_poll(&(this_ptr->mgr), 1000);
            }
        };

        m_thread = std::thread(dpfunc, this);
        m_thread.detach();

        return 1;
    }

    void Dispath(NetworkMessage &network_message)
    {
        network_message.ExtractPathArgument();
        m_event_dispather.Dispatch(network_message);
    }

private:
    static void handle_event(mg_connection *connect, int ev, void *ev_data, void *fn_data)
    {
        NetworkWithMongoose* this_ptr = static_cast<NetworkWithMongoose*>(fn_data);
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        LOG(INFO) << "ev: " << ev << "\n";
        switch (ev)
        {
            case MG_EV_HTTP_MSG:
            {
                NetworkMessage network_message;
                network_message.path = std::string(hm->uri.ptr, hm->uri.len);
                network_message.method = std::string(hm->method.ptr, hm->method.len);
                network_message.body = std::string(hm->body.ptr, hm->body.len);

                std::thread tmp_thread = std::thread([](NetworkMessage network_message, NetworkWithMongoose* this_ptr, mg_connection *connect)
                {

                    this_ptr->Dispath(network_message);

                    std::string response_data = R"(
                        {
                            "data":"hello, mongoose"
                        }
                    )";
            
                    mg_http_reply(connect, 200, "application/json", response_data.c_str());
                    connect->is_draining = 1;
                }, network_message, this_ptr, connect);
                tmp_thread.detach();

                break;
            }
            default:
                break;
        }
    }
};

static std::atomic<bool> network_mongoose_object_flag{false};

static void signal_handler(int signum)
{
    printf("receive signal: %d\n", signum);
    network_mongoose_object_flag.store(true);
}

int network_mongoose_object(Message& message)
{
    signal(SIGINT, signal_handler);

    InitConfig init_config;
    NetworkWithMongoose network(init_config);

    network.Start("0.0.0.0", 13001);

    while (true)
    {
        if (network_mongoose_object_flag.load())
        {
            LOG(WARNING) << "function quit...\n";
            break;
        }
    }

    return 1;
}

int test_network(Message& message)
{
    LOG(INFO) << "----test network begin----\n";

    std::map<std::string, std::function<int(Message&)>>cmd_map = {
        {"beast-http-server-sync", beast_http_server_sync},
        {"beast-http-server-async", beast_http_server_async},
        {"beast-http-client-sync", beast_http_client_sync},
        {"mongoose-http-server", mongoose_http_server},
        {"mongoose-http-server-async", mongoose_http_server_async},
        {"mongoose-http-server-sync", mongoose_http_server_sync},
        {"mongoose-http-client", mongoose_http_client},
        {"test-httplib-server-async-close", test_httplib_server_async_close},
        {"test-mongoose-server-async-close", test_mongoose_server_async_close},
        {"test-mongoose-server-async-close-in-class", test_mongoose_server_async_close_in_class},
        {"test-c-http-server", test_c_http_server},
        {"test-start-server", test_start_server},
        {"test-httpd", test_httpd},
        {"test-httpdv2", test_httpdv2},
        {"test-httpdv3", test_httpdv3},
        {"test-httpdv4", test_httpdv4},
        {"test-http-server-async", tmp_test_http_server_async},
        {"network-mongoose-object", network_mongoose_object}
    };

    std::string cmd = message.second_layer;
    auto it = cmd_map.find(cmd);
    if (it != cmd_map.end())
    {
        it->second(message);
        return 1;
    }
    else 
    {
        LOG(ERROR) << "invalid command: " << cmd << std::endl;
    }

    LOG(INFO) << "----test network end----\n";

    return 1;
}